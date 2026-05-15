#include <pch.h>
#include "Constants.h"
#include "model/ModelSettings.h"
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
		XmlDeserialize(rootNode, data);
	
		return data.version <= FormatVersion
			and not data.modelFilename.empty()
			and data.gpuLayers > 0u;
	}

	static bool WriteXml(XmlWriter& xml, const ModelSettings& data)
	{
		auto rootNode = xml.GetRoot();
		XmlSerialize(rootNode, data);
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

		if (xml.WriteToFile(path))
			return FileError::NoError;
		return FileError::WriteError;
	}

	void ModelSettings::SaveToXml(fig::bytes& buffer) const
	{
		XmlWriter xml(XmlRootName);
		WriteXml(xml, *this);
		xml.WriteToMemory(buffer);
	}
}