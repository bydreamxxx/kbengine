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


namespace KBEngine { 
namespace Network
{

INLINE const Address & NetworkInterface::extTcpAddr() const
{
	return extTcpEndpoint_.addr();
}

INLINE const Address & NetworkInterface::extUdpAddr() const
{
	return extUdpEndpoint_.addr();
}

INLINE const Address & NetworkInterface::intTcpAddr() const
{
	return intTcpEndpoint_.addr();
}

INLINE int32 NetworkInterface::numExtChannels() const
{
	return numExtChannels_;
}

}
}
