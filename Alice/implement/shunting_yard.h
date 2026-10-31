#pragma once
#ifndef _ALICE_SHUNTING_YARD_
#define _ALICE_SHUNTING_YARD_

#ifndef _ALICE_SHUNTING_YARD_RECURSION_MAXIMUM_
#define _ALICE_SHUNTING_YARD_RECURSION_MAXIMUM_ 64
#endif // !_ALICE_SHUNTING_YARD_RECURSION_MAXIMUM_

/**
* unlike JavaScript version(cannot modify the implement of array), C++ version will let member in object which will be write to file to contain the contiguous storage as much as possible
* because it will need to deserialize by nlohmann::json, which will allocate memory for them
*/

#include <bitset>
#include <deque>
#include <stack>
#include <vector>
#include <expected>
#include <algorithm>
#include <utility>
#include <utf8cpp/utf8.h>
#include <dpp/json.h>
#include <dpp/snowflake.h>
#include <scn/scan.h>
#include <fmt/format.h>

#include "./placeholder_replacer.h"
#include "./number.h"

namespace Alice {
	namespace detail {

		using iter_char_t = utf8::utfchar32_t;

		inline bool is_letter(const iter_char_t c)noexcept { return (c >= 'A'_u && c <= 'Z'_u) || (c >= 'a'_u && c <= 'z'_u); }
		inline bool is_number(const iter_char_t c)noexcept { return c >= '0'_u && c <= '9'_u; }
		inline bool is_dot(const iter_char_t c)noexcept { return c == '.'_u; }
		inline bool is_dash(const iter_char_t c)noexcept { return c == '_'_u; }
		inline bool is_arg_separator(const iter_char_t c)noexcept { return c == ','_u; }
		inline bool is_bracket(const iter_char_t c)noexcept { return c == '('_u || c == ')'_u; }
		inline bool is_right_combine(const iter_char_t c)noexcept { return c == '!'_u || c == '^'_u; }
		inline bool may_switch_sign(const iter_char_t c)noexcept { return c == '+'_u || c == '-'_u; }
		inline bool is_vaild_function_name(const iter_char_t c)noexcept { return is_letter(c) || is_number(c) || is_dash(c); }
		inline bool is_whitespace(const iter_char_t c)noexcept { return c == ' '_u; }
		inline bool is_operator(const iter_char_t c)noexcept {
			return c == '+'_u ||
				c == '-'_u ||
				c == '*'_u ||
				c == '/'_u ||
				c == '%'_u ||
				c == '^'_u ||
				c == '!'_u;
		}

		enum class stats :uint32_t {
			Integer,
			Floating,
			Function,
			Identifier,
			Operator,
			Switch_sign,
			Right_bracket,
			Left_bracket,
			Unknow
		};

		NLOHMANN_JSON_SERIALIZE_ENUM(stats,
									 {
										 {stats::Integer,"integer"},
										 {stats::Floating,"float"},
										 {stats::Function,"function"},
										 {stats::Identifier,"identifier"},
										 {stats::Operator,"operator"},
										 {stats::Switch_sign,"switch_sign"}
									 });

		inline int rank_op(const iter_char_t c)noexcept {
			switch (c) {
			case '^'_u:
				return 5;
			case '!'_u:
				return 4;
			case '*'_u:case '/'_u:case '%'_u:
				return 3;
			case '+'_u:case '-'_u:
				return 2;
			default: [[unlikely]]
				std::unreachable();
				return 0;
			}
		}

		inline uint32_t operator_arg(const iter_char_t c)noexcept {
			switch (c) {
			case '^'_u:case '*'_u:case '/'_u:case '%'_u:case '+'_u:case '-'_u:
				return 2;
			case '!'_u:
				return 1;
			case '('_u:case ')'_u:
				return 0;
			default: [[unlikely]]
				std::unreachable();
				return -1;
			}
		}

		inline const char map_op_to_ASCII(const iter_char_t c)noexcept { return static_cast<char>(c); }

