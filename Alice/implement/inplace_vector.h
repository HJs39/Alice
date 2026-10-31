#pragma once
#ifndef _ALICE_INPLACE_VECTOR_
#define _ALICE_INPLACE_VECTOR_

#include <cstddef>
#include <new>
#include <type_traits>
#include <memory>
#include <initializer_list>
#include <stdexcept>
#include <optional>
#include <ranges>

namespace Alice {
	template<typename base_t,size_t N>
	class inplace_vector {
	private:
		alignas(base_t) std::byte storage[sizeof(base_t) * N + alignof(base_t)];
		size_t contained;

		inline static constexpr size_t type_size = sizeof(base_t);

		inline constexpr base_t* to_pointer(size_t idx)noexcept {
			return std::launder(reinterpret_cast<base_t*>(storage + type_size * idx));
		}
		inline const constexpr base_t* to_pointer(size_t idx)const noexcept {
			return std::launder(reinterpret_cast<const base_t*>(storage + type_size * idx));
		}

		template<typename...Args>
		constexpr inline void construct(size_t idx, Args&&... args) {
			::new (static_cast<void*>(storage + type_size * idx)) base_t(std::forward<Args>(args)...);
		}
		constexpr inline void distory(size_t idx)noexcept {
			to_pointer(idx)->~base_t();
		}

	public:
		using value_type = base_t;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using iterator = pointer;
		using const_iterator = const_pointer;

		constexpr inplace_vector() = default;
		constexpr explicit inplace_vector(size_type count)noexcept(std::is_nothrow_default_constructible_v<value_type>) :contained{0} {
			static_assert(std::is_default_constructible_v<value_type>, "is not a default-constructible type");
			constexpr size_type maximum = count > N ? N : count;
			for (size_type i{ 0 }; i < maximum;) {
				construct(i++);
				++contained;
			}
		};
		constexpr inplace_vector(std::initializer_list<value_type> list)
			noexcept(N == 0 ||std::is_nothrow_copy_constructible_v<value_type>) {
			static_assert(std::is_copy_constructible_v<value_type>, "is not a copy-constructible type");
			auto it = list.begin();
			constexpr size_type maximum = list.size() > N ? N : list.size();
			for (size_type i{ 0 }; i < maximum;) {
				construct(i++, *(it++));
			}
			contained = maximum;
		}
		template<typename T>
		constexpr inplace_vector(std::initializer_list<T> list)
			noexcept(N == 0 || std::is_nothrow_copy_constructible_v<value_type>) {
			static_assert(std::is_copy_constructible_v<value_type>, "is not a copy-constructible type");
			auto it = list.begin();
			constexpr size_type maximum = list.size() > N ? N : list.size();
			for (size_type i{ 0 }; i < maximum;) {
				construct(i, *(it++));
				++contained;
			}
		}
		constexpr inplace_vector(const inplace_vector& other)noexcept(N == 0 || std::is_nothrow_copy_constructible_v<value_type>) :contained{ other.contained } {
			static_assert(std::is_copy_constructible_v<value_type>, "is not a copy-constructible type");
			for (size_type i{ 0 }; i < contained;) construct(i, *other.to_pointer(i));
		}
		constexpr inplace_vector(inplace_vector&& other)noexcept(N == 0 || std::is_nothrow_move_constructible_v<value_type>) :contained{ other.contained } {
			static_assert(std::is_move_constructible_v<value_type>, "is not a move-constructible type");
			for (size_type i{ 0 }; i < contained;) construct(i, std::move(*other.to_pointer(i)));
			other.clear();
		}
		
