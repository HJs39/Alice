#pragma once
#ifndef _ALICE_LIBRARY_
#define _ALICE_LIBRARY_

#ifndef _ALICE_SUB_LIBRARY_MAXIMUM_
//define for changing number of maximum sub-library in library group conveniently
#define _ALICE_SUB_LIBRARY_MAXIMUM_ 5
#endif // !_ALICE_SUB_LIBRARY_MAXIMUM_


#include <string>
#include <optional>
#include <vector>
#include <dpp/json.h>

#include "../shunting_yard.h"

namespace Alice {
	struct library_t {
		using string = std::string;
		string title;
		string description;
		string detail;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
		library_t,
		title,
		description,
		detail
	);

	enum class filter_type :uint32_t {
		And,
		Or,
		Avarage,
		Custom //use processor implement in "implement/shunting_yard.h"
	};

	inline constexpr int sub_library_maximum = _ALICE_SUB_LIBRARY_MAXIMUM_;
	
	/**
	 * @brief base type of personas library function
	 * 
	 * one group can only contain number of sub_library_maximum library as sub-library
	 */
	struct library_group_t {
		using string = std::string;
		inline static constexpr uint32_t all = static_cast<uint32_t>(-1);
		/**
		 * @brief required, use to display on discord conpomemt
		 */
		string title;
		/**
		 * @brief optional, a simple summary for JEV to understand what content in this group
		 */
		string description;
		/**
		 * @brief filter type enumeration; if value is "Custom", the user-defined function will be called
		 */
		filter_type filter;
		/**
		 * @brief required, conditions for JEV to judge
		 */
		std::vector<string> questions;

		std::vector<Alice::detail::shunting_yard_obj_t> expression;
		std::vector<library_t> librarys;
		/**
		 * @brief number of library to send when pass the condition, default is 1
		 */
		uint32_t trigger_number;
	};

}

#endif // !_ALICE_LIBRARY_