		struct shunting_yard_obj_t {
			stats type;
			std::string content;
			uint32_t argument_count;
			shunting_yard_obj_t() :type{ stats::Unknow }, content{}, argument_count{ 0 } {}
			shunting_yard_obj_t(stats stat) :type{ stat }, content{}, argument_count{ 0 } {}
			shunting_yard_obj_t(stats stat, std::string_view str) :type{ stat }, content{ str }, argument_count{ 0 } {}
			shunting_yard_obj_t(stats stat, std::string_view str, uint32_t arg) :type{ stat }, content{ str }, argument_count{ arg } {}
			shunting_yard_obj_t(stats stat, const char c, uint32_t arg) :type{ stat }, content{ c }, argument_count{ arg } {}
			shunting_yard_obj_t(stats stat, const iter_char_t c, uint32_t arg) :type{ stat }, content{ static_cast<char>(c)}, argument_count{arg} {}
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(shunting_yard_obj_t,
										   type,
										   content,
										   argument_count);

		struct shunting_yard_function_head_t {
			std::vector<shunting_yard_obj_t> statements;
			std::vector<std::string> argument_list;
			std::string description;
			dpp::snowflake author;

			
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(shunting_yard_function_head_t,
										   statements,
										   argument_list,
										   description,
										   author);

		struct shunting_yard_function_tag_t {
			std::string name;
			uint32_t arg_count;

			friend bool operator==(const shunting_yard_function_tag_t& rtag,const shunting_yard_function_tag_t& ltag)noexcept {
				return rtag.name == ltag.name && rtag.arg_count == ltag.arg_count;
			}
		};
	}
}
namespace std {
	template<>
	struct hash<Alice::detail::shunting_yard_function_tag_t> {
		size_t operator()(const Alice::detail::shunting_yard_function_tag_t& tag)const {
			size_t result{ 39 };
			result ^= hash<string>()(tag.name) + 0x9e3779b9 + (39 << 6) + (39 >> 2);
			result ^= hash<uint32_t>()(tag.arg_count) + 0x9e3779b9 + (result << 6) + (result >> 2);
			return result;
		}
	};
}
namespace Alice{
	namespace detail{
		enum class expression_error_code :uint32_t {
			invalid_function_name,
			mismatch_bracket,
			unknown_token,
			parse_error,
			excution_error
		};

		class shunting_yard {
		private:
			using function_map_t = std::unordered_map<uint32_t, std::unordered_map<std::string, shunting_yard_function_head_t>>;
			function_map_t defined_functions;

			shunting_yard(function_map_t&& functions) :defined_functions{ std::move(functions) } {};

			inline std::string_view split_arg_list(std::string_view::const_iterator& it, std::string_view::const_iterator end) {
				std::string_view::const_iterator arg_begin{ it }, cache{ it };
				iter_char_t cur{};
				bool param_start{ false };
				while (it != end) {
					cur = utf8::unchecked::next(it);
					if (!param_start) {
						if (!is_whitespace(cur) && !is_arg_separator(cur)) {
							arg_begin = cache;
							param_start = true;
						}
					} else if (is_arg_separator(cur) || is_whitespace(cur)) {
						return { arg_begin,it };
					}
					cache = it;
				}
				return {};
			}

		public:
			shunting_yard() = delete;
			shunting_yard(const shunting_yard&) = delete;
			shunting_yard(shunting_yard&& other)noexcept :defined_functions{ std::move(other.defined_functions) } {};
			shunting_yard& operator=(const shunting_yard&) = delete;
			shunting_yard& operator=(shunting_yard&& other)noexcept {
				defined_functions = std::move(other.defined_functions);
			}

			using register_t = std::deque<number>;

			struct parse_err {
				expression_error_code code;
				const char* message;
			};

			static std::unordered_map<shunting_yard_function_tag_t, number(*)(register_t::iterator)> builtin_functions;

			static shunting_yard setup(nlohmann::json source) {
				function_map_t map{};
				for (auto& i : source.items()) {
					auto [it, success] = map.try_emplace(scn::scan_int<uint32_t>(i.key()).value().value());
					if (success) {
						for (auto& func : i.value().items()) {
							it->second.try_emplace(func.key(), func.value().get<shunting_yard_function_head_t>());
						}
					}
				}
				return map;
			}

