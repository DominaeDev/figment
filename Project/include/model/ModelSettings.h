#ifndef MODEL_SETTINGS_H__
#define MODEL_SETTINGS_H__
#pragma once

#include "Types.h"
#include "llm/PromptTemplateTypes.h"
#include "fs/FileError.h"
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
		fig::io::FileError LoadFromXml(const fig::byte_span& input_buffer) noexcept;

		fig::io::FileError SaveToXml(const fig::path& path) const;
		void SaveToXml(fig::bytes& output_buffer) const;
		
	public:
		uint8_t version;
		fig::string name;

		struct LLMModel {
			LLMModel();

			fig::path filename;
			PromptTemplateType promptTemplate { PromptTemplateType::Undefined };
			ContextSize contextSize;
			float contextWindowKeepRatio;
			uint8_t gpuLayers;
			bool bUseMlock;
			bool bUseMmap;
			int32_t microBatchSize;
			int32_t maxSequences;

			static auto GetXmlFields();
		} model;

		struct EmbeddingModel {
			fig::path filename;

			static auto GetXmlFields();
		} embeddingModel;
	
	public:
		static auto GetXmlFields();
	};
}

#endif