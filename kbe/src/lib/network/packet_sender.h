/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2018 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KBE_NETWORKPACKET_SENDER_H
#define KBE_NETWORKPACKET_SENDER_H

#include "common/common.h"
#include "common/timer.h"
#include "helper/debug_helper.h"
#include "network/common.h"
#include "network/interfaces.h"

namespace KBEngine { 
namespace Network
{
class Packet;
class EndPoint;
class Channel;
class Address;
class NetworkInterface;
class EventDispatcher;

class PacketSender : public OutputNotificationHandler, public PoolObject
{
public:
	enum PACKET_SENDER_TYPE
	{
		TCP_PACKET_SENDER = 0,
		UDP_PACKET_SENDER = 1
	};

public:
	PacketSender();
	PacketSender(EndPoint & endpoint, NetworkInterface & networkInterface);
	virtual ~PacketSender();

	EventDispatcher& dispatcher();

	void onReclaimObject() override
	{
		pEndpoint_ = NULL;
		pChannel_ = NULL;
		pNetworkInterface_ = NULL;
	}

	void pEndPoint(EndPoint* pEndpoint) noexcept {
		pChannel_ = NULL;
		pEndpoint_ = pEndpoint; 
	}

	EndPoint* pEndPoint() const noexcept{ 
		return pEndpoint_; 
	}

	NetworkInterface* pNetworkInterface() const noexcept
	{
		return pNetworkInterface_;
	}

	void pNetworkInterface(NetworkInterface* v) noexcept
	{
		pNetworkInterface_ = v;
	}

	virtual PACKET_SENDER_TYPE type() const noexcept
	{
		return TCP_PACKET_SENDER;
	}

	virtual int handleOutputNotification(int fd);

	virtual Reason processPacket(Channel* pChannel, Packet * pPacket);
	virtual Reason processFilterPacket(Channel* pChannel, Packet * pPacket) = 0;

	static Reason checkSocketErrors(const EndPoint * pEndpoint);

	virtual Channel* getChannel();

	virtual bool processSend(Channel* pChannel, int userarg) = 0;

protected:
	EndPoint* pEndpoint_;
	Channel* pChannel_;
	NetworkInterface* pNetworkInterface_;
};

}
}

#ifdef CODE_INLINE
#include "packet_sender.inl"
#endif
#endif // KBE_NETWORKPACKET_SENDER_H
