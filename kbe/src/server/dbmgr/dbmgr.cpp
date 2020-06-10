/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2018 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "dbmgr.h"
#include "dbmgr_interface.h"
#include "dbtasks.h"
#include "profile.h"
#include "interfaces_handler.h"
#include "sync_app_datas_handler.h"
#include "update_dblog_handler.h"
#include "db_mysql/kbe_table_mysql.h"
#include "network/common.h"
#include "network/tcp_packet.h"
#include "network/udp_packet.h"
#include "network/message_handler.h"
#include "thread/threadpool.h"
#include "server/components.h"
#include "server/globaldata_server.h"
#include "server/telnet_server.h"
#include "db_interface/db_interface.h"
#include "db_mysql/db_interface_mysql.h"
#include "entitydef/scriptdef_module.h"

#include "baseapp/baseapp_interface.h"
#include "cellapp/cellapp_interface.h"
#include "baseappmgr/baseappmgr_interface.h"
#include "cellappmgr/cellappmgr_interface.h"
#include "loginapp/loginapp_interface.h"
#include "centermgr/centermgr_interface.h"

namespace KBEngine{

ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(Dbmgr);

//-------------------------------------------------------------------------------------
Dbmgr::Dbmgr(Network::EventDispatcher& dispatcher, 
			 Network::NetworkInterface& ninterface, 
			 COMPONENT_TYPE componentType,
			 COMPONENT_ID componentID):
	PythonApp(dispatcher, ninterface, componentType, componentID),
	loopCheckTimerHandle_(),
	mainProcessTimer_(),
	idServer_(1, 1024),
	pGlobalData_(NULL),
	pBaseAppData_(NULL),
	pCellAppData_(NULL),
	pCenterData_(NULL),
	bufferedDBTasksMaps_(),
	numWrittenEntity_(0),
	numRemovedEntity_(0),
	numQueryEntity_(0),
	numExecuteRawDatabaseCommand_(0),
	numCreatedAccount_(0),
	pInterfacesHandlers_(),
	pSyncAppDatasHandler_(NULL),
	pUpdateDBServerLogHandler_(NULL),
	pTelnetServer_(NULL),
	loseBaseappts_(),
	centermgrInfo_(NULL)
{
	KBEngine::Network::MessageHandlers::pMainMessageHandlers = &DbmgrInterface::messageHandlers;
}

//-------------------------------------------------------------------------------------
Dbmgr::~Dbmgr()
{
	loopCheckTimerHandle_.cancel();
	mainProcessTimer_.cancel();
	KBEngine::sleep(300);

	for (std::vector<InterfacesHandler*>::iterator iter = pInterfacesHandlers_.begin(); iter != pInterfacesHandlers_.end(); ++iter)
	{
		SAFE_RELEASE((*iter));
	}

	SAFE_RELEASE(centermgrInfo_);
}

//-------------------------------------------------------------------------------------
bool Dbmgr::canShutdown()
{
	if (getEntryScript().get() && PyObject_HasAttrString(getEntryScript().get(), "onReadyForShutDown") > 0)
	{
		// 所有脚本都加载完毕
		PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(),
			const_cast<char*>("onReadyForShutDown"),
			const_cast<char*>(""));

		if (pyResult != NULL)
		{
			bool isReady = (pyResult == Py_True);
			Py_DECREF(pyResult);

			if (isReady)
				return true;
			else
				return false;
		}
		else
		{
			SCRIPT_ERROR_CHECK();
			return false;
		}
	}

	KBEUnordered_map<std::string, Buffered_DBTasks>::iterator bditer = bufferedDBTasksMaps_.begin();
	for (; bditer != bufferedDBTasksMaps_.end(); ++bditer)
	{
		if (bditer->second.size() > 0)
		{
			thread::ThreadPool* pThreadPool = DBUtil::pThreadPool(bditer->first);
			KBE_ASSERT(pThreadPool);

			INFO_MSG(fmt::format("Dbmgr::canShutdown(): Wait for the task to complete, dbInterface={}, tasks{}=[{}], threads={}/{}, threadpoolDestroyed={}!\n",
				bditer->first, bditer->second.size(), bditer->second.getTasksinfos(), (pThreadPool->currentThreadCount() - pThreadPool->currentFreeThreadCount()),
				pThreadPool->currentThreadCount(), pThreadPool->isDestroyed()));

			return false;
		}
	}

	Components::COMPONENTS& cellapp_components = Components::getSingleton().getComponents(CELLAPP_TYPE);
	if(cellapp_components.size() > 0)
	{
		std::string s;
		for(size_t i=0; i<cellapp_components.size(); ++i)
		{
			s += fmt::format("{}, ", cellapp_components[i].cid);
		}

		INFO_MSG(fmt::format("Dbmgr::canShutdown(): Waiting for cellapp[{}] destruction!\n", 
			s));

		return false;
	}

