#pragma once
#ifndef __ALICE_API_INTEGRATION__
#define __ALICE_API_INTEGRATION__

#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <expected>
#include <ranges>
#include <dpp/json.h>
#include <fmt/format.h>

#include "./session.h"

// construct a nlohmann::json, use this to express it would be send to JEV API
#define JEV_request_data(...) nlohmann::json{__VA_ARGS__}
// construct a sub-object for "stats" field, expect appeared in JEV_request_data
#define make_stats(...) {"stats",{__VA_ARGS__}}
// construct a sub-object for "questions" field, expect appeared in JEV_request_data
#define make_questions(...) {"questions",{__VA_ARGS__}}
// construct a choice question appear in make_questions
#define make_choice(name,instruction,...) {name,{{"type","choice"},{"instructions",instruction},{"criteria",{__VA_ARGS__}}}}
// construct a noul question appear in make_questions
#define make_noul(name,instruction) {name,{{"type","noul"},{"instructions",instruction}}}
// construct a score question appear in make_questions
#define make_score(name,instruction,...) {name,{{"type","score"},{"instructions",instruction},{"criteria",{__VA_ARGS__}}}}
// no effect, just for readable code
#define question_list(...) __VA_ARGS__
#define make_kv_pair(k,v) {k,v}

namespace Alice {
	namespace detail {