			static std::expected<std::vector<shunting_yard_obj_t>,parse_err> parse(std::string_view expr) {
				std::string_view::const_iterator it{ expr.begin() }, end{ expr.end() };

				std::stack<shunting_yard_obj_t> operator_stack{};
				std::deque<shunting_yard_obj_t> output{};
				std::stack<uint32_t> argument_list{};
				std::bitset<_ALICE_SHUNTING_YARD_RECURSION_MAXIMUM_> add_additional_arg{};

				std::string unproduct_content{};
				uint32_t function_call{ 0 };
				bool is_function{ false };
				bool may_be_function{ false };
				bool is_identifier{ false };
				bool is_first{ true };
				bool is_float{ false };
				iter_char_t last_char{ ' '_u }, cur{};

				while (it != end) {
					cur = utf8::unchecked::next(it);
					if (may_be_function) {
						if (is_whitespace(cur)) {
							/*read a not number leading content prior and current is not a function argument token,i.e "example "*/
							may_be_function = false;
							output.emplace_back(stats::Identifier,unproduct_content);
							unproduct_content.clear();
							is_first = true;
							continue;
						} else if (cur == '('_u) {
							/*read a not number leading content prior and current is a function argument token,i.e "example("*/
							is_function = true;
							may_be_function = false;
							function_call += 1;
							//default argument count is 0, it wiil be replace to the real count when ")" is be parsed
							operator_stack.push({ stats::Function,unproduct_content });
							operator_stack.push({ stats::Left_bracket,'(',0 });
							argument_list.push(0);
							unproduct_content.clear();
							last_char = cur;
							is_first = true;
							continue;
						} else if (cur == ')'_u) {
							may_be_function = false;
							output.emplace_back(stats::Identifier,unproduct_content);
							while (!operator_stack.empty() && operator_stack.top().content != "(") {
								output.push_back(operator_stack.top());
								operator_stack.pop();
							}
							if (operator_stack.empty())return std::unexpected<parse_err>(std::in_place,expression_error_code::mismatch_bracket,"mismatch bracket when parse the calculate expression");
							operator_stack.pop();
							unproduct_content.clear();
							last_char = cur;
							is_first = true;
							continue;
						} else if (is_vaild_function_name(cur)) {
							utf8::append(cur, unproduct_content);
							continue;
						} else if (is_operator(cur)) {
							output.emplace_back(stats::Identifier,unproduct_content);
							operator_stack.push({ stats::Operator,map_op_to_ASCII(cur),operator_arg(cur) });
							last_char = cur;
							is_first = true;
							unproduct_content.clear();
							continue;
						} else {
							return std::unexpected<parse_err>(std::in_place, expression_error_code::invalid_function_name, "read a invalid token in function name or identifier");
						}
						// may function
					} else if (is_function) {
						if (is_identifier) {
							// if current content has been marked as identifier, it would not be the first
							if (is_whitespace(cur)) {
								output.emplace_back(stats::Identifier, unproduct_content);
								unproduct_content.clear();
								is_first = true;
								is_identifier = false;
								continue;
							} else if (cur == ','_u) {
								bool add_arg{ false };
								if (unproduct_content.length() != 0) {
									add_arg = true;
									output.emplace_back(stats::Identifier, unproduct_content);
								}
								while (!operator_stack.empty() && operator_stack.top().content != "(") {
									output.push_back(operator_stack.top());
									operator_stack.pop();
									add_arg = true;
								}
								if (operator_stack.empty())return std::unexpected<parse_err>(std::in_place, expression_error_code::mismatch_bracket, "mismatch bracket when parse the calculate expression");

								if (add_arg) {
									argument_list.top() += 1;
								}
								unproduct_content.clear();
								last_char = cur;
								is_identifier = false;
								add_additional_arg.set(function_call, false);
								is_first = true;
								continue;
							} else if (cur == '('_u) {
								operator_stack.push({ stats::Function,unproduct_content });
								operator_stack.push({ stats::Left_bracket,'(',0 });
								argument_list.push(0);
								function_call += 1;
								unproduct_content.clear();
								last_char = cur;
								is_first = true;
								is_identifier = false;
								continue;
							} else if (cur == ')'_u) {
								bool add_arg{ false };
								if (unproduct_content.length() != 0) {
									output.emplace_back(stats::Identifier, unproduct_content);
									add_arg = true;
								}
								while (!operator_stack.empty() && operator_stack.top().content != "(") {
									output.push_back(operator_stack.top());
									operator_stack.pop();
									add_additional_arg.set(function_call, false);
								}
								if (operator_stack.empty())return std::unexpected<parse_err>(std::in_place, expression_error_code::mismatch_bracket, "mismatch bracket when parse the calculate expression");

								operator_stack.pop();

								if (operator_stack.top().type == stats::Function) {
									if (add_arg) argument_list.top() += 1;
									if (add_additional_arg.test(function_call))argument_list.top() += 1;
									operator_stack.top().argument_count = argument_list.top();
									argument_list.pop();
									output.push_back(operator_stack.top());
									operator_stack.pop();
									add_additional_arg.set(function_call, false);
									function_call -= 1;
								}
								if (function_call == 0) {
									is_function = false;
								} else {
									add_additional_arg.set(function_call, true);
								}
								unproduct_content.clear();
								last_char = cur;
								is_first = true;
								is_identifier = false;
								continue;
							} else if (is_operator(cur)) {
								if (may_switch_sign(cur) &&
									(is_operator(last_char) ||
									 last_char == '('_u ||
									 last_char == ','_u)) {
									operator_stack.push({ stats::Switch_sign,cur,1 });
									last_char = cur;
									is_first = true;
									continue;
								}
								if (unproduct_content.length() != 0) {
									output.emplace_back(stats::Identifier, unproduct_content);
									unproduct_content.clear();
								}
								while (!operator_stack.empty() && (
									operator_stack.top().type == stats::Switch_sign ||
									(!is_right_combine(cur) && rank_op(cur) <= rank_op(map_op_to_ASCII(operator_stack.top().content.front()))) ||
									(is_right_combine(cur) && rank_op(cur) < rank_op(map_op_to_ASCII(operator_stack.top().content.front())))
									)) {
									output.push_back(operator_stack.top());
									operator_stack.pop();
								}
								operator_stack.push({ stats::Operator,cur,operator_arg(cur) });
								last_char = cur;
								is_first = true;
								continue;
							} else {
								utf8::append(cur, unproduct_content);
								last_char = cur;
								continue;
							}
							// is identifier
						} else {
							if (is_first) {
								if (is_whitespace(cur))continue;
								if (is_operator(cur)) {
									if (may_switch_sign(cur) &&
										(is_operator(last_char) ||
										 last_char == '('_u ||
										 last_char == ','_u)) {
										operator_stack.push({ stats::Switch_sign,cur,1 });
										last_char = cur;
										continue;
									}
									while (!operator_stack.empty() && (
										operator_stack.top().type == stats::Switch_sign ||
										(!is_right_combine(cur) && rank_op(cur) <= rank_op(map_op_to_ASCII(operator_stack.top().content.front()))) ||
										(is_right_combine(cur) && rank_op(cur) < rank_op(map_op_to_ASCII(operator_stack.top().content.front())))
										)) {
										output.push_back(operator_stack.top());
										operator_stack.pop();
									}
									operator_stack.push({ stats::Operator,cur,operator_arg(cur) });
									last_char = cur;
									continue;
								} else if (cur == '('_u) {
									operator_stack.push({ stats::Left_bracket,cur,0 });
									last_char = 0;
									continue;
								} else if (cur == ')'_u) {
									while (!operator_stack.empty() && operator_stack.top().content.front() != '('_u) {
										std::println("{}", operator_stack.top().content.front());
										output.push_back(operator_stack.top());
										operator_stack.pop();
									}
									if (operator_stack.empty())return std::unexpected<parse_err>(std::in_place, expression_error_code::mismatch_bracket, "mismatch bracket when parse the calculate expression");

									operator_stack.pop();

									if (operator_stack.top().type == stats::Function) {
										if (add_additional_arg.test(function_call))argument_list.top() += 1;
										operator_stack.top().argument_count = argument_list.top();
										argument_list.pop();
										output.push_back(operator_stack.top());
										operator_stack.pop();
										add_additional_arg.set(function_call, false);
										function_call -= 1;
									}
									if (function_call == 0) {
										is_function = false;
									} else {
										add_additional_arg.set(function_call, true);
									}
									last_char = cur;
									continue;
								} else if (cur == ','_u) {
									bool add_arg{ false };
									while (!operator_stack.empty() && operator_stack.top().content.front() != '('_u) {
										output.push_back(operator_stack.top());
										operator_stack.pop();
										add_arg = true;
										add_additional_arg.set(function_call, false);
									}
									if (operator_stack.empty())return std::unexpected<parse_err>(std::in_place, expression_error_code::mismatch_bracket, "mismatch bracket when parse the calculate expression");

									if (add_arg) argument_list.top() += 1;
									last_char = cur;
									continue;
								} else {
									is_first = false;
									utf8::append(cur, unproduct_content);
									if (!is_number(cur))is_identifier = true;
									if (is_dot(cur))return std::unexpected<parse_err>(std::in_place, expression_error_code::unknown_token, "an isolated dot(unknown token) has been analysised");
									last_char = cur;
									continue;
								}
								// is first
							} else {
								if (is_whitespace(cur)) {
									if (is_dot(last_char)) return std::unexpected<parse_err>(std::in_place, expression_error_code::parse_error, "a dot is tailing the number");

									output.emplace_back(is_float ? stats::Floating : stats::Integer, unproduct_content);
									unproduct_content.clear();
									is_float = false;
									is_first = true;
									continue;
								} else if (cur == ','_u) {
									bool add_arg{ false };
									if (unproduct_content.length() != 0) {
										output.emplace_back(is_float ? stats::Floating : stats::Integer, unproduct_content);
										add_arg = true;
									}
									while (!operator_stack.empty() && operator_stack.top().content.front() != '(') {
										output.push_back(operator_stack.top());
										operator_stack.pop();
										add_additional_arg.set(function_call, false);
										add_arg = true;
									}
									if (operator_stack.empty())return std::unexpected<parse_err>(std::in_place, expression_error_code::mismatch_bracket, "mismatch bracket when parse the calculate expression");

									if (add_arg)argument_list.top() += 1;

									unproduct_content.clear();
									last_char = cur;
									is_first = true;
									is_float = false;
									continue;
								} else if (is_operator(cur)) {
									if (may_switch_sign(cur) &&
										(is_operator(last_char) ||
										 last_char == '('_u ||
										 last_char == ','_u)) {
										operator_stack.push({ stats::Switch_sign,cur,1 });
										last_char = cur;
										is_first = true;
										continue;
									}
									if (unproduct_content.length() != 0) {
										output.emplace_back(is_float ? stats::Floating : stats::Integer, unproduct_content);
										unproduct_content.clear();
									}
									while (!operator_stack.empty() && (
										operator_stack.top().type == stats::Switch_sign ||
										(!is_right_combine(cur) && rank_op(cur) <= rank_op(map_op_to_ASCII(operator_stack.top().content.front()))) ||
										(is_right_combine(cur) && rank_op(cur) < rank_op(map_op_to_ASCII(operator_stack.top().content.front())))
										)) {
										output.push_back(operator_stack.top());
										operator_stack.pop();
									}
									operator_stack.push({ stats::Operator,cur,operator_arg(cur) });
									last_char = cur;
									is_float = false;
									is_first = true;
									continue;
								} else if (cur == '('_u) {
									return std::unexpected<parse_err>(std::in_place, expression_error_code::invalid_function_name, "invalid function name(number-like function name) is parsed");
								} else if (cur == ')'_u) {
									bool add_arg{ false };
									if (unproduct_content.length() != 0) {
										output.emplace_back(is_float ? stats::Floating : stats::Integer, unproduct_content);
										add_arg = true;
									}
									while (!operator_stack.empty() && operator_stack.top().content.front() != '(') {
										output.push_back(operator_stack.top());
										operator_stack.pop();
									}
									if (operator_stack.empty())return std::unexpected<parse_err>(std::in_place, expression_error_code::mismatch_bracket, "mismatch bracket when parse the calculate expression");

									operator_stack.pop();

									if (operator_stack.top().type == stats::Function) {
										if (add_arg) argument_list.top() += 1;
										if (add_additional_arg.test(function_call))argument_list.top() += 1;
										operator_stack.top().argument_count = argument_list.top();
										argument_list.pop();
										output.push_back(operator_stack.top());
										operator_stack.pop();
										add_additional_arg.set(function_call, false);
										function_call -= 1;
									}
									if (function_call == 0) {
										is_function = false;
									} else {
										add_additional_arg.set(function_call, true);
									}

									unproduct_content.clear();
									is_float = false;
									is_first = true;
									last_char = cur;
									continue;
								} else {
									// letter is allowed in number as well as it is a valid alpha number
									utf8::append(cur, unproduct_content);
									last_char = cur;
									if (cur == '.'_u)is_float = true;
									continue;
								}
								// not first
							}
							// not identifier
						}
						// is function
					} else if (cur == '('_u) {
						if (unproduct_content.length() != 0) {
							output.emplace_back(is_float ? stats::Floating : stats::Integer, unproduct_content);
							unproduct_content.clear();
						}
						operator_stack.push({ stats::Left_bracket,cur,0 });
						is_first = true;
						is_float = false;
						continue;
					} else if (cur == ')'_u) {
						if (unproduct_content.length() != 0) {
							output.emplace_back(is_float ? stats::Floating : stats::Integer, unproduct_content);
							unproduct_content.clear();
						}
						while (!operator_stack.empty() && operator_stack.top().content.front() != '(') {
							output.push_back(operator_stack.top());
							operator_stack.pop();
						}
						if (operator_stack.empty())return std::unexpected<parse_err>(std::in_place, expression_error_code::mismatch_bracket, "mismatch bracket when parse the calculate expression");
						operator_stack.pop();
						is_first = true;
						is_float = false;
						continue;
					} else if (is_operator(cur)) {
						if (may_switch_sign(cur) &&
							(is_operator(last_char) ||
							 last_char == '('_u ||
							 last_char == ','_u ||
							 last_char == ' '_u)) {
							operator_stack.push({ stats::Switch_sign,cur,1 });
							last_char = cur;
							is_first = true;
							continue;
						}
						if (unproduct_content.length() != 0) {
							output.emplace_back(is_float ? stats::Floating : stats::Integer, unproduct_content);
							unproduct_content.clear();
						}
						while (!operator_stack.empty() && (
							operator_stack.top().type == stats::Switch_sign ||
							(!is_right_combine(cur) && rank_op(cur) <= rank_op(map_op_to_ASCII(operator_stack.top().content.front()))) ||
							(is_right_combine(cur) && rank_op(cur) < rank_op(map_op_to_ASCII(operator_stack.top().content.front())))
							)) {
							output.push_back(operator_stack.top());
							operator_stack.pop();
						}
						operator_stack.push({ stats::Operator,cur,operator_arg(cur) });
						last_char = cur;
						is_first = true;
						is_float = false;
						continue;
					} else {
						if (is_first) {
							if (is_whitespace(cur))continue;
							utf8::append(cur, unproduct_content);
							if (!is_number(cur)) {
								may_be_function = true;
							}
							last_char = cur;
							is_first = false;
							continue;
						} else {
							if (is_whitespace(cur)) {
								output.emplace_back(is_float ? stats::Floating : stats::Integer, unproduct_content);
								unproduct_content.clear();
								last_char = cur;
								is_first = true;
								continue;
							} else {
								utf8::append(cur, unproduct_content);
								last_char = cur;
								if (cur == '.'_u)is_float = true;
								continue;
							}
						}
					}
				}
				if (unproduct_content.length() != 0) {
					if (is_function || may_be_function) output.emplace_back(stats::Identifier, unproduct_content);
					else output.emplace_back( is_float ? stats::Floating : stats::Integer, unproduct_content);
				}
				while (!operator_stack.empty()) {
					if (operator_stack.top().type == stats::Function) {
						if (add_additional_arg.test(function_call))argument_list.top() += 1;
						operator_stack.top().argument_count = argument_list.top();
						argument_list.pop();
						output.push_back(operator_stack.top());
						operator_stack.pop();
						add_additional_arg.set(function_call, false);
						function_call -= 1;
					} else if (operator_stack.top().type == stats::Left_bracket) {
						return std::unexpected<parse_err>(std::in_place, expression_error_code::mismatch_bracket, "mismatch bracket when parse the calculate expression");
					} else {
						output.push_back(operator_stack.top());
						operator_stack.pop();
					}
				}

				return std::vector<shunting_yard_obj_t>{ output.begin(), output.end()};
			}

