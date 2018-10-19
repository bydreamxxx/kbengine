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
#include "entity.h"
#include "navigate_handler.h"	
#include "navigation/navigation.h"

namespace KBEngine{	


//-------------------------------------------------------------------------------------
NavigateHandler::NavigateHandler(KBEShared_ptr<Controller>& pController, const Position3D& destPos, 
											 float velocity, float distance, bool faceMovement, 
											 float maxMoveDistance, VECTOR_POS3D_PTR paths_ptr,
											PyObject* userarg):
MoveToPointHandler(pController, pController->pEntity()->layer(), pController->pEntity()->position(), velocity, distance, faceMovement, false, userarg),
realDestPos_(destPos),
realDistance(distance),
destPosIdx_(0),
paths_(paths_ptr),
maxMoveDistance_(maxMoveDistance)
{
	// ������Լ����Ӷ�����ѵ������ݴ���������ת�Q�ɱ�������
	Entity* self = this->pController_->pEntity();
	if (self && self->parent())
	{
		self->parent()->positionWorldToLocal((*paths_)[destPosIdx_++], destPos_);
	}
	else
	{
		destPos_ = (*paths_)[destPosIdx_++];
	}

	//���·�����յ����С�����⣬����С�������Ϊ0���Ա���ȫ�ִ�·�㣻������С����ȡ��ֵ
	Vector3 movement = destPos_ - realDestPos_;
	float dist = KBEVec3Length(&movement);
	if (dist >= realDistance)
	{
		distance_ = 0.0f;
	}
	else
	{
		distance_ = realDistance - dist;
	}
	
	updatableName = "NavigateHandler";
}

//-------------------------------------------------------------------------------------
NavigateHandler::NavigateHandler():
MoveToPointHandler(),
destPosIdx_(0),
paths_(),
maxMoveDistance_(0.f)
{
	updatableName = "NavigateHandler";
}

//-------------------------------------------------------------------------------------
NavigateHandler::~NavigateHandler()
{
}

//-------------------------------------------------------------------------------------
void NavigateHandler::addToStream(KBEngine::MemoryStream& s)
{
	MoveToPointHandler::addToStream(s);
	s << maxMoveDistance_;
}

//-------------------------------------------------------------------------------------
void NavigateHandler::createFromStream(KBEngine::MemoryStream& s)
{
	MoveToPointHandler::createFromStream(s);
	s >> maxMoveDistance_;
}

//-------------------------------------------------------------------------------------
bool NavigateHandler::requestMoveOver(const Position3D& oldPos)
{
	//�Ѿ���������·��Ͳ�����
	Vector3 realMovement = this->pController_->pEntity()->position() - realDestPos_;
	float realDist = KBEVec3Length(&realMovement);
	if (realDist <= distance_)
	{
		return MoveToPointHandler::requestMoveOver(oldPos);
	}

	if (destPosIdx_ == ((int)paths_->size()))
	{
		return MoveToPointHandler::requestMoveOver(oldPos);
	}
	else
	{
		// ������Լ����Ӷ�����ѵ������ݴ���������ת�Q�ɱ�������
		Entity* self = this->pController_->pEntity();
		if (self && self->parent())
		{
			self->parent()->positionWorldToLocal((*paths_)[destPosIdx_++], destPos_);
		}
		else
		{
			destPos_ = (*paths_)[destPosIdx_++];
		}

		//���·�����յ����С�����⣬����С�������Ϊ0���Ա���ȫ�ִ�·�㣻������С����ȡ��ֵ
		Vector3 movement = destPos_ - realDestPos_;
		float dist = KBEVec3Length(&movement);
		if (dist >= realDistance)
		{
			distance_ = 0.0f;
		}
		else
		{
			distance_ = realDistance - dist;
		}
	}

	return false;
}

//-------------------------------------------------------------------------------------
}

