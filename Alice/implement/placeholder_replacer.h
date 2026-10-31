#pragma once
#ifndef _ALICE_PLACEHOLDER_REPLACER_
#define _ALICE_PLACEHOLDER_REPLACER_

#include <utility>
#include <string_view>
#include <string>
#include <functional>
#include <type_traits>
#include <coroutine>
#include <exception>
#include <optional>
#include <unordered_map>
#include <variant>
#include <initializer_list>

#include <utf8cpp/utf8.h>

namespace Alice {
	namespace detail {
#define uncheck_peek(it) utf8::unchecked::peek_next(it)
		struct macro_parse_result_t {
			std::string_view current;
			std::string unmatched_content;
			std::string_view macro_name;
			std::string_view macro_param;
		};
		struct parsed_macro_t {
			std::string_view current;
			std::string_view macro_name;
			std::string_view macro_param;
		};

		class replacer_t {
		private:
			using replace_func = std::function<std::string(std::string_view, std::string_view)>;
			std::variant<replace_func, std::string> handle_replace;
		public:
			replacer_t(replace_func func) :handle_replace{ func } {};
			replacer_t(std::string_view str) :handle_replace{ std::string{str} } {};

			inline std::string operator()(std::string_view macro_name, std::string_view macro_param) {
				return handle_replace.index() ? std::get<std::string>(handle_replace) : std::get<replace_func>(handle_replace)(macro_name, macro_param);
			}
		};

		template<typename T>
		concept is_macro_parse_result = std::is_same_v<T, macro_parse_result_t> || std::is_same_v<T, parsed_macro_t>;

		template<is_macro_parse_result T>
		class co_macro_parse_result_t {
		public:
			struct promise_type;
			using handle_type = std::coroutine_handle<promise_type>;
		private:
			bool complete;
			handle_type handle;
		public:
			co_macro_parse_result_t(const handle_type h) :handle{ h }, complete{ false } {}
			struct promise_type {
				std::optional<T> cache;
				std::exception_ptr e;
				co_macro_parse_result_t get_return_object() {
					return co_macro_parse_result_t{ handle_type::from_promise(*this) };
				}
				std::suspend_never initial_suspend()noexcept { return {}; }
				std::suspend_always final_suspend()noexcept { return {}; }
				std::suspend_always yield_value(T&& v) {
					cache = std::forward<T>(v);
					return {};
				}
				void await_transform() = delete;
				void unhandled_exception()noexcept { e = std::current_exception(); }
				void return_void()noexcept { cache = std::nullopt; }
			};

			inline void next() {
				if (complete) return;
				handle.resume();
				if (handle.promise().cache == std::nullopt)complete = true;
			}

			operator bool() { return !complete; }
			std::optional<T>& operator()() {
				if (complete) return std::nullopt;
				handle.resume();
				if (handle.promise().cache == std::nullopt)complete = true;
				return handle.promise().cache;
			}

			class iterator {
			private:
				friend class co_macro_parse_result_t;
				co_macro_parse_result_t& result;
				iterator(co_macro_parse_result_t& r) :result{ r } {}
			public:
				iterator() = delete;
				iterator(const iterator&) = default;
				iterator(iterator&&) = default;
				iterator& operator=(const iterator&) = default;
				iterator& operator=(iterator&&) = default;
				inline void operator++() {
					result.next();
				}
				inline T& operator*() {
					return *result.handle.promise().cache;
				}
				inline const T& operator*()const {
					return *result.handle.promise().cache;
				}
				inline bool operator==(std::default_sentinel_t)const {
					return !result;
				}
			};

			inline iterator begin()noexcept {
				return iterator{ *this };
			}
			inline std::default_sentinel_t end()noexcept { return {}; }

			~co_macro_parse_result_t() { handle.destroy(); }
		};
		
		/**
		 * @brief get next position of iterator without move the original iterator by simply copied and advanced it
		 * @tparam Iter any iterator type which allowed copy construct and meeted input iterator at least
		 * @param i an iterator which is pointed to a utf-8 character sequence
		 * @return an iterator pointed to the next position of i
		 */
		template<typename Iter>
		Iter get_next_utf8_iter_pos(Iter i) {
			utf8::unchecked::advance(i, 1);
			return i;
		}
		
		/**
		 * @brief get prior position of iterator without move the original iterator by simply copied and advanced it
		 * @tparam Iter any iterator type which allowed copy construct and meeted input iterator at least
		 * @param i an iterator which is pointed to a utf-8 character sequence
		 * @return an iterator pointed to the before position of i
		 */
		template<typename Iter>
		Iter get_prior_utf8_iter_pos(Iter i) {
			utf8::unchecked::advance(i, -1);
			return i;
		}

