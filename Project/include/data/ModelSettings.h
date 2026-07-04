#pragma once

#include "Figment.h"
#include "llm/PromptTemplateTypes.h"
#include "io/Error.h"
#include "io/XmlData.h"

namespace fig::data
{
	struct ModelSettings : public XmlData<"ModelSettings", 0>
	{
		ModelSettings();

		fig::string name;

		struct LLMModel
		{
			LLMModel();

			fig::path filename;
			fig::llm::PromptTemplateType promptTemplate { fig::llm::PromptTemplateType::Undefined };
			int32_t contextSize;
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
			static auto XmlFields()
			{
				LLMModel Defaults {};

				return Fields(
					Element { "Path", &LLMModel::filename },
					Element { "PromptTemplate", &LLMModel::promptTemplate, SerializePromptTemplate, DeserializePromptTemplate }
						.Default(fig::llm::PromptTemplateType::Default),

					Element { "ContextSize", &LLMModel::contextSize }
						.Default(Defaults.contextSize)
						.Validator([](auto& n) { return static_cast<int32_t>(n) >= static_cast<int32_t>(fig::llm::ContextSize::_2K) and static_cast<int32_t>(n) <= static_cast<int32_t>(fig::llm::ContextSize::_32K); }),

					Element { "ContextKeepRatio", &LLMModel::contextWindowKeepRatio, }
						.Default(Defaults.contextWindowKeepRatio)
						.Validator([](auto& value) { return value >= 0.0f and value <= 1.0f; }),

					Element { "GPULayers", &LLMModel::gpuLayers }.Default(Defaults.gpuLayers),
					Element { "UseMLock", &LLMModel::bUseMlock }.Default(Defaults.bUseMlock),
					Element { "UseMMap", &LLMModel::bUseMmap }.Default(Defaults.bUseMmap),
					Element { "MicroBatchSize", &LLMModel::microBatchSize }.Default(Defaults.microBatchSize),
					Element { "MaxSequences", &LLMModel::maxSequences }.Default(Defaults.maxSequences)
				);
			}
		} model;

		struct EmbeddingModel
		{
			fig::path filename;

			static auto XmlFields()
			{
				return Fields(
					Element { "Path",	&EmbeddingModel::filename }
				);
			}
		} embeddingModel;

		static auto XmlFields()
		{
			return Fields(
				Element { "Name",			&ModelSettings::name, },
				Element { "Model",		&ModelSettings::model, },
				Element { "Embedding",	&ModelSettings::embeddingModel, }
			);
		}
	};
}
