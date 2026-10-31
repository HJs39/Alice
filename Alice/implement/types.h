#pragma once
#ifndef _ALICE_TYPES_
#define _ALICE_TYPES_

#include <string>
#include <vector>
#include <dpp/json.h>

namespace Alice {

	/*common use*/
	struct bot_assets_t {
		using string = std::string;
		using str_arr = std::vector<string>;
		string token;
		string client_id;
		string owner;
		string test_server_guild_id;
		string opening_channel;
		string debug_channel;
		string COT_channel;
		str_arr quote_command_available;
		str_arr battle_command_available;
		str_arr chatable_channel;
		str_arr banned_global;
		str_arr banned_chat;
		bool chat_closed;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
		bot_assets_t,
		token, client_id,
		owner,
		test_server_guild_id,
		opening_channel,
		debug_channel,
		COT_channel,
		quote_command_available,
		battle_command_available,
		chatable_channel,
		banned_global,
		banned_chat,
		chat_closed
	);

	struct cat_node_t {
		using string = std::string;
		string image;
		string file_name;
		string provider;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
		cat_node_t,
		image,
		file_name,
		provider
	);

	struct quote_node_t {
		using string = std::string;
		string quote;
		string file_name;
		string author;
		string text;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
		quote_node_t,
		quote,
		file_name,
		author,
		text
	);
}

#endif