		inline consteval utf8::utfchar32_t operator""_u(const char c) { return static_cast<utf8::utfchar32_t>(c); }
		
		/**
		 * @brief consume a macro after current content
		 * 
		 * the macro pattern is ${name:param} and will be escaped if there is a "\" before "$"
		 * @param origin the string to parse
		 * @return - current is empty if there is no macro after current position and unmatched_content will same as original string provided(or same as string macro has been escaped)
		 * 
		 * - current is pointed to the last string after macro and unmatched_content will be the content before macro, macro_param is empty if there is no param following the macro name
		 */
		macro_parse_result_t consume_macro(std::string_view origin) {
			std::string unmatched_content{};
			std::string_view::const_iterator begin{ origin.begin() }, it{ begin }, cache{ it }, macro_name_start{ begin }, macro_name_end{ begin }, param_begin{ begin }, end{ origin.end() };
			bool start_parse{ false };
			utf8::utfchar32_t c{};

			while (it != end) {
				c = utf8::unchecked::next(it);
				if (start_parse) {
					if (c == ':'_u && macro_name_end == begin) {
						macro_name_end = cache;
						param_begin = it;
					} else if (c == '}'_u) {
						if (macro_name_end == begin) {
							//no param
							return {
								{it,end},
								unmatched_content,
								{macro_name_start,cache},
								{}
							};
						} else {
							//have param
							return {
								{it,end},
								unmatched_content,
								{macro_name_start,macro_name_end},
								{param_begin,cache}
							};
						}
						start_parse = false;
						macro_name_start = begin;
						macro_name_end = begin;
						param_begin = begin;
						unmatched_content.clear();
					}
				} else if (c == '$'_u) {
					if (uncheck_peek(it) == '{'_u) {
						start_parse = true;
						utf8::unchecked::advance(it, 1);
						macro_name_start = it;
					} else {
						utf8::append(c, unmatched_content);
					}
				} else if (c == '\\'_u) {
					if (it != end) {
						utf8::append(utf8::unchecked::next(it), unmatched_content);
					} else {
						utf8::append(c, unmatched_content);
					}
				} else {
					utf8::append(c, unmatched_content);
				}
				cache = it;
			}
			return {
				{},
				std::move(unmatched_content),
				{},
				{}
			};
		}
		
		/**
		 * @brief consume a macro after current content
		 * 
		 * the macro pattern is ${name:param} and will be escaped if there is a "\" before "$"
		 * @param origin the string to parse
		 * @param dest the destination to put unmatched content
		 * @return - current is empty if there is no macro after current position
		 * 
		 * - current is pointed to the last string after macro, macro_param is empty if there is no param following the macro name
		 */
		parsed_macro_t consume_macro(std::string_view origin, std::string& dest) {
			std::string_view::const_iterator begin{ origin.begin() }, it{ begin }, cache{ it }, macro_name_start{ begin }, macro_name_end{ begin }, param_begin{ begin }, end{ origin.end() };
			bool start_parse{ false };
			uint32_t c{};

			while (it != end) {
				c = utf8::unchecked::next(it);
				if (start_parse) {
					if (c == ':'_u && macro_name_end == begin) {
						macro_name_end = cache;
						param_begin = it;
					} else if (c == '}'_u) {
						if (macro_name_end == begin) {
							//no param
							return {
								{it,end},
								{macro_name_start,cache},
								{}
							};
						} else {
							//have param
							return {
								{it,end},
								{macro_name_start,macro_name_end},
								{param_begin,cache}
							};
						}
						start_parse = false;
						macro_name_start = begin;
						macro_name_end = begin;
						param_begin = begin;
					}
				} else if (c == '$'_u) {
					if (uncheck_peek(it) == '{'_u) {
						start_parse = true;
						utf8::unchecked::advance(it, 1);
						macro_name_start = it;
					} else {
						utf8::append(c, dest);
					}
				} else if (c == '\\'_u) {
					if (it != end) {
						utf8::append(utf8::unchecked::next(it), dest);
					} else {
						utf8::append(c, dest);
					}
				} else {
					utf8::append(c, dest);
				}
				cache = it;
			}
			return {
				{},
				{},
				{}
			};
		}
		
