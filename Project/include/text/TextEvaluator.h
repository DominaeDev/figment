#ifndef TEXT_EVALUATOR_H__
#define TEXT_EVALUATOR_H__
#pragma once

#include "text/Contextual.h"

namespace fig::text
{
	class TextEvaluator
	{
	public:
		static fig::string Evaluate(const fig::string& input, const Contextual& context) noexcept;
	};
}

#endif