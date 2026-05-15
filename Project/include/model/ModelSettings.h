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
	struct ModelSettings
	{
		ModelSettings();
		ModelSettings(const ModelSettings&) = default;
		ModelSettings(ModelSettings&&) = default;
		ModelSettings& operator=(const ModelSettings&) = default;
		ModelSettings& operator=(ModelSettings&&) = default;
		~ModelSettings() = default;

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

		uint8_t gpuLayers;
		bool bUseMlock;
		bool bUseMmap;
		int32_t microBatchSize;
		int32_t maxSequences;
	
	public:
		static constexpr auto XmlFields()
		{
			static ModelSettings Defaults {};
			return std::make_tuple(
				fig::io::AsAttribute { &ModelSettings::version,	"version", uint8_t(-1) },

				fig::io::AsElement { &ModelSettings::name, "Name" },
				fig::io::AsElement { &ModelSettings::modelFilename, "Model" },
				fig::io::AsElement { &ModelSettings::promptTemplate, "PromptTemplate", Defaults.promptTemplate,
					[](auto& value) { return fig::util::enum_deserialize(value, fig::llm::PromptTemplateMapping); },
					[](auto& value) { return fig::util::enum_serialize(value, fig::llm::PromptTemplateMapping); }
				},
				fig::io::AsElement { &ModelSettings::contextSize, "ContextSize", Defaults.contextSize,
					[](auto& value) { return fig::util::enum_deserialize(value, ContextSizeMapping, ContextSize::Undefined); },
					[](auto& value) { return fig::util::enum_serialize(value, ContextSizeMapping); }
				},
				fig::io::AsElement { &ModelSettings::contextWindowKeepRatio, "ContextKeepRatio", Defaults.contextWindowKeepRatio,
					[](auto& value) { return std::clamp(value, 0.0f, 1.0f); }
				},
				fig::io::AsElement { &ModelSettings::gpuLayers, "GPULayers", Defaults.gpuLayers },
				fig::io::AsElement { &ModelSettings::bUseMlock, "UseMLock", Defaults.bUseMlock },
				fig::io::AsElement { &ModelSettings::bUseMmap, "UseMMap", Defaults.bUseMmap },
				fig::io::AsElement { &ModelSettings::microBatchSize, "MicroBatchSize", Defaults.microBatchSize },
				fig::io::AsElement { &ModelSettings::maxSequences, "MaxSequences", Defaults.maxSequences },
				fig::io::AsElement { &ModelSettings::embeddingModelFilename, "EmbeddingModel" }
			);
		}
	};
}

#endif