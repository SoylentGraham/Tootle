/*------------------------------------------------------

	PC memory header - will be moved into it's own library eventually
 
-------------------------------------------------------*/
#pragma once

#include "../TLTypes.h"

#include <cstddef>	// std::size_t

namespace TLMemory
{
	namespace Platform
	{
		void				Initialise();
		void				Shutdown();

		void*	MemAlloc(u32 Size);
		void	MemDealloc(void* pMem);
		void	MemCopy(void* pDest,const void* pSrc,u32 Size);
		void	MemMove(void* pDest,const void* pSrc,u32 Size);
		void*	MemRealloc(void* pMem,u32 Size);
		void	MemValidate(void* pMem);

		std::size_t	MemSize(void* pMem);

		void	MemOuputAllocations();

#ifdef _DEBUG
		void	MemFillPattern(void* pMem, u32 Size, u8 Pattern);
#else
		FORCEINLINE void	MemFillPattern(void* pMem, u32 Size, u8 Pattern)	{}
#endif
	}
}