	Components::COMPONENTS& baseapp_components = Components::getSingleton().getComponents(BASEAPP_TYPE);
	if(baseapp_components.size() > 0)
	{
		std::string s;
		for(size_t i=0; i<baseapp_components.size(); ++i)
		{
			s += fmt::format("{}, ", baseapp_components[i].cid);
		}

		INFO_MSG(fmt::format("Dbmgr::canShutdown(): Waiting for baseapp[{}] destruction!\n", 
			s));

		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------	
void Dbmgr::onShutdownBegin()
{
	PythonApp::onShutdownBegin();

	// 通知脚本
	SCOPED_PROFILE(SCRIPTCALL_PROFILE);
	SCRIPT_OBJECT_CALL_ARGS0(getEntryScript().get(), const_cast<char*>("onDBMgrShutDown"), false);
}

//-------------------------------------------------------------------------------------	
void Dbmgr::onShutdownEnd()
{
	PythonApp::onShutdownEnd();
}

void Dbmgr::onAllComponentFound()
{
	ServerApp::onAllComponentFound();

	if (isCentermgrEnable())
		findCentermgr();
}

//-------------------------------------------------------------------------------------
bool Dbmgr::isCentermgrEnable()
{
	// TODO: 检查配置是否启用了 centermgr
	return true;
}

//-------------------------------------------------------------------------------------
bool Dbmgr::isCentermgrChannel(Network::Channel *channel)
{
	return centermgrInfo_ != NULL && centermgrInfo_->pChannel == channel;
}

//-------------------------------------------------------------------------------------
void Dbmgr::findCentermgr()
{
	Network::EndPoint *ep = Network::EndPoint::createPoolObject(OBJECTPOOL_POINT);
	ep->socket(SOCK_STREAM);
	if (!ep->good())
	{
		ERROR_MSG("Components::findCentermgr: couldn't create a socket\n");
		Network::EndPoint::reclaimPoolObject(ep);
		return;
	}

	ep->setnonblocking(true);
	u_int32_t ipint;
	Network::Address::string2ip(g_serverConfig.getCenterMgr().externalAddress, ipint);
	u_int16_t port = ntohs(g_serverConfig.getCenterMgr().externalPorts_min);
	ep->addr(port, ipint);

	struct timeval tv = { 0, 1000000 };
	bool success = false;
	for (int itry = 0; itry < 3; ++itry)
	{
		INFO_MSG(fmt::format("Components::findCentermgr: connect centermgr result({})\n", 99999999));
		fd_set	frds, fwds;
		FD_ZERO(&frds);
		FD_ZERO(&fwds);
		FD_SET((int)(*ep), &frds);
		FD_SET((int)(*ep), &fwds);

		if (ep->connect() == -1)
		{
			INFO_MSG(fmt::format("Components::findCentermgr: connect({}) centermgr result({})\n", ep->c_str(), 77777777));
			if (select((*ep) + 1, &frds, &fwds, NULL, &tv) > 0)
			{
				INFO_MSG(fmt::format("Components::findCentermgr: connect centermgr result({}) \n", 1111111));
				if (FD_ISSET((*ep), &frds) || FD_ISSET((*ep), &fwds))
				{
					ep->connect();
					INFO_MSG(fmt::format("Components::findCentermgr: connect centermgr result({}) \n", 333333));
					int error = kbe_lasterror();
#if KBE_PLATFORM == PLATFORM_WIN32
					if (error == WSAEISCONN || error == 0)
#else
					if (error == EISCONN)
#endif
					{
						INFO_MSG(fmt::format("Components::findCentermgr: connect centermgr result({})\n", 555555));
						success = true;
						break;
					}
				}
			}
			else
			{
				ERROR_MSG(fmt::format("Components::findCentermgr: count({}) select wait failed.\n", itry));
			}
		}
		else
		{
			INFO_MSG(fmt::format("Components::findCentermgr: connect centermgr result({})\n", 22222));
			success = true;
			break;
		}
	}
	INFO_MSG(fmt::format("Components::findCentermgr: connect centermgr({}) result({}).\n", ep->c_str(), success));

	if (success)
	{
		Network::Channel* pChannel = Network::Channel::createPoolObject(OBJECTPOOL_POINT);
		success = pChannel->initialize(networkInterface_, ep, Network::Channel::INTERNAL);
		if (!success)
		{
			ERROR_MSG(fmt::format("Components::findCentermgr: channel initialize({}) is failed!\n",
				pChannel->c_str()));
			return;
		}

		if (!networkInterface_.registerChannel(pChannel))
		{
			ERROR_MSG(fmt::format("Components::findCentermgr: registerChannel channel({}) is failed!\n",
				pChannel->c_str()));

			pChannel->destroy();
			Network::Channel::reclaimPoolObject(pChannel);
		}
		else
		{
			Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
			(*pBundle).newMessage(CentermgrInterface::onAppRegister);
			CentermgrInterface::onAppRegisterArgs7::staticAddToBundle((*pBundle), componentType_, componentID_,
				networkInterface_.intaddr().ip, networkInterface_.intaddr().port,
				networkInterface_.extaddr().ip, networkInterface_.extaddr().port, g_kbeSrvConfig.getConfig().externalAddress);
			pChannel->send(pBundle);

			centermgrInfo_ = new Components::ComponentInfos;
			centermgrInfo_->componentType = CENTERMGR_TYPE;
			centermgrInfo_->pChannel = pChannel;

			INFO_MSG(fmt::format("Components::findCentermgr: register  to centermgr({}) success.\n", ep->c_str()));
		}
	}
	else
	{
		ERROR_MSG(fmt::format("Components::findCentermgr: connect({}) is failed! {}.\n",
			ep->addr().c_str(), kbe_strerror()));

		Network::EndPoint::reclaimPoolObject(ep);
		return;
	}
}

//-------------------------------------------------------------------------------------
void Dbmgr::onComponentActiveTickTimeout()
{
	if (centermgrInfo_ != NULL && centermgrInfo_->pChannel != NULL)
	{
		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		pBundle->newMessage(CentermgrInterface::onAppActiveTick);
		(*pBundle) << g_componentType;
		(*pBundle) << g_componentID;
		centermgrInfo_->pChannel->send(pBundle);
		INFO_MSG(fmt::format("Dbmgr::onComponentActiveTickTimeout: CentermgrInterface tick at time {}\n", timestamp()));
	}
}

void Dbmgr::onAppActiveTick(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID)
{
	// TODO: 需要验证centermgr的身份
	if (componentType == CENTERMGR_TYPE && centermgrInfo_ != NULL && centermgrInfo_->pChannel != NULL)
	{
		INFO_MSG(fmt::format("Dbmgr::onAppActiveTick: app({}:{}) tick at time {}\n", COMPONENT_NAME_EX(componentType), pChannel->c_str(), timestamp()));
		centermgrInfo_->pChannel->updateLastReceivedTime();
		return;
	}

	ServerApp::onAppActiveTick(pChannel, componentType, componentID);
}

//-------------------------------------------------------------------------------------
bool Dbmgr::initializeWatcher()
{
	WATCH_OBJECT("numWrittenEntity", numWrittenEntity_);
	WATCH_OBJECT("numRemovedEntity", numRemovedEntity_);
	WATCH_OBJECT("numQueryEntity", numQueryEntity_);
	WATCH_OBJECT("numExecuteRawDatabaseCommand", numExecuteRawDatabaseCommand_);
	WATCH_OBJECT("numCreatedAccount", numCreatedAccount_);

	KBEUnordered_map<std::string, Buffered_DBTasks>::iterator bditer = bufferedDBTasksMaps_.begin();
	for (; bditer != bufferedDBTasksMaps_.end(); ++bditer)
	{
		WATCH_OBJECT(fmt::format("DBThreadPool/{}/dbid_tasksSize", bditer->first).c_str(), &bditer->second, &Buffered_DBTasks::dbid_tasksSize);
		WATCH_OBJECT(fmt::format("DBThreadPool/{}/entityid_tasksSize", bditer->first).c_str(), &bditer->second, &Buffered_DBTasks::entityid_tasksSize);
		WATCH_OBJECT(fmt::format("DBThreadPool/{}/printBuffered_dbid", bditer->first).c_str(), &bditer->second, &Buffered_DBTasks::printBuffered_dbid);
		WATCH_OBJECT(fmt::format("DBThreadPool/{}/printBuffered_entityID", bditer->first).c_str(), &bditer->second, &Buffered_DBTasks::printBuffered_entityID);
	}

	return ServerApp::initializeWatcher() && DBUtil::initializeWatcher();
}

//-------------------------------------------------------------------------------------
bool Dbmgr::run()
{
	return PythonApp::run();
}

//-------------------------------------------------------------------------------------
void Dbmgr::handleTimeout(TimerHandle handle, void * arg)
{
	PythonApp::handleTimeout(handle, arg);

	switch (reinterpret_cast<uintptr>(arg))
	{
		case TIMEOUT_TICK:
			this->handleMainTick();
			break;
		case TIMEOUT_CHECK_STATUS:
			this->handleCheckStatusTick();
			break;
		default:
			break;
	}
}

//-------------------------------------------------------------------------------------
void Dbmgr::handleMainTick()
{
	AUTO_SCOPED_PROFILE("mainTick");
	
	 //time_t t = ::time(NULL);
	 //DEBUG_MSG("Dbmgr::handleGameTick[%"PRTime"]:%u\n", t, time_);
	
	threadPool_.onMainThreadTick();
	DBUtil::handleMainTick();
	networkInterface().processChannels(&DbmgrInterface::messageHandlers);
}

//-------------------------------------------------------------------------------------
void Dbmgr::handleCheckStatusTick()
{
	// 检查丢失的组件进程，如果在一段时间之内仍然无法发现，需要清理数据库中entitylog
	if (loseBaseappts_.size() > 0)
	{
		std::map<COMPONENT_ID, uint64>::iterator iter = loseBaseappts_.begin();
		for (; iter != loseBaseappts_.end();)
		{
			if (timestamp() > iter->second)
			{
				Components::ComponentInfos* cinfo = Components::getSingleton().findComponent(iter->first);
				if (!cinfo)
				{
					ENGINE_COMPONENT_INFO& dbcfg = g_kbeSrvConfig.getDBMgr();
					std::vector<DBInterfaceInfo>::iterator dbinfo_iter = dbcfg.dbInterfaceInfos.begin();
					for (; dbinfo_iter != dbcfg.dbInterfaceInfos.end(); ++dbinfo_iter)
					{
						std::string dbInterfaceName = dbinfo_iter->name;

						DBUtil::pThreadPool(dbInterfaceName)->
							addTask(new DBTaskEraseBaseappEntityLog(iter->first));
					}
				}

				loseBaseappts_.erase(iter++);
			}
			else
			{
				++iter;
			}
		}
	}
}

//-------------------------------------------------------------------------------------
bool Dbmgr::initializeBegin()
{
	idServer_.set_range_step(g_kbeSrvConfig.getDBMgr().ids_increasing_range);
	return true;
}

//-------------------------------------------------------------------------------------
bool Dbmgr::inInitialize()
{
	// 初始化所有扩展模块
	// assets/scripts/
	if (!PythonApp::inInitialize())
		return false;

	std::vector<PyTypeObject*>	scriptBaseTypes;
	if(!EntityDef::initialize(scriptBaseTypes, componentType_)){
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
bool Dbmgr::initializeEnd()
{
	PythonApp::initializeEnd();

	// 添加一个timer， 每秒检查一些状态
	loopCheckTimerHandle_ = this->dispatcher().addTimer(1000000, this,
							reinterpret_cast<void *>(TIMEOUT_CHECK_STATUS));

	mainProcessTimer_ = this->dispatcher().addTimer(1000000 / 50, this,
							reinterpret_cast<void *>(TIMEOUT_TICK));

	// 添加globalData, baseAppData, cellAppData支持
	pGlobalData_ = new GlobalDataServer(GlobalDataServer::GLOBAL_DATA);
	pBaseAppData_ = new GlobalDataServer(GlobalDataServer::BASEAPP_DATA);
	pCellAppData_ = new GlobalDataServer(GlobalDataServer::CELLAPP_DATA);
	pCenterData_ = new GlobalDataServer(GlobalDataServer::CENTER_DATA);
	pGlobalData_->addConcernComponentType(CELLAPP_TYPE);
	pGlobalData_->addConcernComponentType(BASEAPP_TYPE);
	pBaseAppData_->addConcernComponentType(BASEAPP_TYPE);
	pCellAppData_->addConcernComponentType(CELLAPP_TYPE);
	pCenterData_->addConcernComponentType(CELLAPP_TYPE);
	pCenterData_->addConcernComponentType(BASEAPP_TYPE);

	INFO_MSG(fmt::format("Dbmgr::initializeEnd: digest({})\n", 
		EntityDef::md5().getDigestStr()));
	
	SCOPED_PROFILE(SCRIPTCALL_PROFILE);

	// 所有脚本都加载完毕
	PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(), 
										const_cast<char*>("onDBMgrReady"), 
										const_cast<char*>(""));

	if(pyResult != NULL)
		Py_DECREF(pyResult);
	else
		SCRIPT_ERROR_CHECK();

	pTelnetServer_ = new TelnetServer(&this->dispatcher(), &this->networkInterface());
	pTelnetServer_->pScript(&this->getScript());

	bool ret = pTelnetServer_->start(g_kbeSrvConfig.getDBMgr().telnet_passwd,
		g_kbeSrvConfig.getDBMgr().telnet_deflayer,
		g_kbeSrvConfig.getDBMgr().telnet_port);

	Components::getSingleton().extraData4(pTelnetServer_->port());
	
	return ret && initInterfacesHandler() && initDB();
}

//-------------------------------------------------------------------------------------		
void Dbmgr::onInstallPyModules()
{
	PyObject * module = getScript().getModule();

	for (int i = 0; i < SERVER_ERR_MAX; i++)
	{
		if(PyModule_AddIntConstant(module, SERVER_ERR_STR[i], i))
		{
			ERROR_MSG( fmt::format("Dbmgr::onInstallPyModules: Unable to set KBEngine.{}.\n", SERVER_ERR_STR[i]));
		}
	}
}

//-------------------------------------------------------------------------------------		
bool Dbmgr::initInterfacesHandler()
{
	std::vector< Network::Address > addresses = g_kbeSrvConfig.interfacesAddrs();
	std::string type = addresses.size() == 0 ? "dbmgr" : "interfaces";

	if (type == "dbmgr")
	{
		InterfacesHandler* pInterfacesHandler = InterfacesHandlerFactory::create(type);

		INFO_MSG(fmt::format("Dbmgr::initInterfacesHandler: interfaces addr({}), accountType:({}), chargeType:({}).\n",
			Network::Address::NONE.c_str(),
			type,
			type));

		if (!pInterfacesHandler->initialize())
			return false;

		pInterfacesHandlers_.push_back(pInterfacesHandler);
	}
	else
	{
		std::vector< Network::Address >::iterator iter = addresses.begin();
		for (; iter != addresses.end(); ++iter)
		{
			InterfacesHandler* pInterfacesHandler = InterfacesHandlerFactory::create(type);

			const Network::Address& addr = (*iter);

			INFO_MSG(fmt::format("Dbmgr::initInterfacesHandler: interfaces addr({}), accountType:({}), chargeType:({}).\n",
				addr.c_str(),
				type,
				type));

			((InterfacesHandler_Interfaces*)pInterfacesHandler)->setAddr(addr);

			if (!pInterfacesHandler->initialize())
				return false;

			pInterfacesHandlers_.push_back(pInterfacesHandler);
		}
	}

	return pInterfacesHandlers_.size() > 0;
}

//-------------------------------------------------------------------------------------		
bool Dbmgr::initDB()
{
	ScriptDefModule* pModule = EntityDef::findScriptModule(DBUtil::accountScriptName());
	if(pModule == NULL)
	{
		ERROR_MSG(fmt::format("Dbmgr::initDB(): not found account script[{}]!\n", 
			DBUtil::accountScriptName()));

		return false;
	}

	ENGINE_COMPONENT_INFO& dbcfg = g_kbeSrvConfig.getDBMgr();
	if (dbcfg.dbInterfaceInfos.size() == 0)
	{
		ERROR_MSG(fmt::format("DBUtil::initialize: not found dbInterface! (kbengine[_defs].xml->dbmgr->databaseInterfaces)\n"));
		return false;
	}

	if (!DBUtil::initialize())
	{
		ERROR_MSG("Dbmgr::initDB(): can't initialize dbInterface!\n");
		return false;
	}

	bool hasDefaultInterface = false;

	std::vector<DBInterfaceInfo>::iterator dbinfo_iter = dbcfg.dbInterfaceInfos.begin();
	for (; dbinfo_iter != dbcfg.dbInterfaceInfos.end(); ++dbinfo_iter)
	{
		Buffered_DBTasks buffered_DBTasks;
		bufferedDBTasksMaps_.insert(std::make_pair((*dbinfo_iter).name, buffered_DBTasks));
		BUFFERED_DBTASKS_MAP::iterator buffered_DBTasks_iter = bufferedDBTasksMaps_.find((*dbinfo_iter).name);
		buffered_DBTasks_iter->second.dbInterfaceName((*dbinfo_iter).name);
	}

	for (dbinfo_iter = dbcfg.dbInterfaceInfos.begin(); dbinfo_iter != dbcfg.dbInterfaceInfos.end(); ++dbinfo_iter)
	{
		DBInterface* pDBInterface = DBUtil::createInterface((*dbinfo_iter).name);
		if(pDBInterface == NULL)
		{
			ERROR_MSG("Dbmgr::initDB(): can't create dbInterface!\n");
			return false;
		}

		bool ret = DBUtil::initInterface(pDBInterface);
		pDBInterface->detach();
		SAFE_RELEASE(pDBInterface);

		if(!ret)
			return false;

		if (std::string("default") == (*dbinfo_iter).name)
			hasDefaultInterface = true;
	}

	if (!hasDefaultInterface)
	{
		ERROR_MSG("Dbmgr::initDB(): \"default\" dbInterface was not found! (kbengine[_defs].xml->dbmgr->databaseInterfaces)\n");
		return false;
	}

	if(pUpdateDBServerLogHandler_ == NULL)
		pUpdateDBServerLogHandler_ = new UpdateDBServerLogHandler();

	return true;
}

//-------------------------------------------------------------------------------------
void Dbmgr::finalise()
{
	SAFE_RELEASE(pUpdateDBServerLogHandler_);
	
	SAFE_RELEASE(pGlobalData_);
	SAFE_RELEASE(pBaseAppData_);
	SAFE_RELEASE(pCellAppData_);
	SAFE_RELEASE(pCenterData_);

	if (pTelnetServer_)
	{
		pTelnetServer_->stop();
		SAFE_RELEASE(pTelnetServer_);
	}

	DBUtil::finalise();
	PythonApp::finalise();
}

//-------------------------------------------------------------------------------------
InterfacesHandler* Dbmgr::findBestInterfacesHandler()
{
	if (pInterfacesHandlers_.size() == 0)
		return NULL;

	static size_t i = 0;
	
	return pInterfacesHandlers_[i++ % pInterfacesHandlers_.size()];
}

//-------------------------------------------------------------------------------------
void Dbmgr::onReqAllocEntityID(Network::Channel* pChannel, COMPONENT_ORDER componentType, COMPONENT_ID componentID)
{
	KBEngine::COMPONENT_TYPE ct = static_cast<KBEngine::COMPONENT_TYPE>(componentType);

	// 获取一个id段 并传输给IDClient
	std::pair<ENTITY_ID, ENTITY_ID> idRange = idServer_.allocRange();
	Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);

	if(ct == BASEAPP_TYPE)
		(*pBundle).newMessage(BaseappInterface::onReqAllocEntityID);
	else	
		(*pBundle).newMessage(CellappInterface::onReqAllocEntityID);

	(*pBundle) << idRange.first;
	(*pBundle) << idRange.second;
	pChannel->send(pBundle);
}

void Dbmgr::onRegisterCentermgr(Network::Channel * pChannel, COMPONENT_ORDER centerID)
{
	ServerApp::onRegisterCentermgr(pChannel, centerID);

	// 通知所有 baseapp 和 cellapp
	Components::COMPONENTS& componets = Components::getSingleton().getComponents(BASEAPP_TYPE);
	Components::COMPONENTS::iterator iter = componets.begin();
	for (; iter != componets.end(); iter++)
	{
		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		pBundle->newMessage(BaseappInterface::onRegisterCentermgr);
		(*pBundle) << centerID;
		iter->pChannel->send(pBundle);
	}

	componets = Components::getSingleton().getComponents(CELLAPP_TYPE);
	iter = componets.begin();
	for (; iter != componets.end(); iter++)
	{
		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		pBundle->newMessage(CellappInterface::onRegisterCentermgr);
		(*pBundle) << centerID;
		iter->pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
void Dbmgr::onRegisterNewApp(Network::Channel* pChannel, int32 uid, std::string& username, 
						COMPONENT_TYPE componentType, COMPONENT_ID componentID, COMPONENT_ORDER globalorderID, COMPONENT_ORDER grouporderID,
						uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx)
{
	if(pChannel->isExternal())
		return;

	ServerApp::onRegisterNewApp(pChannel, uid, username, componentType, componentID, globalorderID, grouporderID,
						intaddr, intport, extaddr, extport, extaddrEx);

	KBEngine::COMPONENT_TYPE tcomponentType = (KBEngine::COMPONENT_TYPE)componentType;
	
	COMPONENT_ORDER startGroupOrder = 1;
	COMPONENT_ORDER startGlobalOrder = Components::getSingleton().getGlobalOrderLog()[getUserUID()];

	if(grouporderID > 0)
		startGroupOrder = grouporderID;

	if(globalorderID > 0)
		startGlobalOrder = globalorderID;

	if(pSyncAppDatasHandler_ == NULL)
		pSyncAppDatasHandler_ = new SyncAppDatasHandler(this->networkInterface());

	// 下一步:
	// 如果是连接到dbmgr则需要等待接收app初始信息
	// 例如：初始会分配entityID段以及这个app启动的顺序信息（是否第一个baseapp启动）
	if(tcomponentType == BASEAPP_TYPE || 
		tcomponentType == CELLAPP_TYPE || 
		tcomponentType == LOGINAPP_TYPE)
	{
		switch(tcomponentType)
		{
		case BASEAPP_TYPE:
			{
				if(grouporderID <= 0)
					startGroupOrder = Components::getSingleton().getBaseappGroupOrderLog()[getUserUID()];
			}
			break;
		case CELLAPP_TYPE:
			{
				if(grouporderID <= 0)
					startGroupOrder = Components::getSingleton().getCellappGroupOrderLog()[getUserUID()];
			}
			break;
		case LOGINAPP_TYPE:
			if(grouporderID <= 0)
				startGroupOrder = Components::getSingleton().getLoginappGroupOrderLog()[getUserUID()];

			break;
		default:
			break;
		}
	}

	pSyncAppDatasHandler_->pushApp(componentID, startGroupOrder, startGlobalOrder);

	// 如果是baseapp或者cellapp则将自己注册到所有其他baseapp和cellapp
	if(tcomponentType == BASEAPP_TYPE || 
		tcomponentType == CELLAPP_TYPE)
	{
		KBEngine::COMPONENT_TYPE broadcastCpTypes[2] = {BASEAPP_TYPE, CELLAPP_TYPE};
		for(int idx = 0; idx < 2; ++idx)
		{
			Components::COMPONENTS& cts = Components::getSingleton().getComponents(broadcastCpTypes[idx]);
			Components::COMPONENTS::iterator fiter = cts.begin();
			for(; fiter != cts.end(); ++fiter)
			{
				if((*fiter).cid == componentID)
					continue;

				Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
				ENTITTAPP_COMMON_NETWORK_MESSAGE(broadcastCpTypes[idx], (*pBundle), onGetEntityAppFromDbmgr);
				
				if(tcomponentType == BASEAPP_TYPE)
				{
					BaseappInterface::onGetEntityAppFromDbmgrArgs11::staticAddToBundle((*pBundle), 
						uid, username, componentType, componentID, startGlobalOrder, startGroupOrder,
							intaddr, intport, extaddr, extport, g_kbeSrvConfig.getConfig().externalAddress);
				}
				else
				{
					CellappInterface::onGetEntityAppFromDbmgrArgs11::staticAddToBundle((*pBundle), 
						uid, username, componentType, componentID, startGlobalOrder, startGroupOrder,
							intaddr, intport, extaddr, extport, g_kbeSrvConfig.getConfig().externalAddress);
				}
				
				KBE_ASSERT((*fiter).pChannel != NULL);
				(*fiter).pChannel->send(pBundle);
			}
		}
	}
}

//-------------------------------------------------------------------------------------
void Dbmgr::onGlobalDataClientLogon(Network::Channel* pChannel, COMPONENT_TYPE componentType)
{
	if(BASEAPP_TYPE == componentType)
	{
		pBaseAppData_->onGlobalDataClientLogon(pChannel, componentType);
		pGlobalData_->onGlobalDataClientLogon(pChannel, componentType);
		pCenterData_->onGlobalDataClientLogon(pChannel, componentType);
	}
	else if(CELLAPP_TYPE == componentType)
	{
		pGlobalData_->onGlobalDataClientLogon(pChannel, componentType);
		pCellAppData_->onGlobalDataClientLogon(pChannel, componentType);
		pCenterData_->onGlobalDataClientLogon(pChannel, componentType);
	}
	else
	{
		ERROR_MSG(fmt::format("Dbmgr::onGlobalDataClientLogon: nonsupport {}!\n",
			COMPONENT_NAME_EX(componentType)));
	}
}

//-------------------------------------------------------------------------------------
void Dbmgr::onBroadcastGlobalDataChanged(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	uint8 dataType;
	std::string key, value;
	bool isDelete;
	COMPONENT_TYPE componentType;

	s >> dataType;
	s >> isDelete;

	s.readBlob(key);

	if(!isDelete)
	{
		s.readBlob(value);
	}

	s >> componentType;

	switch(dataType)
	{
	case GlobalDataServer::GLOBAL_DATA:
		if(isDelete)
			pGlobalData_->del(pChannel, componentType, key);
		else
			pGlobalData_->write(pChannel, componentType, key, value);
		break;
	case GlobalDataServer::BASEAPP_DATA:
		if(isDelete)
			pBaseAppData_->del(pChannel, componentType, key);
		else
			pBaseAppData_->write(pChannel, componentType, key, value);
		break;
	case GlobalDataServer::CELLAPP_DATA:
		if(isDelete)
			pCellAppData_->del(pChannel, componentType, key);
		else
			pCellAppData_->write(pChannel, componentType, key, value);
		break;
	case GlobalDataServer::CENTER_DATA:
		if (isDelete)
			pCenterData_->del(pChannel, componentType, key);
		else
			pCenterData_->write(pChannel, componentType, key, value);

		// 除了本服广播，还需通过中枢服务器广播
		if (centermgrInfo_)
		{
			Network::Bundle *centerDataBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
			(*centerDataBundle).newMessage(CentermgrInterface::onBroadcastCenterDataChanged);

			(*centerDataBundle).append(s);

			(*centerDataBundle) << dataType;
			(*centerDataBundle) << isDelete;
			ArraySize len = key.length();
			(*centerDataBundle) << len;
			(*centerDataBundle).assign(key.data(), len);
			if (!isDelete)
			{
				len = value.length();
				(*centerDataBundle) << len;
				(*centerDataBundle).assign(value.data(), len);
			}
			(*centerDataBundle) << componentType;

			centermgrInfo_->pChannel->send(centerDataBundle);
		}
		else
		{
			WARNING_MSG("Dbmgr::onBroadcastGlobalDataChanged: centermgr is not enable.");
		}

		break;
	default:
		KBE_ASSERT(false && "dataType is error!\n");
		break;
	};
}

//-------------------------------------------------------------------------------------
void Dbmgr::onBroadcastCenterDataChanged(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	std::string key, value;
	bool isDelete;
	COMPONENT_TYPE componentType;

	s >> isDelete;

	s.readBlob(key);

	if (!isDelete)
	{
		s.readBlob(value);
	}

	s >> componentType;

	if (isDelete)
		pCenterData_->del(pChannel, componentType, key);
	else
		pCenterData_->write(pChannel, componentType, key, value);
}

//-------------------------------------------------------------------------------------
void Dbmgr::reqCreateAccount(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	std::string registerName, password, datas;
	uint8 uatype = 0;

	s >> registerName >> password >> uatype;
	s.readBlob(datas);

	if(registerName.size() == 0)
	{
		ERROR_MSG("Dbmgr::reqCreateAccount: registerName is empty.\n");
		return;
	}

	findBestInterfacesHandler()->createAccount(pChannel, registerName, password, datas, ACCOUNT_TYPE(uatype));
	numCreatedAccount_++;
}

//-------------------------------------------------------------------------------------
void Dbmgr::onCreateAccountCBFromInterfaces(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	findBestInterfacesHandler()->onCreateAccountCB(s);
}

//-------------------------------------------------------------------------------------
void Dbmgr::onAccountLogin(Network::Channel* pChannel, KBEngine::MemoryStream& s) 
{
	std::string loginName, password, datas;
	s >> loginName >> password;
	s.readBlob(datas);

	if(loginName.size() == 0)
	{
		ERROR_MSG("Dbmgr::onAccountLogin: loginName is empty.\n");
		return;
	}

	findBestInterfacesHandler()->loginAccount(pChannel, loginName, password, datas);
}

//-------------------------------------------------------------------------------------
void Dbmgr::onLoginAccountCBBFromInterfaces(Network::Channel* pChannel, KBEngine::MemoryStream& s) 
{
	findBestInterfacesHandler()->onLoginAccountCB(s);
}

//-------------------------------------------------------------------------------------
void Dbmgr::queryAccount(Network::Channel* pChannel, 
						 std::string& accountName, 
						 std::string& password,
						 bool needCheckPassword,
						 COMPONENT_ID componentID,
						 ENTITY_ID entityID,
						 DBID entityDBID, 
						 uint32 ip, 
						 uint16 port)
{
	if(accountName.size() == 0)
	{
		ERROR_MSG("Dbmgr::queryAccount: accountName is empty.\n");
		return;
	}

	Buffered_DBTasks* pBuffered_DBTasks = 
		findBufferedDBTask(Dbmgr::getSingleton().selectAccountDBInterfaceName(accountName));

	if (!pBuffered_DBTasks)
	{
		ERROR_MSG(fmt::format("Dbmgr::queryAccount: not found dbInterface({})!\n", 
			Dbmgr::getSingleton().selectAccountDBInterfaceName(accountName)));
		return;
	}

	pBuffered_DBTasks->addTask(new DBTaskQueryAccount(pChannel->addr(), accountName, password, needCheckPassword,
		componentID, entityID, entityDBID, ip, port));

	numQueryEntity_++;
}

//-------------------------------------------------------------------------------------
void Dbmgr::onAccountOnline(Network::Channel* pChannel, 
							std::string& accountName, 
							COMPONENT_ID componentID, 
							ENTITY_ID entityID)
{
	// bufferedDBTasks_.addTask(new DBTaskAccountOnline(pChannel->addr(), 
	//	accountName, componentID, entityID));
}

//-------------------------------------------------------------------------------------
void Dbmgr::onEntityOffline(Network::Channel* pChannel, DBID dbid, ENTITY_SCRIPT_UID sid, uint16 dbInterfaceIndex)
{
	Buffered_DBTasks* pBuffered_DBTasks = findBufferedDBTask(g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex));
	if (!pBuffered_DBTasks)
	{
		ERROR_MSG(fmt::format("Dbmgr::onEntityOffline: not found dbInterfaceIndex({})!\n", dbInterfaceIndex));
		return;
	}

	pBuffered_DBTasks->addTask(new DBTaskEntityOffline(pChannel->addr(), dbid, sid));
}

//-------------------------------------------------------------------------------------
void Dbmgr::executeRawDatabaseCommand(Network::Channel* pChannel, 
									  KBEngine::MemoryStream& s)
{
	ENTITY_ID entityID = -1;
	s >> entityID;

	uint16 dbInterfaceIndex = 0;
	s >> dbInterfaceIndex;

	std::string dbInterfaceName = g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex);
	if (dbInterfaceName.size() == 0)
	{
		ERROR_MSG(fmt::format("Dbmgr::executeRawDatabaseCommand: not found dbInterface({})!\n", dbInterfaceName));
		s.done();
		return;
	}

	if (entityID == -1)
	{
		thread::ThreadPool* pThreadPool = DBUtil::pThreadPool(dbInterfaceName);
		if (!pThreadPool)
		{
			ERROR_MSG(fmt::format("Dbmgr::executeRawDatabaseCommand: not found pThreadPool(dbInterface={})!\n", dbInterfaceName));
			s.done();
			return;
		}

		pThreadPool->addTask(new DBTaskExecuteRawDatabaseCommand(pChannel->addr(), s));
	}
	else
	{
		Buffered_DBTasks* pBuffered_DBTasks = findBufferedDBTask(dbInterfaceName);
		if (!pBuffered_DBTasks)
		{
			ERROR_MSG(fmt::format("Dbmgr::executeRawDatabaseCommand: not found pBuffered_DBTasks(dbInterface={})!\n", dbInterfaceName));
			s.done();
			return;
		}

		pBuffered_DBTasks->addTask(new DBTaskExecuteRawDatabaseCommandByEntity(pChannel->addr(), s, entityID));
	}

	s.done();

	++numExecuteRawDatabaseCommand_;
}

//-------------------------------------------------------------------------------------
void Dbmgr::writeEntity(Network::Channel* pChannel, 
						KBEngine::MemoryStream& s)
{
	ENTITY_ID eid;
	DBID entityDBID;
	COMPONENT_ID componentID;
	uint16 dbInterfaceIndex;

	s >> componentID >> eid >> entityDBID >> dbInterfaceIndex;

	Buffered_DBTasks* pBuffered_DBTasks = findBufferedDBTask(g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex));
	if (!pBuffered_DBTasks)
	{
		ERROR_MSG(fmt::format("Dbmgr::writeEntity: not found dbInterfaceIndex({})!\n", dbInterfaceIndex));
		s.done();
		return;
	}

	pBuffered_DBTasks->addTask(new DBTaskWriteEntity(pChannel->addr(), componentID, eid, entityDBID, s));
	s.done();

	++numWrittenEntity_;
}

//-------------------------------------------------------------------------------------
void Dbmgr::removeEntity(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	ENTITY_ID eid;
	DBID entityDBID;
	COMPONENT_ID componentID;
	uint16 dbInterfaceIndex;

	s >> dbInterfaceIndex >> componentID >> eid >> entityDBID;
	KBE_ASSERT(entityDBID > 0);

	Buffered_DBTasks* pBuffered_DBTasks = findBufferedDBTask(g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex));
	if (!pBuffered_DBTasks)
	{
		ERROR_MSG(fmt::format("Dbmgr::removeEntity: not found dbInterfaceIndex({})!\n", dbInterfaceIndex));
		s.done();
		return;
	}

