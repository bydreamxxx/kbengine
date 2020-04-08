
#include "centermgr/centermgr.h"
#include "centermgr/centermgr_interface.h"
#include "network/message_handler.h"
#include "dbmgr/dbmgr_interface.h"

#include "helper/debug_helper.h"

namespace KBEngine 
{

	ServerConfig g_serverConfig;
	KBE_SINGLETON_INIT(Centermgr);

	Centermgr::Centermgr(Network::EventDispatcher& dispatcher,
		Network::NetworkInterface& ninterface,
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID):
		ServerApp(dispatcher, ninterface, componentType, componentID),
		apps_(),
		tickTimer_()
	{
		KBEngine::Network::MessageHandlers::pMainMessageHandlers = &CentermgrInterface::messageHandlers;
	}

	Centermgr::~Centermgr()
	{
		for (APP_INFOS::iterator iter = apps_.begin(); iter != apps_.end(); ++iter)
		{
			SAFE_RELEASE(iter->second);
		}
		apps_.clear();

		tickTimer_.cancel();
	}

	bool Centermgr::run()
	{
		return ServerApp::run();
	}

	void Centermgr::mainProcess()
	{
		networkInterface_.processChannels(KBEngine::Network::MessageHandlers::pMainMessageHandlers);
	}

	bool Centermgr::initializeEnd()
	{
		tickTimer_ = this->dispatcher().addTimer(1000000 / g_kbeSrvConfig.gameUpdateHertz(), this,
			reinterpret_cast<void *>(CENTERMGR_TIMEOUT_TICK));

		return true;
	}

	void Centermgr::handleTimeout(TimerHandle handle, void * arg)
	{
		switch (reinterpret_cast<uintptr>(arg))
		{
		case CENTERMGR_TIMEOUT_TICK:
			mainProcess();
			return;
		default:
			break;
		}

		ServerApp::handleTimeout(handle, arg);
	}

	void Centermgr::onComponentActiveTickTimeout()
	{
		for (APP_INFOS::iterator iter = apps_.begin(); iter != apps_.end(); iter++)
		{
			if (iter->second->pChannel == NULL)
				continue;

			Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
			pBundle->newMessage(DbmgrInterface::onAppActiveTick);
			(*pBundle) << g_componentType;
			(*pBundle) << g_componentID;
			iter->second->pChannel->send(pBundle);
			INFO_MSG(fmt::format("Centermgr::onComponentActiveTickTimeout: app tick at time {}\n", timestamp()));
		}
	}

	void Centermgr::onChannelDeregister(Network::Channel * pChannel)
	{
		for (APP_INFOS::iterator iter = apps_.begin(); iter != apps_.end(); iter++)
		{
			if (iter->second->pChannel == pChannel)
			{
				SAFE_RELEASE(iter->second);
				apps_.erase(iter->first);
				return;
			}
		}

		ServerApp::onChannelDeregister(pChannel);
	}

	void Centermgr::onAppRegister(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID, 
		uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx)
	{
		// TODO: 检查连接进来的ip是否合法(如在配置表中）

		INFO_MSG(fmt::format("Centermgr::onAppRegister: componentType:{0}, "
			"componentID:{1}, intaddr:{2}, intport:{3}, extaddr:{4}, extport:{5},  from {6}.\n",
			COMPONENT_NAME_EX((COMPONENT_TYPE)componentType),
			componentID,
			inet_ntoa((struct in_addr&)intaddr),
			ntohs(intport),
			(extaddr != 0 ? inet_ntoa((struct in_addr&)extaddr) : "nonsupport"),
			ntohs(extport),
			pChannel->c_str()));

		APP_INFO *appInfo = new APP_INFO;
		appInfo->componentType = componentType;
		appInfo->pChannel = pChannel;
		appInfo->cid = componentID;
		appInfo->pIntAddr.reset(new Network::Address(intaddr, intport));	// pIntAddr 的类型是 KBEShared_ptr
		appInfo->pExtAddr.reset(new Network::Address(extaddr, extport));
		if (extaddrEx.size() > 0)
			strncpy(appInfo->externalAddressEx, extaddrEx.c_str(), MAX_NAME);

		apps_.insert(std::pair<COMPONENT_ID, APP_INFO*>(componentID, appInfo));
	}

	void Centermgr::onAppActiveTick(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID)
	{
		INFO_MSG(fmt::format("Centermgr::onAppActiveTick: app({}:{}) tick at time {}\n", COMPONENT_NAME_EX(componentType), pChannel->c_str(), timestamp()));
		APP_INFOS::iterator iter = apps_.find(componentID);
		if (iter != apps_.end())
		{
			if(iter->second->pChannel != NULL)
				iter->second->pChannel->updateLastReceivedTime();
		}
		else
		{
			ERROR_MSG(fmt::format("Centermgr::onAppActiveTick: cant find app by componentID({})\n", componentID));
		}
	}
}

