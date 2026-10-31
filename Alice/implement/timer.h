#pragma once
#ifndef _ALICE_TIMER_
#define _ALICE_TIMER_
#include <thread>
#include <chrono>
#include <concepts>

namespace Alice {
	namespace detail {
		class timer {
		public:
			static constexpr const char* time_format_str = "%FT%TZ";
			using clock_t = std::chrono::system_clock;
			
			/**
			 * @brief equal to std::this_thread::sleep_for
			 * @param milliseconds the time for thread to sleep
			 */
			inline static void wait(std::chrono::milliseconds milliseconds)noexcept {
				std::this_thread::sleep_for(milliseconds);
			}
			inline static void wait(int seccond)noexcept {
				std::this_thread::sleep_for(std::chrono::milliseconds(seccond));
			}
			
			/**
			 * @brief similar to std::this_thread::sleep_until but check by function
			 * @tparam awaiter_t function type
			 * @param awaiter the function determined whether this thread need to continue sleeping or not
			 */
			template<std::predicate<void> awaiter_t>
			inline static void wait_until(const awaiter_t&& awaiter)noexcept {
				do {
					std::this_thread::yield();
				} while (awaiter());
			}
		};
	}
	using timer = detail::timer;
}

#endif // !_ALICE_TIMER_