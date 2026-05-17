#include <pch.h>
#include "io/data/ModelSettings.h"
#include "io/Xml.h"

using namespace fig::io;

namespace fig::llm
{
	static const fig::string XmlRootName { "ModelSettings" };

	ModelSettings::ModelSettings() : IXmlSerializable(XmlRootName),
		version { FormatVersion },
		model {},
		embeddingModel {}
	{
		static_assert(XmlSerializable<ModelSettings>);
		static_assert(XmlSerializable<ModelSettings::LLMModel>);
		static_assert(XmlSerializable<ModelSettings::EmbeddingModel>);
	}

	ModelSettings::LLMModel::LLMModel() :
		contextSize { Constants::DefaultModelSettings::ContextSize },
		contextWindowKeepRatio { Constants::DefaultModelSettings::ContextWindowKeepRatio },
		gpuLayers { Constants::DefaultModelSettings::GPULayers },
		bUseMlock { Constants::DefaultModelSettings::UseMLock },
		bUseMmap { Constants::DefaultModelSettings::UseMMap },
		microBatchSize { Constants::DefaultModelSettings::MicroBatchSize },
		maxSequences { Constants::DefaultModelSettings::MaxSequences }
	{
	}

	fig::string ModelSettings::LLMModel::SerializePromptTemplate(PromptTemplateType value)
	{
		return enum_serialize(value, fig::llm::PromptTemplateMapping);
	}

	PromptTemplateType ModelSettings::LLMModel::DeserializePromptTemplate(const fig::string& value)
	{
		return enum_deserialize(value, fig::llm::PromptTemplateMapping);
	}
}