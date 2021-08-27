
#ifndef KBE_NETWORKUDPLISTENER_RECEIVER_H
#define KBE_NETWORKUDPLISTENER_RECEIVER_H

#include "common/common.h"
#include "common/timer.h"
#include "helper/debug_helper.h"
#include "network/common.h"
#include "network/interfaces.h"
#include "network/packet.h"
#include "network/channel.h"
#include "network/listener_receiver.h"

namespace KBEngine {
	namespace Network
	{
		class ListenerUdpReceiver : public ListenerReceiver
		{
		public:
			ListenerUdpReceiver(class EndPoint& endpoint, Channel::Traits traits, class NetworkInterface& networkInterface);
			virtual ~ListenerUdpReceiver();

		 int handleInputNotification(int fd) override;

		protected:
			class UDPPacketReceiver* pUDPPacketReceiver_;

		};
	}
}

#ifdef CODE_INLINE
#include "listener_udp_receiver.inl"
#endif
#endif // KBE_NETWORKUDPLISTENER_RECEIVER_H
