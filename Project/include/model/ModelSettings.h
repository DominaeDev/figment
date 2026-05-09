#ifndef MODEL_SETTINGS_H__
#define MODEL_SETTINGS_H__
#pragma once

#include "Types.h"
#include "llm/PromptTemplateTypes.h"

namespace fig::llm
{
	// Sizes
	enum class ContextSize : int32_t
	{
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

	struct ModelSettings
	{
		ModelSettings();
		~ModelSettings() = default;
		ModelSettings(const ModelSettings&) = default;
		ModelSettings(ModelSettings&&) = default;
		ModelSettings& operator=(const ModelSettings&) = default;
		ModelSettings& operator=(ModelSettings&&) = default;

		fig::path modelFilename;
		fig::path embeddingModelFilename;

		PromptTemplateType promptTemplate { PromptTemplateType::Undefined };
		ContextSize contextSize;
		float contextWindowKeepRatio;

		int32_t gpuLayers;
		bool bUseMlock;
		bool bUseMmap;
		int32_t microBatchSize;
		int32_t maxSequences;

	};
}

#endif