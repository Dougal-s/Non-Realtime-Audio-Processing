#pragma once
#include <array>
#include <string>
#include <vector>
#include <filesystem>

#include "api.h"
#include "Dynamic_Library.hpp"

struct Port {

	enum Type {
		/**
		 * An audio port passes audio as an array of 32 bit floating point numbers
		 */
		audio,
		/**
		 * The parameter port is used to control the value of a plugin parameter
		 */
		parameter
	};

	enum Properties : int {
		none = 0,
		/**
		 * Only applicable to parameter ports
		 * The port passes the parameter values
		 * as an array with a value for each sample
		 */
		automatable = 1
	};

	std::string name;
	Port::Type type;
	Port::Properties properties = Port::Properties::none;

	// parameter specific options
	std::string units;
	float min = 0.f, max = 1.f;
	float default_value = 0.5;

	const float* value_arr;
	float value = 0.5f;
	bool automated = false;
};

inline Port::Properties operator&(Port::Properties lhs, Port::Properties rhs) {
	return static_cast<Port::Properties>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

inline Port::Properties operator|(Port::Properties lhs, Port::Properties rhs) {
	return static_cast<Port::Properties>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline Port::Properties operator|=(Port::Properties& lhs, Port::Properties rhs) {
	return lhs = lhs | rhs;
}

struct Plugin {

	enum Supports : int {
		none = 0,
		/**
		 * Allows output ports to point to the same location as input ports
		 */
		inplace = 0x00000001
	};

	std::filesystem::path path;

	// info file
	std::string name;
	std::array<unsigned int, 3> version;
	std::string description;
	std::string author;

	Plugin::Supports supports;

	std::filesystem::path binary;

	std::vector<Port> input_port_infos;
	std::vector<const float*> input_ports;
	std::vector<Port> output_port_infos;
	std::vector<float*> output_ports;

	// Plugin Binary

	Dynamic_Library plugin_library;
	Process_Function pfn_process;

	// Member Functions

	void parse_plugin_file(const std::filesystem::path& path);

	void set_parameter(const std::string& parameter_name, float new_value);

	void reset_parameter(const std::string& parameter_name);

	void set_automation(const std::string& parameter_name, const float* automation);

	void clear_automation(const std::string& parameter_name);

	void load_plugin();

	void run(size_t n_samples, double sample_rate);

	operator bool() const { return !path.empty(); };
};

inline Plugin::Supports operator|(Plugin::Supports lhs, Plugin::Supports rhs) {
	return static_cast<Plugin::Supports>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline Plugin::Supports operator|=(Plugin::Supports& lhs, Plugin::Supports rhs) {
	return lhs = lhs | rhs;
}
