#ifndef KBE_NETWORKKCPPACKET_SENDER_H
#define KBE_NETWORKKCPPACKET_SENDER_H

#include "network/udp_packet_sender.h"
#include "network/ikcp.h"

namespace KBEngine 
{
namespace Network
{

class Channel;
class Packet;

class KCPPacketSender : public UDPPacketSender
{
public:
	typedef KBEShared_ptr< SmartPoolObject< KCPPacketSender > > SmartPoolObjectPtr;

	static SmartPoolObjectPtr createSmartPoolObj(const std::string& logPoint);
	static ObjectPool<KCPPacketSender>& ObjPool();
	static KCPPacketSender* createPoolObject(const std::string& logPoint);
	static void reclaimPoolObject(KCPPacketSender* obj);
	static void destroyObjPool();

	void onReclaimObject() override;

public:
	KCPPacketSender() = default;
	KCPPacketSender(class EndPoint& endpoint, class NetworkInterface& networkInterface);
	~KCPPacketSender() = default;

	KCPPacketSender(const KCPPacketSender&) = default;
	KCPPacketSender& operator=(const KCPPacketSender&) = default;

	KCPPacketSender(KCPPacketSender&&) = default;
	KCPPacketSender& operator=(KCPPacketSender&&) = default;

	int kcp_output(const char* buf, int len, ikcpcb* kcp, Channel* pChannel);

protected:
	void onSent(Packet* pPacket) override;
	Reason processFilterPacket(Channel* pChannel, Packet* pPacket, int userarg) override;

};

}	// end namespace Network
}	// end namespace KBEngine

#ifdef CODE_INLINE
#include "kcp_packet_sender.inl"
#endif

#endif	// KBE_NETWORKKCPPACKET_SENDER_H