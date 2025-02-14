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
#include "globaldata_client.h"
#include "components.h"
#include "serverapp.h"
#include "network/channel.h"
#include "entitydef/entity_call.h"
#include "entitydef/entitycall_cross_server.h"

#include "../../server/dbmgr/dbmgr_interface.h"

namespace KBEngine{ 


SCRIPT_METHOD_DECLARE_BEGIN(GlobalDataClient)
SCRIPT_METHOD_DECLARE_END()


SCRIPT_MEMBER_DECLARE_BEGIN(GlobalDataClient)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(GlobalDataClient)
SCRIPT_GETSET_DECLARE_END()
SCRIPT_INIT(GlobalDataClient, 0, 0, &Map::mappingMethods, 0, 0)
	
//-------------------------------------------------------------------------------------
GlobalDataClient::GlobalDataClient(COMPONENT_TYPE componentType, GlobalDataServer::DATA_TYPE dataType):
script::Map(getScriptType(), false),
serverComponentType_(componentType),
dataType_(dataType)
{
}

//-------------------------------------------------------------------------------------
GlobalDataClient::~GlobalDataClient()
{
}

//-------------------------------------------------------------------------------------
bool GlobalDataClient::write(PyObject* pyKey, PyObject* pyValue)
{
	bool ret = false;
	if(pyKey && pyValue)
	{
		if (PyDict_SetItem(pyDict_, pyKey, pyValue) == -1)
		{
			ERROR_MSG(fmt::format("Map::write: is eror! key={}, val={}\n", 
				PyBytes_AsString(pyKey), PyBytes_AsString(pyValue)));
		}
		else
		{
			ret = true;

			// 此处不能减引用，因为需要被pyDict_引用
			// Py_XDECREF(pyKey);
			// Py_XDECREF(pyValue);
		}
	}
	else
	{
		ERROR_MSG(fmt::format("Map::write:unpickle error. key={}, val={}\n",
			(pyKey ? PyBytes_AsString(pyKey) : "NULL"), (pyValue ? PyBytes_AsString(pyValue)  : "NULL")));

		PyErr_Print();
	}

	return ret;	
}

//-------------------------------------------------------------------------------------
bool GlobalDataClient::del(PyObject* pyKey)
{
	bool ret = false;

	if(pyKey)
	{
		PyObject* pyVal = PyDict_GetItem(pyDict_, pyKey);
		if (pyVal && PyDict_DelItem(pyDict_, pyKey) == -1)
		{
			ERROR_MSG(fmt::format("Map::del: delete key is failed! key={}.\n", PyBytes_AsString(pyKey)));
			PyErr_Clear();
		}
		else
		{
			ret = true;
		}

		// PyDict_GetItem为弱引用
		// Py_XDECREF(pyVal);
	}
	else
	{
		ERROR_MSG(fmt::format("Map::del: delete key error! key={}.\n", "NULL"));
		PyErr_Print();
	}

	return ret;	
}

//-------------------------------------------------------------------------------------
void GlobalDataClient::onDataChanged(PyObject* key, PyObject* value, bool isDelete)
{
	std::string skey = script::Pickler::pickle(key, 0);
	std::string sval = "";

	if (value)
		sval = script::Pickler::pickle(value, 0);

	Components::COMPONENTS& channels = Components::getSingleton().getComponents(serverComponentType_);
	Components::COMPONENTS::iterator iter1 = channels.begin();
	uint8 dataType = dataType_;
	ArraySize slen = 0;

	for(; iter1 != channels.end(); ++iter1)
	{
		Network::Channel* lpChannel = iter1->pChannel;
		KBE_ASSERT(lpChannel != NULL);
		
		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		
		(*pBundle).newMessage(DbmgrInterface::onBroadcastGlobalDataChanged);
		
		(*pBundle) << dataType;
		(*pBundle) << isDelete;

		slen = skey.size();
		(*pBundle) << slen;
		(*pBundle).assign(skey.data(), slen);

		if(!isDelete)
		{
			slen = sval.size();
			(*pBundle) << slen;
			(*pBundle).assign(sval.data(), slen);
		}

		(*pBundle) << g_componentType;

		lpChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
}
