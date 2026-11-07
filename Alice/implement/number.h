#pragma once
#ifndef _ALICE_NUMBER_
#define _ALICE_NUMBER_
#include <cinttypes>
#include <charconv>
#include <type_traits>
#include <string_view>
#include <utility>
#include <algorithm>
#include <format>
#include <compare>
#include <cmath>

#pragma warning(push)
#pragma warning(disable:4242)

namespace Alice {
	/**
	 * @brief JavaScript like number class
	 */
	class number {
	private:
		enum state {
			Integer = 0,
			Floating = 1
		};
		union {
			std::int64_t i;
			double d;
		};
		state cur_state;
		friend class std::formatter<number>;
	public:
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		constexpr number(const integer_t i)noexcept :i{ i }, cur_state{ state::Integer } {};
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		constexpr number(const floating_t f)noexcept :d{ f }, cur_state{ state::Floating } {};
		explicit number(std::string_view str)noexcept {
			if (str.contains('.')) {
				std::from_chars(str.data(), str.data() + str.size(), d);
				cur_state = state::Floating;
			}
			else {
				std::from_chars(str.data(), str.data() + str.size(), i);
				cur_state = state::Integer;
			}
		};
		constexpr number()noexcept :i{ 0 }, cur_state{ state::Integer } {};
		constexpr number(const number& n)noexcept = default;
		constexpr number(number&& n)noexcept = default;
		friend constexpr void swap(number& first, number& second)noexcept {
			std::swap(first.cur_state, second.cur_state);
			std::swap(first.d, second.d);
		};
		constexpr number& operator=(number other)noexcept {
			swap(*this, other);
			return *this;
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>>
		constexpr number& operator=(integer_t integer)noexcept {
			i = integer;
			cur_state = state::Integer;
			return *this;
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>>
		constexpr number& operator=(floating_t f)noexcept {
			d = f;
			cur_state = state::Floating;
			return *this;
		}
		explicit constexpr operator double()const noexcept {
			if (cur_state) return d;
			return static_cast<double>(i);
		}
		explicit constexpr operator float()const noexcept {
			if (cur_state) return d;
			return static_cast<float>(i);
		}
		explicit constexpr operator std::int64_t()const noexcept {
			if (cur_state)return static_cast<std::int64_t>(d);
			return i;
		}
		explicit constexpr operator std::int32_t()const noexcept {
			if (cur_state)return static_cast<std::int32_t>(d);
			return i;
		}
		explicit constexpr operator std::uint64_t()const noexcept {
			if (cur_state)return static_cast<std::uint64_t>(d);
			return static_cast<std::uint64_t>(i);
		}
		explicit constexpr operator std::uint32_t()const noexcept {
			if (cur_state)return static_cast<std::uint32_t>(d);
			return static_cast<std::uint32_t>(i);
		}
		explicit constexpr operator bool()const noexcept {
			return cur_state ? d != 0.0 : i != 0;
		}
		
		/**
		 * @brief cast current number to an integer type
		 * @tparam integer_t integer type
		 * @tparam  filter
		 * @return value after cast
		 */
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		constexpr integer_t cast()noexcept {
			if (cur_state) i = static_cast<integer_t>(d);
			cur_state = Integer;
			return i;
		}
		
		/**
		 * @brief cast current number to an floating point
		 * @tparam floating_t floating point
		 * @tparam  filter
		 * @return value after cast
		 */
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		constexpr floating_t cast()noexcept {
			if (!cur_state)d = static_cast<floating_t>(i);
			cur_state = Floating;
			return d;
		}

		/**
		 * @brief get the base value by specific type(would not modify the stored value)
		 * @tparam num_t integer type
		 * @tparam  filter
		 * @return the value of num_t
		 */
		template<typename num_t, std::enable_if_t<std::is_floating_point_v<num_t> || std::is_integral_v<num_t>>* = nullptr>
		constexpr num_t get()const noexcept {
			return static_cast<num_t>(cur_state ? d : i);
		}

		constexpr bool is_infinity()const noexcept {
			return cur_state ? std::isinf(d) : false;
		}
		constexpr bool is_nan()const noexcept {
			return cur_state ? std::isnan(d) : false;
		}

		std::string to_string(int base = 10)const noexcept {
			char buffer[64]{};
			if (cur_state)std::to_chars(buffer, buffer + 63, d);
			else std::to_chars(buffer, buffer + 63, i, base);
			return buffer;
		}
		std::string to_string(std::chars_format fmt)const noexcept {
			char buffer[64]{};
			if (cur_state)std::to_chars(buffer, buffer + 63, d, fmt);
			else std::to_chars(buffer, buffer + 63, i);
			return buffer;
		}
		std::string to_string(std::chars_format fmt, int precision)const noexcept {
			char buffer[64]{};
			if (cur_state)std::to_chars(buffer, buffer + 63, d, fmt, precision);
			else std::to_chars(buffer, buffer + 63, i);
			return buffer;
		}

		constexpr number operator-()const noexcept {
			return cur_state ? -d : -i;
		}
		constexpr number operator-(number other)const noexcept {
			if (cur_state == other.cur_state) {
				if (cur_state)return number(d - other.d);
				return number(i - other.i);
			} else {
				if (cur_state)return number(d - other.i);
				return number(i - other.d);
			}
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		constexpr number operator-(integer_t integer)const noexcept {
			if (cur_state)return number(d - integer);
			return number(i - integer);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		constexpr number operator-(floating_t f)const noexcept {
			if (cur_state)return number(d - f);
			return number(i - f);
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		friend constexpr number operator-(integer_t integer, number num)noexcept {
			if (num.cur_state)return number(integer - num.d);
			return number(integer - num.i);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		friend constexpr number operator-(floating_t f, number num)noexcept {
			if (num.cur_state)return number(f - num.d);
			return number(f - num.i);
		}
		constexpr number operator+()const noexcept {
			return cur_state ? +d : +i;
		}
		constexpr number operator+(number other)const noexcept {
			if (cur_state == other.cur_state) {
				if (cur_state)return number(d + other.d);
				return number(i + other.i);
			} else {
				if (cur_state)return number(d + other.i);
				return number(i + other.d);
			}
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		constexpr number operator+(integer_t integer)const noexcept {
			if (cur_state)return number(d + integer);
			return number(i + integer);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		constexpr number operator+(floating_t f)const noexcept {
			if (cur_state)return number(d + f);
			return number(i + f);
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		friend constexpr number operator+(integer_t integer, number num)noexcept {
			if (num.cur_state)return number(integer + num.d);
			return number(integer + num.i);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		friend constexpr number operator+(floating_t f, number num)noexcept {
			if (num.cur_state)return number(f + num.d);
			return number(f + num.i);
		}
		constexpr number operator*(number other)const noexcept {
			if (cur_state == other.cur_state) {
				if (cur_state)return number(d * other.d);
				return number(i * other.i);
			} else {
				if (cur_state)return number(d * other.i);
				return number(i * other.d);
			}
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		constexpr number operator*(integer_t integer)const noexcept {
			if (cur_state)return number(d * integer);
			return number(i * integer);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		constexpr number operator*(floating_t f)const noexcept {
			if (cur_state)return number(d * f);
			return number(i * f);
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		friend constexpr number operator*(integer_t integer, number num)noexcept {
			if (num.cur_state)return number(integer * num.d);
			return number(integer * num.i);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		friend constexpr number operator*(floating_t f, number num)noexcept {
			if (num.cur_state)return number(f * num.d);
			return number(f * num.i);
		}
		constexpr number operator/(number other)const noexcept {
			if (cur_state == other.cur_state) {
				if (cur_state)return number(d / other.d);
				return number(i / other.i);
			} else {
				if (cur_state)return number(d / other.i);
				return number(i / other.d);
			}
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		constexpr number operator/(integer_t integer)const noexcept {
			if (cur_state)return number(d / integer);
			return number(i / integer);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		constexpr number operator/(floating_t f)const noexcept {
			if (cur_state)return number(d / f);
			return number(i / f);
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		friend constexpr number operator/(integer_t integer, number num)noexcept {
			if (num.cur_state)return number(integer / num.d);
			return number(integer / num.i);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		friend constexpr number operator/(floating_t f, number num)noexcept {
			if (num.cur_state)return number(f / num.d);
			return number(f / num.i);
		}
		constexpr number operator%(number other)const noexcept {
			if (cur_state == other.cur_state) {
				if (cur_state)return number(static_cast<int64_t>(d) % static_cast<int64_t>(other.d));
				return number(i % other.i);
			} else {
				if (cur_state)return number(static_cast<int64_t>(d) % other.i);
				return number(i % static_cast<int64_t>(other.d));
			}
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		constexpr number operator%(integer_t integer)const noexcept {
			if (cur_state)return number(static_cast<int64_t>(d) % integer);
			return number(i % integer);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		constexpr number operator%(floating_t f)const noexcept {
			if (cur_state)return number(static_cast<int64_t>(d) % static_cast<int64_t>(f));
			return number(i % static_cast<int64_t>(f));
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		friend constexpr number operator%(integer_t integer, number num)noexcept {
			if (num.cur_state)return number(integer % static_cast<int64_t>(num.d));
			return number(integer % num.i);
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		friend constexpr number operator%(floating_t f, number num)noexcept {
			if (num.cur_state)return number(static_cast<int64_t>(f) % static_cast<int64_t>(num.d));
			return number(static_cast<int64_t>(f) % num.i);
		}
		std::partial_ordering operator<=>(const number& other)const noexcept {
			if (cur_state == other.cur_state) {
				if (cur_state)return d <=> other.d;
				else return i <=> other.i;
			}
			else {
				std::int64_t i{};
				double d{};
				if (cur_state) {
					d = this->d;
					i = other.i;
				}
				else {
					d = other.d;
					i = this->i;
				}
				return d <=> i;
			}
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		friend std::partial_ordering operator<=>(const number& num, const integer_t& integer)noexcept {
			if (num.cur_state)return num.d <=> integer;
			else return num.i <=> integer;
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		friend std::partial_ordering operator<=>(const number& num, const floating_t& f)noexcept {
			if (num.cur_state)return num.d <=> f;
			else return num.i <=> f;
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		friend std::partial_ordering operator<=>(const integer_t& integer, const number& num)noexcept {
			if (num.cur_state)return num.d <=> integer;
			else return num.i <=> integer;
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		friend std::partial_ordering operator<=>(const floating_t& f, const number& num)noexcept {
			if (num.cur_state)return num.d <=> f;
			else return num.i <=> f;
		}
		bool operator==(const number& other)const noexcept {
			if (cur_state == other.cur_state) {
				if (cur_state)return d == other.d;
				else return i == other.i;
			}
			else {
				std::int64_t i{};
				double d{};
				if (cur_state) {
					d = this->d;
					i = other.i;
				}
				else {
					d = other.d;
					i = this->i;
				}
				return d == i;
			}
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		friend bool operator==(const number& num, const integer_t& integer)noexcept {
			if (num.cur_state)return num.d == integer;
			else return num.i == integer;
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		friend bool operator==(const number& num, const floating_t& f)noexcept {
			if (num.cur_state)return num.d == f;
			else return num.i == f;
		}
		template<typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>>* = nullptr>
		friend bool operator==(const integer_t& integer, const number& num)noexcept {
			if (num.cur_state)return num.d == integer;
			else return num.i == integer;
		}
		template<typename floating_t, std::enable_if_t<std::is_floating_point_v<floating_t>>* = nullptr>
		friend bool operator==(const floating_t& f, const number& num)noexcept {
			if (num.cur_state)return num.d == f;
			else return num.i == f;
		}
		friend std::formatter<number>;

		friend inline number abs(number v)noexcept {
			if (v.cur_state)return std::abs(v.d);
			else return std::abs(v.i);
		}
		friend inline number cbrt(number v)noexcept {
			if (v.cur_state)return std::cbrt(v.d);
			else return std::cbrt(v.i);
		}
		friend inline number ceil(number v)noexcept {
			if (v.cur_state)return std::ceil(v.d);
			else return std::ceil(v.i);
		}
		friend inline number cos(number v)noexcept {
			if (v.cur_state)return std::cos(v.d);
			else return std::cos(v.i);
		}
		friend inline number cosh(number v)noexcept {
			if (v.cur_state)return std::cosh(v.d);
			else return std::cosh(v.i);
		}
		friend inline number exp(number v)noexcept {
			if (v.cur_state)return std::exp(v.d);
			else return std::exp(v.i);
		}
		friend inline number floor(number v)noexcept {
			if (v.cur_state)return std::floor(v.d);
			else return std::floor(v.i);
		}
		friend inline number log(number v)noexcept {
			if (v.cur_state)return std::log(v.d);
			else return std::log(v.i);
		}
		friend inline number log2(number v)noexcept {
			if (v.cur_state)return std::log2(v.d);
			else return std::log2(v.i);
		}
		friend inline number log10(number v)noexcept {
			if (v.cur_state)return std::log10(v.d);
			else return std::log10(v.i);
		}
		friend inline number log(number r, number l)noexcept {
			if (r.cur_state == l.cur_state) {
				if (r.cur_state)return std::log(r.d) / std::log(l.d);
				else return std::log(r.i) / std::log(l.i);
			} else {
				if (r.cur_state)return std::log(r.d) / std::log(l.i);
				else return std::log(r.i) / std::log(l.d);
			}
		}
		friend inline number neg(number v)noexcept {
			if (v.cur_state)return v.d > 0 ? -v.d : v.d;
			else return v.i > 0 ? -v.i : v.i;
		}
		friend inline number pow(number r, number l)noexcept {
			if (r.cur_state == l.cur_state) {
				if (r.cur_state)return std::pow(r.d, l.d);
				else return std::pow(r.i, l.i);
			} else {
				if (r.cur_state)return std::pow(r.d, l.i);
				else return std::pow(r.i, l.d);
			}
		}
		friend inline number round(number v)noexcept {
			if (v.cur_state)return std::round(v.d);
			else return std::round(v.i);
		}
		friend inline number sin(number v)noexcept {
			if (v.cur_state)return std::sin(v.d);
			else return std::sin(v.i);
		}
		friend inline number sinh(number v)noexcept {
			if (v.cur_state)return std::sinh(v.d);
			else return std::sinh(v.i);
		}
		friend inline number sqrt(number v)noexcept {
			if (v.cur_state)return std::sqrt(v.d);
			else return std::sqrt(v.i);
		}
		friend inline number tan(number v)noexcept {
			if (v.cur_state)return std::tan(v.d);
			else return std::tan(v.i);
		}
		friend inline number tanh(number v)noexcept {
			if (v.cur_state)return std::tanh(v.d);
			else return std::tanh(v.i);
		}
		friend inline number trunc(number v)noexcept {
			if (v.cur_state)return std::trunc(v.d);
			else return std::trunc(v.i);
		}
	};
}
namespace std {
	template<>
	struct std::formatter<Alice::number> {
		bool is_float = false;
		bool specific = true;
		std::formatter<std::int64_t> i_formatter = std::formatter<std::int64_t>();
		std::formatter<double> d_formatter = std::formatter<double>();
		constexpr auto parse(std::format_parse_context& ctx) {
			auto it = ctx.begin();
			if (*it == 'f') {
				std::string_view temp{ ctx.begin(),ctx.end() };
				temp.remove_prefix(1);
				ctx.advance_to(++it);
				is_float = true;
				return d_formatter.parse(ctx);
			}
			else if (*it == 'i') {
				std::string_view temp{ ctx.begin(),ctx.end() };
				temp.remove_prefix(1);
				ctx.advance_to(++it);
				return i_formatter.parse(ctx);
			}
			else {
				specific = false;
				return ctx.end();
			}
		}

		auto format(const Alice::number& n, std::format_context& ctx)const {
			if (specific) {
				if (is_float) {
					return d_formatter.format(n.cur_state ? n.d : static_cast<double>(n.i), ctx);
				}
				else {
					return i_formatter.format(n.cur_state ? static_cast<std::int64_t>(n.d) : n.i, ctx);
				}
			}
			else {
				if (n.cur_state) {
					return d_formatter.format(n.d, ctx);
				}
				else {
					return i_formatter.format(n.i, ctx);
				}
			}
		}
	};
}

#pragma warning(pop)

#endif