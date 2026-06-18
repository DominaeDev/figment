#include <pch.h>
#include "util/DebugUtils.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/Asset.h"
#include "data/ModelSettings.h"
#include "data/CharacterData.h"
#include "chat/ChatStaging.h"
#include "chat/PromptScaffold.h"
#include "text/TextEvaluator.h"

using namespace fig::io;
using namespace fig::chat;
using namespace fig::data;

namespace fig
{
	void DebugUtility::Initialize()
	{
		if constexpr (Debugging)
		{
//			ImportTestCharacters("./import/characters");
//			ShuffleCards();
//			CreateModelSettings();
		}

		PromptScaffold scaffold;
		scaffold.LoadFromXml("./resources/prompting/chat.scaffold");
		int k = 0;

	}

	void DebugUtility::CreateNewProfile(const fig::string& name, const fig::string& password)
	{
		if constexpr (Debugging)
		{
			auto& userMngr = Global::GetUserManager();

			// Create new profile
			userMngr.CreateProfile(name, password);
		}
	}

	void DebugUtility::ImportTestCharacters(const fig::path& path, size_t max_count)
	{
		if constexpr (Debugging)
		{
			auto& userMngr = Global::GetUserManager();

			if (userMngr.SignInDefaultProfile())
			{
				auto& content = userMngr.GetContent();

				// Delete all characters
				auto remove_characters = content.GetAssetManager().GetCharacterAssets()
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				content.GetAssetManager().DeleteAssets(remove_characters);

				content.ImportCharactersInDirectory(path, max_count);
				userMngr.SignOut();
			}
		}
	}

	void DebugUtility::ImportTestScenario()
	{
		if constexpr (Debugging)
		{
			auto& userMngr = Global::GetUserManager();

			if (userMngr.SignInDefaultProfile())
			{
				auto& content = userMngr.GetContent();

				auto remove_scenarios = content.GetAssetManager().GetScenarioAssets()
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				content.GetAssetManager().DeleteAssets(remove_scenarios);

				auto _ignored = content.ImportScenario(fig::path("./import/scenario.xml"));
				userMngr.SignOut();
			}
		}
	}

	void DebugUtility::ShuffleCards()
	{
		if constexpr (Debugging)
		{
			auto& userMngr = Global::GetUserManager();

			if (userMngr.SignInDefaultProfile())
			{
				auto& content = userMngr.GetContent();

				auto characterAssets = content.GetAssetManager().GetAssets()
					| std::views::filter([](auto& a) { return a.asset_type == AssetType::Character; })
					| std::views::transform([](auto& a) { return std::ref(a); })
					| std::ranges::to<std::vector>();

				auto rng = std::random_device {};
				std::ranges::shuffle(characterAssets, rng);

				int32_t count = 0;
				auto now = utc_now();
				for (auto& assetRef : characterAssets)
				{
					auto& asset = assetRef.get();
					asset.SetMeta(MetaTag::CreatedAt, now);
					asset.SetMeta(MetaTag::LastUsedAt, now);
					asset.SetMeta(MetaTag::UpdatedAt, now);
					now -= static_cast<fig::timestamp>(std::chrono::milliseconds(100).count());
				}

				userMngr.SignOut();
			}
		}
	}

	void DebugUtility::CreateProfilePic(const fig::path& path)
	{
		if constexpr (Debugging)
		{
			auto& userMngr = Global::GetUserManager();

			if (userMngr.SignInDefaultProfile())
			{
				auto& assetMngr = userMngr.GetContent().GetAssetManager();
				assetMngr.CreateProfilePicture(userMngr.GetActiveProfile(), path);
				userMngr.SignOut();
			}
		}
	}

	void DebugUtility::CreateModelSettings()
	{
		if constexpr (Debugging)
		{
			auto& userMngr = Global::GetUserManager();

			fig::data::ModelSettings modelSettings;
			if (not Success(modelSettings.LoadFromXml(fig::path { "./import/model_settings.xml" }))) //! @temp
				modelSettings = fig::data::ModelSettings {};

			if (userMngr.SignInDefaultProfile())
			{
				auto& assetMngr = userMngr.GetContent().GetAssetManager();

				auto remove_settings = assetMngr.GetAssetsOfType(AssetType::ModelSettings)
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				assetMngr.DeleteAssets(remove_settings);

				fig::bytes buf;
				modelSettings.SaveToXml(buf);
				assetMngr.CreateAsset(fig::io::AssetType::ModelSettings, buf);

				userMngr.SignOut();
			}
		}
	}

	void DebugUtility::DebugCharacter(const fig::uuid& characterId)
	{
		if constexpr (Debugging)
		{
			auto& userMngr = Global::GetUserManager();
			if (userMngr.IsSignedIn())
			{
				if (auto try_character = userMngr.GetContent().GetCharacter(characterId))
				{
					auto& character = try_character.value();

					PromptScaffold scaffold;
					if (!Success(scaffold.LoadFromXml(fig::path(Constants::Paths::PromptScaffold))))
						return;

					ChatStaging staging(std::move(scaffold), Constants::LLM::DefaultChatOptions);
					if (!staging.AddCharacter(characterId, Role::Bot1, character))
						return;

					CharacterData user;
					if (not (Success(user.LoadFromXml(fig::path { "./characters/user.xml" })) 
						and staging.AddCharacter({}, Role::User, user))) //! @temp
						return;

					auto blocks = staging.GetPromptBlocks();

					fig::string text = blocks
						| std::views::transform([](auto&& b) { return b.content; })
						| std::views::join
						| std::ranges::to<std::string>();
					LogLn(text);
				}
			}
		}
	}
}