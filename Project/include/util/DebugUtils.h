#pragma once

#include "Figment.h"

namespace fig
{
	class DebugUtility
	{
	public:
		static void Initialize();

		static void CreateNewProfile(const fig::string& name = "New profile", const fig::string& password = "");
		static void ImportTestCharacters(const fig::path& path, size_t max_count = 0uz);
		static void ImportTestScenario();
		static void ShuffleCards();
		static void CreateProfilePic(const fig::path& path = fig::path("./import/profile_pic.png"));
		static void CreateModelSettings();
		
		static void DebugCharacter(const fig::uuid& characterId);
	};
}