		/**
		 * @brief parse the whole string and replace the macro by @p replacer
		 * @param origin the string to parse
		 * @param replacer the function or string to replace the matched macro
		 * @return the result
		 */
		std::string parse_macro(std::string_view origin, replacer_t replacer) {
			std::string result{};
			std::string_view::const_iterator begin{ origin.begin() }, it{ begin }, cache{ it }, macro_name_start{ begin }, macro_name_end{ begin }, param_begin{ begin }, end{ origin.end() };
			bool start_parse{ false };
			utf8::utfchar32_t c{};

			while (it != end) {
				c = utf8::unchecked::next(it);
				if (start_parse) {
					if (c == ':'_u && macro_name_end == begin) {
						macro_name_end = cache;
						param_begin = it;
					} else if (c == '}'_u) {
						if (macro_name_end == begin) {
							//no param
							result.append(replacer({ macro_name_start,macro_name_end }, {}));
						} else {
							//have param
							result.append(replacer({ macro_name_start,macro_name_end }, { param_begin,it }));
						}
						start_parse = false;
						macro_name_start = begin;
						macro_name_end = begin;
						param_begin = begin;
					}
				} else if (c == '$'_u) {
					if (uncheck_peek(it) == '{'_u) {
						start_parse = true;
						utf8::unchecked::advance(it, 1);
						macro_name_start = it;
					} else {
						utf8::append(c, result);
					}
				} else if (c == '\\'_u) {
					if (it != end) {
						utf8::append(utf8::unchecked::next(it), result);
					} else {
						utf8::append(c, result);
					}
				} else {
					utf8::append(c, result);
				}
				cache = it;
			}

			return std::move(result);
		}
		
		/**
		 * @brief parse the whole string the put the result to dest
		 * @param origin the string to parse
		 * @param dest the destination to put the result
		 * @param replacer the function or string to replace the matched macro
		 */
		void parse_macro(std::string_view origin,std::string& dest, replacer_t replacer) {
			std::string_view::const_iterator begin{ origin.begin() }, it{ begin }, cache{ it }, macro_name_start{ begin }, macro_name_end{ begin }, param_begin{ begin }, end{ origin.end() };
			bool start_parse{ false };
			utf8::utfchar32_t c{};

			while (it != end) {
				c = utf8::unchecked::next(it);
				if (start_parse) {
					if (c == ':'_u && macro_name_end == begin) {
						macro_name_end = cache;
						param_begin = it;
					} else if (c == '}'_u) {
						if (macro_name_end == begin) {
							//no param
							dest.append(replacer({ macro_name_start,macro_name_end }, {}));
						} else {
							//have param
							dest.append(replacer({ macro_name_start,macro_name_end }, { param_begin,it }));
						}
						start_parse = false;
						macro_name_start = begin;
						macro_name_end = begin;
						param_begin = begin;
					}
				} else if (c == '$'_u) {
					if (uncheck_peek(it) == '{'_u) {
						start_parse = true;
						utf8::unchecked::advance(it, 1);
						macro_name_start = it;
					} else {
						utf8::append(c, dest);
					}
				} else if (c == '\\'_u) {
					if (it != end) {
						utf8::append(utf8::unchecked::next(it), dest);
					} else {
						utf8::append(c, dest);
					}
				} else {
					utf8::append(c, dest);
				}
				cache = it;
			}
		}
		
		/**
		 * @brief parse the macro in target string concurrently
		 * @param origin the string to parse
		 * @return the controller of parse process
		 */
		co_macro_parse_result_t<macro_parse_result_t> co_parse_macro(std::string_view origin) {
			std::string unmatched_content{};
			std::string_view::const_iterator begin{ origin.begin() }, it{ begin }, cache{ it }, macro_name_start{ begin }, macro_name_end{ begin }, param_begin{ begin }, end{ origin.end() };
			bool start_parse{ false };
			utf8::utfchar32_t c{};
			
			while (it != end) {
				c = utf8::unchecked::next(it);
				if (start_parse) {
					if (c == ':'_u && macro_name_end == begin) {
						macro_name_end = cache;
						param_begin = it;
					} else if (c == '}'_u) {
						if (macro_name_end == begin) {
							//no param
							co_yield {
								{it,end},
								unmatched_content,
								{macro_name_start,cache},
								{}
							};
						} else {
							//have param
							co_yield{
								{it,end},
								unmatched_content,
								{macro_name_start,macro_name_end},
								{param_begin,cache}
							};
						}
						start_parse = false;
						macro_name_start = begin;
						macro_name_end = begin;
						param_begin = begin;
						unmatched_content.clear();
					}
				} else if (c == '$'_u) {
					if (uncheck_peek(it) == '{'_u) {
						start_parse = true;
						utf8::unchecked::advance(it, 1);
						macro_name_start = it;
					} else {
						utf8::append(c, unmatched_content);
					}
				} else if (c == '\\'_u) {
					if (it != end) {
						utf8::append(utf8::unchecked::next(it), unmatched_content);
					} else {
						utf8::append(c, unmatched_content);
					}
				} else {
					utf8::append(c, unmatched_content);
				}
				cache = it;
			}
		}
		