		struct token_usage_t {
			uint32_t input;
			uint32_t output;
			uint32_t think;
			uint32_t total;
			token_usage_t(uint32_t i, uint32_t o, uint32_t th, uint32_t to) :input{ i }, output{ o }, think{ th }, total{ to } {};
		};
		struct API_result_t {
			std::string COT;
			std::string content;
			token_usage_t token_usage;
			API_result_t(std::string COT, std::string content, uint32_t i, uint32_t o, uint32_t th, uint32_t to) :COT{ COT }, content{ content }, token_usage{ i,o,th,to } {};
			API_result_t() :COT{}, content{}, token_usage{ 0,0,0,0 } {};
		};
		struct noul_t {
			double result;
		};
		struct choice_t {
			std::string result;
			double confidence;
			std::map<std::string, double> probabilities;
			std::optional<double> none_probability;
		};
		struct score_t {
			double accumulate;
			double confidence;
			std::map<std::string, double> probabilities;
			std::map<std::string, std::string> legend;
		};
		struct metadata_t {
			uint32_t input;
			uint32_t output;
			std::string model;
		};
		/**
		 * @brief reture type of JEV API call
		 */
		struct JEV_result_t {
			std::map<std::string, noul_t> noul;
			std::map<std::string, choice_t> choice;
			std::map<std::string, score_t> score;
			metadata_t metadata;
		};
		/**
		 * @brief the basic device to process any API we need, the design is referenced Node.js's OpenAI module
		 */
		class API_integration {
		private:
			session_t session;
			class OpenAI_t {
			private:
				API_integration& integration;
				class chat_t {
				private:
					API_integration& integration;
				public:
					chat_t(API_integration& i) :integration{ i } {};
					std::expected<API_result_t, response_err_t> completions(std::string_view url, std::string_view key, json& data, bool stream = false) {
						auto& handle = integration.session.get_useable_handle();
						handle.set_base_url(fmt::format("{}/chat/completions", url));
						if (!key.empty())handle.set_header(fmt::format("Authorization: Bearer {}", key));
						handle.set_header("Content-Type: application/json");
						if (stream) {
							API_result_t result{};
							auto response = handle.stream_create(data.dump(), [&result](json j) {
								auto& choice_0 = j.at("choice").at(0);
								if (choice_0.contains("delta")) {
									auto& delta = choice_0.at("delta");
									if (delta.contains("reasoning_content")) result.COT += delta.at("reasoning_content").get<std::string>();
									else if (delta.contains("reasoning"))result.COT += delta.at("reasoning").get<std::string>();
									if (delta.contains("content"))result.content += delta.at("content").get<std::string>();
								}
								if (j.contains("usage")) {
									auto& usage = j.at("usage");
									result.token_usage.input = usage.at("prompt_token").get<uint32_t>();
									result.token_usage.output = usage.at("completion_tokens").get<uint32_t>();
									result.token_usage.total = usage.at("total_tokens").get<uint32_t>();
									result.token_usage.think = result.token_usage.total - result.token_usage.input - result.token_usage.output;
								}
																 });
							integration.session.release_handle(handle);
							if (!response.error) {
								return result;
							} else {
								return result;
							}
						} else {
							auto response = handle.create(data.dump());
							integration.session.release_handle(handle);
							if (!response.error) {
								auto& choice_0 = response.body.at("choices").at(0);
								if (response.body.contains("usage")) {
									auto& usage = response.body.at("usage");
									return API_result_t{
										choice_0.at("message").contains("reasoning_content") ? choice_0.at("message").at("reasoning_content").get<std::string>() : (choice_0.at("message").contains("reasoning") ? choice_0.at("message").at("reasoning").get<std::string>() : ""),
										choice_0.at("message").at("content").get<std::string>(),
										usage.at("prompt_token").get<uint32_t>(),
										usage.at("completion_tokens").get<uint32_t>(),
										usage.at("total_tokens").get<uint32_t>() - usage.at("prompt_token").get<uint32_t>() - usage.at("completion_tokens").get<uint32_t>(),
										usage.at("total_tokens").get<uint32_t>()
									};
								} else {
									return API_result_t{
										choice_0.at("message").contains("reasoning_content") ? choice_0.at("message").at("reasoning_content").get<std::string>() : (choice_0.at("message").contains("reasoning") ? choice_0.at("message").at("reasoning").get<std::string>() : ""),
										choice_0.at("message").at("content").get<std::string>(),
										0,
										0,
										0,
										0
									};
								}

							} else {
								response_err_t err{};
								return std::unexpected(response.body.get<response_err_t>());
							}
						}
					}
				};
				std::vector<std::string> models(std::string_view url, std::string_view key) {
					auto& handle = integration.session.get_useable_handle();
					handle.set_base_url(fmt::format("{}/v1/models", url));
					handle.set_header(fmt::format("Authorization: Bearer {}", key));
					auto response = handle.create();
					integration.session.release_handle(handle);
					if (!response.error) {
						std::vector<std::string> result{};
						for (auto& j : response.body.at("data").items()) {
							result.emplace_back(j.value().at("id").get<std::string>());
						}
						return result;
					}
					return {};
				}
			public:
				chat_t chat;
				OpenAI_t(API_integration& i) :integration{ i }, chat{ i } {};
			};

			class JEV_t {
			private:
				API_integration& integration;
			public:
				JEV_t(API_integration& i) :integration{ i } {};
				/**
				 * @brief call the JEV API by ".../systemone" endpost
				 * @param url base url to send request
				 * @param key optional, if length is 0, the key would not be set
				 * @param data the data to send, require "stats" and "questions" field
				 * @return formatted JEV result
				 */
				std::expected<JEV_result_t, response_err_t> systemone(std::string_view url, std::string_view key, json& data) {
					auto& handle = integration.session.get_useable_handle();
					handle.set_base_url(fmt::format("{}/systemone", url));
					if (!key.empty())handle.set_header(fmt::format("Authorization: Bearer {}", key));
					handle.set_header("Content-Type: application/json");
					auto response = handle.create(data.dump());
					integration.session.release_handle(handle);
					if (!response.error) {
						JEV_result_t result{};
						for (auto& j : response.body.at("answers").items()) {
							auto&& type = j.value().at("type").get<std::string>();
							if (type == "noul") {
								result.noul.try_emplace(j.key(), j.value().at("noul").get<double>());
							} else if (type == "choice") {
								result.choice.try_emplace(j.key(),
														  j.value().at("choice").get<std::string>(),
														  j.value().at("confidence").get<double>(),
														  j.value().at("probabilities").get<std::map<std::string, double>>(),
														  j.value().contains("none_probability") ? std::optional{ j.value().at("none_probability").get<double>() } : std::nullopt);
							} else {
								std::map<std::string, double> probabilities{};
								std::map<std::string, std::string> legend{};
								std::string rank{};
								for (auto& l : j.value().at("legend").items()) {
									rank = l.value().get<std::string>();
									probabilities.emplace(l.key(), j.value().at("probabilities").at(rank).get<double>());
									legend.emplace(l.key(), rank);
								}
								result.score.try_emplace(j.key(),
														 j.value().at("score").get<double>(),
														 j.value().at("confidence").get<double>(),
														 std::move(probabilities),
														 std::move(legend));
							}
						}
						return result;
					} else {
						return std::unexpected(response.body.get<response_err_t>());
					}
				}

