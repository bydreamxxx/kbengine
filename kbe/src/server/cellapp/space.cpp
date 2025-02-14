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

#include "cellapp.h"
#include "space.h"	
#include "entity.h"
#include "witness.h"	
#include "navigation/navigation.h"
#include "loadnavmesh_threadtasks.h"
#include "entitydef/entities.h"
#include "client_lib/client_interface.h"
#include "network/network_stats.h"

#include "../../server/baseappmgr/baseappmgr_interface.h"
#include "../../server/cellappmgr/cellappmgr_interface.h"
#include "../../server/baseapp/baseapp_interface.h"
#include "../../server/cellapp/cellapp_interface.h"
#include "../../server/dbmgr/dbmgr_interface.h"

namespace KBEngine{	

//-------------------------------------------------------------------------------------
Space::Space(SPACE_ID spaceID, const std::string& scriptModuleName) :
id_(spaceID),
scriptModuleName_(scriptModuleName),
entities_(),
hasGeometry_(false),
pCell_(NULL),
coordinateSystem_(),
pNavHandle_(),
state_(STATE_NORMAL),
destroyTime_(0)
{
	Network::Channel* pChannel = Components::getSingleton().getCellappmgrChannel();
	if (pChannel != NULL)
	{
		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		(*pBundle).newMessage(CellappmgrInterface::updateSpaceData);
		
		(*pBundle) << g_componentID;
		(*pBundle) << id_;
		(*pBundle) << scriptModuleName_;
		(*pBundle) << false;
		(*pBundle) << "";

		pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
Space::~Space()
{
	_clearGhosts();
	entities_.clear();
	
	this->coordinateSystem_.releaseNodes();
	
	if (pNavHandle_ && pNavHandle_->type() == NavigationHandle::NAV_TILE) 
	{
		SAFE_RELEASE(pNavHandle_);
	}
	//pNavHandle_.clear();

	SAFE_RELEASE(pCell_);	

	Network::Channel* pChannel = Components::getSingleton().getCellappmgrChannel();
	if (pChannel != NULL)
	{
		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		(*pBundle).newMessage(CellappmgrInterface::updateSpaceData);

		(*pBundle) << g_componentID;
		(*pBundle) << id_;
		(*pBundle) << scriptModuleName_;
		(*pBundle) << true;
		(*pBundle) << "";

		pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
void Space::_clearGhosts()
{
	// 因为space在destroy时做过一次清理，因此这里理论上剩下的是ghosts实体
	if(entities_.size() == 0)
		return;
	
	std::vector<ENTITY_ID> entitieslog;
	
	SPACE_ENTITIES::const_iterator log_iter = this->entities().begin();
	for(; log_iter != this->entities().end(); ++log_iter)
		entitieslog.push_back((*log_iter).get()->id());

	std::vector<ENTITY_ID>::iterator iter = entitieslog.begin();
	for(; iter != entitieslog.end(); ++iter)
	{
		Entity* entity = Cellapp::getSingleton().findEntity((*iter));
		if(entity != NULL && !entity->isDestroyed() && entity->spaceID() == this->id())
		{
			entity->destroyEntity();
		}
		else
		{
			entity = findEntity((*iter));
			if(entity != NULL && !entity->isDestroyed() && entity->spaceID() == this->id())
			{
				removeEntity(entity);
			}
		}
	}
	
	entities_.clear();	
}

//-------------------------------------------------------------------------------------
PyObject* Space::__py_GetSpaceGeometryMapping(PyObject* self, PyObject* args)
{
	SPACE_ID spaceID = 0;

	if(PyTuple_Size(args) != 1)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::getSpaceGeometryMapping: (argssize != 1) error!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(PyArg_ParseTuple(args, "I", &spaceID) == -1)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::getSpaceGeometryMapping: args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	Space* space = Spaces::findSpace(spaceID);
	if(space == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::getSpaceGeometryMapping: (spaceID=%u) not found!", 
			spaceID);

		PyErr_PrintEx(0);
		return 0;
	}

	return PyUnicode_FromString(space->getGeometryPath().c_str());
}

//-------------------------------------------------------------------------------------
PyObject* Space::__py_AddSpaceGeometryMapping(PyObject* self, PyObject* args)
{
	SPACE_ID spaceID = 0;
	char* path = NULL;
	bool shouldLoadOnServer = true;
	PyObject* mapper = NULL;
	PyObject* py_params = NULL;
	std::map< int, std::string > params;
	
	int argCount = PyTuple_Size(args);
	if(argCount < 3 || argCount > 5)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::addSpaceGeometryMapping: (argssize[spaceID, mapper, path, shouldLoadOnServer] < 3 || > 5) error!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(argCount == 4)
	{
		if(PyArg_ParseTuple(args, "I|O|s|b", &spaceID, &mapper, &path, &shouldLoadOnServer) == -1)
		{
			PyErr_Format(PyExc_AssertionError, "KBEngine::addSpaceGeometryMapping: args error!");
			PyErr_PrintEx(0);
			return 0;
		}
	}
	else if(argCount == 5)
	{
		if(PyArg_ParseTuple(args, "I|O|s|b|O", &spaceID, &mapper, &path, &shouldLoadOnServer, &py_params) == -1)
		{
			PyErr_Format(PyExc_AssertionError, "KBEngine::addSpaceGeometryMapping: args error!");
			PyErr_PrintEx(0);
			return 0;
		}
		
		if(py_params)
		{
			PyObject *key, *value;
			Py_ssize_t pos = 0;

			if(!PyDict_Check(py_params))
			{
				PyErr_Format(PyExc_AssertionError, "KBEngine::addSpaceGeometryMapping: args(params) not is PyDict!");
				PyErr_PrintEx(0);					
				return 0;
			}

			while(PyDict_Next(py_params, &pos, &key, &value)) 
			{
				if(!PyLong_Check(key) || !PyUnicode_Check(value))
				{
					PyErr_Format(PyExc_AssertionError, "KBEngine::addSpaceGeometryMapping: args(params) error!");
					PyErr_PrintEx(0);					
					return 0;
				}
				
				params[PyLong_AsLong(key)] = PyUnicode_AsUTF8AndSize(value, NULL);
			}
			
			SCRIPT_ERROR_CHECK();
		}
	}
	else
	{
		if(PyArg_ParseTuple(args, "I|O|s", &spaceID, &mapper, &path) == -1)
		{
			PyErr_Format(PyExc_AssertionError, "KBEngine::addSpaceGeometryMapping: args wrong!");
			PyErr_PrintEx(0);
			return 0;
		}
	}

	Space* space = Spaces::findSpace(spaceID);
	if(space == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::addSpaceGeometryMapping: spaceID error! spaceID=%u respath=%s",
			spaceID, path);

		PyErr_PrintEx(0);
		return 0;
	}

	SCRIPT_ERROR_CHECK();

	if (Resmgr::getSingleton().matchPath(path).size() == 0)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::addSpaceGeometryMapping: path error! spaceID=%u respath=%s",
			spaceID, path);

		PyErr_PrintEx(0);
		return 0;
	}

	if(!space->addSpaceGeometryMapping(path, shouldLoadOnServer, params))
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::addSpaceGeometryMapping: (spaceID=%u respath=%s) error!", 
			spaceID, path);

