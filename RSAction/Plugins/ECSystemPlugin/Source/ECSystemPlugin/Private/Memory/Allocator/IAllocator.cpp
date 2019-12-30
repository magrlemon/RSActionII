/*
	Author : magi
	Date   : 9th July, 2019
	File   : IAllocator.cpp

	Base allocator class.

	All Rights Reserved. (c) Copyright 2019.
*/

#include "Memory/Allocator/IAllocator.h"

namespace ECS { namespace Memory { namespace Allocator {

	IAllocator::IAllocator(const size_t memSize, const void* mem) :
		m_MemorySize(memSize),
		m_MemoryFirstAddress(mem),
		m_MemoryUsed(0),
		m_MemoryAllocations(0)
	{}

	IAllocator::~IAllocator()
	{}

}}} // ECS::Memory::Allocator