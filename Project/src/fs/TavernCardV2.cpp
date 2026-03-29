#include <pch.h>
#include <json.hpp>
#include "fs/TavernCardV2.h"

namespace fig::io
{
	bool TavernCardV2::Parse(const fig::string& json)
	{
		try
		{
			nlohmann::json j = nlohmann::json::parse(json);

			if (j.value("spec", "") != spec)
				return false;
			if (j.value("spec_version", "") != spec_version)
				return false;

			const auto& d = j.at("data");

			// Required
			d.at("name").get_to(data.name);
			d.at("description").get_to(data.description);
			d.at("personality").get_to(data.personality);
			d.at("scenario").get_to(data.scenario);
			d.at("first_mes").get_to(data.greeting);
			d.at("mes_example").get_to(data.example);

			// Optional
			data.system = d.value("system_prompt", "");
			data.creator_notes = d.value("creator_notes", "");
			data.post_history_instructions = d.value("post_history_instructions", "");
			data.creator = d.value("creator", "");
			data.character_version = d.value("character_version", "");
			data.alternate_greetings = d.value("alternate_greetings", std::vector<std::string>{});
			data.tags = d.value("tags", std::vector<std::string>{});

			return true;
		}
		catch (const nlohmann::json::exception&)
		{
			return false;
		}
	}
}