	pBuffered_DBTasks->addTask(new DBTaskRemoveEntity(pChannel->addr(),
		componentID, eid, entityDBID, s));

	s.done();

	++numRemovedEntity_;
}

//-------------------------------------------------------------------------------------
void Dbmgr::entityAutoLoad(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	COMPONENT_ID componentID;
	ENTITY_SCRIPT_UID entityType;
	ENTITY_ID start;
	ENTITY_ID end;
	uint16 dbInterfaceIndex = 0;

	s >> dbInterfaceIndex >> componentID >> entityType >> start >> end;

	DBUtil::pThreadPool(g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex))->
		addTask(new DBTaskEntityAutoLoad(pChannel->addr(), componentID, entityType, start, end));
}

//-------------------------------------------------------------------------------------
void Dbmgr::deleteEntityByDBID(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	COMPONENT_ID componentID;
	ENTITY_SCRIPT_UID sid;
	CALLBACK_ID callbackID = 0;
	DBID entityDBID;
	uint16 dbInterfaceIndex = 0;

	s >> dbInterfaceIndex >> componentID >> entityDBID >> callbackID >> sid;
	KBE_ASSERT(entityDBID > 0);

	DBUtil::pThreadPool(g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex))->
		addTask(new DBTaskDeleteEntityByDBID(pChannel->addr(), componentID, entityDBID, callbackID, sid));
}

