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
#include <glaze/json.hpp>

#include "./session.h"

// construct a glz::obj, use this to express it would be send to JEV API
#define JEV_request_data(...) glz::obj{__VA_ARGS__}
// construct a sub-object for "stats" field, expect appeared in JEV_request_data
#define make_state(...) "state",glz::obj{__VA_ARGS__}
// construct a sub-object for "questions" field, expect appeared in JEV_request_data
#define make_questions(...) "questions",glz::obj{__VA_ARGS__}
// construct a choice question appear in make_questions
#define make_choice(name,instruction,...) name,glz::obj{"type","choice","instructions",instruction,"criteria",glz::obj{__VA_ARGS__}}
// construct a noul question appear in make_questions
#define make_noul(name,instruction) name,glz::obj{"type","noul","instructions",instruction}
// construct a score question appear in make_questions
#define make_score(name,instruction,...) name,glz::obj{"type","score","instructions",instruction,"criteria",glz::arr{__VA_ARGS__}}
// no effect, just for readable code
#define question_list(...) __VA_ARGS__
#define make_kv_pair(k,v) k,v

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
					template<typename...Ts>
					std::expected<API_result_t, response_err_t> completions(std::string_view url, std::string_view key, glz::obj<Ts...>& data, bool stream = false) {
						auto& handle = integration.session.get_useable_handle();
						handle.set_base_url(fmt::format("{}/chat/completions", url));
						if (!key.empty())handle.set_header(fmt::format("Authorization: Bearer {}", key));
						handle.set_header("Content-Type: application/json");
						if (stream) {
							API_result_t result{};
							auto response = handle.stream_create(glz::write_json(data), [&result](glz::generic j) {
								auto& choice_0 = j["choice"][0];
								if (choice_0.contains("delta")) {
									auto& delta = choice_0["delta"];
									if (delta.contains("reasoning_content")) result.COT += delta["reasoning_content"].get<std::string>();
									else if (delta.contains("reasoning"))result.COT += delta["reasoning"].get<std::string>();
									if (delta.contains("content"))result.content += delta["content"].get<std::string>();
								}
								if (j.contains("usage")) {
									auto& usage = j["usage"];
									result.token_usage.input = usage["prompt_token"].get<uint32_t>();
									result.token_usage.output = usage["completion_tokens"].get<uint32_t>();
									result.token_usage.total = usage["total_tokens"].get<uint32_t>();
									result.token_usage.think = result.token_usage.total - result.token_usage.input - result.token_usage.output;
								}
																 });
							integration.session.release_handle(handle);
							if (!response.error) {
								return result;
							} else {
								return std::unexpected(response.body.get<response_err_t>());
							}
						} else {
							auto response = handle.create(glz::write_json(data).value());
							integration.session.release_handle(handle);
							if (!response.error) {
								auto& choice_0 = response.body["choices"][0];
								if (response.body.contains("usage")) {
									auto& usage = response.body["usage"];
									return API_result_t{
										choice_0["message"].contains("reasoning_content") ? choice_0["message"]["reasoning_content"].get<std::string>() : (choice_0["message"].contains("reasoning") ? choice_0["message"]["reasoning"].get<std::string>() : ""),
										choice_0["message"]["content"].get<std::string>(),
										usage["prompt_token"].get<uint32_t>(),
										usage["completion_tokens"].get<uint32_t>(),
										usage["total_tokens"].get<uint32_t>() - usage["prompt_token"].get<uint32_t>() - usage["completion_tokens"].get<uint32_t>(),
										usage["total_tokens"].get<uint32_t>()
									};
								} else {
									return API_result_t{
										choice_0["message"].contains("reasoning_content") ? choice_0["message"]["reasoning_content"].get<std::string>() : (choice_0["message"].contains("reasoning") ? choice_0["message"]["reasoning"].get<std::string>() : ""),
										choice_0["message"]["content"].get<std::string>(),
										0,
										0,
										0,
										0
									};
								}

							} else {
								response_err_t temp{};
								glz::read_json(temp, response.body);
								return std::unexpected(std::move(temp));
							}
						}
					}
				};
				std::vector<std::string> models(std::string_view url, std::string_view key) {
					auto& handle = integration.session.get_useable_handle();
					handle.set_base_url(fmt::format("{}/models", url));
					if (!key.empty())handle.set_header(fmt::format("Authorization: Bearer {}", key));
					auto response = handle.get();
					integration.session.release_handle(handle);
					if (!response.error) {
						std::vector<std::string> result{};
						for (auto& j : response.body["data"].get_array()) {
							result.emplace_back(j["id"].get<std::string>());
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
				 * @tparam ...Ts type for glz::obj
				 * @param url base url to send request
				 * @param key optional, if length is 0, the key would not be set
				 * @param data the data to send, require "stats" and "questions" field
				 * @return formatted JEV result
				 */
				template<typename...Ts>
				std::expected<JEV_result_t, response_err_t> systemone(std::string_view url, std::string_view key, glz::obj<Ts...>& data) {
					auto& handle = integration.session.get_useable_handle();
					handle.set_base_url(fmt::format("{}/systemone", url));
					if (!key.empty())handle.set_header(fmt::format("Authorization: Bearer {}", key));
					handle.set_header("Content-Type: application/json");
					auto response = handle.create(glz::write_json(data).value());
					integration.session.release_handle(handle);
					if (!response.error) {
						JEV_result_t result{};
						for (auto& [k, v] : response.body["answers"].get_object()) {
							const std::string type{ v["type"].get<std::string>() };
							if (type == "noul") {
								result.noul.try_emplace(
									k,
									v["noul"].get<double>()
								);
							} else if (type == "choice") {
								std::map<std::string, double> temp{};
								glz::read_json(temp, v["probabilities"]);
								result.choice.try_emplace(
									k,
									v["choice"].get<std::string>(),
									v["confidence"].get<double>(),
									std::move(temp),
									v.contains("none_probability") ? std::optional{ v["none_probability"].get<double>() } : std::nullopt
								);
							} else {
								std::map<std::string, double> probabilities{};
								std::map<std::string, std::string> legend{};
								std::string rank{};
								for (const auto& [leg_k, leg_v] : v["legend"].get_object()) {
									probabilities.emplace(leg_v.get<std::string>(), v["probabilities"][leg_k].get<double>());
									legend.emplace(leg_v.get<std::string>(), leg_k);
								}
								result.score.try_emplace(
									k,
									v["score"].get<double>(),
									v["confidence"].get<double>(),
									std::move(probabilities),
									std::move(legend)
								);
							}
						}
						return result;
					} else {
						response_err_t temp{};
						glz::read_json(temp, response.body);
						return std::unexpected(std::move(temp));
					}
				}

				/**
				 * @brief call the JEV API by ".../decisions" endpost
				 * @tparam ...Ts type for glz::obj
				 * @param url base url to send request
				 * @param key optional, if length is 0, the key would not be set
				 * @param data the data to send, require "stats" and "questions" field
				 * @return formatted JEV result
				 */
				template<typename...Ts>
				std::expected<JEV_result_t, response_err_t> decisions(std::string_view url, std::string_view key, glz::obj<Ts...>& data) {
					auto& handle = integration.session.get_useable_handle();
					handle.set_base_url(fmt::format("{}/systemone", url));
					if (!key.empty())handle.set_header(fmt::format("Authorization: Bearer {}", key));
					handle.set_header("Content-Type: application/json");
					auto response = handle.create(glz::write_json(data).value());
					integration.session.release_handle(handle);
					if (!response.error) {
						JEV_result_t result{};
						for (auto& [k, v] : response.body["answers"].get_object()) {
							const std::string type{ v["type"].get<std::string>() };
							if (type == "noul") {
								result.noul.try_emplace(
									k,
									v["noul"].get<double>()
								);
							} else if (type == "choice") {
								std::map<std::string, double> temp{};
								glz::read_json(temp, v["probabilities"]);
								result.choice.try_emplace(
									k,
									v["choice"].get<std::string>(),
									v["confidence"].get<double>(),
									std::move(temp),
									v.contains("none_probability") ? v["none_probability"].get<double>() : std::nullopt
								);
							} else {
								std::map<std::string, double> probabilities{};
								std::map<std::string, std::string> legend{};
								std::string rank{};
								for (const auto& [leg_k, leg_v] : v["legend"].get_object()) {
									probabilities.emplace(leg_v.get<std::string>(), v["probabilities"][leg_k].get<double>());
									legend.emplace(leg_v.get<std::string>(), leg_k);
								}
								result.score.try_emplace(
									k,
									v["score"].get<double>(),
									v["confidence"].get<double>(),
									std::move(probabilities),
									std::move(legend)
								);
							}
						}
						return result;
					} else {
						response_err_t temp{};
						glz::read_json(temp, response.body);
						return std::unexpected(std::move(temp));
					}
				}

				std::vector<std::string> models(std::string_view url, std::string_view key) {
					auto& handle = integration.session.get_useable_handle();
					handle.set_base_url(fmt::format("{}/models", url));
					if (!key.empty())handle.set_header(fmt::format("Authorization: Bearer {}", key));
					auto response = handle.get();
					integration.session.release_handle(handle);
					if (!response.error) {
						std::vector<std::string> result{};
						if (response.body.contains("models")) {
							for (const auto& m : response.body["models"].get_array()) {
								result.emplace_back(m.get<std::string>());
							}
						} else {
							result.emplace_back(response.body["model"].get<std::string>());
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
