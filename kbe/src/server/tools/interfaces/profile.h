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

#ifndef KBE_APP_PROFILE_H
#define KBE_APP_PROFILE_H

#include "common/common.h"
#include "helper/debug_helper.h"
#include "helper/profile.h"
#include "helper/eventhistory_stats.h"

namespace KBEngine{

extern ProfileVal SCRIPTCALL_PROFILE;
extern ProfileVal SCRIPTCALL_CREATEACCOUNT_PROFILE;
extern ProfileVal SCRIPTCALL_ACCOUNTLOGIN_PROFILE;
extern ProfileVal SCRIPTCALL_CHARGE_PROFILE;

extern EventHistoryStats g_privateClientEventHistoryStats;
extern EventHistoryStats g_publicClientEventHistoryStats;

}
#endif