			static number call_operator(const char op,number lhs, number rhs)noexcept {
				switch (op) {
				case '+':
					return lhs + rhs;
				case '-':
					return lhs - rhs;
				case '*':
					return lhs * rhs;
				case '/':
					return lhs / rhs;
				case '%':
					return lhs % rhs;
				case '^':
					return pow(lhs, rhs);
				case '!':
				{
					int64_t base{ 1 }, top{ lhs.get<int64_t>() };
					for (int i{ 0 }; i <= top;)base *= i++;
					return base;
				}
				default: [[unlikely]]
					return 0;
				}
			}

			std::expected<number, parse_err> execute_function(shunting_yard_function_tag_t& func, register_t::reverse_iterator args) {
				if (!defined_functions.contains(func.arg_count) || !defined_functions[func.arg_count].contains(func.name))return std::unexpected<parse_err>(std::in_place, expression_error_code::excution_error, "undefined function");
				std::deque<number> reg{};
				shunting_yard_function_head_t& statement = defined_functions[func.arg_count][func.name];
				for (auto& op : statement.statements) {
					if (op.type == stats::Integer) {
						auto result = scn::scan_int<int64_t>(op.content, 0);
						if (result && result.value().begin() == result.value().end()) reg.emplace_back(result.value().value());
						else return std::unexpected<parse_err>(std::in_place, expression_error_code::parse_error, "not a valid integer");
					} else if (op.type == stats::Floating) {
						auto result = scn::scan<double>(op.content, "{}");
						if (result && result.value().begin() == result.value().end()) reg.emplace_back(result.value().value());
						else return std::unexpected<parse_err>(std::in_place, expression_error_code::parse_error, "not a valid float");
					} else if (op.type == stats::Operator) {
						if (op.content.front() == '!') {
							reg[reg.size() - 1] = call_operator(op.content.front(), reg[reg.size() - 1], 0);
						} else {
							reg[reg.size() - 2] = call_operator(op.content.front(), reg[reg.size() - 1], reg[reg.size() - 2]);
							reg.pop_back();
						}
					} else if (op.type == stats::Switch_sign) {
						if (op.content.front() == '+')reg[reg.size() - 1] = abs(reg[reg.size() - 1]);
						else reg[reg.size() - 1] = neg(reg[reg.size() - 1]);
					} else if (op.type == stats::Function) {
						shunting_yard_function_tag_t check{ op.content,op.argument_count };
						if (builtin_functions.contains(check)) {
							if (op.argument_count > 1) {
								reg[reg.size() - 2] = builtin_functions[check](reg.end() - 2);
							}
						} else {
							auto result = execute_function(check, reg.rbegin());
							if (result) {
								reg[reg.size() - op.argument_count - 1] = result.value();
								for (uint32_t i{ op.argument_count - 1 }; i > 0; --i)reg.pop_back();
							} else {
								return std::unexpected<parse_err>(result.error());
							}
						}
					} else {
						auto it = std::find(statement.argument_list.begin(), statement.argument_list.end(), op.content);
						if (it == statement.argument_list.end())return std::unexpected<parse_err>(std::in_place, expression_error_code::excution_error, "unknow identifier");
						reg.emplace_back(*(args + (it - statement.argument_list.begin())));
					}
				}
				return reg.back();
			}

