#include <pch.h>
#include "chat/PromptBuilder.h"
#include "chat/ChatStaging.h"
#include "text/TextEvaluator.h"

namespace fig::chat
{
	static void ApplyOptions(Context& context, const PromptScaffold& scaffold)
	{
		for (auto& option : scaffold.options)
		{
			if (auto pToggle = std::get_if<PromptOptionToggle>(&option))
			{
				pToggle->defaultValue ? context.SetFlag(pToggle->id) : context.RemoveFlag(pToggle->id);
			}
			else if (auto pNumber = std::get_if<PromptOptionNumber>(&option))
			{
				context.SetValue(pNumber->id, pNumber->defaultValue);
			}
			else if (auto pText = std::get_if<PromptOptionText>(&option))
			{
				context.SetValue(pText->id, pText->defaultValue);
			}
		}
	}

	std::vector<PromptBlock> PromptBuilder::GetBlocks(const PromptScaffold& scaffold, ChatStaging& staging) noexcept
	{
		std::vector<PromptBlock> out_blocks;
		for (size_t i = 0; i < scaffold.blocks.size(); ++i)
		{
			auto& block = scaffold.blocks[i];
			
			if (block.type == PromptBlockInfo::Type::Persona) // Repeat for each bot
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
				});
			}
			else
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
				});
			}
		}
		return out_blocks;
	}
}