//-------------------------------------------------------------------------------------
void Dbmgr::lookUpEntityByDBID(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	COMPONENT_ID componentID;
	ENTITY_SCRIPT_UID sid;
	CALLBACK_ID callbackID = 0;
	DBID entityDBID;
	uint16 dbInterfaceIndex = 0;

	s >> dbInterfaceIndex >> componentID >> entityDBID >> callbackID >> sid;
	KBE_ASSERT(entityDBID > 0);

	DBUtil::pThreadPool(g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex))->
		addTask(new DBTaskLookUpEntityByDBID(pChannel->addr(), componentID, entityDBID, callbackID, sid));
}

//-------------------------------------------------------------------------------------
void Dbmgr::queryEntity(Network::Channel* pChannel, uint16 dbInterfaceIndex, COMPONENT_ID componentID, int8 queryMode, DBID dbid,
	std::string& entityType, CALLBACK_ID callbackID, ENTITY_ID entityID)
{
	bufferedDBTasksMaps_[g_kbeSrvConfig.dbInterfaceIndex2dbInterfaceName(dbInterfaceIndex)].
		addTask(new DBTaskQueryEntity(pChannel->addr(), queryMode, entityType, dbid, componentID, callbackID, entityID));

	numQueryEntity_++;
}

//-------------------------------------------------------------------------------------
void Dbmgr::syncEntityStreamTemplate(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	int rpos = s.rpos();
	EntityTables::ENTITY_TABLES_MAP::iterator iter = EntityTables::sEntityTables.begin();
	for (; iter != EntityTables::sEntityTables.end(); ++iter)
	{
		KBEAccountTable* pTable =
			static_cast<KBEAccountTable*>(iter->second.findKBETable(KBE_TABLE_PERFIX "_accountinfos"));

		KBE_ASSERT(pTable);

		s.rpos(rpos);
		pTable->accountDefMemoryStream(s);
	}

	s.done();
}

