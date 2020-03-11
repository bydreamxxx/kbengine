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
#include "spaces.h"	
#include "loadnavmesh_threadtasks.h"
#include "server/serverconfig.h"
#include "common/deadline.h"
#include "navigation/navigation.h"
#include "navigation/navigation_tile_handle.h"

namespace KBEngine{

//-------------------------------------------------------------------------------------
bool LoadNavmeshTask::process()
{
	Navigation::getSingleton().loadNavigation(resPath_, params_);
	return false;
}

//-------------------------------------------------------------------------------------
thread::TPTask::TPTaskState LoadNavmeshTask::presentMainThread()
{
	NavigationHandlePtr pNavigationHandle = Navigation::getSingleton().findNavigation(resPath_);

	if (pNavigationHandle && pNavigationHandle->type() == NavigationHandle::NAV_TILE)
	{
		// 由于tile需要做碰撞， 每一个space都需要一份新的数据， 我们这里采用拷贝的方式来增加构造速度
		NavTileHandle* pNavTileHandle = new NavTileHandle(*(KBEngine::NavTileHandle*)pNavigationHandle);
		DEBUG_MSG(fmt::format("LoadNavmeshTask::presentMainThread: copy NavTileHandle({:p})!\n", (void*)pNavTileHandle));
		
		pNavigationHandle = NavigationHandlePtr(pNavTileHandle);
	}

	if (spaceID_ != 0)
	{
		Space* pSpace = Spaces::findSpace(spaceID_);
		if (pSpace == NULL || !pSpace->isGood())
		{
			ERROR_MSG(fmt::format("LoadNavmeshTask::presentMainThread(): not found space({})\n",
				spaceID_));
		}
		else
		{
			pSpace->onLoadedSpaceGeometryMapping(pNavigationHandle);
		}
	}
	else
	{
		// 通知脚本
		Cellapp::onLoadedGeometryMapping(resPath_);
		
	}

	return thread::TPTask::TPTASK_STATE_COMPLETED; 
}

//-------------------------------------------------------------------------------------
}