		constexpr inplace_vector& operator=(const inplace_vector& other)
			noexcept(N == 0 ||
				(std::is_nothrow_copy_assignable_v<value_type> &&
					std::is_nothrow_copy_constructible_v<value_type>)) {
			static_assert(std::is_copy_constructible_v<value_type>, "is not a copy-constructible type");
			if (this == &other) [[unlikely]] return *this;
			const size_type cache{ contained };
			if (contained > other.contained) {
				for (; contained > other.contained;) {
					distory(contained--);
				}
			}
			if constexpr (std::is_copy_assignable_v<value_type>) {
				for (contained = 0; contained < other.contained;) {
					if (contained < cache) {
						*to_pointer(contained) = *other.to_pointer(contained++);
					}
					else {
						construct(contained, *other.to_pointer(contained++));
					}
				}
			}
			else {
				for (contained = 0; contained < other.contained;) {
					if (contained < cache) {
						distory(contained);
						construct(contained, *other.to_pointer(contained++));
					}
					else {
						construct(contained, *other.to_pointer(contained++));
					}
				}
			}
			return *this;
		}
		constexpr inplace_vector& operator=(inplace_vector&& other)
			noexcept(N == 0 ||
				(std::is_nothrow_move_assignable_v<value_type> &&
					std::is_nothrow_move_constructible_v<value_type>)) {
			static_assert(std::is_move_constructible_v<value_type>, "is not a move-constructible type");
			if (this == &other) [[unlikely]] return *this;
			const size_type cache{ contained };
			if (contained > other.contained) {
				for (; contained > other.contained;) {
					distory(contained--);
				}
			}
			if constexpr (std::is_copy_assignable_v<value_type>) {
				for (contained = 0; contained < other.contained;) {
					if (contained < cache) {
						*to_pointer(contained) = std::move(*other.to_pointer(contained++));
					}
					else {
						construct(contained, std::move(*other.to_pointer(contained++)));
					}
				}
			}
			else {
				for (contained = 0; contained < other.contained;) {
					if (contained < cache) {
						distory(contained);
						construct(contained, std::move(*other.to_pointer(contained++)));
					}
					else {
						construct(contained, std::move(*other.to_pointer(contained++)));
					}
				}
			}
			return *this;
		}

		constexpr void assign(size_type count, const_reference value)
			noexcept(std::is_nothrow_copy_assignable_v<value_type>&& std::is_nothrow_copy_constructible_v<value_type>) {
			static_assert(std::is_copy_constructible_v<value_type>, "is not a copy-constructible type");
			if (count > N)throw std::bad_alloc{};
			for (size_t i{ 0 }; i < count;) {
				if (i < contained) {
					if constexpr (std::is_copy_assignable_v<value_type>) {
						*to_pointer(i++) = value;
					}
					else {
						distory(i);
						construct(i++, value);
					}
				}
				else {
					construct(i++, value);
				}
			}
			if (count > contained)contained = count;
		}
		template<typename Input_iterator>
		constexpr void assign(Input_iterator begin, Input_iterator end)
			noexcept(std::is_nothrow_copy_assignable_v<value_type>&& std::is_nothrow_copy_constructible_v<value_type>) {
			static_assert(std::is_copy_constructible_v<value_type>, "is not a copy-constructible type");
			const size_type count = std::ranges::distance(begin, end);
			if (count > N)throw std::bad_alloc{};
			for (size_t i{ 0 }; begin != end;) {
				if (i < contained) {
					if constexpr (std::is_copy_assignable_v<value_type>) {
						*to_pointer(i++) = *(begin++);
					}
					else {
						distory(i);
						construct(i++, *(begin++));
					}
				}
				else {
					construct(i++, *(begin++));
				}
			}
			if (count > contained)contained = count;
		}
		constexpr void assign(std::initializer_list<value_type> list)
			noexcept(std::is_nothrow_copy_assignable_v<value_type>&& std::is_nothrow_copy_constructible_v<value_type>) {
			static_assert(std::is_copy_constructible_v<value_type>, "is not a copy-constructible type");
			if (list.size() > N)throw std::bad_alloc{};
			auto begin = list.begin();
			auto end = list.end();
			for (size_t i{ 0 }; begin != end;) {
				if (i < contained) {
					if constexpr (std::is_copy_assignable_v<value_type>) {
						*to_pointer(i++) = *(begin++);
					}
					else {
						distory(i);
						construct(i++, *(begin++));
					}
				}
				else {
					construct(i++, *(begin++));
				}
			}
			if (list.size() > contained)contained = list.size();
		}

