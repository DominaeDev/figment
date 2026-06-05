#ifndef TEXT_EVALUATOR_H__
#define TEXT_EVALUATOR_H__
#pragma once

#include "text/Context.h"

namespace fig
{
	struct TextEvaluator
	{
		static fig::string Evaluate(const fig::string& input, const Context& context) noexcept;
	};
}

#endif