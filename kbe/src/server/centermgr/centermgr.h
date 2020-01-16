#pragma once

#include "server/serverapp.h"
#include "common/singleton.h"

namespace KBEngine
{

	class Centermgr: public ServerApp,
					public Singleton<Centermgr>
	{
	public:
		Centermgr(Network::EventDispatcher& dispatcher,
			Network::NetworkInterface& ninterface,
			COMPONENT_TYPE componentType,
			COMPONENT_ID componentID);

		~Centermgr();
	};

}	// end namespace KBEngine