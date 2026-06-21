#ifndef MODEL_SETTINGS_H__
#define MODEL_SETTINGS_H__
#pragma once

#include "Figment.h"
#include "llm/PromptTemplateTypes.h"
#include "io/FileError.h"
#include "io/IXmlSerializable.h"

namespace fig::data
{
	struct ModelSettings : public fig::io::IXmlSerializable
	{
		static constexpr uint8_t FormatVersion { 0 };

		ModelSettings();

		uint8_t version = FormatVersion;
		fig::string name;

		struct LLMModel
		{
			LLMModel();

			fig::path filename;
			fig::llm::PromptTemplateType promptTemplate { fig::llm::PromptTemplateType::Undefined };
			fig::llm::ContextSize contextSize;
			float contextWindowKeepRatio;
			uint8_t gpuLayers;
			bool bUseMlock;
			bool bUseMmap;
			int32_t microBatchSize;
			int32_t maxSequences;

		private:
			static fig::string SerializePromptTemplate(fig::llm::PromptTemplateType value);
			static fig::llm::PromptTemplateType DeserializePromptTemplate(const fig::string& value);

		public:
			static auto SerializeInfo()
			{
				using namespace fig::io;
				LLMModel Defaults {};
				return Fields(
					AsElement { "Path", &LLMModel::filename },
					AsElement { "PromptTemplate", &LLMModel::promptTemplate, SerializePromptTemplate, DeserializePromptTemplate }
						.Default(fig::llm::PromptTemplateType::Default),

					AsElement { "ContextSize", &LLMModel::contextSize, SerializeEnum<fig::llm::ContextSize>, DeserializeEnum<fig::llm::ContextSize> }
						.Default(Defaults.contextSize)
						.Validator([](auto& n) { return static_cast<int32_t>(n) >= static_cast<int32_t>(fig::llm::ContextSize::_2K) and static_cast<int32_t>(n) <= static_cast<int32_t>(fig::llm::ContextSize::_32K); }),

					AsElement { "ContextKeepRatio", &LLMModel::contextWindowKeepRatio, }
						.Default(Defaults.contextWindowKeepRatio)
						.Validator([](auto& value) { return value >= 0.0f and value <= 1.0f; }),

					AsElement { "GPULayers", &LLMModel::gpuLayers }.Default(Defaults.gpuLayers),
					AsElement { "UseMLock", &LLMModel::bUseMlock }.Default(Defaults.bUseMlock),
					AsElement { "UseMMap", &LLMModel::bUseMmap }.Default(Defaults.bUseMmap),
					AsElement { "MicroBatchSize", &LLMModel::microBatchSize }.Default(Defaults.microBatchSize),
					AsElement { "MaxSequences", &LLMModel::maxSequences }.Default(Defaults.maxSequences)
				);
			}
		} model;

		struct EmbeddingModel
		{
			fig::path filename;

			static auto SerializeInfo()
			{
				using namespace fig::io;
				return Fields(
					AsElement { "Path",	&EmbeddingModel::filename }
				);
			}
		} embeddingModel;

		static auto SerializeInfo()
		{
			using namespace fig::io;
			return Fields(
				AsAttribute { "version",	&ModelSettings::version }
					.MustExist()
					.Validator([](auto& v) { return v <= FormatVersion; }),
				AsElement { "Name",		&ModelSettings::name, },
				AsElement { "Model",		&ModelSettings::model, },
				AsElement { "Embedding",	&ModelSettings::embeddingModel, }
			);
		}
	};
}

#endif