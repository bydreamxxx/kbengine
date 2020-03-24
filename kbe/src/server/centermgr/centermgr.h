#pragma once

#include <map>

#include "server/serverapp.h"
#include "common/singleton.h"
#include "common//timer.h"

namespace KBEngine
{

class Centermgr: public ServerApp,
				public Singleton<Centermgr>
{
public:
	enum TimeOutType
	{
		CENTERMGR_TIMEOUT_TICK = TIMEOUT_SERVERAPP_MAX + 1,	// +1 �����븸���ͻ
		CENTERMGR_TIMEOUT_MAX
	};

	Centermgr(Network::EventDispatcher& dispatcher,
		Network::NetworkInterface& ninterface,
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~Centermgr();

	virtual bool run();

	void mainProcess();

	virtual bool initializeEnd();

	virtual void onComponentActiveTickTimeout();

	virtual void onChannelDeregister(Network::Channel * pChannel);

	virtual void handleTimeout(TimerHandle handle, void * arg);

	/* ����ӿ�
	 * ĳ�� app ����ע��
	*/
	void onAppRegister(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID,
		uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx);

	/* ����ӿ�
	 * ���ӵ� app ����״̬
	 */
	virtual void onAppActiveTick(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID);

private:
	typedef Components::ComponentInfos APP_INFO;
	// key ������ ip��ͬһ ip �п����ж��ͬ�� app�������� componentID��Ҫ���������з�����cid ����Ψһ
	typedef std::map<COMPONENT_ID, APP_INFO*> APP_INFOS;

	APP_INFOS apps_;

	TimerHandle	tickTimer_;
};

}	// end namespace KBEngine