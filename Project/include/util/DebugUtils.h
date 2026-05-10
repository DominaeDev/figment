#pragma once

#include "Types.h"

namespace fig
{
	class DebugUtility
	{
	public:
		static void Initialize();

		static void CreateNewProfile(const fig::string& name = "New profile", const fig::string& password = "");
		static void ImportTestCharacters(const fig::path& path = "./import/characters");
		static void ImportTestScenario();
		static void ShuffleCards();
		static void CreateProfilePic(const fig::path& path = fig::path("./import/profile_pic.png"));
		static void CreateModelSettings();
	};
}