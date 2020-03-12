
#include "centermgr/centermgr.h"
#include "centermgr/centermgr_interface.h"
#include "network/message_handler.h"

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
}

