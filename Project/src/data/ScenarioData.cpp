#include <pch.h>
#include "data/ScenarioData.h"
#include "text/ConditionParser.h"
#include "io/Xml.h"
#include <cassert>

using namespace fig::io;
using namespace fig::chat;

namespace fig::data
{
	bool ScenarioData::Validate() const noexcept
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

	static void ReadBlockSequences(XmlReaderElement xml, std::vector<ScenarioData::BlockSequence>& seqs)
	{
		auto try_node = xml.GetFirstElementAny();
		while (try_node)
		{
			auto& node = try_node.value();
			auto nodeName = node.GetName();
			ScenarioData::BlockSequence blockSeq;

			if (nodeName == "UserMessage")
			{
				blockSeq.type = ScenarioData::BlockSequence::Type::UserMessage;
				blockSeq.role = Role::Undefined;
			}
			else if (nodeName == "Message")
			{
				blockSeq.type = ScenarioData::BlockSequence::Type::Message;
				blockSeq.role = enum_deserialize(node["role"].Get<fig::string>(), RoleMapping, Role::Undefined);
			}
			else if (nodeName == "Director")
			{
				blockSeq.type = ScenarioData::BlockSequence::Type::Message;
				blockSeq.role = Role::Director;
			}
			else if (nodeName == "Narrator")
			{
				blockSeq.type = ScenarioData::BlockSequence::Type::Message;
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

	FileError ScenarioData::Story::LoadFromXml(fig::io::XmlReaderElement xml) noexcept
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

	void ScenarioData::Story::SaveToXml(fig::io::XmlWriterElement xml) const noexcept
	{
		// ...
	}
}