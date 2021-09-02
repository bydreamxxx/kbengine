
#ifndef KBE_KCP_PACKET_READER_H
#define KBE_KCP_PACKET_READER_H

#include "network/packet_reader.h"

namespace KBEngine
{
namespace Network
{

class KCPPacketReader : public PacketReader
{
public:
	KCPPacketReader(Channel* pChannel);

	PacketReader::PACKET_READER_TYPE type() const override
	{ 
		return PACKET_READER_TYPE_KCP; 
	}
};


}	// end namespace Network
}	// end namespace KBEngine

#endif	// KBE_KCP_PACKET_READER_H
