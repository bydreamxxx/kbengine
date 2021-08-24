

#ifndef KBE_NETWORKTCPLISTENER_RECEIVER_H
#define KBE_NETWORKTCPLISTENER_RECEIVER_H

#include "network/listener_receiver.h"
#include "network/channel.h"

namespace KBEngine {
namespace Network {

class EndPoint;
class NetworkInterface;

class ListenerTcpReceiver : public ListenerReceiver
{
public:
	ListenerTcpReceiver(EndPoint &endpoint, Channel::Traits traits, NetworkInterface &networkInterface);
	
	virtual ~ListenerTcpReceiver();

	int handleInputNotification(int fd) override;
};


}	// end namespace Network
}	// end namespace KBEngine

#endif	// KBE_NETWORKTCPLISTENER_RECEIVER_H