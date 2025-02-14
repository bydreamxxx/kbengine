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

#ifndef KBE_WEBSOCKET_PROTOCOL_H
#define KBE_WEBSOCKET_PROTOCOL_H

#include "common/common.h"
#include "helper/debug_helper.h"

namespace KBEngine{

class MemoryStream;

namespace Network
{
	class Channel;
	class Packet;

namespace websocket{


/*	WebSocket FRC6544
*/

class WebSocketProtocol
{
public:
	enum FrameType 
	{
		// 下一帧与结束
		NEXT_FRAME = 0x0,
		END_FRAME = 0x80,

		ERROR_FRAME = 0xFF00,
		INCOMPLETE_FRAME = 0xFE00,

		OPENING_FRAME = 0x3300,
		CLOSING_FRAME = 0x3400,

		// 未完成的帧
		INCOMPLETE_TEXT_FRAME = 0x01,
		INCOMPLETE_BINARY_FRAME = 0x02,

		// 文本帧与二进制帧 END_FRAME + *_FRAME
		TEXT_FRAME = 0x81,
		BINARY_FRAME = 0x82,

		// END_FRAME + *_FRAME
		PING_FRAME = 0x89,
		PONG_FRAME = 0x8A,

		// 关闭连接
		CLOSE_FRAME = 0x08
	};

	/**
		是否是websocket协议
	*/
	static bool isWebSocketProtocol(MemoryStream* s);
	
	/**
		websocket协议握手
	*/
	static bool handshake(Network::Channel* pChannel, MemoryStream* s);

	/**
		帧解析相关
	*/
	static int makeFrame(FrameType frame_type, Packet* pInPacket, Packet* pOutPacket);
	static int getFrame(Packet* pPacket, uint8& msg_opcode, uint8& msg_fin, uint8& msg_masked, uint32& msg_mask, 
		int32& msg_length_field, uint64& msg_payload_length, FrameType& frameType);

	static bool decodingDatas(Packet* pPacket, uint8 msg_masked, uint32 msg_mask);

	static std::string getFrameTypeName(FrameType frame_type);
};

}
}
}

#endif // KBE_WEBSOCKET_PROTOCOL_H

