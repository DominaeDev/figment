#ifndef MODEL_SETTINGS_H__
#define MODEL_SETTINGS_H__
#pragma once

#include "Figment.h"
#include "llm/PromptTemplateTypes.h"
#include "io/FileError.h"
#include "io/IXmlSerializable.h"

namespace fig::llm
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
			PromptTemplateType promptTemplate { PromptTemplateType::Undefined };
			ContextSize contextSize;
			float contextWindowKeepRatio;
			uint8_t gpuLayers;
			bool bUseMlock;
			bool bUseMmap;
			int32_t microBatchSize;
			int32_t maxSequences;

		private:
			static fig::string SerializePromptTemplate(PromptTemplateType value);
			static PromptTemplateType DeserializePromptTemplate(const fig::string& value);

		public:
			static auto GetXmlFields()
			{
				using namespace fig::io;
				LLMModel Defaults {};
				return XmlFields(
					XmlElement { "Path", &LLMModel::filename },
					XmlElement { "PromptTemplate", &LLMModel::promptTemplate, SerializePromptTemplate, DeserializePromptTemplate }
						.Default(PromptTemplateType::Default),

					XmlElement { "ContextSize", &LLMModel::contextSize, XmlConvertEnum<ContextSize>, XmlParseEnum<ContextSize> }
						.Default(Defaults.contextSize)
						.Validator([](auto& n) { return static_cast<int32_t>(n) >= static_cast<int32_t>(ContextSize::_2K) and static_cast<int32_t>(n) <= static_cast<int32_t>(ContextSize::_32K); }),

					XmlElement { "ContextKeepRatio", &LLMModel::contextWindowKeepRatio, }
						.Default(Defaults.contextWindowKeepRatio)
						.Validator([](auto& value) { return value >= 0.0f and value <= 1.0f; }),

					XmlElement { "GPULayers", &LLMModel::gpuLayers }.Default(Defaults.gpuLayers),
					XmlElement { "UseMLock", &LLMModel::bUseMlock }.Default(Defaults.bUseMlock),
					XmlElement { "UseMMap", &LLMModel::bUseMmap }.Default(Defaults.bUseMmap),
					XmlElement { "MicroBatchSize", &LLMModel::microBatchSize }.Default(Defaults.microBatchSize),
					XmlElement { "MaxSequences", &LLMModel::maxSequences }.Default(Defaults.maxSequences)
				);
			}
		} model;

		struct EmbeddingModel
		{
			fig::path filename;

			static auto GetXmlFields()
			{
				using namespace fig::io;
				return XmlFields(
					XmlElement { "Path",	&EmbeddingModel::filename }
				);
			}
		} embeddingModel;

		static auto GetXmlFields()
		{
			using namespace fig::io;
			return XmlFields(
				XmlAttribute { "version",	&ModelSettings::version }
					.MustExist()
					.Validator([](auto& v) { return v <= FormatVersion; }),
				XmlElement { "Name",		&ModelSettings::name, },
				XmlElement { "Model",		&ModelSettings::model, },
				XmlElement { "Embedding",	&ModelSettings::embeddingModel, }
			);
		}
	};
}

#endif