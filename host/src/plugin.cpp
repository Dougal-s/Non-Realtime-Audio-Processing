#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <cctype>

#include <thread>
#include <chrono>

#include "plugin.hpp"

// reads the contents of a file and returns it as a string
static std::string read_file(const std::filesystem::path& path) {
	if (!std::filesystem::exists(path))
		throw std::runtime_error("attempted to read non-existant file: " + path.native());
	std::ifstream file(path, std::ios::in | std::ios::ate);

	size_t size = file.tellg();
	file.seekg(0);

	std::cout << path << "\n";
	std::string file_contents(size, ' ');
	file.read(file_contents.data(), size);

	file.close();

	return file_contents;
}

// returns the string with the leading and trailing whitespaces removed
static void trim(std::string& str) {
	// index of the first nonwhitespace character
	const size_t start = std::find_if_not(str.begin(), str.end(), ::isspace) - str.begin();
	// index of the last nonwhitespace character
	const size_t end = str.size() - (std::find_if_not(str.rbegin(), str.rend(), ::isspace) - str.rbegin());
	str = str.substr(start, end-start);
}

static size_t find_matching_bracket(const std::string& str, size_t start) {
	size_t matching_bracket = start;
	const char open_bracket = str[start];
	const char close_bracket = str[start]+2;
	int open_brackets = 1;
	while (open_brackets) {
		++matching_bracket;
		if (str.size() <= matching_bracket)
			throw std::runtime_error(std::string("Expected ") + close_bracket + " instead reached end of file!");

		if (str[matching_bracket] == open_bracket) ++open_brackets;
		else if (str[matching_bracket] == close_bracket) --open_brackets;
	}

	return matching_bracket;
}

static std::vector<std::string> parse_array(const std::string& str) {
	std::vector<std::string> array;
	size_t start = 1;
	size_t end = start;

	// check if it is an empty array
	while (std::isspace(str[start])) { ++start; }
	if (start == str.size()-1) return array;

	while (end != std::string::npos) {
		// set start to the first non-whitespace character
		while (std::isspace(str[start])) { ++start; }

		if (str[start] == '[' || str[start] == '{')
			end = find_matching_bracket(str, start);
		end = str.find(',', start);

		std::string element;
		if (end == std::string::npos) element = str.substr(start, str.size()-start-1);
		else element = str.substr(start, end-start);

		trim(element);
		if (element.empty()) throw std::runtime_error("Unexpected symbol ',' expected ']'!");
		array.push_back(element);

		start = end+1;
	}

	return array;
}

static std::map<std::string, std::string> parse_object(const std::string& str) {
	std::map<std::string, std::string> object;

	// find key value pairs and add them to object
	for (size_t line_start = 0; line_start < str.size();) {
		// find parameter name
		size_t colon = str.find(':', line_start);
		if (colon == std::string::npos)
			throw std::runtime_error("Expected a ':' instead reached end of file!");
		std::string param_name = str.substr(line_start, colon-line_start);
		trim(param_name);

		// find the start of the parameter value
		size_t param_value_start = colon+1;
		while (std::isspace(str[param_value_start])) { ++param_value_start; }

		size_t param_value_end = param_value_start;
		if (str[param_value_start] == '[' || str[param_value_start] == '{') {
			param_value_end = find_matching_bracket(str, param_value_start);
		}

		param_value_end = str.find(';', param_value_end);
		if (param_value_end == std::string::npos)
			throw std::runtime_error("Expected a ';' instead reached end of file!");

		std::string param_value = str.substr(param_value_start, param_value_end-param_value_start);
		trim(param_value);

		object.insert({param_name, param_value});

		line_start = param_value_end+1;
		while (std::isspace(str[line_start])) ++line_start;
	}

	return object;
}


static std::string parse_string(const std::string& str) {
	const size_t start = str.find('"') + 1; // character after the quote
	return str.substr(start, str.rfind('"')-start);
}

static std::array<unsigned int, 3> parse_version(const std::string& str) {
	char* processed = const_cast<char*>(str.data());
	unsigned int major = std::strtoul(processed, &processed, 10);
	++processed;
	unsigned int minor = std::strtoul(processed, &processed, 10);
	++processed;
	unsigned int build = std::strtoul(processed, &processed, 10);
	return {major, minor, build};
}

static Plugin::Supports parse_supports(const std::string& str) {
	Plugin::Supports features = Plugin::Supports::none;
	const auto feature_list = parse_array(str);
	for (const auto& feature : feature_list) {
		if (feature == "inplace") features |= Plugin::Supports::inplace;
	}
	return features;
}

static std::filesystem::path parse_binary(const std::string& str) {
	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		constexpr const char* platform = "windows";
	#elif __APPLE__
		constexpr const char* platform = "macos";
	#elif __linux__
		constexpr const char* platform = "linux";
	#else
		#error "Unkown compiler!"
	#endif
	size_t index = str.find(platform);
	if (index == std::string::npos)
		throw std::runtime_error("the selected plugin does not support the current system!");
	size_t start = str.find(':', index)+1;
	size_t end = str.find(';', start);
	return parse_string(str.substr(start, end-start));
}

