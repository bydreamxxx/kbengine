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

	virtual bool run();

	/* 网络接口
	 * 某个 app 请求注册
	*/
	void onAppRegister(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID,
		uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx);
};

}	// end namespace KBEngine