#include <pch.h>
#include "data/Scenario.h"
#include "text/ConditionParser.h"
#include "io/Xml.h"
#include <cassert>

using namespace fig::io;
using namespace fig::chat;

namespace fig::data
{
	bool Scenario::OnLoadFromXml(XmlReaderElement xml)
	{
		size_t bot_index = 0uz;
		for (auto& slot : roles)
		{
			if (slot.flags.IsSet(RoleSlot::Flag::User))
				slot.role = Role::User;
			else
				slot.role = bot_from_index(bot_index++);
		}

		return true;
	}

	bool Scenario::OnValidate() const noexcept
	{
		if (roles.empty())
			return false;
		
		// Ensure the scenario can assign roles to at least one bot and one user
		int32_t idx_bot { -1 };
		int32_t idx_user { -1 };
		for (size_t idx = 0; idx < roles.size(); ++idx)
		{
			if (idx_user < 0 and roles[idx].flags.IsSet(RoleSlot::Flag::User))
			{
				idx_user = static_cast<int32_t>(idx);
				continue;
			}
			if (idx_bot < 0 and idx != idx_user)
			{
				idx_bot = static_cast<int32_t>(idx);
				continue;
			}
		}

		if (idx_bot == -1 or idx_user == -1)
			return false; // No exclusive roles
		return true;
	}

	static void ReadMessages(XmlReaderElement xml, std::vector<Story::Message>& messages)
	{
		auto try_node = xml.GetFirstElementAny();
		while (try_node)
		{
			auto& node = try_node.value();
			auto nodeName = node.GetName();
			Story::Message message;

			if (nodeName == "Info")
			{
				message.type = Story::Message::Type::UserMessage;
			}
			else if (nodeName == "InvokeBot")
			{
				message.type = Story::Message::Type::Message;
				message.role_handle = node["role"].Get<fig::string>();
				message.ttl = node["duration"].Get<int32_t>(-1);
			}
			else if (nodeName == "InvokeNarrator")
			{
				message.type = Story::Message::Type::Message;
				message.role_handle = "narrator";
				message.ttl = node["duration"].Get<int32_t>(-1);
			}
			else if (nodeName == "Instruct")
			{
				message.type = Story::Message::Type::Message;
				message.role_handle = "director";
				message.ttl = node["duration"].Get<int32_t>(2);
			}
			else
				goto next;

			message.condition = Condition(node["condition"].Get<fig::string>(), true);
			message.content = node.GetValue<fig::string>();

			messages.emplace_back(std::move(message));

		next:
			try_node = node.GetNextSiblingAny();
		}
	}

	static void WriteMessages(XmlWriterElement xml, const std::vector<Story::Message>& messages)
	{
		for (auto& message : messages)
		{
			fig::string elementName;
			switch (message.type)
			{
				case Story::Message::Type::Message:
					elementName = "Message";
					break;
				case Story::Message::Type::UserMessage:
					elementName = "UserMessage";
					break;
				default:
					continue;
			}
			auto node = xml.AddChild(elementName);

			if (not message.role_handle.empty())
				node["role"].Set(message.role_handle);
			node["duration"].Set(message.ttl);
			node["condition"].Set((fig::string)message.condition);
		}
	}

	FileError Story::LoadFromXml(XmlReaderElement xml) noexcept
	{
		// Read chapters
		chapters.clear();
		if (auto chaptersNode = xml.GetFirstElement("Chapters"))
		{
			if (auto chapterNode = (*chaptersNode).GetFirstElement("Chapter"))
			{
				while (chapterNode)
				{
					Chapter chapter;
					if (XmlDeserialize(chapterNode.value(), chapter))
						chapters.emplace_back(std::move(chapter));

					chapterNode = chapterNode.value().GetNextSibling();
				}
			}
		}

		// Read intro
		if (auto introNode = xml.GetFirstElement("Intro"))
			ReadMessages(introNode.value(), intro);

		// Read outro
		if (auto outroNode = xml.GetFirstElement("Outro"))
			ReadMessages(outroNode.value(), outro);

		return FileError::NoError;
	}

	void Story::SaveToXml(XmlWriterElement xml) const noexcept
	{
		if (not intro.empty())
		{
			auto introNode = xml.AddChild("Intro");
			WriteMessages(introNode, intro);
		}

		if (not outro.empty())
		{
			auto outroNode = xml.AddChild("Outro");
			WriteMessages(outroNode, outro);
		}

		if (not chapters.empty())
		{
			auto chaptersNode = xml.AddChild("Chapters");
			for (auto& chapter : chapters)
			{
				auto chapterNode = chaptersNode.AddChild("Chapter");
				XmlSerialize(chapterNode, chapter);
			}
		}
	}

	fig::optional_cref<Story::Step> Story::GetStep(size_t idx_chapter, size_t idx_step) const noexcept
	{
		if (idx_chapter >= chapters.size())
			return std::nullopt;

		auto& chapter = chapters[idx_chapter];
		if (idx_step >= chapter.steps.size())
			return std::nullopt;

		return chapter.steps[idx_step];
	}
}