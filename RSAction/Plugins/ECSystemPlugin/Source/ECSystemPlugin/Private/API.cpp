///-------------------------------------------------------------------------------------------------
/// File:	src\API.cpp.
///
/// Summary:	Implements the API.


#include "API.h"

#include "Memory/ECSMM.h"

#include "ECSEngine.h"

namespace ECS
{
	namespace Log {

		namespace Internal {

		}

	} // namespace Log


	namespace Memory { 

		namespace Internal {

			MemoryManager*				ECSMemoryManager = new Memory::Internal::MemoryManager();
		}


		GlobalMemoryUser::GlobalMemoryUser() : ECS_MEMORY_MANAGER(Internal::ECSMemoryManager)
		{}

		GlobalMemoryUser::~GlobalMemoryUser()
		{}

		inline const void* GlobalMemoryUser::Allocate(size_t memSize, const char* user)
		{
			return ECS_MEMORY_MANAGER->Allocate(memSize, user);
		}

		inline void GlobalMemoryUser::Free(void* pMem)
		{
			ECS_MEMORY_MANAGER->Free(pMem);
		}

	} // namespace Memory

	ECSEngine*		ECS_Engine = nullptr;// new ECSEngine();

	void Initialize()
	{
		if(ECS_Engine == nullptr)
		ECS_Engine = new ECSEngine();
	}

	void Terminate()
	{
		if (ECS_Engine != nullptr)
		{
			delete ECS_Engine;
			ECS_Engine = nullptr;
		}

		// check for memory leaks
		Memory::Internal::ECSMemoryManager->CheckMemoryLeaks();
	}
} // namespace ECS