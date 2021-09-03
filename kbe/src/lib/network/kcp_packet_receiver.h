#ifndef KBE_NETWORKKCPPACKET_RECEIVER_H
#define KBE_NETWORKKCPPACKET_RECEIVER_H

#include "network/udp_packet_receiver.h"


namespace KBEngine
{
namespace Network
{

class KCPPacketReceiver : public UDPPacketReceiver
{
public:
	typedef KBEShared_ptr< SmartPoolObject< KCPPacketReceiver > > SmartPoolObjectPtr;
	static SmartPoolObjectPtr createSmartPoolObj(const std::string& logPoint);
	static ObjectPool<KCPPacketReceiver>& ObjPool();
	static KCPPacketReceiver* createPoolObject(const std::string& logPoint);
	static void reclaimPoolObject(KCPPacketReceiver* obj);
	static void destroyObjPool();

	KCPPacketReceiver() = default;
	KCPPacketReceiver(EndPoint& endpoint, NetworkInterface& networkInterface);

	bool processRecv(UDPPacket* pReceiveWindow) override;

	Reason processPacket(Channel* pChannel, Packet* pPacket) override;

	ProtocolSubType protocolSubType() const override {
		return SUB_PROTOCOL_KCP;
	}

};

}	// end namespace Network
}	// end namespace KBEngine

#ifdef CODE_INLINE
#include "kcp_packet_receiver.inl"
#endif

#endif	// KBE_NETWORKKCPPACKET_RECEIVER_H