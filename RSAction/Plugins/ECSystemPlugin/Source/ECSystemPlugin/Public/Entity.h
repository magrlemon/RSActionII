/*
	Author : magi
	Date   : 3rd July, 2019
	File   : Entity.h
	
	Enity class.

	All Rights Reserved. (c) Copyright 2019.
*/

#ifndef __ENTITY_H__
#define __ENTITY_H__


#include "IEntity.h"

namespace ECS {

	///-------------------------------------------------------------------------------------------------
	/// Class:	Entity
	///
	/// Summary:	CRTP class. Any entity object should derive form the Entity class and passes itself
	/// as template parameter to the Entity class.
	///
	/// Author:	magi
	///
	/// Date:	30/09/2019
	///
	/// Typeparams:
	/// E - 	Type of the e.
	///-------------------------------------------------------------------------------------------------

	template<class E>
	class Entity : public IEntity
	{
		// Entity destruction always happens through EntityManager !!!
		void operator delete(void*) = delete;
		void operator delete[](void*) = delete;

	public:

		static const EntityTypeId STATIC_ENTITY_TYPE_ID;

	public:

		virtual const EntityTypeId GetStaticEntityTypeID() const override { return STATIC_ENTITY_TYPE_ID; }

		Entity() 
		{}

		virtual ~Entity()
		{}
	};

	// set unique type id for this Entity<T>
	template<class E>
	const EntityTypeId Entity<E>::STATIC_ENTITY_TYPE_ID = util::Internal::FamilyTypeID<IEntity>::Get<E>();
}

#endif // __ENTITY_H__