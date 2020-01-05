/*
	Author : magi
	Date   : 3rd July, 2019
	File   : Entity.cpp
	
	Entity class.

	All Rights Reserved. (c) Copyright 2019.
*/

#include "IEntity.h"

namespace ECS
{
	//DEFINE_STATIC_LOGGER(IEntity, "Entity")
		
	IEntity::IEntity() :
		m_Active(true)
	{}

	IEntity::~IEntity()
	{}

	void IEntity::SetActive(bool active)
	{
		if (this->m_Active == active)
			return;

		if (active == false)
		{
			this->OnDisable();
		}
		else
		{
			this->OnEnable();
		}

		this->m_Active = active;
	}
}