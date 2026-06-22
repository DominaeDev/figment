#ifndef SCENARIO_DATA_H__
#define SCENARIO_DATA_H__
#pragma once

#include "Figment.h"
#include "chat/ChatOptions.h"
#include "chat/UserDefinedOptions.h"
#include "text/Condition.h"
#include "io/Xml.h"
#include "io/IXmlSerializable.h"

namespace fig::data
{
	struct Story
	{
		struct BlockSequence
		{
			enum class Type {
				Undefined,
				UserMessage,
				Message,
			};

			Type type {};
			fig::string content {};
			fig::chat::Role role {};
			Condition condition {};
			int32_t ttl { -1 };
		};

		struct Step
		{
			fig::handle id;
			Condition condition;
			int32_t ttl = -1;
			fig::string content;

			static auto SerializeInfo() noexcept
			{
				using namespace fig::io;

				return Fields(
					AsAttribute { "id",				&Step::id },
					AsElement { "Condition",		&Step::condition },
					AsElement { "Duration",		&Step::ttl },
					AsText { &Step::content }
				);

				static_assert(XmlSerializable<Step>);
			}
		};

		struct Chapter
		{
			fig::string title;
			std::vector<Step> steps;

			static auto SerializeInfo() noexcept
			{
				using namespace fig::io;

				return Fields(
					AsElement { "Title",		&Chapter::title },
					AsElement { "Step",			&Chapter::steps }
					.Collection("Steps")
				);

				static_assert(XmlSerializable<Chapter>);
			}
		};

		std::vector<Chapter> chapters;
		std::vector<BlockSequence> introBlocks;
		std::vector<BlockSequence> outroBlocks;

		fig::io::FileError LoadFromXml(fig::io::XmlReaderElement xml) noexcept;
		void SaveToXml(fig::io::XmlWriterElement xml) const noexcept;

		fig::optional_cref<Step> GetStep(size_t chapter, size_t step) const noexcept;
	};

	class ScenarioData : public fig::io::IXmlSerializable
	{
	public:
		struct RoleSlot
		{
			enum class Flag
			{
				Required = 1 << 0,	// Musn't be empty
				User = 1 << 1,	// Is user
			};
			using Flags = EnumFlags<Flag>;

			static constexpr std::array<std::pair<Flag, std::string_view>, 2> FlagMapping {
				std::pair { Flag::Required,	"required" },
				std::pair { Flag::User,		"user" },
			};

			fig::string id;
			fig::string label;
			fig::string brief;
			Validation validation {};
			Flags flags {};

			constexpr bool IsValid() const
			{
				return not (id.empty() or label.empty());
			}

			static auto SerializeInfo() noexcept
			{
				using namespace fig::io;

				return Fields(
					AsAttribute { "id",			&RoleSlot::id },
					AsElement { "Label",		&RoleSlot::label },
					AsElement { "Brief",		&RoleSlot::brief },
					AsElement { "Condition",	&RoleSlot::validation },
					AsElement { "Flags",		&RoleSlot::flags,
						[](const Flags& value) { return enum_serialize_flags(value, FlagMapping); },
						[](auto& value) { return enum_deserialize_flags(value, FlagMapping); }
					}
				);

				static_assert(XmlSerializable<RoleSlot>);
			}
		};

		ScenarioData() : fig::io::IXmlSerializable("Scenario")
		{
		}
		bool Validate() const noexcept override;

		const std::vector<RoleSlot>& GetRoleSlots() const noexcept { return roles; }
		std::pair<fig::string, fig::string> GetInfo() const noexcept { return std::make_pair(title, description); }
		const Story& GetStory() const noexcept { return story; }

	private:
		fig::string title;
		fig::string description;
		fig::chat::UserDefinedOptions userOptions;
		std::vector<RoleSlot> roles {};
		Story story {};

	public:
		static auto SerializeInfo() noexcept
		{
			using namespace fig::io;

			return Fields(
				AsElement { "Title",		&ScenarioData::title },
				AsElement { "Description",	&ScenarioData::description },
				AsElement { "Options",		&ScenarioData::userOptions },
				AsElement { "Role",			&ScenarioData::roles },
				AsElement { "Story",		&ScenarioData::story }
			);

			static_assert(XmlSerializable<ScenarioData>);
		}
	};
}

#endif