#pragma once
#ifndef _ALICE_CONTEXT_
#define _ALICE_CONTEXT_

#include <string>
#include <optional>
#include <span>
#include <spanstream>
#include <chrono>
#include <format>
#include <dpp/json.h>
#include <dpp/snowflake.h>

namespace nlohmann {
	template<>
	struct adl_serializer<std::chrono::system_clock::time_point> {
		static void to_json(nlohmann::json& j, const std::chrono::system_clock::time_point& t) {
			j = std::format("{:%FT%TZ}", t);
		}
		static void from_json(const nlohmann::json& j, std::chrono::system_clock::time_point& t) {
			std::string cache = j.get<std::string>();
			std::span span{ cache };
			std::ispanstream ss{ span };
			ss >> std::chrono::parse("%FT%TZ", t);
		}
	};
}

namespace Alice {
	namespace detail {

		using date_t = std::chrono::system_clock::time_point;

		enum class role_t :uint32_t {
			User,
			Assistant,
			History,
			Library,
			User_input,
		};

		NLOHMANN_JSON_SERIALIZE_ENUM(
			role_t,
			{
				{role_t::User,"user"},
				{role_t::Assistant,"assistant"},
				{role_t::History,"history"},
				{role_t::Library,"library"},
				{role_t::User_input,"user_input"},
			}
			);

		/**
		 * @brief a structure same as OpenAI's "messages" field, used in persona's "instructions", "summary_instructions" field and building request data.
		 */
		struct chat_interaction_t {
			/**
			 * @brief default role of OpenAI provided(user or assistant, without system) or placeholder we provided.
			 */
			role_t role;
			/**
			 * @brief raw context
			 */
			std::string content;
			/**
			 * @brief name field of OpenAI's message structure, use string for persona to use
			 */
			std::optional<std::string> name;
		};

		inline void to_json(nlohmann::json& j, const chat_interaction_t& ci) {
			j["role"] = ci.role;
			j["content"] = ci.content;
			if (ci.name != std::nullopt)j["name"] = *ci.name;
		}
		inline void from_json(const nlohmann::json& j, chat_interaction_t& ci) {
			j.at("role").get_to(ci.role);
			j.at("content").get_to(ci.content);
			if (j.at("name") && !j.at("name").is_null())ci.name = j.at("name").get<std::string>();
			else ci.name = std::nullopt;
		}

		/**
		 * @brief a object to contain the chat context
		 */
		struct context_t {
			/**
			 * @brief present the time this context created at
			 */
			date_t send_at;
			/**
			 * @brief the id of persona this context belong to
			 */
			uint64_t persona_id;
			/**
			 * @brief the user's snowflake who create this context
			 */
			dpp::snowflake user;
			/**
			 * @brief raw response from AI
			 */
			std::string assistant_message;
			/**
			 * @brief formatted user's input(to speed up request data process)
			 */
			std::string user_message;
			/**
			 * @brief weather this context should be send when summary(check by JEV)
			 */
			bool to_summary;
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
			context_t,
			send_at,
			persona_id,
			user,
			assistant_message,
			user_message,
			to_summary
		)
	}
}

#endif // !_ALICE_CONTEXT_