				/**
				 * @brief call the JEV API by ".../decisions" endpost
				 * @param url base url to send request
				 * @param key optional, if length is 0, the key would not be set
				 * @param data the data to send, require "stats" and "questions" field
				 * @return formatted JEV result
				 */
				std::expected<JEV_result_t, response_err_t> decisions(std::string_view url, std::string_view key, json& data) {
					auto& handle = integration.session.get_useable_handle();
					handle.set_base_url(fmt::format("{}/systemone", url));
					if (!key.empty())handle.set_header(fmt::format("Authorization: Bearer {}", key));
					handle.set_header("Content-Type: application/json");
					auto response = handle.create(data.dump());
					integration.session.release_handle(handle);
					if (!response.error) {
						JEV_result_t result{};
						for (auto& j : response.body.at("answers").items()) {
							auto&& type = j.value().at("type").get<std::string>();
							if (type == "noul") {
								result.noul.try_emplace(j.key(), j.value().at("noul").get<double>());
							} else if (type == "choice") {
								result.choice.try_emplace(j.key(),
														  j.value().at("choice").get<std::string>(),
														  j.value().at("confidence").get<double>(),
														  j.value().at("probabilities").get<std::map<std::string, double>>(),
														  j.value().contains("none_probability") ? std::optional{ j.value().at("none_probability").get<double>() } : std::nullopt);
							} else {
								std::map<std::string, double> probabilities{};
								std::map<std::string, std::string> legend{};
								std::string rank{};
								for (auto& l : j.value().at("legend").items()) {
									rank = l.value().get<std::string>();
									probabilities.emplace(l.key(), j.value().at("probabilities").at(rank).get<double>());
									legend.emplace(l.key(), rank);
								}
								result.score.try_emplace(j.key(),
														 j.value().at("score").get<double>(),
														 j.value().at("confidence").get<double>(),
														 std::move(probabilities),
														 std::move(legend));
							}
						}
						return result;
					} else {
						return std::unexpected(response.body.get<response_err_t>());
					}
				}

				std::vector<std::string> models(std::string_view url, std::string_view key) {
					auto& handle = integration.session.get_useable_handle();
					handle.set_base_url(fmt::format("{}/models", url));
					if (!key.empty())handle.set_header(fmt::format("Authorization: Bearer {}", key));
					auto response = handle.create();
					integration.session.release_handle(handle);
					if (!response.error) {
						std::vector<std::string> result{};
						if (response.body.contains("models")) {
							for (auto& m : response.body.at("models").items()) {
								result.emplace_back(m.value().get<std::string>());
							}
						} else {
							result.emplace_back(response.body.at("model").get<std::string>());
						}
						return result;
					} else {
						return {};
					}
				}
			};
		public:
			OpenAI_t OpenAI;
			JEV_t JEV;
			API_integration(size_t concurrent_count) :session{ concurrent_count }, OpenAI{ *this }, JEV{ *this } {};
		};
	}
}


#endif // !__ALICE_API_INTEGRATION__
