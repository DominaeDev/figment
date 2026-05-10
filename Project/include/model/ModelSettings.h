#ifndef MODEL_SETTINGS_H__
#define MODEL_SETTINGS_H__
#pragma once

#include "Types.h"
#include "llm/PromptTemplateTypes.h"
#include "fs/FileError.h"

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

	const std::map<ContextSize, fig::string> ContextSizeMapping {
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

	struct ModelSettings
	{
		ModelSettings();
		~ModelSettings() = default;
		ModelSettings(const ModelSettings&) = default;
		ModelSettings(ModelSettings&&) = default;
		ModelSettings& operator=(const ModelSettings&) = default;
		ModelSettings& operator=(ModelSettings&&) = default;

		fig::io::FileError LoadFromXml(const fig::path& path) noexcept;
		fig::io::FileError LoadFromXml(const fig::bytes& buffer) noexcept;

		fig::io::FileError SaveToXml(const fig::path& path) const;
		void SaveToXml(fig::bytes& buffer) const;
		
	public:
		uint8_t version;
		fig::string name;
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