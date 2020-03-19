
#include "centermgr/centermgr.h"
#include "centermgr/centermgr_interface.h"
#include "network/message_handler.h"

#include "helper/debug_helper.h"

namespace KBEngine 
{

	ServerConfig g_serverConfig;
	KBE_SINGLETON_INIT(Centermgr);

	Centermgr::Centermgr(Network::EventDispatcher& dispatcher,
		Network::NetworkInterface& ninterface,
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID):
		ServerApp(dispatcher, ninterface, componentType, componentID)
	{
		KBEngine::Network::MessageHandlers::pMainMessageHandlers = &CentermgrInterface::messageHandlers;
	}

	Centermgr::~Centermgr()
	{}

	bool Centermgr::run()
	{
		return ServerApp::run();
	}

	void Centermgr::onAppRegister(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID, 
		uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx)
	{
		INFO_MSG("Centermgr::onAppRegister --->>>");
	}
}

