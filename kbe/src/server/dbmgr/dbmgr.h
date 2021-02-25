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

#ifndef KBE_DBMGR_H
#define KBE_DBMGR_H

#include "db_interface/db_threadpool.h"
#include "buffered_dbtasks.h"
#include "server/kbemain.h"
#include "pyscript/script.h"
#include "pyscript/pyobject_pointer.h"
#include "entitydef/entitydef.h"
#include "entitydef/entity_call.h"
#include "server/components.h"
#include "server/python_app.h"
#include "server/idallocate.h"
#include "server/serverconfig.h"
#include "server/globaldata_client.h"
#include "server/globaldata_server.h"
#include "common/timer.h"
#include "network/endpoint.h"
#include "resmgr/resmgr.h"
#include "thread/threadpool.h"

namespace KBEngine{

class DBInterface;
class TelnetServer;
class InterfacesHandler;
class SyncAppDatasHandler;
class UpdateDBServerLogHandler;

class Dbmgr :	public PythonApp, 
				public Singleton<Dbmgr>
{
public:
	enum TimeOutType
	{
		TIMEOUT_TICK = TIMEOUT_PYTHONAPP_MAX + 1,
		TIMEOUT_CHECK_STATUS
	};
	
	Dbmgr(Network::EventDispatcher& dispatcher, 
		Network::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~Dbmgr();
	
	bool run();
	
	void handleTimeout(TimerHandle handle, void * arg);
	void handleMainTick();
	void handleCheckStatusTick();

	/* 初始化相关接口 */
	bool initializeBegin();
	bool inInitialize();
	bool initializeEnd();
	void finalise();
	void onInstallPyModules();
	
	bool initInterfacesHandler();

	bool initDB();

	virtual bool canShutdown();

	virtual void onShutdownBegin();
	virtual void onShutdownEnd();

	virtual void onAllComponentFound();
	bool isCentermgrEnable();
	bool isCentermgrChannel(Network::Channel *channel);
	void findCentermgr();
	
	virtual void onComponentActiveTickTimeout();

	/** 获取ID服务器指针 */
	IDServer<ENTITY_ID>& idServer(void){ return idServer_; }

	/** 网络接口
	某个app向本app告知处于活动状态。
	*/
	virtual void onAppActiveTick(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID);

	/** 网络接口
		请求分配一个ENTITY_ID段
	*/
	void onReqAllocEntityID(Network::Channel* pChannel, COMPONENT_ORDER componentType, COMPONENT_ID componentID);

	/** 网络接口
		dbmgr 注册到 centermgr 成功
	*/
	virtual void onRegisterCentermgr(Network::Channel* pChannel, COMPONENT_ORDER centerID);

	/** 网络接口
	收到跨服call请求, 由某个app上的entityCallCrossServer发起(只限与服务器内部使用)
	*/
	void requestEntityCallCrossServer(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
	收到跨服call请求, 由某个app上的entityCallCrossServer发起,调用到本服某个app
	*/
	void onEntityCallCrossServer(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
	跨服登录请求
	*/
	void requestAcrossServer(Network::Channel *pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
	接收跨服登录请求
	*/
	void receiveAcrossServerRequest(Network::Channel *pChannel, KBEngine::MemoryStream& s);

	/* 网络接口
	* 请求跨服登录成功
	*/
	void requestAcrossServerSuccess(Network::Channel *pChannel, KBEngine::MemoryStream& s);

	/* 网络接口
	* 收到跨服请求成功的通知
	*/
	void receiveAcrossServerSuccess(Network::Channel *pChannel, KBEngine::MemoryStream& s);

	/* 网络接口
		注册一个新激活的baseapp或者cellapp或者dbmgr
		通常是一个新的app被启动了， 它需要向某些组件注册自己。
	*/
	virtual void onRegisterNewApp(Network::Channel* pChannel, 
							int32 uid, 
							std::string& username, 
							COMPONENT_TYPE componentType, COMPONENT_ID componentID, COMPONENT_ORDER globalorderID, COMPONENT_ORDER grouporderID,
							uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx);


	/** 网络接口
		dbmgr广播global数据的改变
	*/
	void onGlobalDataClientLogon(Network::Channel* pChannel, COMPONENT_TYPE componentType);
	void onBroadcastGlobalDataChanged(Network::Channel* pChannel, KBEngine::MemoryStream& s);
	void onBroadcastCenterDataChanged(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		请求创建账号
	*/
	void reqCreateAccount(Network::Channel* pChannel, KBEngine::MemoryStream& s);
	void onCreateAccountCBFromInterfaces(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		请求擦除客户端请求任务
	*/
	void eraseClientReq(Network::Channel* pChannel, std::string& logkey);

	/** 网络接口
		一个新用户登录， 需要检查合法性
	*/
	void onAccountLogin(Network::Channel* pChannel, KBEngine::MemoryStream& s);
	void onLoginAccountCBBFromInterfaces(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		baseapp请求查询account信息
	*/
	void queryAccount(Network::Channel* pChannel, std::string& accountName, std::string& password, std::string& dbInterfaceName, bool needCheckPassword,
		COMPONENT_ID componentID, ENTITY_ID entityID, DBID entityDBID, uint32 ip, uint16 port);

	/** 网络接口
		实体自动加载功能
	*/
	void entityAutoLoad(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		账号从baseapp上线了
	*/
	void onAccountOnline(Network::Channel* pChannel, std::string& accountName, 
		COMPONENT_ID componentID, ENTITY_ID entityID);

	/** 网络接口
		entity-baseapp下线了
	*/
	void onEntityOffline(Network::Channel* pChannel, DBID dbid, ENTITY_SCRIPT_UID sid, uint16 dbInterfaceIndex);

	/** 网络接口
		执行数据库查询
	*/
	void executeRawDatabaseCommand(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		某个entity存档
	*/
	void writeEntity(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		删除某个entity的存档数据
	*/
	void removeEntity(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		通过dbid从数据库中删除一个实体的回调
	*/
	void deleteEntityByDBID(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		通过dbid查询一个实体是否从数据库检出
	*/
	void lookUpEntityByDBID(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		请求从db获取entity的所有数据
	*/
	void queryEntity(Network::Channel* pChannel, uint16 dbInterfaceIndex, COMPONENT_ID componentID, int8	queryMode, DBID dbid, 
		std::string& entityType, CALLBACK_ID callbackID, ENTITY_ID entityID);

	/** 网络接口
		同步entity流模板
	*/
	void syncEntityStreamTemplate(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	virtual bool initializeWatcher();

	/** 网络接口
		请求充值
	*/
	void charge(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 网络接口
		充值回调
	*/
	void onChargeCB(Network::Channel* pChannel, KBEngine::MemoryStream& s);


	/** 网络接口
		激活回调
	*/
	void accountActivate(Network::Channel* pChannel, std::string& scode);

	/** 网络接口
		账号重置密码
	*/
	void accountReqResetPassword(Network::Channel* pChannel, std::string& accountName);
	void accountResetPassword(Network::Channel* pChannel, std::string& accountName, 
		std::string& newpassword, std::string& code);

	/** 网络接口
		账号绑定邮箱
	*/
	void accountReqBindMail(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
		std::string& password, std::string& email);
	void accountBindMail(Network::Channel* pChannel, std::string& username, std::string& scode);

	/** 网络接口
		账号修改密码
	*/
	void accountNewPassword(Network::Channel* pChannel, ENTITY_ID entityID, std::string& accountName, 
		std::string& password, std::string& newpassword);
	
	SyncAppDatasHandler* pSyncAppDatasHandler() const { return pSyncAppDatasHandler_; }
	void pSyncAppDatasHandler(SyncAppDatasHandler* p){ pSyncAppDatasHandler_ = p; }

	std::string selectAccountDBInterfaceName(const std::string& name);

	Buffered_DBTasks* findBufferedDBTask(const std::string& dbInterfaceName)
	{
		BUFFERED_DBTASKS_MAP::iterator dbin_iter = bufferedDBTasksMaps_.find(dbInterfaceName);
		if (dbin_iter == bufferedDBTasksMaps_.end())
			return NULL;

		return &dbin_iter->second;
	}

	virtual void onChannelDeregister(Network::Channel * pChannel);

	PyObject* tryGetEntityByEntityCall(COMPONENT_ID componentID, ENTITY_ID eid);

	Network::Channel * findChannelByEntityCall(EntityCall & entitycall);

	InterfacesHandler* findBestInterfacesHandler();

protected:
	TimerHandle											loopCheckTimerHandle_;
	TimerHandle											mainProcessTimer_;

	// entityID分配服务端
	IDServer<ENTITY_ID>									idServer_;

	// globalData
	GlobalDataServer*									pGlobalData_;

	// baseAppData
	GlobalDataServer*									pBaseAppData_;

	// cellAppData
	GlobalDataServer*									pCellAppData_;

	// 跨服全局数据 KBEngine.centerData
	GlobalDataServer*									pCenterData_;

	typedef KBEUnordered_map<std::string, Buffered_DBTasks> BUFFERED_DBTASKS_MAP;
	BUFFERED_DBTASKS_MAP								bufferedDBTasksMaps_;

	// Statistics
	uint32												numWrittenEntity_;
	uint32												numRemovedEntity_;
	uint32												numQueryEntity_;
	uint32												numExecuteRawDatabaseCommand_;
	uint32												numCreatedAccount_;

	std::vector<InterfacesHandler*>						pInterfacesHandlers_;

	SyncAppDatasHandler*								pSyncAppDatasHandler_;
	UpdateDBServerLogHandler*							pUpdateDBServerLogHandler_;
	
	TelnetServer*										pTelnetServer_;

	std::map<COMPONENT_ID, uint64>						loseBaseappts_;

	Components::ComponentInfos							*centermgrInfo_;
};

}

#endif // KBE_DBMGR_H
