// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com

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
MoveToPointHandler(pController, pController->pEntity()->layer(), pController->pEntity()->position(), velocity, distance, faceMovement, true, userarg),
realDestPos_(destPos),
destPosIdx_(0),
paths_(paths_ptr),
maxMoveDistance_(maxMoveDistance)
{
	// 如果我自己是子对象，则把导航数据从世界坐标转換成本地坐标
	Entity* self = this->pController_->pEntity();
	if (self && self->parent())
	{
		self->parent()->positionWorldToLocal((*paths_)[destPosIdx_++], destPos_);
	}
	else
	{
		destPos_ = (*paths_)[destPosIdx_++];
	}

	//如果路点在终点的最小距离外，则将最小距离调整为0，以便完全抵达路点；否则最小距离取差值
	Vector3 movement = destPos_ - realDestPos_;
	float dist = KBEVec3Length(&movement);
	if (dist >= distance_)
	{
		distance_ = 0.0f;
	}
	else
	{
		distance_ -= dist;
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
	//已经到达，后面的路点就不走了
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
		// 如果我自己是子对象，则把导航数据从世界坐标转換成本地坐标
		Entity* self = this->pController_->pEntity();
		if (self && self->parent())
		{
			self->parent()->positionWorldToLocal((*paths_)[destPosIdx_++], destPos_);
		}
		else
		{
			destPos_ = (*paths_)[destPosIdx_++];
		}

		//如果路点在终点的最小距离外，则将最小距离调整为0，以便完全抵达路点；否则最小距离取差值
		Vector3 movement = destPos_ - realDestPos_;
		float dist = KBEVec3Length(&movement);
		if (dist >= distance_)
		{
			distance_ = 0.0f;
		}
		else
		{
			distance_ -= dist;
		}
	}

	return false;
}

//-------------------------------------------------------------------------------------
}

