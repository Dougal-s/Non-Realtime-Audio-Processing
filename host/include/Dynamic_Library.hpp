#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#include "windows.h"
#elif __APPLE__ || __linux__
	#include <dlfcn.h>
#else
	#error "Unkown compiler!"
#endif

#include <filesystem>

class Dynamic_Library {
public:
	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		using native_handle_type = HINSTANCE;
	#elif __APPLE__ || __linux__
		using native_handle_type = void*;
	#endif

	Dynamic_Library();
	Dynamic_Library(Dynamic_Library&& other) noexcept;
	explicit Dynamic_Library(const std::filesystem::path& path);
	Dynamic_Library(const Dynamic_Library& other) = delete;

	~Dynamic_Library();

	Dynamic_Library& operator=(Dynamic_Library&& other) noexcept;

	void* get_function_address(const char* symbol);

	native_handle_type native_handle();

	explicit operator bool() const noexcept;

private:
	native_handle_type m_handle = nullptr;
	bool m_initialized = false;
};
