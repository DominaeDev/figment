#pragma once

#include "Figment.h"
#include "io/XmlData.h"
#include "data/VersionNumber.h"

namespace fig::tts
{
	struct TTSBackendInfo
	{
		fig::uuid id;
		fig::string name;
		fig::data::VersionNumber version;
		fig::uuid packageId;

		struct Parameters
		{
			fig::string backend { };

			static auto XmlFields() noexcept
			{
				using namespace fig::data;
				return Fields(
					Element("Backend", &Parameters::backend)
						.Default("cuda")
				);
			}
		} parameters {};

		static auto XmlFields() noexcept
		{
			using namespace fig::data;
			return Fields(
				Attribute("id", &TTSBackendInfo::id)
					.MustExist(),
				Element("Name", &TTSBackendInfo::name)
					.MustExist(),
				Element("Package", &TTSBackendInfo::packageId),
				Element("Version", &TTSBackendInfo::version),
				Element("Parameters", &TTSBackendInfo::parameters)
			);
		}
	};

	struct TTSBackendSettings : fig::data::XmlData<"TTSBackends", 0>
	{
		std::vector<TTSBackendInfo> backends;

		static auto XmlFields() noexcept
		{
			using namespace fig::data;
			return Fields(
				Element("TTSBackend", &TTSBackendSettings::backends)
			);
			static_assert(IsXmlSerializable<TTSBackendSettings>);
		}
	};
};