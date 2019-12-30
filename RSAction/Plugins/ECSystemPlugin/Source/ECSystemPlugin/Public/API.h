///-------------------------------------------------------------------------------------------------
/// File:	include\API.h.
///
/// Summary:	API.
/// 

/*
Preprocessor defines:

	ECS_DISABLE_LOGGING			- Disable logging feature.



*/

#ifndef __ECS_API_H__
#define __ECS_API_H__
#pragma once

#define ENITY_LUT_GROW						1024

#define ENITY_T_CHUNK_SIZE					512

#define COMPONENT_LUT_GROW					1024

#define COMPONENT_T_CHUNK_SIZE				512

// 4MB 
#define ECS_EVENT_MEMORY_BUFFER_SIZE		4194304

// 8MB
#define ECS_SYSTEM_MEMORY_BUFFER_SIZE		8388608


#include "Platform.h"

namespace ECS 
{
	namespace Log {

		namespace Internal
		{

		}
	}

	namespace Memory
	{
		namespace Internal
		{
			class  MemoryManager;
			extern MemoryManager*				ECSMemoryManager;
		}
	}

	namespace Event
	{
		class EventHandler;
	}


	class EntityManager;
	class SystemManager;
	class ComponentManager;



	namespace Memory
	{
		///-------------------------------------------------------------------------------------------------
		/// Class:	GlobalMemoryUser
		///
		/// Summary:	Any class that wants to use the global memory must derive from this class.
		///
		/// Author:	magi
		///
		/// Date:	9/09/2019
		///-------------------------------------------------------------------------------------------------
		
		class ECSYSTEMPLUGIN_API GlobalMemoryUser
		{
		private:

			Internal::MemoryManager* ECS_MEMORY_MANAGER;

		public:

			GlobalMemoryUser();
			virtual ~GlobalMemoryUser();

			inline const void* Allocate(size_t memSize, const char* user = nullptr);
			inline void Free(void* pMem);
		};

	} // namespace ECS::Memory



	class ECSEngine;

	ECSYSTEMPLUGIN_API extern ECSEngine*		ECS_Engine;

	ECSYSTEMPLUGIN_API void					Initialize();
	ECSYSTEMPLUGIN_API void					Terminate();

} // namespace ECS

#endif // __ECS_API_H__