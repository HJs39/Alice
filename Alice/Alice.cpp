#define BOT_CLUSTER bot

#include "Alice.h"

using json = nlohmann::json;
using namespace std::literals;

// just some test use code
int main() {
	auto& loader = Alice::config_loader::get_loader();
	auto bot_assets = loader.load_json(Alice::assets_path::Assets, "bot_assets.json");
	json data = JEV_request_data(
		make_stats(
			make_kv_pair("stats one","stats data test")
		),
		make_questions(
			make_noul(
				"noul",
				"this is a noul"
			),
			make_score(
				"score",
				"this is a score",
				question_list(
					"first",
					"second",
					"third"
				)
			),
			make_choice(
				"choice",
				"this is a choice",
				question_list(
					make_kv_pair("k1","v1"),
					make_kv_pair("k2","v2")
				)
			)
		)
	);
	auto log = [](dpp::loglevel level,const char* mes) {
		if (level >= dpp::ll_critical) {
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::dark_red) | fmt::emphasis::bold, "[Critical]"), mes, std::chrono::system_clock::now());
		} else if (level >= dpp::ll_error) {
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::red) | fmt::emphasis::bold, "[Error]"), mes, std::chrono::system_clock::now());
		} else if (level >= dpp::ll_warning) {
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::orange) | fmt::emphasis::bold, "[Warning]"), mes, std::chrono::system_clock::now());
		} else if (level >= dpp::ll_info) {
			fmt::println("[Info]: {}\n  when: {:%Y-%m-%d %H:%M:%S}", mes, std::chrono::system_clock::now());
		} else if (level >= dpp::ll_debug) {
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::alice_blue) | fmt::emphasis::bold, "[Debug]"), mes, std::chrono::system_clock::now());
		} else if (level >= dpp::ll_trace) {
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::sky_blue) | fmt::emphasis::bold, "[Trace]"), mes, std::chrono::system_clock::now());
		}
		};
	log(dpp::ll_info, data.dump(2).c_str());
	log(dpp::ll_info, bot_assets.dump(2).c_str());
	std::system("pause");
	return 0;
	dpp::cluster bot(bot_assets["token"].get<std::string>());
	bot.on_log([](const dpp::log_t& log) {
		if (log.severity >= dpp::ll_critical) {
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::dark_red) | fmt::emphasis::bold, "[Critical]"), log.message, std::chrono::system_clock::now());
		}else if (log.severity >= dpp::ll_error) {
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::red) | fmt::emphasis::bold, "[Error]"), log.message, std::chrono::system_clock::now());
		} else if (log.severity >= dpp::ll_warning) {
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::orange) | fmt::emphasis::bold, "[Warning]"), log.message, std::chrono::system_clock::now());
		} else if (log.severity >= dpp::ll_info) {
			fmt::println("[Info]: {}\n  when: {:%Y-%m-%d %H:%M:%S}", log.message, std::chrono::system_clock::now());
		}else if(log.severity>=dpp::ll_debug){
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::alice_blue) | fmt::emphasis::bold, "[Debug]"), log.message, std::chrono::system_clock::now());
		} else if (log.severity >= dpp::ll_trace) {
			fmt::println("{}: {}\n  when: {:%Y-%m-%d %H:%M:%S}", fmt::format(fmt::fg(fmt::color::sky_blue) | fmt::emphasis::bold, "[Trace]"), log.message, std::chrono::system_clock::now());
		}
		});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_command>()) {
			std::vector<dpp::slashcommand> commands{
				{"ping","Ping pong!",bot.me.id}
			};
			bot.log(dpp::loglevel::ll_info, "log in and start to regester command");
			bot.global_bulk_command_create(commands);
		}
		});
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event)->dpp::task<void> {
		if (event.command.get_command_name() == "test") {
			co_await event.co_reply("this is a test message!");
		}
		co_return;
		});
	try {
		bot.start(dpp::st_wait);
	}
	catch (dpp::exception& e) {
		std::println("Catught an error while start: {}", e.what());
	}
	return 0;
}