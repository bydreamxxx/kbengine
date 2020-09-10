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

#pragma once

#include "entitydef/entitycall_cross_server.h"
#include "entitydef/entity_call.h"
#include "pyscript/scriptobject.h"

namespace KBEngine
{

class EntityCallCrossServer : public EntityCall
{
	/** ���໯ ��һЩpy�������������� */
	INSTANCE_SCRIPT_HREADER(EntityCallCrossServer, EntityCall)

public:
	EntityCallCrossServer(COMPONENT_ORDER centerID, EntityCall *entitycall);

	EntityCallCrossServer(ScriptDefModule* pScriptModule, const Network::Address* pAddr, COMPONENT_ID componentID,
		ENTITY_ID eid, ENTITYCALL_TYPE type, ENTITYCALL_TYPE prototype, COMPONENT_ORDER centerID);

	virtual ~EntityCallCrossServer();

	PyObject* onScriptGetAttribute(PyObject* attr);

	virtual void c_str(char* s, size_t size);

	virtual Network::Channel* getChannel();

	virtual void newCall(Network::Bundle& bundle);

	COMPONENT_ORDER centerID() { return centerID_; }

	/**
	unpickle����
	*/
	static PyObject* __unpickle__(PyObject* self, PyObject* args);

	/**
	�ű�����װʱ������
	*/
	static void onInstallScript(PyObject* mod);

	static PyObject* __py_reduce_ex__(PyObject* self, PyObject* protocol);

protected:
	ENTITYCALL_TYPE prototype_;	// ԭ EntityCall ����

	COMPONENT_ORDER centerID_;	// centermgr�����EntityCall���ڷ�������ȫ��id
};

}	// end namespace KBengine

