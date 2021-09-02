

#ifndef KBE_NETWORKUDPPACKET_SENDER_H
#define KBE_NETWORKUDPPACKET_SENDER_H

#include "network/packet_sender.h"
#include "common/objectpool.h"

namespace KBEngine {
namespace Network {

class Channel;

class UDPPacketSender : public PacketSender 
{
public:
	typedef KBEShared_ptr< SmartPoolObject< UDPPacketSender > > SmartPoolObjectPtr;

	static SmartPoolObjectPtr createSmartPoolObj(const std::string& logPoint);
	static ObjectPool<UDPPacketSender>& ObjPool();
	static UDPPacketSender* createPoolObject(const std::string& logPoint);
	static void reclaimPoolObject(UDPPacketSender* obj);
	static void destroyObjPool();

	void onReclaimObject() override;

public:
	UDPPacketSender() = default;
	UDPPacketSender(class EndPoint& endpoint, class NetworkInterface& networkInterface);
	~UDPPacketSender() = default;

	UDPPacketSender(const UDPPacketSender&) = default;
	UDPPacketSender& operator=(const UDPPacketSender&) = default;

	UDPPacketSender(UDPPacketSender&&) = default;
	UDPPacketSender& operator=(UDPPacketSender&&) = default;

	bool processSend(Channel* pChannel, int userarg) override;

	PacketSender::PACKET_SENDER_TYPE type() const noexcept override
	{
		return UDP_PACKET_SENDER;
	}

	Reason processFilterPacket(Channel* pChannel, Packet* pPacket, int userarg) override;

protected:
	virtual void onGetError(Channel* pChannel, const std::string& err);
	virtual void onSent(Packet* pPacket);

protected:
	uint8 sendfailCount_;

};


}	// end namespace Network
}	// end namespace KBEngine

#ifdef CODE_INLINE
#include "udp_packet_sender.inl"
#endif

#endif // !KBE_NETWORKUDPPACKET_SENDER_H
