#include <pch.h>
#include "Constants.h"
#include "model/ModelSettings.h"
#include "fs/Xml.h"
#include "util/StringUtility.h"

using namespace fig::io;
using namespace fig::util;

namespace fig::llm
{
	static constexpr uint8_t FormatVersion { 0 };
	static const fig::string XmlRootName { "ModelSettings" };

	ModelSettings::ModelSettings() :
		version { FormatVersion },
		contextSize { Constants::DefaultModelSettings::ContextSize },
		contextWindowKeepRatio { Constants::DefaultModelSettings::ContextWindowKeepRatio },
		gpuLayers { Constants::DefaultModelSettings::GPULayers },
		bUseMlock { Constants::DefaultModelSettings::UseMlock },
		bUseMmap { Constants::DefaultModelSettings::UseMmap },
		microBatchSize { Constants::DefaultModelSettings::MicroBatchSize },
		maxSequences { Constants::DefaultModelSettings::MaxSequences }
	{}

	static bool ReadXml(XmlReader& xml, ModelSettings& data)
	{
		if (not xml.IsOk())
			return false;

		auto rootNode = xml.GetRootElement();

		data.version = rootNode["version"].AsInt<uint8_t>().value_or(0);
		data.name = trim(rootNode.GetElementText("Name").value_or("Untitled"));
		data.modelFilename = trim(rootNode.GetElementText("Model").value_or(""));
		data.embeddingModelFilename = trim(rootNode.GetElementText("EmbeddingModel").value_or(""));

		if (auto try_prompt = rootNode.GetElementText("PromptTemplate"))
			data.promptTemplate = find_key(PromptTemplateMapping, try_prompt.value()).value_or(PromptTemplateType::Default);
		else
			data.promptTemplate = PromptTemplateType::Default;

		if (auto try_context_size = rootNode.GetElementText("ContextSize"))
			data.contextSize = find_key(ContextSizeMapping, try_context_size.value()).value_or(Constants::DefaultModelSettings::ContextSize);
		else
			data.contextSize = Constants::DefaultModelSettings::ContextSize;

		data.contextWindowKeepRatio = std::clamp(rootNode.GetElementFloat("ContextKeepRatio", Constants::DefaultModelSettings::ContextWindowKeepRatio), 0.0f, 1.0f);
		data.gpuLayers = std::max(rootNode.GetElementInt("GPULayers", Constants::DefaultModelSettings::GPULayers), 0);
		data.bUseMlock = rootNode.GetElementBool("UseMLock", Constants::DefaultModelSettings::UseMlock);
		data.bUseMmap = rootNode.GetElementBool("UseMMap", Constants::DefaultModelSettings::UseMmap);
		data.microBatchSize = std::max(rootNode.GetElementInt("MicroBatchSize", Constants::DefaultModelSettings::MicroBatchSize), 0);
		data.maxSequences = std::max(rootNode.GetElementInt("MaxSequences", Constants::DefaultModelSettings::MaxSequences), 0);
		return true;
	}

	static bool WriteXml(XmlWriter& xml, const ModelSettings& data)
	{
		auto rootNode = xml.GetRoot();

		rootNode["version"] = static_cast<int32_t>(data.version);
		rootNode.SetElementValue("Name", data.name);
		rootNode.SetElementValue("Model", data.modelFilename.u8string());
		rootNode.SetElementValue("EmbeddingModel", data.embeddingModelFilename.u8string());
		if (PromptTemplateMapping.contains(data.promptTemplate))
			rootNode.SetElementValue("PromptTemplate", PromptTemplateMapping.at(data.promptTemplate));
		if (ContextSizeMapping.contains(data.contextSize))
			rootNode.SetElementValue("ContextSize", ContextSizeMapping.at(data.contextSize));
		rootNode.SetElementValue("ContextKeepRatio", data.contextWindowKeepRatio);
		rootNode.SetElementValue("GPULayers", data.gpuLayers);
		rootNode.SetElementValue("UseMLock", data.bUseMlock);
		rootNode.SetElementValue("UseMMap", data.bUseMmap);
		rootNode.SetElementValue("MicroBatchSize", data.microBatchSize);
		rootNode.SetElementValue("MaxSequences", data.maxSequences);

		return true;
	}

	FileError ModelSettings::LoadFromXml(const fig::path& path) noexcept
	{
		if (not (std::filesystem::exists(path) and std::filesystem::is_regular_file(path)))
			return FileError::NotFound;

		XmlReader xml(path, XmlRootName);
		if (not xml.IsOk())
			return FileError::UnrecognizedFormat;

		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	FileError ModelSettings::LoadFromXml(const fig::bytes& buffer) noexcept
	{
		XmlReader xml(fig::string_view { (const char*)buffer.data(), buffer.size() });
		if (not xml.IsOk() or xml.GetRootElement().GetName() != XmlRootName)
			return FileError::UnrecognizedFormat;

		return ReadXml(xml, *this) ? FileError::NoError : FileError::UnrecognizedFormat;
	}

	FileError ModelSettings::SaveToXml(const fig::path& path) const
	{
		XmlWriter xml(XmlRootName);
		WriteXml(xml, *this);

		if (xml.Save(path))
			return FileError::NoError;
		return FileError::WriteError;
	}

	void ModelSettings::SaveToXml(fig::bytes& buffer) const
	{
		XmlWriter xml(XmlRootName);
		WriteXml(xml, *this);
		xml.SaveToMemory(buffer);
	}
}