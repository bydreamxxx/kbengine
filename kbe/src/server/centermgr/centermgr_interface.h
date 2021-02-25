/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2020 KBEngine.

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

#if defined(DEFINE_IN_INTERFACE)
	#undef KBE_CENTERMGR_INTERFACE_H
#endif // DEFINE_IN_INTERFACE

#ifndef KBE_CENTERMGR_INTERFACE_H
#define KBE_CENTERMGR_INTERFACE_H

// common include	
#if defined(CENTERMGR)
#include "centermgr/centermgr.h"
#endif	// CENTERMGR

#include "centermgr/centermgr_interface_macro.h"
#include "network/interface_defs.h"


namespace KBEngine {

/**
	centermgr������Ϣ�ӿ��ڴ˶���
*/
NETWORK_INTERFACE_DECLARE_BEGIN(CentermgrInterface)

// �� centermgr ע���Լ�����Ϣ
CENTERMGR_MESSAGE_DECLARE_ARGS7(onAppRegister, NETWORK_VARIABLE_MESSAGE,
	COMPONENT_TYPE, componentType,
	COMPONENT_ID, componentID,
	uint32, intaddr,
	uint16, intport,
	uint32, extaddr,
	uint16, extport,
	std::string, extaddrEx)

// �� centermgr ע���Լ�����Ϣ
CENTERMGR_MESSAGE_DECLARE_ARGS2(onAppActiveTick, NETWORK_VARIABLE_MESSAGE,
	COMPONENT_TYPE, componentType,
	COMPONENT_ID, componentID)

CENTERMGR_MESSAGE_DECLARE_STREAM(onBroadcastCenterDataChanged, NETWORK_VARIABLE_MESSAGE)

// �յ����call����, ��ĳ��app�ϵ�EntityCallCrossServer����DBMgrת������
CENTERMGR_MESSAGE_DECLARE_STREAM(onEntityCallCrossServer, NETWORK_VARIABLE_MESSAGE)

// �յ������¼����
CENTERMGR_MESSAGE_DECLARE_STREAM(requestAcrossServer, NETWORK_VARIABLE_MESSAGE)

// �����¼����ɹ�
CENTERMGR_MESSAGE_DECLARE_STREAM(requestAcrossServerSuccess, NETWORK_VARIABLE_MESSAGE)

NETWORK_INTERFACE_DECLARE_END()

#ifdef DEFINE_IN_INTERFACE
	#undef DEFINE_IN_INTERFACE
#endif

}	// end namespace KBEngine


#endif	// KBE_CENTERMGR_INTERFACE_H


