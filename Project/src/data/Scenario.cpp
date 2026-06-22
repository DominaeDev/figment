#include <pch.h>
#include "data/Scenario.h"
#include "text/ConditionParser.h"
#include "io/Xml.h"
#include <cassert>

using namespace fig::io;
using namespace fig::chat;

namespace fig::data
{
	bool Scenario::Validate() const noexcept
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

	static void ReadBlockSequences(XmlReaderElement xml, std::vector<Story::BlockSequence>& seqs)
	{
		auto try_node = xml.GetFirstElementAny();
		while (try_node)
		{
			auto& node = try_node.value();
			auto nodeName = node.GetName();
			Story::BlockSequence blockSeq;

			if (nodeName == "UserMessage")
			{
				blockSeq.type = Story::BlockSequence::Type::UserMessage;
				blockSeq.role = Role::Undefined;
			}
			else if (nodeName == "Message")
			{
				blockSeq.type = Story::BlockSequence::Type::Message;
				blockSeq.role = enum_deserialize(node["role"].Get<fig::string>(), RoleMapping, Role::Undefined);
			}
			else if (nodeName == "Director")
			{
				blockSeq.type = Story::BlockSequence::Type::Message;
				blockSeq.role = Role::Director;
			}
			else if (nodeName == "Narrator")
			{
				blockSeq.type = Story::BlockSequence::Type::Message;
				blockSeq.role = Role::Narrator;
			}
			else
				goto next;

			blockSeq.content = node.GetValue<fig::string>();
			blockSeq.ttl = node["duration"].Get<int32_t>(-1);
			blockSeq.condition = Condition(node["duration"].Get<fig::string>(), true);

			seqs.emplace_back(std::move(blockSeq));

		next:
			try_node = node.GetNextSiblingAny();
		}
	}

	static void WriteBlockSequences(XmlWriterElement xml, const std::vector<Story::BlockSequence>& seqs)
	{
		for (auto& blockSeq : seqs)
		{
			fig::string elementName;
			switch (blockSeq.type)
			{
				case Story::BlockSequence::Type::Message:
					elementName = "Message";
					break;
				case Story::BlockSequence::Type::UserMessage:
					elementName = "UserMessage";
					break;
				default:
					continue;
			}
			auto node = xml.AddChild(elementName);

			if (blockSeq.role != Role::Undefined)
				node["role"].Set(enum_serialize(blockSeq.role, RoleMapping));
			node["duration"].Set(blockSeq.ttl);
			node["condition"].Set((fig::string)blockSeq.condition);
		}
	}

	FileError Story::LoadFromXml(fig::io::XmlReaderElement xml) noexcept
	{
		// Read chapters
		chapters.clear();
		if (auto chapterNode = xml.GetFirstElement("Chapter"))
		{
			while (chapterNode)
			{
				Chapter chapter;
				if (XmlDeserialize(chapterNode.value(), chapter))
					chapters.emplace_back(std::move(chapter));

				chapterNode = chapterNode.value().GetNextSibling();
			}
		}

		// Read intro
		if (auto introNode = xml.GetFirstElement("Intro"))
			ReadBlockSequences(introNode.value(), introBlocks);

		// Read outro
		if (auto outroNode = xml.GetFirstElement("Outro"))
			ReadBlockSequences(outroNode.value(), outroBlocks);

		return FileError::NoError;
	}

	void Story::SaveToXml(fig::io::XmlWriterElement xml) const noexcept
	{
		if (not introBlocks.empty())
		{
			auto introNode = xml.AddChild("Intro");
			WriteBlockSequences(introNode, introBlocks);
		}

		if (not outroBlocks.empty())
		{
			auto outroNode = xml.AddChild("Outro");
			WriteBlockSequences(outroNode, outroBlocks);
		}

		for (auto& chapter : chapters)
		{
			auto chapterNode = xml.AddChild("Chapter");
			XmlSerialize(chapterNode, chapter);
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