		PyErr_PrintEx(0);
		return 0;
	}

	S_Return;
}

//-------------------------------------------------------------------------------------
bool Space::addSpaceGeometryMapping(std::string respath, bool shouldLoadOnServer, const std::map< int, std::string >& params)
{
	INFO_MSG(fmt::format("KBEngine::addSpaceGeometryMapping: spaceID={}, respath={}, shouldLoadOnServer={}!\n",
		id(), respath, shouldLoadOnServer));

	hasGeometry_ = true;
	if(getGeometryPath() == respath)
	{
		WARNING_MSG(fmt::format("KBEngine::addSpaceGeometryMapping: spaceID={}, respath={} exist!\n",
			id(), respath));

		return true;
	}

	setGeometryPath(respath);

	if(shouldLoadOnServer)
		loadSpaceGeometry(params);

	return true;
}

//-------------------------------------------------------------------------------------
void Space::loadSpaceGeometry(const std::map< int, std::string >& params)
{
	KBE_ASSERT(pNavHandle_ == NULL);
	Cellapp::getSingleton().threadPool().addTask(new LoadNavmeshTask(getGeometryPath(), this->id(), params));
}

//-------------------------------------------------------------------------------------
void Space::unLoadSpaceGeometry()
{
	Network::Channel* pChannel = Components::getSingleton().getCellappmgrChannel();
	if (pChannel != NULL)
	{
		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		(*pBundle).newMessage(CellappmgrInterface::updateSpaceData);

		(*pBundle) << g_componentID;
		(*pBundle) << id_;
		(*pBundle) << scriptModuleName_;
		(*pBundle) << false;
		(*pBundle) << "";

		pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
void Space::onLoadedSpaceGeometryMapping(NavigationHandlePtr pNavHandle)
{
	pNavHandle_ = pNavHandle;
	INFO_MSG(fmt::format("KBEngine::onLoadedSpaceGeometryMapping: spaceID={}, respath={}!\n",
			id(), getGeometryPath()));

	// 通知脚本
	{
		SCOPED_PROFILE(SCRIPTCALL_PROFILE);
		SCRIPT_OBJECT_CALL_ARGS2(Cellapp::getSingleton().getEntryScript().get(), const_cast<char*>("onSpaceGeometryLoaded"), 
			const_cast<char*>("Is"), this->id(), getGeometryPath().c_str(), false);
	}

	onAllSpaceGeometryLoaded();

	Network::Channel* pChannel = Components::getSingleton().getCellappmgrChannel();
	if (pChannel != NULL)
	{
		Network::Bundle* pBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		(*pBundle).newMessage(CellappmgrInterface::updateSpaceData);

		(*pBundle) << g_componentID;
		(*pBundle) << id_;
		(*pBundle) << scriptModuleName_;
		(*pBundle) << false;
		(*pBundle) << getGeometryPath();

		pChannel->send(pBundle);
	}
}

//-------------------------------------------------------------------------------------
void Space::onAllSpaceGeometryLoaded()
{
	SCOPED_PROFILE(SCRIPTCALL_PROFILE);

	// 通知脚本
	SCRIPT_OBJECT_CALL_ARGS3(Cellapp::getSingleton().getEntryScript().get(), const_cast<char*>("onAllSpaceGeometryLoaded"), 
		const_cast<char*>("Iis"), this->id(), true, getGeometryPath().c_str(), false);
}

//-------------------------------------------------------------------------------------
bool Space::update()
{
	if(destroyTime_ > 0 && timestamp() - destroyTime_ >= uint64( 3.f * stampsPerSecond() ))
	{
		if(entities_.size() == 0)
			return false;
	}

	this->coordinateSystem_.releaseNodes();

	if(destroyTime_ > 0 && timestamp() - destroyTime_ >= uint64( 30.f * stampsPerSecond() ))
	{
		_clearGhosts();
		KBE_ASSERT(entities_.size() == 0);
		this->coordinateSystem_.releaseNodes();
	}
		
	return true;
}

//-------------------------------------------------------------------------------------
void Space::addEntityAndEnterWorld(Entity* pEntity, bool isRestore)
{
	addEntity(pEntity);
	addEntityToNode(pEntity);
	onEnterWorld(pEntity);
}

//-------------------------------------------------------------------------------------
void Space::addEntity(Entity* pEntity)
{
	pEntity->spaceID(this->id_);
	pEntity->spaceEntityIdx(entities_.size());
	entities_.push_back(pEntity);
	pEntity->onEnterSpace(this);
}

//-------------------------------------------------------------------------------------
void Space::addEntityToNode(Entity* pEntity)
{
	pEntity->installCoordinateNodes(&coordinateSystem_);
}

//-------------------------------------------------------------------------------------
void Space::removeEntity(Entity* pEntity)
{
	KBE_ASSERT(pEntity->spaceID() == id());

	pEntity->spaceID(0);
	
	// 先获取到所在位置
	SPACE_ENTITIES::size_type idx = pEntity->spaceEntityIdx();

	KBE_ASSERT(idx < entities_.size());
	KBE_ASSERT(entities_[ idx ] == pEntity);

	// 如果有2个或以上的entity则将最后一个entity移至删除的这个entity所在位置
	Entity* pBack = entities_.back().get();
	pBack->spaceEntityIdx(idx);
	entities_[idx] = pBack;
	pEntity->spaceEntityIdx(SPACE_ENTITIES::size_type(-1));
	entities_.pop_back();

	onLeaveWorld(pEntity);

	// 这句必须在onLeaveWorld之后， 因为可能rangeTrigger需要参考pEntityCoordinateNode
	pEntity->uninstallCoordinateNodes(&coordinateSystem_);
	pEntity->onLeaveSpace(this);

	// 如果没有entity了则需要销毁space, 因为space最少存在一个entity
	if(entities_.empty() && state_ == STATE_NORMAL)
	{
		Spaces::destroySpace(this->id(), 0);
	}
}

//-------------------------------------------------------------------------------------
void Space::_onEnterWorld(Entity* pEntity)
{
	if(!pEntity->isReal() || !pEntity->pScriptModule()->hasClient())
		return;

	if(pEntity->hasWitness())
	{
		_addSpaceDatasToEntityClient(pEntity);
		pEntity->pWitness()->onEnterSpace(this);
	}
}

//-------------------------------------------------------------------------------------
void Space::onEnterWorld(Entity* pEntity)
{
	KBE_ASSERT(pEntity != NULL);
	
	// 如果是一个有Witness(通常是玩家)则需要将当前场景已经创建的有client部分的entity广播给他
	// 否则是一个普通的entity进入世界， 那么需要将这个entity广播给所有看见他的有Witness的entity。
	if(pEntity->hasWitness())
	{
		_onEnterWorld(pEntity);
	}
}

//-------------------------------------------------------------------------------------
void Space::onEntityAttachWitness(Entity* pEntity)
{
	KBE_ASSERT(pEntity != NULL && pEntity->hasWitness());
	_onEnterWorld(pEntity);
}

//-------------------------------------------------------------------------------------
void Space::onLeaveWorld(Entity* pEntity)
{
	if(!pEntity->isReal() || !pEntity->pScriptModule()->hasClient())
		return;
	
	// 向其他人客户端广播自己的离开
	// 向客户端发送onLeaveWorld消息
	if(pEntity->hasWitness())
	{
		pEntity->pWitness()->onLeaveSpace(this);
	}
}

//-------------------------------------------------------------------------------------
Entity* Space::findEntity(ENTITY_ID entityID)
{
	SPACE_ENTITIES::const_iterator iter = this->entities().begin();
	for(; iter != this->entities().end(); ++iter)
	{
		const Entity* entity = (*iter).get();
			
		if(entity->id() == entityID)
			return const_cast<Entity*>(entity);
	}

	return NULL;
}

//-------------------------------------------------------------------------------------
bool Space::destroy(ENTITY_ID entityID, bool ignoreGhost)
{
	if(state_ != STATE_NORMAL)
		return false;

	state_ = STATE_DESTROYING;
	destroyTime_ = timestamp();
	
	std::vector<ENTITY_ID> entitieslog;
	
	{
		SPACE_ENTITIES::const_iterator iter = this->entities().begin();
		for(; iter != this->entities().end(); ++iter)
		{
			const Entity* entity = (*iter).get();
			entitieslog.push_back(entity->id());
		}
	}

	{
		std::vector<ENTITY_ID>::iterator iter = entitieslog.begin();
		for(; iter != entitieslog.end(); ++iter)
		{
			Entity* entity = Cellapp::getSingleton().findEntity((*iter));
			if(entity != NULL && !entity->isDestroyed() && entity->spaceID() == this->id() && entity->isReal())
			{
				entity->onSpaceGone();
			}
		}
	}
	
	state_ = STATE_DESTROYED;
	
	if(this->entities().size() == 0)
		return true;

	std::vector<ENTITY_ID>::iterator iter = entitieslog.begin();
	for(; iter != entitieslog.end(); ++iter)
	{
		Entity* entity = Cellapp::getSingleton().findEntity((*iter));
		if(entity != NULL && entity->isReal() && !entity->isDestroyed() && entity->spaceID() == this->id())
		{
			entity->destroyEntity();
		}
		else
		{
			entity = findEntity((*iter));
			if(entity != NULL && entity->isReal() && !entity->isDestroyed() && entity->spaceID() == this->id())
			{
				removeEntity(entity);
			}
		}
	}

	if(!ignoreGhost)
		_clearGhosts();

	return true;
}

//-------------------------------------------------------------------------------------
void Space::setGeometryPath(const std::string& path)
{ 
	return setSpaceData("_mapping", path); 
}

//-------------------------------------------------------------------------------------
const std::string& Space::getGeometryPath()
{ 
	return getSpaceData("_mapping"); 
}

//-------------------------------------------------------------------------------------
void Space::setSpaceData(const std::string& key, const std::string& value)
{
	SPACE_DATA::iterator iter = datas_.find(key);
	if(iter == datas_.end())
		datas_.insert(SPACE_DATA::value_type(key, value)); 
	else
		if(iter->second == value)
			return;
		else
			datas_[key] = value;

	onSpaceDataChanged(key, value, false);
}

//-------------------------------------------------------------------------------------
bool Space::hasSpaceData(const std::string& key)
{
	SPACE_DATA::iterator iter = datas_.find(key);
	if(iter == datas_.end())
		return false;

	return true;
}

//-------------------------------------------------------------------------------------
const std::string& Space::getSpaceData(const std::string& key)
{
	SPACE_DATA::iterator iter = datas_.find(key);
	if(iter == datas_.end())
	{
		static const std::string null = "";
		return null;
	}

	return iter->second;
}

//-------------------------------------------------------------------------------------
void Space::delSpaceData(const std::string& key)
{
	SPACE_DATA::iterator iter = datas_.find(key);
	if(iter == datas_.end())
		return;

	datas_.erase(iter);

	onSpaceDataChanged(key, "", true);
}

//-------------------------------------------------------------------------------------
void Space::onSpaceDataChanged(const std::string& key, const std::string& value, bool isdel)
{
	// 通知脚本
	if(!isdel)
	{
		SCOPED_PROFILE(SCRIPTCALL_PROFILE);
		SCRIPT_OBJECT_CALL_ARGS3(Cellapp::getSingleton().getEntryScript().get(), const_cast<char*>("onSpaceData"), 
			const_cast<char*>("Iss"), this->id(), key.c_str(), value.c_str(), false);
	}
	else
	{
		SCOPED_PROFILE(SCRIPTCALL_PROFILE);
		SCRIPT_OBJECT_CALL_ARGS3(Cellapp::getSingleton().getEntryScript().get(), const_cast<char*>("onSpaceData"), 
			const_cast<char*>("IsO"), this->id(), key.c_str(), Py_None, false);
	}

	SPACE_ENTITIES::const_iterator iter = this->entities().begin();
	for(; iter != this->entities().end(); ++iter)
	{
		const Entity* pEntity = (*iter).get();

		if(pEntity == NULL || pEntity->isDestroyed() || !pEntity->hasWitness())
			continue;

		Network::Bundle* pSendBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
		NETWORK_ENTITY_MESSAGE_FORWARD_CLIENT_BEGIN(pEntity->id(), (*pSendBundle));
		
		if(!isdel)
		{
			ENTITY_MESSAGE_FORWARD_CLIENT_BEGIN(pSendBundle, ClientInterface::setSpaceData, set);
			(*pSendBundle) << this->id();
			(*pSendBundle) << key;
			(*pSendBundle) << value;
			ENTITY_MESSAGE_FORWARD_CLIENT_END(pSendBundle, ClientInterface::setSpaceData, set);
		}
		else
		{
			ENTITY_MESSAGE_FORWARD_CLIENT_BEGIN(pSendBundle, ClientInterface::delSpaceData, del);
			(*pSendBundle) << this->id();
			(*pSendBundle) << key;
			ENTITY_MESSAGE_FORWARD_CLIENT_END(pSendBundle, ClientInterface::delSpaceData, del);
		}

		if(!isdel)
			pEntity->pWitness()->sendToClient(ClientInterface::setSpaceData, pSendBundle);
		else
			pEntity->pWitness()->sendToClient(ClientInterface::delSpaceData, pSendBundle);
	}
}

//-------------------------------------------------------------------------------------
void Space::_addSpaceDatasToEntityClient(const Entity* pEntity)
{
	if(!pEntity)
	{
		return;
	}

	if(pEntity->isDestroyed())
	{
		return;
	}

	if(!pEntity->hasWitness())
	{
		WARNING_MSG(fmt::format("Space::_addSpaceDatasToEntityClient: entity {} no client!\n", 
			pEntity->id()));

		return;
	}

	Network::Bundle* pSendBundle = Network::Bundle::createPoolObject(OBJECTPOOL_POINT);
	NETWORK_ENTITY_MESSAGE_FORWARD_CLIENT_BEGIN(pEntity->id(), (*pSendBundle));

	ENTITY_MESSAGE_FORWARD_CLIENT_BEGIN(pSendBundle, ClientInterface::initSpaceData, init);
	(*pSendBundle) << this->id();

	SPACE_DATA::iterator iter = datas_.begin();
	for(; iter != datas_.end(); ++iter)
	{
		(*pSendBundle) << iter->first;
		(*pSendBundle) << iter->second;
	}

	ENTITY_MESSAGE_FORWARD_CLIENT_END(pSendBundle, ClientInterface::initSpaceData, init);

	pEntity->pWitness()->sendToClient(ClientInterface::initSpaceData, pSendBundle);
}

//-------------------------------------------------------------------------------------
PyObject* Space::__py_SetSpaceData(PyObject* self, PyObject* args)
{
	SPACE_ID spaceID = 0;

	if(PyTuple_Size(args) != 3)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::setSpaceData: (argssize != (spaceID, key, value)) error!");
		PyErr_PrintEx(0);
		return 0;
	}
	
	char* key = NULL, *value = NULL;
	if(PyArg_ParseTuple(args, "Iss", &spaceID, &key, &value) == -1)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::setSpaceData: args error!");
		PyErr_PrintEx(0);
		return 0;
	}
	
	if(key == NULL || value == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::setSpaceData: key or value error, not is string!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(strlen(key) == 0)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::setSpaceData: key is empty!");
		PyErr_PrintEx(0);
		return 0;
	}

	Space* space = Spaces::findSpace(spaceID);
	if(space == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::setSpaceData: (spaceID=%u) not found!", 
			spaceID);

		PyErr_PrintEx(0);
		return 0;
	}
	
	if(kbe_stricmp(key, "_mapping") == 0)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::setSpaceData: key{_mapping} is protected!", 
			spaceID);

		PyErr_PrintEx(0);
		return 0;
	}

	space->setSpaceData(key, value);
	S_Return;
}