		template<typename...Args>
		constexpr iterator emplace_back(Args&&... args) {
			if (contained == N)throw std::bad_alloc{};
			construct(contained++, std::forward<Args>(args)...);
			return to_pointer(contained - 1);
		}
		template<typename...Args>
		constexpr std::optional<reference> try_emplace_back(Args&&... args) {
			if (contained == N)return std::nullopt;
			construct(contained++, std::forward<Args>(args)...);
			return *to_pointer(contained - 1);
		}
		template<typename...Args>
		constexpr reference unchecked_emplace_back(Args&&...args) {
			construct(contained++, std::forward<Args>(args)...);
			return *to_pointer(contained - 1);
		}

		inline constexpr void pop_back() {
			distory(contained--);
		}

		inline constexpr reference at(size_t idx) {
			if (idx >= contained)throw std::out_of_range{};
			return *to_pointer(idx);
		}
		inline constexpr const_reference at(size_t idx)const {
			if (idx >= contained)throw std::out_of_range{};
			return *to_pointer(idx);
		}
		inline constexpr void resize(size_t size)
			noexcept(std::is_nothrow_default_constructible_v<value_type>) {
			if (size > N)throw std::bad_alloc{};
			else if (size == contained)return;
			else if (size > contained) {
				static_assert(std::is_default_constructible_v<value_type>,"is not a default-constructible type");
				for (; contained < size;) construct(contained++);
			}
			else {
				for (; contained > size;)distory(contained--);
			}
		}
		inline constexpr void resize(size_t size,const value_type& value)
			noexcept(std::is_nothrow_default_constructible_v<value_type> && 
				((std::is_copy_assignable_v<value_type>&&std::is_nothrow_copy_assignable_v<value_type>) ||
					(std::is_copy_constructible_v<value_type>&&std::is_nothrow_copy_constructible_v<value_type>))) {
			if (size > N)throw std::bad_alloc{};
			else if (size == contained)return;
			else if (size > contained) {
				if constexpr (std::is_copy_assignable_v<value_type>) {
					for (; contained < size;)*to_pointer(contained++) = value;
				}
				else if constexpr (std::is_copy_constructible_v<value_type>) {
					for (; contained < size;) {
						distory(contained);
						construct(contained++, value);
					}
				}
				else {
					static_assert(false, "is not copy-assignable or copy-constructible type");
				}
				
			}
			else {
				for (; contained > size;)distory(contained--);
			}
		}
		static inline void reserve(size_t new_cap) {
			if (new_cap > N)throw std::bad_alloc{};
		}
		static constexpr void shrink_to_fit()noexcept {}
		inline constexpr size_type size()const noexcept {
			return contained;
		}
		static inline constexpr size_type max_size()noexcept {
			return N;
		}
		static inline constexpr size_type capacity()noexcept {
			return N;
		}
		inline constexpr bool empty()const noexcept {
			return contained == 0;
		}
		inline constexpr void clear()noexcept {
			for (size_type i{ 0 }; i < contained;)distory(i++);
			contained = 0;
		}

		inline constexpr reference operator[](size_t idx)noexcept {
			return *to_pointer(idx);
		}
		inline constexpr const_reference operator[](size_t idx)const noexcept {
			return *to_pointer(idx);
		}
		inline constexpr reference front()noexcept {
			return *to_pointer(0);
		}
		inline constexpr const_reference front()const noexcept {
			return *to_pointer(0);
		}
		inline constexpr reference back()noexcept {
			return *to_pointer(contained - 1);
		}
		inline constexpr const_reference back()const noexcept {
			return *to_pointer(contained - 1);
		}

		inline constexpr pointer begin()noexcept {
			return to_pointer(0);
		}
		inline constexpr const_pointer begin()const noexcept {
			return to_pointer(0);
		}
		inline constexpr pointer end()noexcept {
			return to_pointer(contained);
		}
		inline constexpr const_pointer end()const noexcept {
			return to_pointer(contained);
		}
		inline constexpr const_pointer cbegin()const noexcept {
			return to_pointer(0);
		}
		inline constexpr const_pointer cend()const noexcept {
			return to_pointer(contained);
		}
		inline constexpr pointer data()noexcept {
			return to_pointer(0);
		}

		constexpr ~inplace_vector() {
			clear();
		}
	};

}

#endif // !_ALICE_INPLACE_VECTOR_
