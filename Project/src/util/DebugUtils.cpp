#include <pch.h>
#include "util/DebugUtils.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/Asset.h"
#include "data/ModelSettings.h"
#include "data/Character.h"
#include "data/Scenario.h"
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
//			EraseChats();
//			CreateModelSettings();
//			ShuffleCards();
//			GenerateUUIDs(10uz);
		}
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
				auto remove_characters = content.GetCharacters()
					| std::views::transform([](auto& a) -> fig::uuid { return a.get().id; })
					| std::ranges::to<std::vector>();
				content.GetAssets().DeleteAssets(remove_characters);

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

				auto remove_scenarios = content.GetScenarios()
					| std::views::transform([](auto& a) -> fig::uuid { return a.get().id; })
					| std::ranges::to<std::vector>();
				content.GetAssets().DeleteAssets(remove_scenarios);

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

				auto characterAssets = content.GetCharacters();

				auto rng = std::random_device {};
				std::ranges::shuffle(characterAssets, rng);

				int32_t count = 0;
				auto now = fig::now();
				for (auto& assetRef : characterAssets)
				{
					content.GetAssets().ModifyAsset(assetRef.get(), [&now](auto& asset) {
						asset.SetMeta(MetaTag::CreatedAt, now);
						asset.SetMeta(MetaTag::UpdatedAt, now);
						now -= std::chrono::milliseconds(100);
					});
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
				if (auto profile = userMngr.GetActiveProfile())
				{
					auto& assetMngr = userMngr.GetContent().GetAssets();
					assetMngr.CreateProfilePicture(*profile, path);
					userMngr.SignOut();
				}
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
				auto& assetMngr = userMngr.GetContent().GetAssets();

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
			LogLn((fig::string)characterId);

			auto& userMngr = Global::GetUserManager();
			if (userMngr.IsSignedIn())
			{
				if (auto try_character = userMngr.GetContent().Get<Character>(characterId))
				{
					auto& character = try_character.value();

					PromptScaffold scaffold;
					if (!Success(scaffold.LoadFromXml(fig::path(Constants::Paths::PromptScaffold))))
						return;

					Scenario scenario;
					if (!Success(scenario.LoadFromXml(fig::path(Constants::Paths::DefaultScenario))))
						return;

					ChatStaging staging(std::move(scenario), std::move(scaffold), Constants::LLM::DefaultChatOptions);
					if (!staging.AddCharacter(characterId, Role::Bot1, character))
						return;

					Character user;
					if (not (Success(user.LoadFromXml(fig::path { "./characters/user.xml" })) 
						and staging.AddCharacter({}, Role::User, user))) //! @temp
						return;

					auto blocks = staging.GetStagingBlocks();

					fig::string text = blocks
						| std::views::transform([](auto&& b) { return b.content; })
						| std::views::join
						| std::ranges::to<std::string>();
					LogLn(text);
				}
			}
		}
	}

	void DebugUtility::EraseChats()
	{
		if constexpr (Debugging)
		{
			auto& userMngr = Global::GetUserManager();

			if (userMngr.SignInDefaultProfile())
			{
				auto& assetMngr = userMngr.GetContent().GetAssets();

				auto remove_chats = assetMngr.GetAssetsOfType(AssetType::Chat, ChatAssetType::Instance)
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				assetMngr.DeleteAssets(remove_chats);

				userMngr.SignOut();
			}
		}
	}

	void DebugUtility::GenerateUUIDs(size_t count)
	{
		for (size_t i = 0uz; i < count; ++i)
			LogLn((fig::string)_CreateUUID());
	}

	void DebugUtility::ChangePassword(fig::string_view oldPassword, fig::string_view newPassword)
	{
		if constexpr (Debugging)
		{
			auto& userMngr = Global::GetUserManager();

			if (userMngr.SignInDefaultProfile())
			{
				userMngr.ChangePassword(userMngr.GetActiveProfile().value().id, fig::string { oldPassword }, fig::string { newPassword });
				userMngr.SignOut();
			}
		}
	}
}