//-------------------------------------------------------------------------------------
PyObject* Space::__py_GetSpaceData(PyObject* self, PyObject* args)
{
	SPACE_ID spaceID = 0;

	if(PyTuple_Size(args) != 2)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::getSpaceData: (argssize != (spaceID, key)) error!");
		PyErr_PrintEx(0);
		return 0;
	}
	
	char* key = NULL;
	if(PyArg_ParseTuple(args, "Is", &spaceID, &key) == -1)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::getSpaceData: args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(key == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::getSpaceData: key not is string!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(strlen(key) == 0)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::getSpaceData: key is empty!");
		PyErr_PrintEx(0);
		return 0;
	}

	Space* space = Spaces::findSpace(spaceID);
	if(space == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::getSpaceData: (spaceID=%u) not found!", 
			spaceID);

		PyErr_PrintEx(0);
		return 0;
	}
	
	if(!space->hasSpaceData(key))
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::getSpaceData: (spaceID=%u, key=%s) not found!", 
			spaceID, key);

		PyErr_PrintEx(0);
		return 0;
	}

	return PyUnicode_FromString(space->getSpaceData(key).c_str());
}

//-------------------------------------------------------------------------------------
PyObject* Space::__py_DelSpaceData(PyObject* self, PyObject* args)
{
	SPACE_ID spaceID = 0;

	if(PyTuple_Size(args) != 2)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::delSpaceData: (argssize != (spaceID, key)) error!");
		PyErr_PrintEx(0);
		return 0;
	}
	
	char* key = NULL;
	if(PyArg_ParseTuple(args, "Is", &spaceID, &key) == -1)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::delSpaceData: args error!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(key == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::delSpaceData: key not is string!");
		PyErr_PrintEx(0);
		return 0;
	}

	if(strlen(key) == 0)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::delSpaceData: key is empty!");
		PyErr_PrintEx(0);
		return 0;
	}

	Space* space = Spaces::findSpace(spaceID);
	if(space == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::delSpaceData: (spaceID=%u) not found!", 
			spaceID);

		PyErr_PrintEx(0);
		return 0;
	}

	if(!space->hasSpaceData(key))
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::delSpaceData: (spaceID=%u, key=%s) not found!", 
			spaceID, key);

		PyErr_PrintEx(0);
		return 0;
	}
	
	if(kbe_stricmp(key, "_mapping") == 0)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::delSpaceData: key{_mapping} is protected!", 
			spaceID);

		PyErr_PrintEx(0);
		return 0;
	}

	space->delSpaceData(key);
	S_Return;
}

//-------------------------------------------------------------------------------------
}
