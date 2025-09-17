#include "llm/LLMTypes.h"

int32_t ContextSequence::AssignBlockPositions()
{
	int32_t offset = 0;
	for (auto& block : blocks)
	{
		block.offset = offset;
		offset += block.length();
	}
	return offset;
}