			std::expected<number, parse_err> execute(std::vector<shunting_yard_obj_t>& statement) {
				std::deque<number> reg{};
				for (auto& op : statement) {
					if (op.type == stats::Integer) {
						auto result = scn::scan_int<int64_t>(op.content, 0);
						if (result && result.value().begin() == result.value().end()) reg.emplace_back(result.value().value());
						else return std::unexpected<parse_err>(std::in_place, expression_error_code::parse_error, "not a valid integer");
					} else if (op.type == stats::Floating) {
						auto result = scn::scan<double>(op.content, "{}");
						if (result && result.value().begin() == result.value().end()) reg.emplace_back(result.value().value());
						else return std::unexpected<parse_err>(std::in_place, expression_error_code::parse_error, "not a valid float");
					} else if (op.type == stats::Operator) {
						if (op.content.front() == '!') {
							reg[reg.size() - 1] = call_operator(op.content.front(), reg[reg.size() - 1], 0);
						} else {
							reg[reg.size() - 2] = call_operator(op.content.front(), reg[reg.size() - 1], reg[reg.size() - 2]);
							reg.pop_back();
						}
					} else if (op.type == stats::Switch_sign) {
						if (op.content.front() == '+')reg[reg.size() - 1] = abs(reg[reg.size() - 1]);
						else reg[reg.size() - 1] = neg(reg[reg.size() - 1]);
					} else if (op.type == stats::Function) {
						shunting_yard_function_tag_t check{ op.content,op.argument_count };
						if (builtin_functions.contains(check)) {
							if (op.argument_count > 1) {
								reg[reg.size() - 2] = builtin_functions[check](reg.end() - 2);
								reg.pop_back();
							} else {
								reg[reg.size() - 1] = builtin_functions[check](reg.end() - 1);
							}
						} else {
							auto result = execute_function(check, reg.rbegin());
							if (result) {
								reg[reg.size() - op.argument_count - 1] = result.value();
								for (uint32_t i{ op.argument_count - 1 }; i > 0; --i)reg.pop_back();
							} else {
								return std::unexpected<parse_err>(result.error());
							}
						}
					} else {
						return std::unexpected<parse_err>(std::in_place, expression_error_code::excution_error, "identifier cannot appear in top expression");
					}
				}
				return reg.back();
			}

