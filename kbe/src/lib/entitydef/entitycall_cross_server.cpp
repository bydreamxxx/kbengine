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

#include "entitydef/entitycall_cross_server.h"
#include "entitydef/entitycallabstract.h"
#include "entitydef/scriptdef_module.h"
#include "entitydef/remote_entity_method.h"
#include "network/channel.h"


namespace KBEngine
{
	EntityCallCrossServer::EntityCallCrossServer(COMPONENT_ORDER centerID, EntityCall *entitycall, const Network::Address *addr) :
		EntityCall(entitycall->pScriptDefModule(), addr, entitycall->componentID(), entitycall->id(), entitycall->type()),
		prototype_(entitycall->type()),
		centerID_(centerID)
	{
		switch (prototype_)
		{
		case ENTITYCALL_TYPE_CELL:
		case ENTITYCALL_TYPE_CELL_VIA_BASE:
			type_ = ENTITYCALL_TYPE_CROSS_SERVER_CELL;
			break;
		case ENTITYCALL_TYPE_BASE:
		case ENTITYCALL_TYPE_BASE_VIA_CELL:
			type_ = ENTITYCALL_TYPE_CROSS_SERVER_BASE;
			break;
		case ENTITYCALL_TYPE_CLIENT:
		case ENTITYCALL_TYPE_CLIENT_VIA_CELL:
		case ENTITYCALL_TYPE_CLIENT_VIA_BASE:
			type_ = ENTITYCALL_TYPE_CROSS_SERVER_CLIENT;
			break;
		default:
			break;
		}
	}

	EntityCallCrossServer::~EntityCallCrossServer()
	{}

	PyObject* EntityCallCrossServer::onScriptGetAttribute(PyObject* attr)
	{
		wchar_t* PyUnicode_AsWideCharStringRet0 = PyUnicode_AsWideCharString(attr, NULL);
		char* ccattr = strutil::wchar2char(PyUnicode_AsWideCharStringRet0);
		PyMem_Free(PyUnicode_AsWideCharStringRet0);

		MethodDescription* pMethodDescription = NULL;

		switch (prototype_)
		{
		case ENTITYCALL_TYPE_CELL:
			pMethodDescription = pScriptModule_->findCellMethodDescription(ccattr);
			break;
		case ENTITYCALL_TYPE_BASE:
			pMethodDescription = pScriptModule_->findBaseMethodDescription(ccattr);
			break;
		case ENTITYCALL_TYPE_CLIENT:
			pMethodDescription = pScriptModule_->findClientMethodDescription(ccattr);
			break;
		case ENTITYCALL_TYPE_CELL_VIA_BASE:
			pMethodDescription = pScriptModule_->findCellMethodDescription(ccattr);
			break;
		case ENTITYCALL_TYPE_BASE_VIA_CELL:
			pMethodDescription = pScriptModule_->findBaseMethodDescription(ccattr);
			break;
		case ENTITYCALL_TYPE_CLIENT_VIA_CELL:
			pMethodDescription = pScriptModule_->findClientMethodDescription(ccattr);
			break;
		case ENTITYCALL_TYPE_CLIENT_VIA_BASE:
			pMethodDescription = pScriptModule_->findClientMethodDescription(ccattr);
			break;
		default:
			break;
		};

		if (pMethodDescription != NULL)
		{
			free(ccattr);

			return createRemoteMethod(pMethodDescription);
		}

		free(ccattr);
		return ScriptObject::onScriptGetAttribute(attr);
	}

	void EntityCallCrossServer::c_str(char * s, size_t size)
	{
		const char * entitycallName =
			(type_ == ENTITYCALL_TYPE_CROSS_SERVER_CELL) ? "CellCrossServer" :
			(type_ == ENTITYCALL_TYPE_CROSS_SERVER_BASE) ? "BaseCrossServer" :
			(type_ == ENTITYCALL_TYPE_CROSS_SERVER_CLIENT) ? "ClientCrossServer" : "???";

		kbe_snprintf(s, size, "%s centerid:%d, utype:%u, component=%s[%" PRIu64 "], addr: %s.",
			entitycallName, centerID_, utype_,
			COMPONENT_NAME_EX(ENTITYCALL_COMPONENT_TYPE_MAPPING[type_]),
			componentID_, addr_.c_str());
	}

	void EntityCallCrossServer::newCall(Network::Bundle & bundle)
	{
	}


}	// end namespace KBEngine