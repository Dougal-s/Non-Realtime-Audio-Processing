#include <stdexcept>
#include <utility>

#include "Dynamic_Library.hpp"

Dynamic_Library::Dynamic_Library() {}

Dynamic_Library::Dynamic_Library(Dynamic_Library&& other) noexcept {
	std::swap(m_handle, other.m_handle);
	std::swap(m_initialized, other.m_initialized);
}

Dynamic_Library::Dynamic_Library(const std::filesystem::path& path) : m_initialized(true) {
	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		m_handle = LoadLibrary(path.c_str());
		if (!m_handle) throw std::runtime_error(GetLastError());
	#elif __APPLE__ || __linux__
		m_handle = dlopen(path.c_str(), RTLD_LAZY);
		if (!m_handle) throw std::runtime_error(dlerror());
	#endif
}

Dynamic_Library::~Dynamic_Library() {
	if (m_initialized) {
		#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			FreeLibrary(m_handle);
		#elif __APPLE__ || __linux__
			dlclose(m_handle);
		#endif
	}
}

Dynamic_Library& Dynamic_Library::operator=(Dynamic_Library&& other) noexcept {
	std::swap(m_handle, other.m_handle);
	std::swap(m_initialized, other.m_initialized);
	return *this;
}

void* Dynamic_Library::get_function_address(const char* symbol) {
	if (!m_initialized)
		throw std::logic_error("attempted to get function pointer from an uninitialized dynamic library");


	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		auto function_ptr = GetProcAddress(m_handle, symbol);
		if (!function_ptr) throw std::runtime_error(GetLastError());
	#elif __APPLE__ || __linux__
		dlerror(); // clear any existing errors
		auto function_ptr = dlsym(m_handle, symbol);
		char* err = dlerror();
		if (err != NULL) throw std::runtime_error(err);
	#endif
	return function_ptr;
}

Dynamic_Library::native_handle_type Dynamic_Library::native_handle() { return m_handle; }


Dynamic_Library::operator bool() const noexcept { return m_initialized; }
