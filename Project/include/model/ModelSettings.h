#ifndef MODEL_SETTINGS_H__
#define MODEL_SETTINGS_H__
#pragma once

#include "Types.h"
#include "llm/PromptTemplateTypes.h"
#include "fs/FileError.h"
#include "fs/XmlSerializable.h"
#include "util/Common.h"

namespace fig::llm
{
	// Sizes
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
	
	public:
		static constexpr auto XmlFields()
		{
			return std::make_tuple(
				fig::io::AsAttribute	{ "version",			&ModelSettings::version },
				fig::io::AsElement		{ "Name",				&ModelSettings::name },
				fig::io::AsElement		{ "Model",				&ModelSettings::modelFilename},
				fig::io::AsElement		{ "PromptTemplate",		&ModelSettings::promptTemplate, 
					[](auto& value) { return fig::util::enum_serialize(value, PromptTemplateMapping); },
					[](auto& value) { return fig::util::enum_deserialize(value, PromptTemplateMapping, PromptTemplateType::Undefined); }
				},

				fig::io::AsElement		{ "ContextSize",		&ModelSettings::contextSize, 
					[](auto& value) { return fig::util::enum_serialize(value, ContextSizeMapping); },
					[](auto& value) { return fig::util::enum_deserialize(value, ContextSizeMapping, ContextSize::Undefined); }
				},
				fig::io::AsElement		{ "ContextKeepRatio",	&ModelSettings::contextWindowKeepRatio },
				fig::io::AsElement		{ "GPULayers",			&ModelSettings::gpuLayers },
				fig::io::AsElement		{ "UseMLock",			&ModelSettings::bUseMlock },
				fig::io::AsElement		{ "UseMMap",			&ModelSettings::bUseMmap },
				fig::io::AsElement		{ "MicroBatchSize",		&ModelSettings::microBatchSize},
				fig::io::AsElement		{ "MaxSequences",		&ModelSettings::maxSequences },
				fig::io::AsElement		{ "EmbeddingModel",		&ModelSettings::embeddingModelFilename }
			);
		}
	};
}

#endif