#ifndef TAVERN_V2_CARD_H__
#define TAVERN_V2_CARD_H__
#pragma once

#include "Figment.h"

namespace fig::data
{
	struct TavernCardV2
	{
		struct Data
		{
			fig::string name {};
			fig::string persona {};
			fig::string personality {};
			fig::string scenario {};
			fig::string greeting {};
			fig::string example {};
			fig::string system {};
			fig::string creator_notes {};
			fig::string post_history_instructions {};
			
			std::vector<std::string> alternate_greetings;
			std::vector<std::string> tags;

			fig::string creator {};
			fig::string character_version {};
		} data;

		fig::string spec { "chara_card_v2" };
		fig::string spec_version { "2.0" };

		bool Parse(const fig::string& json);
	};
}

#endif