		/**
		 * @brief parse the macro in target string concurrently
		 * @param origin the string to parse
		 * @param dest the destination to put unmatched content
		 * @return the controller of parse process
		 */
		co_macro_parse_result_t<parsed_macro_t> co_parse_macro(std::string_view origin, std::string& dest) {
			std::string_view::const_iterator begin{ origin.begin() }, it{ begin }, cache{ it }, macro_name_start{ begin }, macro_name_end{ begin }, param_begin{ begin }, end{ origin.end() };
			bool start_parse{ false };
			uint32_t c{};

			while (it != end) {
				c = utf8::unchecked::next(it);
				if (start_parse) {
					if (c == ':'_u && macro_name_end == begin) {
						macro_name_end = cache;
						param_begin = it;
					} else if (c == '}'_u) {
						if (macro_name_end == begin) {
							//no param
							co_yield{
								{it,end},
								{macro_name_start,cache},
								{}
							};
						} else {
							//have param
							co_yield{
								{it,end},
								{macro_name_start,macro_name_end},
								{param_begin,cache}
							};
						}
						start_parse = false;
						macro_name_start = begin;
						macro_name_end = begin;
						param_begin = begin;
					}
				} else if (c == '$'_u) {
					if (uncheck_peek(it) == '{'_u) {
						start_parse = true;
						utf8::unchecked::advance(it, 1);
						macro_name_start = it;
					} else {
						utf8::append(c, dest);
					}
				} else if (c == '\\'_u) {
					if (it != end) {
						utf8::append(utf8::unchecked::next(it), dest);
					} else {
						utf8::append(c, dest);
					}
				} else {
					utf8::append(c, dest);
				}
				cache = it;
			}
		}

#undef unchecked_peek
	}

	class macro_content_t {
	private:
		friend class placeholder_replacer;
		using content_func = std::function<std::string(std::string_view)>;
		using content_func_p = std::string(*)(std::string_view);
		std::variant<content_func, std::string> content;
	public:
		macro_content_t() :content{ "" } {};
		template<typename T,std::enable_if_t<std::is_constructible_v<content_func,T>>* =nullptr>
		macro_content_t(T&& func) :content{ std::forward<T>(func) } {};
		macro_content_t(std::string_view str) :content{ str.data() } {};
		macro_content_t(const char* str) :content{ str } {};

		macro_content_t& operator=(std::string_view str) {
			content = str.data();
			return *this;
		}
		template<typename T, std::enable_if_t<std::is_constructible_v<content_func, T>>* = nullptr>
		macro_content_t& operator=(content_func&& func) {
			content = std::forward<T>(func);
			return *this;
		}

		inline std::string operator()(std::string_view param) {
			return content.index() ? std::get<std::string>(content) : std::get<content_func>(content)(param);
		}
	};
	
	class placeholder_replacer {
	private:
		
		using placeholders_t = std::unordered_map<std::string, macro_content_t>;
		placeholders_t macros;

		std::string build_macro(std::string_view name, std::string_view param) {
			std::string result{};
			return param.empty() ?
				std::move(result.append("${").append(name).append("}")) :
				std::move(result.append("${").append(name).append(":").append(param).append("}"));
		}
	public:
		placeholder_replacer(std::initializer_list<placeholders_t::value_type> placeholders) :macros{ placeholders } {};
		placeholder_replacer(placeholders_t&& placeholders)noexcept :macros{ std::move(placeholders) } {};
		placeholder_replacer(placeholder_replacer&& other)noexcept :macros{ std::move(other.macros) } {};

		inline void assign(std::string key, std::string_view content) {
			macros[key] = content;
		}
		inline void assign(std::string key, macro_content_t::content_func replacer) {
			macros[key] = replacer;
		}

		std::string replace(std::string_view str) {
			std::string result{}, key{};
			std::string_view left{};
			auto parser = detail::co_parse_macro(str);
			for (auto& i : parser) {
				result.append(i.unmatched_content);
				key = i.macro_name;
				if (auto it = macros.find(key); it != macros.end())result.append(it->second(i.macro_param));
				else result.append(build_macro(i.macro_name, i.macro_param));
				left = i.current;
			}
			result.append(left);
			return std::move(result);
		}
		inline void replace(std::string_view str, std::string& dest) {
			std::string key{};
			auto parser = detail::co_parse_macro(str, dest);
			for (auto& i : parser) {
				if (auto it = macros.find(key); it != macros.end())dest.append(it->second(i.macro_param));
				else dest.append(build_macro(i.macro_name, i.macro_param));
			}
		}
	};

}

#endif // !_ALICE_PLACEHOLDER_REPLACER_
