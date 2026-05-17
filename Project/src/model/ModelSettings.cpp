#include <pch.h>
#include "Constants.h"
#include "model/ModelSettings.h"
#include "util/StringUtility.h"
#include "fs/Xml.h"

using namespace fig::io;
using namespace fig::util;

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
		return fig::util::enum_serialize(value, fig::llm::PromptTemplateMapping);
	}

	PromptTemplateType ModelSettings::LLMModel::DeserializePromptTemplate(const fig::string& value)
	{
		return fig::util::enum_deserialize(value, fig::llm::PromptTemplateMapping);
	}
}