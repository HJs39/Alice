#pragma once
#ifndef _ALICE_BASE_API_REQUEST_
#define _ALICE_BASE_API_REQUEST_

#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <string_view>
#include <dpp/dpp.h>
#include <curl/curl.h>
#include <glaze/json.hpp>
#include <fmt/format.h>

namespace Alice {
	namespace detail {
		/**
		 * @brief openai format response
		 *
		 * if error is true, the body will be a json object which contain a "err_mes" string and "http_header" string
		 */
		struct response_t {
			glz::generic body;
			bool error;
			response_t(glz::generic b, bool e) :body{ b }, error{ e } {};
			response_t() :body{}, error{} {};
		};

		struct response_err_t {
			std::string err_mes;
			std::string http_header;
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(response_err_t, err_mes, http_header)

			class curl_handle_node {
			private:
				std::atomic<bool> is_used;
				void* stream_callback;
				curl_slist* http_header;
				friend class session_t;

				using stream_write_func_t = size_t(*)(void*, size_t, size_t, std::string*);


				size_t stream_write_function(void* ptr, size_t size, size_t nmemb, std::string* data) {
					std::string_view str{ static_cast<const char*>(ptr),size * nmemb };
					if (str.starts_with("data: ") && data->empty()) {
						/**
						* if data is empty, assume it is a new data line
						* or otherwise like "{\"str\":\"..." in data, "data: " would not be discard
						*/
						str.remove_prefix(6);
					}
					if (str == "[DONE]") {
						if (!data->empty()) {
							if (stream_callback) {
								glz::generic j{};
								auto err = glz::read_json(j, *data);
								if (!err)(*static_cast<std::function<void(glz::generic)>*>(stream_callback))(j);
							}
							// do nothing
						}
						return size * nmemb;
					}

					size_t find_pos = str.find("\n\n");
					while (find_pos != std::string_view::npos) {
						data->append(str.substr(0, find_pos));
						try {
							if (stream_callback) {
								glz::generic j{};
								auto err = glz::read_json(j, *data);
								if (!err)(*static_cast<std::function<void(glz::generic)>*>(stream_callback))(j);
							}
						} catch (...) {
							continue;
						}
						data->clear();
						str.remove_prefix(find_pos + 2);// 2 is "\n\n" and find_pos is character count of last data crunk
						if (str.starts_with("data: "))str.remove_prefix(6);
						find_pos = str.find("\n\n");
					}

					if (!str.empty() && str != "[DONE]")data->append(str);

					return size * nmemb;
				}
				static size_t write_function(char* ptr, size_t size, size_t nmemb, void* data) {
					static_cast<std::string*>(data)->append(ptr, size * nmemb);
					return size * nmemb;
				}
				static size_t header_writer(char* ptr, size_t size, size_t nitems, void* userdata) {
					static_cast<std::string*>(userdata)->append(ptr, size * nitems);
					return size * nitems;
				}
			public:
				CURL* handle;
				curl_handle_node() {
					is_used.store(false);
					curl_global_init(CURL_GLOBAL_ALL);
					handle = curl_easy_init();
					stream_callback = nullptr;
					http_header = nullptr;
				}

				inline void set_base_url(std::string_view url) const {
					curl_easy_setopt(handle, CURLOPT_URL, url.data());
				}
				inline void set_header(std::string_view content) {
					http_header = curl_slist_append(http_header, content.data());
				}

				response_t create(std::string_view body = {}) {
					/*set up body*/
					curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, body.length());
					curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.data());
					/*add header*/
					curl_easy_setopt(handle, CURLOPT_HTTPHEADER, http_header);

					curl_easy_setopt(handle, CURLOPT_POST, 1L);

