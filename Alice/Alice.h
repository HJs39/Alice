#pragma once
#include <string>
#include <print>
#include <thread>
#include <chrono>
#include <syncstream>
#include <filesystem>
#pragma warning(push)
#pragma warning(disable: 4251)
#include <dpp/dpp.h>
#pragma warning(pop)
#pragma warning(push)
#pragma warning(disable: 26800)
#include <dpp/json.h>
#pragma warning(pop)
#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/chrono.h>
#include <re2/re2.h>
#pragma warning(push)
#pragma warning(disable: 4244)
#include "implement/number.h"
#pragma warning(pop)
#include "implement/command_expand_macro.h"
#include "implement/randomizer.h"
#include "implement/inplace_vector.h"
#include "implement/placeholder_replacer.h"
#include "implement/shunting_yard.h"
#include "implement/LLM/API_integration.h"
#include "implement/config_loader.h"