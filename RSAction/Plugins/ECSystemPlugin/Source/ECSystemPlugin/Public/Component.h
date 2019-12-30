/*
	Author : magi
	Date   : 2nd July, 2019
	File   : Component.h
	
	Base component class which provides a unique id.

	All Rights Reserved. (c) Copyright 2019.
*/

#ifndef __COMPONENT_H__
#define __COMPONENT_H__

#include "API.h"

#include "IComponent.h"
#include "util/FamilyTypeID.h"
/#include "Component.generated.h"

namespace ECS
{
	template<class T>
	class Component : public IComponent
	{

	public:

		static const ComponentTypeId STATIC_COMPONENT_TYPE_ID;

		Component() 
		{}

		virtual ~Component()
		{}		

		inline ComponentTypeId GetStaticComponentTypeID() const
		{
			return STATIC_COMPONENT_TYPE_ID;
		}	
	};

	// This private member only exists to force the compiler to create an instance of Component T,
	// which will set its unique identifier.
	template<class T>
	const ComponentTypeId Component<T>::STATIC_COMPONENT_TYPE_ID = util::Internal::FamilyTypeID<IComponent>::Get<T>();
}

#endif // __COMPONENT_H__