//-------------------------------------------------------------------------------------
void Dbmgr::charge(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	findBestInterfacesHandler()->charge(pChannel, s);
}

//-------------------------------------------------------------------------------------
void Dbmgr::onChargeCB(Network::Channel* pChannel, KBEngine::MemoryStream& s)
{
	findBestInterfacesHandler()->onChargeCB(s);
}

//-------------------------------------------------------------------------------------
void Dbmgr::eraseClientReq(Network::Channel* pChannel, std::string& logkey)
{
	std::vector<InterfacesHandler*>::iterator iter = pInterfacesHandlers_.begin();
	for (; iter != pInterfacesHandlers_.end(); ++iter)
		(*iter)->eraseClientReq(pChannel, logkey);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountActivate(Network::Channel* pChannel, std::string& scode)
{
	INFO_MSG(fmt::format("Dbmgr::accountActivate: code={}.\n", scode));
	findBestInterfacesHandler()->accountActivate(pChannel, scode);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountReqResetPassword(Network::Channel* pChannel, std::string& accountName)
{
	INFO_MSG(fmt::format("Dbmgr::accountReqResetPassword: accountName={}.\n", accountName));
	findBestInterfacesHandler()->accountReqResetPassword(pChannel, accountName);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountResetPassword(Network::Channel* pChannel, std::string& accountName, std::string& newpassword, std::string& code)
{
	INFO_MSG(fmt::format("Dbmgr::accountResetPassword: accountName={}.\n", accountName));
	findBestInterfacesHandler()->accountResetPassword(pChannel, accountName, newpassword, code);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountReqBindMail(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
							   std::string& password, std::string& email)
{
	INFO_MSG(fmt::format("Dbmgr::accountReqBindMail: accountName={}, email={}.\n", accountName, email));
	findBestInterfacesHandler()->accountReqBindMail(pChannel, entityID, accountName, password, email);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountBindMail(Network::Channel* pChannel, std::string& username, std::string& scode)
{
	INFO_MSG(fmt::format("Dbmgr::accountBindMail: username={}, scode={}.\n", username, scode));
	findBestInterfacesHandler()->accountBindMail(pChannel, username, scode);
}

//-------------------------------------------------------------------------------------
void Dbmgr::accountNewPassword(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
							   std::string& password, std::string& newpassword)
{
	INFO_MSG(fmt::format("Dbmgr::accountNewPassword: accountName={}.\n", accountName));
	findBestInterfacesHandler()->accountNewPassword(pChannel, entityID, accountName, password, newpassword);
}

//-------------------------------------------------------------------------------------
std::string Dbmgr::selectAccountDBInterfaceName(const std::string& name)
{
	std::string dbInterfaceName = "default";

	// 把请求交由脚本处理
	SCOPED_PROFILE(SCRIPTCALL_PROFILE);
	PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(),
		const_cast<char*>("onSelectAccountDBInterface"),
		const_cast<char*>("s"),
		name.c_str());

	if (pyResult != NULL)
	{
		wchar_t* PyUnicode_AsWideCharStringRet0 = PyUnicode_AsWideCharString(pyResult, NULL);
		char* ccattr = strutil::wchar2char(PyUnicode_AsWideCharStringRet0);
		dbInterfaceName = ccattr;
		PyMem_Free(PyUnicode_AsWideCharStringRet0);
		Py_DECREF(pyResult);
		free(ccattr);
	}
	else
	{
		SCRIPT_ERROR_CHECK();
	}

	if (dbInterfaceName == "" || g_kbeSrvConfig.dbInterface(dbInterfaceName) == NULL)
	{
		ERROR_MSG(fmt::format("Dbmgr::selectAccountDBInterfaceName: not found dbInterface({}), accountName={}.\n", dbInterfaceName, name));
		return "default";
	}

	return dbInterfaceName;
}

//-------------------------------------------------------------------------------------
void Dbmgr::onChannelDeregister(Network::Channel * pChannel)
{
	// 如果是app死亡了
	if (pChannel->isInternal())
	{
		Components::ComponentInfos* cinfo = Components::getSingleton().findComponent(pChannel);
		if (cinfo)
		{
			if (cinfo->componentType == BASEAPP_TYPE)
			{
				loseBaseappts_[cinfo->cid] = timestamp() + uint64(60 * stampsPerSecond());
				WARNING_MSG(fmt::format("Dbmgr::onChannelDeregister(): If the process cannot be resumed, the entitylog(baseapp={}) will be cleaned up after 60 seconds!\n", cinfo->cid));
			}
		}
	}

	ServerApp::onChannelDeregister(pChannel);

	if (isCentermgrChannel(pChannel))
	{
		INFO_MSG(fmt::format("Dbmgr::onChannelDeregister: {} :centermgr({}) Abnormal exit(reason={})! Channel(timestamp={}, lastReceivedTime={})\n",
			COMPONENT_NAME_EX(CENTERMGR_TYPE), pChannel->c_str(), pChannel->condemnReason(), timestamp(), pChannel->lastReceivedTime()));

		SAFE_RELEASE(centermgrInfo_);
	}
}

//-------------------------------------------------------------------------------------
}
