#ifndef CONTEXT_SIZE_H__
#define CONTEXT_SIZE_H__
#pragma once

namespace fig::llm
{
	enum class ContextSize : int32_t
	{
		Undefined = 0,
		_2K = 1024 * 2,
		_3K = 1024 * 3,
		_4K = 1024 * 4,
		_6K = 1024 * 6,
		_8K = 1024 * 8,
		_10K = 1024 * 10,
		_12K = 1024 * 12,
		_16K = 1024 * 16,
		_24K = 1024 * 24,
		_32K = 1024 * 32,
	};

	const std::map<ContextSize, fig::string> ContextSizeMapping {
		{ ContextSize::Undefined, "Undefined" },
		{ ContextSize::_2K, "2K" },
		{ ContextSize::_3K, "3K" },
		{ ContextSize::_4K, "4K" },
		{ ContextSize::_6K, "6K" },
		{ ContextSize::_8K, "8K" },
		{ ContextSize::_10K, "10K" },
		{ ContextSize::_12K, "12K" },
		{ ContextSize::_16K, "16K" },
		{ ContextSize::_24K, "24K" },
		{ ContextSize::_32K, "32K" },
	};
}
#endif