#pragma once
#ifndef __ALICE_CONFIG_LOADER__
#define __ALICE_CONFIG_LOADER__

#define STB_IMAGE_IMPLEMENTATION

#include <fstream>
#include <filesystem>
#include <dpp/json.h>
#include <dpp/utility.h>
#include <fmt/format.h>
#include <stb_image.h>

namespace Alice {
	enum class assets_path {
		Local,
		Assets
	};
	constexpr char png_prefix[] = "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A";
	constexpr char jpg_prefix[] = "\xFF\xD8\xFF";
	constexpr char gif_prefix[] = "\x47\x49\x46\x38";
	constexpr char webp_prefix_1[] = "\x52\x49\x46\x46";
	constexpr char webp_prefix_2[] = "\x57\x45\x42\x50";
	class config_loader {
	private:
		config_loader() = default;
		config_loader(const config_loader&) = delete;
		config_loader(config_loader&&) = delete;
		config_loader& operator=(const config_loader&) = delete;
		config_loader& operator=(config_loader&&) = delete;
	public:
		enum image_return_type {
			Dpp_image_data,
			Standard_string
		};
		enum action_when_not_exit {
			Throw,
			Create,
			Return_empty
		};
		static config_loader& get_loader() {
			static config_loader loader{};
			return loader;
		}

		template<image_return_type type = image_return_type::Dpp_image_data>
		auto load_image(const assets_path root, std::string path) {
			if constexpr (type == Dpp_image_data) {
				std::filesystem::path r{ root == assets_path::Assets ? "assets" : "" };
				std::string image{ dpp::utility::read_file((r / path).string()) };
				if (image.starts_with(png_prefix)) {
					return dpp::utility::image_data(dpp::i_png, image);
				} else if (image.starts_with(jpg_prefix)) {
					return dpp::utility::image_data(dpp::i_jpg, image);
				} else if (image.starts_with(gif_prefix)) {
					return dpp::utility::image_data(dpp::i_gif, image);
				} else {
					std::string_view view{ image };
					if (view.size() > 12 && view.starts_with(webp_prefix_1)) {
						view.remove_prefix(7);
						if (view.starts_with(webp_prefix_2))return dpp::utility::image_data(dpp::i_webp, image);
					}
					return dpp::utility::image_data{};
				}
			} else {
				std::filesystem::path r{ root == assets_path::Assets ? "assets/" : "" };
				return dpp::utility::read_file((r / path).string());
			}
		}

		template<action_when_not_exit act = action_when_not_exit::Return_empty>
		nlohmann::json load_json(const assets_path root, std::string path) {
			std::filesystem::path r{ root == assets_path::Assets ? "assets" : "" };
			r /= path;
			if (!std::filesystem::exists(r)) {
				if constexpr (act == action_when_not_exit::Throw) {
					throw std::runtime_error(fmt::format("\"{}\" is not existed", r.string()));
				} else if constexpr (act == action_when_not_exit::Create) {
					std::filesystem::create_directories(std::filesystem::path{ root == assets_path::Assets ? "assets" : "" });
					std::ofstream(r).write("{}", 3);
				} else {
					return {};
				}
			}
			return nlohmann::json::parse(std::ifstream(r));
		}
	};
}

#endif // !__ALICE_CONFIG_LOADER__
