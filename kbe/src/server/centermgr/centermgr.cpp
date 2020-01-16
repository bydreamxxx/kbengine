
#include "centermgr.h"

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

	}

	Centermgr::~Centermgr()
	{}

}