			inline std::expected<number, parse_err> eval(std::string_view expr) {
				auto _expr = parse(expr);
				return _expr ? execute(_expr.value()) : std::unexpected(_expr.error());
			}

			bool assign_func(std::string name, std::string_view arg_list, std::string_view expr,dpp::snowflake author,std::string description) {
				auto parsed = parse(expr);
				uint32_t arg_count{ 0 };
				if (!parsed)return false;
				shunting_yard_function_head_t head{ parsed.value(),{},description,author };
				std::string_view arg{};
				for (std::string_view::const_iterator i{ arg_list.begin() }, end{ arg_list.end() }; i != end;) {
					arg = split_arg_list(i, end);
					if (arg.empty())return false;
					head.argument_list.emplace_back(arg);
					++arg_count;
				}
				if (!defined_functions.contains(arg_count)) {
					defined_functions[arg_count] = {};
				}
				if (defined_functions[arg_count].contains(name))return false;
				defined_functions[arg_count][name] = head;
				return true;
			}
			bool assign_func(std::string& name, std::string_view arg_list, std::string_view expr, dpp::snowflake author, std::string description) {
				auto parsed = parse(expr);
				uint32_t arg_count{ 0 };
				if (!parsed)return false;
				shunting_yard_function_head_t head{ parsed.value(),{},description,author };
				std::string_view arg{};
				for (std::string_view::const_iterator i{ arg_list.begin() }, end{ arg_list.end() }; i != end;) {
					arg = split_arg_list(i, end);
					if (arg.empty())return false;
					head.argument_list.emplace_back(arg);
					++arg_count;
				}
				if (!defined_functions.contains(arg_count)) {
					defined_functions[arg_count] = {};
				}
				if (defined_functions[arg_count].contains(name))return false;
				defined_functions[arg_count][name] = head;
				return true;
			}

