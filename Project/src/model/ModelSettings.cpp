#include <pch.h>
#include "Constants.h"
#include "model/ModelSettings.h"
#include "util/StringUtility.h"
#include "fs/Xml.h"

using namespace fig::io;
using namespace fig::util;

namespace fig::llm
{
	static constexpr uint8_t FormatVersion { 0 };
	static const fig::string XmlRootName { "ModelSettings" };

	ModelSettings::ModelSettings() :
		version { FormatVersion },
		model {},
		embeddingModel {}
	{
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

	auto ModelSettings::GetXmlFields()
	{
		return XmlFields(
			XmlAttribute { "version",	&ModelSettings::version }
				.Default(uint8_t(-1))
				.Validator([](auto& v) { return v <= FormatVersion; }),
			XmlElement { "Name",		&ModelSettings::name, },
			XmlElement { "Model",		&ModelSettings::model, },
			XmlElement { "Embedding",	&ModelSettings::embeddingModel, }
		);
	}

	auto ModelSettings::LLMModel::GetXmlFields()
	{
		namespace Defaults = fig::Constants::DefaultModelSettings;
		return XmlFields(
			XmlElement { "Path", &LLMModel::filename},
			XmlElement { "PromptTemplate", &LLMModel::promptTemplate,
				[](auto& value) { return fig::util::enum_serialize(value, fig::llm::PromptTemplateMapping); },
				[](auto& value) { return fig::util::enum_deserialize(value, fig::llm::PromptTemplateMapping); } }
				.Default(PromptTemplateType::Default),
			XmlElement { "ContextSize", &LLMModel::contextSize, XmlConvertEnum<ContextSize>, XmlParseEnum<ContextSize> }
				.Default(Defaults::ContextSize)
				.Validator([](auto& n) { return static_cast<int32_t>(n) >= static_cast<int32_t>(ContextSize::_2K) and static_cast<int32_t>(n) <= static_cast<int32_t>(ContextSize::_32K); }),
			XmlElement { "ContextKeepRatio", &LLMModel::contextWindowKeepRatio, }
				.Default(Defaults::ContextWindowKeepRatio)
				.Validator([](auto& value) { return value >= 0.0f and value <= 1.0f; }),
			XmlElement { "GPULayers", &LLMModel::gpuLayers }
				.Default(Defaults::GPULayers),
			XmlElement { "UseMLock", &LLMModel::bUseMlock }
				.Default(Defaults::UseMLock),
			XmlElement { "UseMMap", &LLMModel::bUseMmap }
				.Default(Defaults::UseMMap),
			XmlElement { "MicroBatchSize", &LLMModel::microBatchSize }
				.Default(Defaults::MicroBatchSize),
			XmlElement { "MaxSequences", &LLMModel::maxSequences }
				.Default(Defaults::MaxSequences)
		);
	}

	auto ModelSettings::EmbeddingModel::GetXmlFields()
	{
		return XmlFields(
			XmlElement { "Path", &EmbeddingModel::filename }
		);
	}

	FileError ModelSettings::LoadFromXml(const fig::path& path) noexcept
	{
		if (not (std::filesystem::exists(path) and std::filesystem::is_regular_file(path)))
			return FileError::NotFound;

		XmlReader xml(path, XmlRootName);
		if (not xml.IsOk())
			return FileError::UnrecognizedFormat;

		auto rootNode = xml.GetRoot();
		if (not XmlDeserialize(rootNode, *this))
			return FileError::UnrecognizedFormat;

		return FileError::NoError;
	}

	FileError ModelSettings::LoadFromXml(const fig::byte_span& buffer) noexcept
	{
		XmlReader xml(fig::string_view { (const char*)buffer.data(), buffer.size() });
		if (not xml.IsOk() or xml.GetRoot().GetName() != XmlRootName)
			return FileError::UnrecognizedFormat;

		auto rootNode = xml.GetRoot();
		if (not XmlDeserialize(rootNode, *this))
			return FileError::UnrecognizedFormat;

		return FileError::NoError;
	}

	FileError ModelSettings::SaveToXml(const fig::path& path) const
	{
		XmlWriter xml = XmlSerialize(XmlRootName, *this);
		if (xml.WriteToFile(path))
			return FileError::NoError;
		return FileError::WriteError;
	}

	void ModelSettings::SaveToXml(fig::bytes& buffer) const
	{
		XmlWriter xml = XmlSerialize(XmlRootName, *this);
		xml.WriteToMemory(buffer);
	}
}