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

	auto ModelSettings::XmlFields()
	{
		return std::make_tuple(
			XmlAttribute { &ModelSettings::version,	"version" }
				.Default(uint8_t(-1))
				.Validator([](auto& v) { return v <= FormatVersion; }),
			XmlElement { &ModelSettings::name, "Name" },
			XmlElement { &ModelSettings::model, "Model" },
			XmlElement { &ModelSettings::embeddingModel, "Embedding" }
		);
	}

	auto ModelSettings::LLMModel::XmlFields()
	{
		static ModelSettings::LLMModel Defaults {};
		return std::make_tuple(
			XmlElement { &LLMModel::filename, "Path" },
			XmlElement { &LLMModel::promptTemplate, "PromptTemplate",
				[](auto& value) { return fig::util::enum_serialize(value, fig::llm::PromptTemplateMapping); },
				[](auto& value) { return fig::util::enum_deserialize(value, fig::llm::PromptTemplateMapping); } }
				.Default(PromptTemplateType::Default),
			XmlElement { &LLMModel::contextSize, "ContextSize",
				[](auto& value) { return fig::util::enum_serialize(value, ContextSizeMapping); },
				[](auto& value) { return fig::util::enum_deserialize(value, ContextSizeMapping, ContextSize::Undefined); } }
				.Default(Defaults.contextSize),
			XmlElement { &LLMModel::contextWindowKeepRatio, "ContextKeepRatio"}
				.Default(Defaults.contextWindowKeepRatio)
				.Validator([](auto& value) { return value >= 0.0f and value <= 1.0f; }),
			XmlElement { &LLMModel::gpuLayers, "GPULayers" }
				.Default(Defaults.gpuLayers),
			XmlElement { &LLMModel::bUseMlock, "UseMLock" }
				.Default(Defaults.bUseMlock),
			XmlElement { &LLMModel::bUseMmap, "UseMMap" }
				.Default(Defaults.bUseMmap),
			XmlElement { &LLMModel::microBatchSize, "MicroBatchSize" }
				.Default(Defaults.microBatchSize),
			XmlElement { &LLMModel::maxSequences, "MaxSequences" }
				.Default(Defaults.maxSequences)
		);
	}

	auto ModelSettings::EmbeddingModel::XmlFields()
	{
		return std::make_tuple(
			XmlElement { &EmbeddingModel::filename, "Path" }
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