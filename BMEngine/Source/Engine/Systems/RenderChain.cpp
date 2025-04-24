#include "RenderChain.h"

#include <cassert>

namespace RenderChain
{
	static const u32 MaxDrawFunctions = 16;
	static DrawFunction Chain[MaxDrawFunctions];
	static u32 Index;

	void RegisterDraw(DrawFunction Function)
	{
		assert(Index < MaxDrawFunctions);
		Chain[Index] = Function;
		++Index;
	}

	void StartFrame()
	{
		for (u32 i = 0; i < Index; ++i)
		{
			Chain[i]();
		}
	}
}