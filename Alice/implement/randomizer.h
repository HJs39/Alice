#pragma once
#ifndef _ALICE_RANDOMIZER_
#define _ALICE_RANDOMIZER_

#ifndef _ALICE_RANDOMIZER_ENGINE_MAXIMUM_
#define _ALICE_RANDOMIZER_ENGINE_MAXIMUM_ 4
#endif // !_ALICE_RANDOMIZER_ENGINE_MAXIMUM_

#include <type_traits>
#include <array>
#include <atomic>
#include <random>
#include <initializer_list>
#include <optional>
#include "./inplace_vector.h"

namespace Alice {
	class randomizer {
	private:
		struct engine_state_t {
			std::mt19937 engine;
			std::atomic<bool> is_using;
			engine_state_t(std::seed_seq& seq) :engine{ seq }, is_using{ false } {};
		};
		inplace_vector<engine_state_t, _ALICE_RANDOMIZER_ENGINE_MAXIMUM_> engines;
		std::seed_seq seeds;
		static std::random_device rd;
	public:
		using seed_result_t = std::seed_seq::result_type;
		using engine_type = std::mt19937;
		randomizer() :seeds{ rd(),static_cast<std::random_device::result_type>(std::time(nullptr)),39u,static_cast<uint32_t>(std::rand()) }, engines{} {
			for (size_t i{ 0 }; i < engines.max_size(); ++i)engines.unchecked_emplace_back(seeds);
		};
		randomizer(std::initializer_list<uint32_t> seq) :seeds{ seq }, engines{} {
			for (size_t i{ 0 }; i < engines.max_size(); ++i)engines.unchecked_emplace_back(seeds);
		};
		randomizer(const randomizer& other) = delete;
		randomizer(randomizer&& other) = delete;
		randomizer& operator=(const randomizer& other) = delete;
		randomizer& operator=(randomizer&& other) = delete;

		template<typename iterator>
		void generate_seed(iterator begin,iterator end) {
			seeds.generate(begin, end);
		}

		size_t usable_count()const noexcept {
			size_t count{ 0 };
			for (auto& i : engines)if (!i.is_using.load())++count;
			return count;
		}

		engine_type& get_engine()noexcept {
			while (true) {
				for (auto& engine : engines) {
					if (engine.is_using.load(std::memory_order_acquire) && !engine.is_using.exchange(true, std::memory_order_acq_rel)) {
						//std::atomic<bool>.exchange() ensure the action is atomic, so we just need to check whether the result is false or not
						return engine.engine;
					}
				}
			}
		}
		std::optional<engine_type*> try_get_engine()noexcept {
			for (auto& engine : engines) {
				if (engine.is_using.load(std::memory_order_acquire) && !engine.is_using.exchange(true, std::memory_order_acq_rel)) {
					//std::atomic<bool>.exchange() ensure the action is atomic, so we just need to check whether the result is false or not
					return &engine.engine;
				}
			}
			return std::nullopt;
		}

		engine_type::result_type generate_oneshot() {
			while (true) {
				for (auto& engine : engines) {
					if (engine.is_using.load(std::memory_order_acquire) && !engine.is_using.exchange(true, std::memory_order_acq_rel)) {
						//std::atomic<bool>.exchange() ensure the action is atomic, so we just need to check whether the result is false or not
						auto result = engine.engine();
						engine.is_using.store(false, std::memory_order_release);
						return result;
					}
				}
			}
		}
		template<typename Integer_t = engine_type::result_type, std::enable_if_t<std::is_integral_v<Integer_t>>* = nullptr>
		Integer_t generate_oneshot(Integer_t min, Integer_t max = std::numeric_limits<Integer_t>::max()) {
			while (true) {
				for (auto& engine : engines) {
					if (engine.is_using.load(std::memory_order_acquire) && !engine.is_using.exchange(true, std::memory_order_acq_rel)) {
						//std::atomic<bool>.exchange() ensure the action is atomic, so we just need to check whether the result is false or not
						Integer_t result = std::uniform_int_distribution<Integer_t>(min, max)(engine.engine());
						engine.is_using.store(false, std::memory_order_release);
						return result;
					}
				}
			}
		}
		template<typename Floating_t = double, std::enable_if_t<std::is_floating_point_v<Floating_t>>* = nullptr>
		Floating_t generate_oneshot(Floating_t min, Floating_t max = std::numeric_limits<Floating_t>::max()) {
			while (true) {
				for (auto& engine : engines) {
					if (engine.is_using.load(std::memory_order_acquire) && !engine.is_using.exchange(true, std::memory_order_acq_rel)) {
						//std::atomic<bool>.exchange() ensure the action is atomic, so we just need to check whether the result is false or not
						Floating_t result = std::uniform_real_distribution<Floating_t>(min, max)(engine.engine());
						engine.is_using.store(false, std::memory_order_release);
						return result;
					}
				}
			}
		}
		std::optional<engine_type::result_type> try_generate_oneshot() {
			for (auto& engine : engines) {
				if (engine.is_using.load(std::memory_order_acquire) && !engine.is_using.exchange(true, std::memory_order_acq_rel)) {
					//std::atomic<bool>.exchange() ensure the action is atomic, so we just need to check whether the result is false or not
					auto result = engine.engine();
					engine.is_using.store(false, std::memory_order_release);
					return result;
				}
			}
			return std::nullopt;
		}
		template<typename Integer_t = engine_type::result_type, std::enable_if_t<std::is_integral_v<Integer_t>>* = nullptr>
		std::optional<Integer_t> try_generate_oneshot(Integer_t min, Integer_t max = std::numeric_limits<Integer_t>::max()) {
			for (auto& engine : engines) {
				if (engine.is_using.load(std::memory_order_acquire) && !engine.is_using.exchange(true, std::memory_order_acq_rel)) {
					//std::atomic<bool>.exchange() ensure the action is atomic, so we just need to check whether the result is false or not
					Integer_t result = std::uniform_int_distribution<Integer_t>(min, max)(engine.engine());
					engine.is_using.store(false, std::memory_order_release);
					return result;
				}
			}
			return std::nullopt;
		}
		template<typename Floating_t = double, std::enable_if_t<std::is_floating_point_v<Floating_t>>* = nullptr>
		std::optional<Floating_t> try_generate_oneshot(Floating_t min, Floating_t max = std::numeric_limits<Floating_t>::max()) {
			for (auto& engine : engines) {
				if (engine.is_using.load(std::memory_order_acquire) && !engine.is_using.exchange(true, std::memory_order_acq_rel)) {
					//std::atomic<bool>.exchange() ensure the action is atomic, so we just need to check whether the result is false or not
					Floating_t result = std::uniform_real_distribution<Floating_t>(min, max)(engine.engine());
					engine.is_using.store(false, std::memory_order_release);
					return result;
				}
			}
			return std::nullopt;
		}

		bool release(const engine_type& e)noexcept {
			for (auto& engine : engines) {
				if (&engine.engine == &e) {
					engine.is_using.store(false, std::memory_order_release);
					return true;
				}
			}
			return false;
		}
	};
	std::random_device randomizer::rd{};
}

#endif // !_ALICE_RANDOMIZER_
