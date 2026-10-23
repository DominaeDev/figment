#pragma once

#include "Figment.h"
#include "chat/ChatOptions.h"
#include "chat/UserDefinedOptions.h"
#include "chat/ChatTypes.h"
#include "text/Condition.h"
#include "io/Xml.h"
#include "io/XmlData.h"

namespace fig::data
{
	using ScenarioRule = fig::string; // for now

	class Scenario : public XmlData<"Scenario", 0>
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

			fig::chat::Role role;
			fig::handle id;
			fig::string label;
			fig::string brief;
			Validation validation {};
			Flags flags {};

			inline bool IsValid() const
			{
				return not (id.empty() or label.empty());
			}

			static auto XmlFields() noexcept
			{
				return Fields(
					Attribute { "id",			&RoleSlot::id },
					Element	{ "Label",		&RoleSlot::label },
					Element	{ "Brief",		&RoleSlot::brief },
					Element	{ "Condition",	&RoleSlot::validation },
					Element	{ "Flags",		&RoleSlot::flags,
						[](const Flags& value) { return enum_serialize_flags(value, FlagMapping); },
						[](auto& value) { return enum_deserialize_flags(value, FlagMapping); }
					}
				);

				static_assert(IsXmlSerializable<RoleSlot>);
			}
		};
		
		struct Event
		{
			enum class Type
			{
				Invalid,
				Instruct,
				Instigate,
				ChatMessage,
				ShowMessage,
			};

			static constexpr auto EventTypeMapping = std::array<std::pair<Type, fig::string_view>, 5> {
				std::pair { Type::Invalid,			"invalid" },
				std::pair { Type::Instruct,			"instruct" },
				std::pair { Type::Instigate,		"instigate" },
				std::pair { Type::ChatMessage,		"chat" },
				std::pair { Type::ShowMessage,		"info" },
			};

			Type type {};
			Condition condition {};
			fig::chat::Role role { fig::chat::Role::Undefined };
			fig::string content {};
			int32_t ttl { -1 };

			static auto XmlFields() noexcept
			{
				return Fields(
					Attribute { "type",			&Event::type,
						[](auto& value) { return enum_serialize(value, EventTypeMapping); },
						[](auto& value) { return enum_deserialize(value, EventTypeMapping); }
					}	.MustExist(),
					Attribute { "condition",	&Event::condition },
					Attribute { "role", &Event::role,
						[](auto& value) { return enum_serialize(value, fig::chat::RoleMapping); },
						[](auto& value) { return enum_deserialize(value, fig::chat::RoleMapping); }
					}	.SkipEmpty(),
					Attribute { "ttl",			&Event::ttl }
						.Default(-1),
					Text { &Event::content }
				);

				static_assert(IsXmlSerializable<Event>);
			}
		};

		struct Scene
		{
			fig::uuid id;
			fig::string name;
			fig::string content;
			std::vector<Event> events;

			static auto XmlFields() noexcept
			{
				return Fields(
					Attribute { "id",		&Scene::id },
					Element { "Name",		&Scene::name },
					Element { "Content",	&Scene::content },
					Element { "Event",		&Scene::events }
						.Collection("Events")
				);

				static_assert(IsXmlSerializable<Scene>);
			}
		};
		
		const std::vector<RoleSlot>& GetRoleSlots() const noexcept { return roles; }

		bool OnLoadFromXml(XmlReaderElement xml) override;
		bool OnValidate() const noexcept override;

		void SetRules(const std::vector<ScenarioRule>& rules) noexcept;
		void AppendRules(const std::vector<ScenarioRule>& rules) noexcept;
		void ClearRules() noexcept;
		const std::vector<ScenarioRule>& GetRules() const noexcept { return _rules; }

		fig::string GetTitle(const Context& context) const noexcept;
		Scene& CreateScene() noexcept;

		fig::string name;
		fig::string title;
		fig::string about;
		fig::string creator;
		fig::string version;
		fig::chat::UserDefinedOptions userOptions;
		std::vector<RoleSlot> roles {};
		std::vector<ScenarioRule> _rules;
		fig::string_list _tags;

		std::vector<Scene> scenes;
		fig::optional_cref<Scene> GetFirstScene() const noexcept;

	public:
		static auto XmlFields() noexcept
		{
			using namespace fig::io;

			return Fields(
				Element { "Name",			&Scenario::name },
				Element { "Title",			&Scenario::title },
				Element { "Description",	&Scenario::about },
				Element { "Creator",		&Scenario::creator },
				Element { "Version",		&Scenario::version },
				Element { "Tags",			&Scenario::_tags },
				Element { "Options",		&Scenario::userOptions },
				Element { "Role",			&Scenario::roles }
					.Collection("Roles"),
				Element { "Scene",			&Scenario::scenes }
					.Collection("Scenes"),
				Element { "Rule",			&Scenario::_rules }
					.Collection("Rules")
			);

			static_assert(IsXmlSerializable<Scenario>);
		}
	};
}
