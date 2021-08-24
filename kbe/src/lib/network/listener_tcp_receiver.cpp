
#include "network/listener_tcp_receiver.h"
#include "network/endpoint.h"
#include "network/network_interface.h"
#include "network/error_reporter.h"

namespace KBEngine {
namespace Network {

ListenerTcpReceiver::ListenerTcpReceiver(EndPoint& endpoint, Channel::Traits traits, NetworkInterface& networkInterface)
	:ListenerReceiver(endpoint, traits, networkInterface)
{}

ListenerTcpReceiver::~ListenerTcpReceiver()
{}


int ListenerTcpReceiver::handleInputNotification(int fd)
{
	int tickcount = 0;

	while (tickcount++ < 256)
	{
		EndPoint* pNewEndPoint = endpoint_.accept();
		if (pNewEndPoint == NULL) {

			if (tickcount == 1)
			{
				WARNING_MSG(fmt::format("ListenerTcpReceiver::handleInputNotification: accept endpoint({}) {}! channelSize={}\n",
					fd, kbe_strerror(), networkInterface_.channels().size()));

				this->dispatcher().errorReporter().reportException(
					REASON_GENERAL_NETWORK);
			}

			break;
		}
		else
		{
			Channel* pChannel = Network::Channel::createPoolObject(OBJECTPOOL_POINT);
			bool ret = pChannel->initialize(networkInterface_, pNewEndPoint, traits_);
			if (!ret)
			{
				ERROR_MSG(fmt::format("ListenerTcpReceiver::handleInputNotification: initialize({}) is failed!\n",
					pChannel->c_str()));

				pChannel->destroy();
				Network::Channel::reclaimPoolObject(pChannel);
				return 0;
			}

			if (!networkInterface_.registerChannel(pChannel))
			{
				ERROR_MSG(fmt::format("ListenerTcpReceiver::handleInputNotification: registerChannel({}) is failed!\n",
					pChannel->c_str()));

				pChannel->destroy();
				Network::Channel::reclaimPoolObject(pChannel);
			}
		}
	}

	return 0;
}

}	// end namespace Network
}	// end namespace KBEngine