#include <pch.h>
#include "util/DebugUtils.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "model/Asset.h"

using namespace fig::io;

namespace fig
{
	void DebugUtility::Initialize()
	{
		if constexpr (Debugging)
		{
			// ShuffleCards();

			// ...
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

	void DebugUtility::ImportTestCharacters(const fig::path& path)
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

				content.ImportCharactersInDirectory(path);
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
				auto timestamp = fig::util::utc_now();
				for (auto& assetRef : characterAssets)
				{
					auto& asset = assetRef.get();
					content.MarkNew(asset.id, count++ < 10);
					asset.SetMeta(MetaTag::CreatedAt, timestamp);
					asset.SetMeta(MetaTag::LastUsedAt, timestamp);
					asset.SetMeta(MetaTag::UpdatedAt, timestamp);
					timestamp -= static_cast<fig::timestamp>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(100)).count());
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
}