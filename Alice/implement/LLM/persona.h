#pragma once
#ifndef _ALICE_PERSONA_
#define _ALICE_PERSONA_

#include <vector>
#include <string>
#include <dpp/snowflake.h>
#include <dpp/json.h>
#include "./context.h"

namespace Alice {
	namespace detail {

		enum class persona_visible_states : uint32_t {
			System,
			Public,
			Private
		};

		NLOHMANN_JSON_SERIALIZE_ENUM(
			persona_visible_states,
			{
				{persona_visible_states::System,"system"},
				{persona_visible_states::Public,"public"},
				{persona_visible_states::Private,"private"}
			}
		);

		/**
		 * @brief structure of persona's memory
		 */
		struct persona_memory_t {
			/**
			 * @brief number of message to send to AI directly, range will be [0, short_term_max).
			 */
			uint32_t short_term_max;

			/**
			 * @brief start index of message to summarize, not zero base, range will be [summary_start_index, end] and filtered by "summarized" flag.
			 */
			uint32_t summary_start_index;

			/**
			 * @brief a list of messages snowflake, used to check message from discord and index in "message_repository".
			 */
			std::vector<dpp::snowflake> raw_messages_snowflake;

			/**
			 * @brief a list of summarized memory.
			 */
			std::vector<std::string> summarized;
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
			persona_memory_t,
			short_term_max,
			summary_start_index,
			raw_messages_snowflake,
			summarized
		)

		/**
		 * @brief same as <seealso cref="chat_interaction_t"/> but add order for sortable
		 */
		struct instruction_node_t {
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
			/**
			 * @brief sort support
			 */
			uint32_t order;

			inline operator chat_interaction_t()const noexcept {
				return chat_interaction_t{ role,content,name };
			}
		};

		inline void to_json(nlohmann::json& j, const instruction_node_t& in) {
			j["role"] = in.role;
			j["content"] = in.content;
			if (in.name != std::nullopt)j["name"] = *in.name;
			j["order"] = in.order;
		}
		inline void from_json(const nlohmann::json& j, instruction_node_t& in) {
			j.at("role").get_to(in.role);
			j.at("content").get_to(in.content);
			if (j.at("name") && !j.at("name").is_null())in.name = j.at("name").get<std::string>();
			else in.name = std::nullopt;
			j.at("order").get_to(in.order);
		}

		struct persona_t {

		};
	}
}
#endif // !_ALICE_PERSONA_