static std::vector<Port> parse_ports(const std::string& str) {
	std::vector<Port> ports;
	std::vector<std::string> str_ports = parse_array(str);
	ports.resize(str_ports.size());
	for (const auto& str_port : str_ports) {
		const auto obj_port = parse_object(str_port.substr(1, str_port.size()-2));
		Port port;

		// get port name
		if (obj_port.find("name") != obj_port.end())
			port.name = parse_string(obj_port.find("name")->second);
		else
			throw std::runtime_error("MISSING INFO: 'name' field missing in port info!");

		// get port type
		if (obj_port.find("type") != obj_port.end()) {
			const std::string port_type = obj_port.find("type")->second;
			if (port_type == "audio") port.type = Port::Type::audio;
			else if (port_type == "parameter") port.type = Port::Type::parameter;
			else {
				std::cerr << "WARNING: Ignoring port: Unrecognized port type. This plugin may not work correctly!" << std::endl;
				continue;
			};
		} else {
			throw std::runtime_error("MISSING INFO: 'type' field missing in port info!");
		}

		// get port properties
		if (obj_port.find("properties") != obj_port.end()) {
			const std::vector<std::string> port_properties = parse_array(obj_port.find("properties")->second);
			for (const auto& property : port_properties)
				if (property == "automatable") port.properties |= Port::Properties::automatable;
		}

		// get port range
		if (obj_port.find("default") != obj_port.end())
			port.default_value = port.value = std::stof(obj_port.find("default")->second);
		if (obj_port.find("min") != obj_port.end())
			port.min = std::stof(obj_port.find("min")->second);
		if (obj_port.find("max") != obj_port.end())
			port.max = std::stof(obj_port.find("max")->second);

		// get port units
		if (obj_port.find("units") != obj_port.end())
			port.units = parse_string(obj_port.find("units")->second);


		// get port index
		size_t index;
		if (obj_port.find("port_index") != obj_port.end())
			index = std::stoul(obj_port.find("port_index")->second);
		else
			throw std::runtime_error("MISSING INFO: 'port_index' field missing!");

		ports[index] = port;
	}
	return ports;
}

void Plugin::parse_plugin_file(const std::filesystem::path& plugin_path) {
	path = plugin_path;
	const std::string info = read_file(path / "plugin.info");

	const auto parameters = parse_object(info);


	// set plugin name
	if (parameters.find("name") != parameters.end())
		name = parse_string(parameters.find("name")->second);
	else
		throw std::runtime_error("MISSING INFO: 'name' field missing!");

	// set plugin verison
	if (parameters.find("version") != parameters.end())
		version = parse_version(parameters.find("version")->second);
	else
		std::cerr << "WARNING: MISSING INFO: 'version' field missing!";

	// set plugin description
	if (parameters.find("description") != parameters.end())
		description = parse_string(parameters.find("description")->second);
	else
		std::cerr << "WARNING: MISSING INFO: 'description' field missing!";

	// set plugin author
	if (parameters.find("author") != parameters.end())
		author = parse_string(parameters.find("author")->second);
	else
		std::cerr << "WARNING: MISSING INFO: 'author' field missing!";

	// set plugin supports
	if (parameters.find("supports") != parameters.end())
		supports = parse_supports(parameters.find("supports")->second);
	else
		std::cerr << "WARNING: MISSING INFO: 'supports' field missing!";

	// set plugin binary
	if (parameters.find("binary") != parameters.end())
		binary = parse_binary(parameters.find("binary")->second);
	else
		throw std::runtime_error("MISSING INFO: 'binary' field missing!");

	// set plugin input ports
	if (parameters.find("input_ports") != parameters.end())
		input_port_infos = parse_ports(parameters.find("input_ports")->second);
	else
		throw std::runtime_error("WARNING: MISSING INFO: 'input_ports' field missing!");

	input_ports.resize(input_port_infos.size());
	for (size_t port = 0; port < input_port_infos.size(); ++port)
		if (input_port_infos[port].type == Port::Type::parameter)
			input_ports[port] = &input_port_infos[port].value;

	// set plugin output ports
	if (parameters.find("output_ports") != parameters.end())
		output_port_infos = parse_ports(parameters.find("output_ports")->second);
	else
		throw std::runtime_error("WARNING: MISSING INFO: 'output_ports' field missing!");

	output_ports.resize(output_port_infos.size());
}

void Plugin::set_parameter(const std::string& parameter_name, float new_value) {
	for (auto& port : input_port_infos) {
		if (port.type == Port::Type::parameter && port.name == parameter_name) {
			port.value = std::clamp(new_value, port.min, port.max);
			return;
		}
	}
	throw std::invalid_argument("'" + parameter_name + "' does not match any known parameters");
}

void Plugin::load_plugin() {
	plugin_library = Dynamic_Library(path / binary);
	pfn_process = reinterpret_cast<Process_Function>(plugin_library.get_function_address("process"));
}

void Plugin::run(size_t n_samples, double sample_rate) const {
	Global_Parameters params = {
		sample_rate,
		path.c_str()
	};
	(*pfn_process)(&params, input_ports.data(), output_ports.data(), n_samples);
}
