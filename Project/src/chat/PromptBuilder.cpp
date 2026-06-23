#include <pch.h>
#include "chat/PromptBuilder.h"
#include "chat/ChatStaging.h"
#include "text/TextEvaluator.h"

using namespace fig::data;

namespace fig::chat
{
	static void ApplyOptions(Context& context, const PromptScaffold& scaffold)
	{
		for (auto& option : scaffold.options)
		{
			if (auto pToggle = std::get_if<UserDefinedToggle>(&option))
			{
				pToggle->defaultValue ? context.SetFlag(pToggle->id) : context.RemoveFlag(pToggle->id); //! @wrong defaultValue
			}
			else if (auto pNumber = std::get_if<UserDefinedNumber>(&option))
			{
				context.SetValue(pNumber->id, pNumber->defaultValue); //! @wrong defaultValue
			}
			else if (auto pText = std::get_if<UserDefinedText>(&option))
			{
				context.SetValue(pText->id, pText->defaultValue); //! @wrong defaultValue
			}
		}
	}

	std::vector<PromptBlock> PromptBuilder::GetStagingBlocks(ChatStaging& staging) noexcept
	{
		auto& scaffold = staging.GetPromptScaffold();
		std::vector<PromptBlock> out_blocks;

		for (size_t i = 0; i < scaffold.blocks.size(); ++i)
		{
			auto& block = scaffold.blocks[i];
			
			if (block.type == PromptBlockInfo::Type::Persona) // Persona, repeat for each bot
			{
				int32_t bot_idx = 0;
				for (Role role = bot_from_index(bot_idx); role != Role::Undefined; role = bot_from_index(++bot_idx))
				{
					if (not staging.HasCharacter(role))
						continue;

					Context context = staging.GetContext(role);
					ApplyOptions(context, scaffold);
					context.SetValue("persona", staging.GetPersonaOf(role));

					if (not block.condition.Evaluate(context))
						continue;

					auto content = eval_text(block.content, context);

					auto hash = GetHash(content);
					out_blocks.emplace_back(PromptBlock {
						.id = i * 1000 + bot_idx,
						.content = std::move(content),
						.hash = std::move(hash),
						.blockType = PromptBlockInfo::Type::Persona,
						.role = role,
						.ttl = block.ttl,
					});
				}
			}
			else if (block.type == PromptBlockInfo::Type::User) // User
			{
				Context context = staging.GetContext(Role::User);
				ApplyOptions(context, scaffold);
				context.SetValue("persona", staging.GetPersonaOf(Role::User));

				if (not block.condition.Evaluate(context))
					continue;

				auto content = eval_text(block.content, context);
				
				auto hash = GetHash(content);
				out_blocks.emplace_back(PromptBlock {
					.id = i * 1000,
					.content = std::move(content),
					.hash = std::move(hash),
					.blockType = PromptBlockInfo::Type::User,
					.ttl = block.ttl,
				});
			}
			else // Static instructions
			{
				Context context = staging.GetContext();
				ApplyOptions(context, scaffold);

				if (not block.condition.Evaluate(context))
					continue;

				auto content = eval_text(block.content, context);

				auto hash = GetHash(content);
				out_blocks.emplace_back(PromptBlock {
					.id = i * 1000,
					.content = std::move(content),
					.hash = std::move(hash),
					.blockType = PromptBlockInfo::Type::Static,
					.ttl = block.ttl,
				});
			}
		}

		return out_blocks;
	}
}