					std::string raw_response;
					std::string header;
					curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &curl_handle_node::write_function);
					curl_easy_setopt(handle, CURLOPT_WRITEDATA, &raw_response);
					curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &curl_handle_node::header_writer);
					curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);

					CURLcode response = curl_easy_perform(handle);
					curl_slist_free_all(http_header);
					http_header = nullptr;
					response_t res{};
					if (response != CURLE_OK) {
						res.body["err_mes"] = std::string{ curl_easy_strerror(response) };
						res.body["http_header"] = header;
						res.error = true;
					} else {
						auto ctx = glz::read_json(res.body, raw_response);
						if (ctx) {
							res.error = true;
						} else {
							res.error = false;
						}
					}
					return res;
				}

				response_t get() {
					/*add header*/
					curl_easy_setopt(handle, CURLOPT_HTTPHEADER, http_header);

					std::string raw_response;
					std::string header;
					curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &curl_handle_node::write_function);
					curl_easy_setopt(handle, CURLOPT_WRITEDATA, &raw_response);
					curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &curl_handle_node::header_writer);
					curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);

					CURLcode response = curl_easy_perform(handle);
					curl_slist_free_all(http_header);
					http_header = nullptr;
					response_t res{};
					if (response != CURLE_OK) {
						res.body["err_mes"] = std::string{ curl_easy_strerror(response) };
						res.body["http_header"] = header;
						res.error = true;
					} else {
						auto ctx = glz::read_json(res.body, raw_response);
						if (ctx) {
							res.error = true;
						} else {
							res.error = false;
						}
					}
					return res;
				}

				response_t stream_create(std::string_view body = {}, std::function<void(glz::generic)> stream_callback = nullptr) {
					this->stream_callback = &stream_callback;
					/*set up body*/
					curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, body.length());
					curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.data());
					/*add header*/
					curl_easy_setopt(handle, CURLOPT_HTTPHEADER, http_header);

					std::string raw_response;
					std::string header;
					curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &curl_handle_node::stream_write_function);
					curl_easy_setopt(handle, CURLOPT_WRITEDATA, &raw_response);
					curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &curl_handle_node::header_writer);
					curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);

					CURLcode response = curl_easy_perform(handle);
					curl_slist_free_all(http_header);
					http_header = nullptr;
					this->stream_callback = nullptr;
					response_t res{};
					if (response != CURLE_OK) {
						res.body["err_mes"] = std::string{ curl_easy_strerror(response) };
						res.body["http_header"] = header;
						res.error = true;
					} else {
						res.error = false;
					}
					return res;
				}

				response_t stream_create(std::string_view body = {}, stream_write_func_t processor = nullptr) {
					/*set up body*/
					curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, body.length());
					curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.data());
					/*add header*/
					curl_easy_setopt(handle, CURLOPT_HTTPHEADER, http_header);

					std::string raw_response;
					std::string header;
					curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, processor);
					curl_easy_setopt(handle, CURLOPT_WRITEDATA, &raw_response);
					curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &curl_handle_node::header_writer);
					curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);

					CURLcode response = curl_easy_perform(handle);
					curl_slist_free_all(http_header);
					http_header = nullptr;
					response_t res{};
					if (response != CURLE_OK) {
						res.body["err_mes"] = std::string{ curl_easy_strerror(response) };
						res.body["http_header"] = header;
						res.error = true;
					} else {
						res.error = false;
					}
					return res;
				}

				~curl_handle_node() {
					while (is_used.load()) {
						std::this_thread::yield();
					}
					curl_easy_cleanup(handle);
					curl_global_cleanup();
				}
		};

		class session_t {
		private:
			std::vector<curl_handle_node> handles;
			using size_t = std::size_t;
		public:
			session_t(size_t concurrent_count) :handles{ concurrent_count } {}

			/**
			 * @brief get a useable handle managed by session object, it will block the progress if all handle are using
			 * @return the handle object
			 */
			curl_handle_node& get_useable_handle() {
				while (true) {
					for (curl_handle_node& handle : handles) {
						if (handle.is_used.exchange(true, std::memory_order_acq_rel))continue;
						return handle;
					}
					std::this_thread::yield();
				}
			}

			/**
			 * @brief release a handle, reset it into default state.
			 *
			 * if the handle is not belonged this session, it will return false and do nothing.
			 *
			 * if using a handle had been released, it may cause some fatal error.
			 * @param handle handle object
			 * @return true if handle belong to this session, false for otherwise
			 */
			bool release_handle(curl_handle_node& handle) {
				for (curl_handle_node& node : handles) {
					if (&node == &handle) {
						curl_easy_reset(handle.handle);
						handle.is_used.store(false, std::memory_order_release);
						return true;
					}
				}
				return false;
			}
		};
	}
}

#endif