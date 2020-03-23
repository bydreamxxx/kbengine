#pragma once

#include <map>

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

	virtual void onComponentActiveTickTimeout();

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
	// key ������ ip��ͬһ ip �п����ж��ͬ�� app�������� componentID��Ҫ���������з�����cid ����Ψһ
	typedef std::map<COMPONENT_ID, Components::ComponentInfos*> APP_INFOS;
	APP_INFOS apps_;

};

}	// end namespace KBEngine