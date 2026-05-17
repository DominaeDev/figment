#ifndef TAVERN_V2_CARD_H__
#define TAVERN_V2_CARD_H__
#pragma once

#include "Figment.h"

namespace fig::io
{
	struct TavernCardV2
	{
		struct Data
		{
			std::string name {};
			std::string description {};
			std::string personality {};
			std::string scenario {};
			std::string greeting {};
			std::string example {};
			std::string system {};
			std::string creator_notes {};
			std::string post_history_instructions {};
			
			std::vector<std::string> alternate_greetings;
			std::vector<std::string> tags;

			std::string creator {};
			std::string character_version {};
		} data;

		std::string spec { "chara_card_v2" };
		std::string spec_version { "2.0" };

		bool Parse(const fig::string& json);
	};
}

#endif