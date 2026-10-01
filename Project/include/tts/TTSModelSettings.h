#pragma once

#include "Figment.h"
#include "io/XmlData.h"
#include "tts/TTSTypes.h"
#include "data/VersionNumber.h"

namespace fig::tts
{
	struct VoiceModel
	{
		fig::uuid id;
		fig::uuid packageId;
		fig::string name;
		fig::string description;
		fig::string family;
		fig::string version;
		fig::string precision;
		bool recommended { false };

		struct Task
		{
			fig::tts::TTSTask task;
			fig::string id;
			
			static auto XmlFields() noexcept
			{
				using namespace fig::data;
				return Fields (
					Attribute("id", &Task::id)
						.MustExist(),
					Text(&Task::task,
						[](auto&& value) { return enum_serialize(value, fig::tts::TTSTaskMapping); },
						[](auto&& value) { return enum_deserialize(value, fig::tts::TTSTaskMapping); }
					)	.Validator([](auto& value) { return value != fig::tts::TTSTask::Undefined; })
				);
			}
		} task;

		struct Language
		{
			fig::string id;
			fig::string label;

			static auto XmlFields() noexcept
			{
				using namespace fig::data;
				return Fields(
					Attribute("id", &Language::id)
						.MustExist(),
					Text(&Language::label)
				);
			}
		};
		std::vector<Language> supportedLanguages;

		struct Parameters
		{
			fig::fixed temperature { 0_fp };
			int32_t topK { 0 };
			fig::fixed topP { 0_fp };
			fig::fixed repetitionPenalty { 0_fp };
			fig::fixed guidance { 0_fp };
			int32_t chunkSize { 0 };

			static auto XmlFields() noexcept
			{
				using namespace fig::data;
				return Fields(
					Element("Temperature", &Parameters::temperature)
						.Default(0_fp),
					Element("TopK", &Parameters::topK),
					Element("TopP", &Parameters::topP)
						.Default(0_fp),
					Element("RepetitionPenalty", &Parameters::repetitionPenalty)
						.Default(0_fp),
					Element("Guidance", &Parameters::guidance)
						.Default(0_fp),
					Element("ChunkSize", &Parameters::chunkSize)
				);
			}
		} parameters {};

		static auto XmlFields() noexcept
		{
			using namespace fig::data;
			return Fields(
				Attribute("id", &VoiceModel::id)
					.MustExist(),
				Element("Package", &VoiceModel::packageId)
					.MustExist(),
				Element("Name", &VoiceModel::name)
					.MustExist(),
				Element("Description", &VoiceModel::description),
				Element("Version", &VoiceModel::version),
				Element("Family", &VoiceModel::family),
				Element("Task", &VoiceModel::task)
					.MustExist(),
				Element("Precision", &VoiceModel::precision),
				Element("Recommended", &VoiceModel::recommended),
				Element("Language", &VoiceModel::supportedLanguages)
					.Collection("Languages")
					.MustExist(),
				Element("Parameters", &VoiceModel::parameters)
			);
		}
	};

	struct TTSModelSettings : fig::data::XmlData<"Models", 0>
	{
		std::vector<VoiceModel> models;

		static auto XmlFields() noexcept
		{
			using namespace fig::data;
			return Fields(
				Element("Model", &TTSModelSettings::models)
					.Collection("Models")
					.MustExist()
			);
			static_assert(IsXmlSerializable<TTSModelSettings>);
		}
	};
};