			inline void to_json(nlohmann::json& j) {
				for (auto& i : defined_functions) {
					std::string arg_count = fmt::format("{}", i.first);
					for (auto& func : i.second) {
						j[arg_count][func.first] = func.second;
					}
				}
			}
		};

		std::unordered_map<shunting_yard_function_tag_t, number(*)(shunting_yard::register_t::iterator)> shunting_yard::builtin_functions{
			{{"abs",1},[](shunting_yard::register_t::iterator begin) {return abs(*begin); }},
			{{"cbrt",1},[](shunting_yard::register_t::iterator begin) {return cbrt(*begin); }},
			{{"ceil",1},[](shunting_yard::register_t::iterator begin) {return ceil(*begin); }},
			{{"cos",1},[](shunting_yard::register_t::iterator begin) {return cos(*begin); }},
			{{"cosh",1},[](shunting_yard::register_t::iterator begin) {return cosh(*begin); }},
			{{"exp",1},[](shunting_yard::register_t::iterator begin) {return exp(*begin); }},
			{{"floor",1},[](shunting_yard::register_t::iterator begin) {return floor(*begin); }},
			{{"log",1},[](shunting_yard::register_t::iterator begin) {return log(*begin); }},
			{{"log2",1},[](shunting_yard::register_t::iterator begin) {return log2(*begin); }},
			{{"log10",1},[](shunting_yard::register_t::iterator begin) {return log10(*begin); }},
			{{"log",2},[](shunting_yard::register_t::iterator begin) {return log(*(begin + 1),*begin); }},
			{{"neg",1},[](shunting_yard::register_t::iterator begin) {return neg(*begin); }},
			{{"pow",2},[](shunting_yard::register_t::iterator begin) {return pow(*(begin + 1),*begin); }},
			{{"round",1},[](shunting_yard::register_t::iterator begin) {return round(*begin); }},
			{{"sin",1},[](shunting_yard::register_t::iterator begin) {return sin(*begin); }},
			{{"sinh",1},[](shunting_yard::register_t::iterator begin) {return sinh(*begin); }},
			{{"sqrt",1},[](shunting_yard::register_t::iterator begin) {return sqrt(*begin); }},
			{{"tan",1},[](shunting_yard::register_t::iterator begin) {return tan(*begin); }},
			{{"tanh",1},[](shunting_yard::register_t::iterator begin) {return tanh(*begin); }},
			{{"trunc",1},[](shunting_yard::register_t::iterator begin) {return trunc(*begin); }}
		};

	}
}

#endif // !_ALICE_SHUNTING_YARD_
