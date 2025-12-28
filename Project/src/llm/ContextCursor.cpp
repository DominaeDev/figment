#include <pch.h>
#include "llm/ContextCursor.h"

using namespace fig::llm;

ContextCursor ContextCursor::Invalid { -1 };

ContextCursor::ContextCursor(int32_t value) :
	_value {value}
{
}

