#pragma once

#include <map>

#include "server/serverapp.h"
#include "common/singleton.h"
#include "common/timer.h"

namespace KBEngine
{

class CenterDataServer;

class Centermgr: public ServerApp,
				public Singleton<Centermgr>
{
public:
	typedef Components::ComponentInfos APP_INFO;
	// key ������ ip��ͬһ ip �п����ж��ͬ�� app�������� componentID��Ҫ���������з�����cid ����Ψһ
	typedef std::map<COMPONENT_ID, APP_INFO*> APP_INFOS;

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
	virtual void finalise();

	virtual void onComponentActiveTickTimeout();

	virtual void onChannelDeregister(Network::Channel * pChannel);

	virtual void handleTimeout(TimerHandle handle, void * arg);

	APP_INFOS const &getConnectedAppInfos();

	APP_INFO const *getAppInfo(COMPONENT_ORDER centerID);

	/* ����ӿ�
	 * ĳ�� app ����ע��
	*/
	void onAppRegister(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID,
		uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport, std::string& extaddrEx);

	/* ����ӿ�
	 * ���ӵ� app ����״̬
	 */
	virtual void onAppActiveTick(Network::Channel* pChannel, COMPONENT_TYPE componentType, COMPONENT_ID componentID);

	/* ����ӿ�
	* KBEngine.centerData ���ݸı�
	*/
	void onBroadcastCenterDataChanged(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/* ����ӿ�
	* �յ����call����, ��ĳ��app�ϵ�entityCallCrossServer����
	*/
	void onEntityCallCrossServer(Network::Channel* pChannel, KBEngine::MemoryStream& s);

	/* ����ӿ�
	* �����¼����
	*/
	void requestAcrossServer(Network::Channel *pChannel, KBEngine::MemoryStream& s);

	/* ����ӿ�
	* ��������¼�ɹ�
	*/
	void requestAcrossServerSuccess(Network::Channel *pChannel, KBEngine::MemoryStream& s);

private:
	APP_INFOS apps_;

	COMPONENT_ORDER appStartOrder_;	// �������˳��

	TimerHandle	tickTimer_;

	CenterDataServer* centerData_;
};

}	// end namespace KBEngine