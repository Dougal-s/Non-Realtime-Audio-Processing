#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <filesystem>
#include <future>

#include "plugin.hpp"
#include "audio.hpp"

constexpr const char* version_str =
"Audio Thing v0.1.0\n"
"Written By Dougal Stewart\n";

static std::map<std::string, std::string> parse_cmd_line_args(int argc, const char* argv[]) {
	std::map<std::string, std::string> args;

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			std::string argument = argv[i];
			size_t index = argument.find('=');

			std::string value;
			if (index != std::string::npos) {
				argument.resize(index);
				value = argv[i]+argument.size()+1; //
			}

			if (argument[1] != '-') {
				const std::unordered_map<std::string, std::string> arg_map = {
					{"-h", "--help"},
					{"-v", "--version"},
					{"-p", "--plugin"},
					{"-i", "--info"},
					{"-l", "--pad"}
				};

				if (auto it = arg_map.find(argument); it != arg_map.end()) argument = it->second;
				else throw std::invalid_argument(std::string("Unrecognized command line argument: ") + argument);
			}

			// command line options which do not require a value
			const std::unordered_set<std::string> flags = {
				"--help",
				"--info"
			};

			if (value.empty() && flags.find(argument) == flags.end()) {
				++i;
				if (i < argc) value = argv[i];
			}

			args.insert({argument, value});
		} else {
			if (auto it = args.find("--input"); it == args.end()) args.insert({"--input", argv[i]});
			else if (auto it = args.find("--output"); it == args.end()) args.insert({"--output", argv[i]});
			else throw std::invalid_argument(std::string("Invalid command line argument: ") + argv[i]);
		}
	}

	return args;
}

int main(int argc, const char* argv[]) {

	std::map<std::string, std::string> flags = parse_cmd_line_args(argc, argv);

	if (flags.find("--help") != flags.end()) {
		std::cout << "Usage: " << argv[0] << " [OPTION]... input_file output_file\n"
		          << "Options:\n"
		          << "  -h, --help                    Prints this help string\n"
		          << "  -v, --version                 Prints version information\n"
		          << "  -p, --plugin=PLUGIN_PATH      Path to the plugin\n"
		          << "  -i, --info                    Prints information about the\n"
		          << "                                  selected plugin then exits\n"
		          << "  -l, --pad=PADDING_LENGTH      Pads the input audio with PADDING_LENGTH samples\n"
				  << "                                  Negative values will reduce the number of samples\n"
		          << "      --PARAM_NAME=PARAM_VALUE  Sets the plugin parameter\n"
		          << "                                  PARAM_NAME to PARAM_VALUE\n"
		          << version_str;
		return 0;
	}

	if (flags.find("--version") != flags.end()) {
		std::cout << version_str;
		return 0;
	}

	int padding = 0;
	if (flags.find("--pad") != flags.end())
		padding = std::stoi(flags.extract("--pad").mapped());

	Plugin plugin;
	if (flags.find("--plugin") != flags.end())
		plugin.parse_plugin_file(flags.extract("--plugin").mapped());
	else
		std::cout << "Warning: plugin flag missing\n";

	if (flags.find("--info") != flags.end()) {
		if (!plugin)
			throw std::invalid_argument("--info flags specified without a plugin!");

		std::cout << "Plugin: " << plugin.path << "\n"
		          << "name: " << plugin.name << "\n"
		          << "version: v"
		          	<< plugin.version[0] << '.'
		          	<< plugin.version[1] << '.'
		          	<< plugin.version[2] << "\n"
		          << "description: " << plugin.description << "\n"
		          << "author: " << plugin.author << "\n"
		          << "\n"
		          << "Parameters: \n";

		for (const auto& port : plugin.input_port_infos) {
			if (port.type == Port::Type::parameter) {
				std::cout << "  " << port.name << ": \n"
				          << "    properties: "
				          	<< (port.properties & Port::Properties::automatable ? "automatable " : "")
				          << "\n"
				          << "    min: " << port.min << port.units << "\n"
				          << "    max: " << port.max << port.units << "\n"
				          << "    default: " << port.value << port.units << "\n";
			}
		}

		std::cout << "\n"
		          << "Audio:\n"
		          << "  Input:\n";

		for (const auto& port : plugin.input_port_infos)
			if (port.type == Port::Type::audio)
				std::cout << "    " << port.name << "\n";
		std::cout << "  Output:\n";

		for (const auto& port : plugin.output_port_infos)
			if (port.type == Port::Type::audio)
				std::cout << "    " << port.name << "\n";

		return 0;
	}


	std::filesystem::path input_file, output_file;
	if (flags.find("--input") != flags.end())
		input_file = flags.extract("--input").mapped();
	else
		throw std::invalid_argument("input file not specified!");

	if (flags.find("--output") != flags.end())
		output_file = flags.extract("--output").mapped();
	else
		throw std::invalid_argument("output file not specified!");

	for (const auto& arg : flags)
		plugin.set_parameter(arg.first.substr(2), std::stof(arg.second));


	if (plugin) plugin.load_plugin();

	// read audio file
	std::cout << "reading audio from " << input_file << std::endl;
	Audio_Info info;
	auto input_audio = read_audio_file(input_file, info);

	// connect input ports
	{
		// find the number of input audio ports
		if (plugin) {
			size_t input_port_count = 0;
			for (const auto& port : plugin.input_port_infos)
				if (port.type == Port::Type::audio) ++input_port_count;

			input_audio.resize(input_port_count);
		}

		const size_t original_size = input_audio.front().size();
		// add padding
		for (auto& channel : input_audio)
			channel.resize(original_size+padding, 0.f);

		// connect ports
		if (plugin) {
			size_t connected_ports = 0;
			for (size_t port = 0; port < plugin.input_ports.size(); ++port) {
				if (plugin.input_port_infos[port].type == Port::Type::audio) {
					plugin.input_ports[port] = input_audio[connected_ports].data();
					++connected_ports;
				}
			}
		}
	}

	std::vector<std::vector<float>> output_audio;

	// connect output ports
	{
		// find the number of output audio ports
		size_t output_port_count = 0;
		if (plugin) {
			for (const auto& port : plugin.output_port_infos)
				if (port.type == Port::Type::audio) ++output_port_count;
		} else {
			output_port_count = input_audio.size();
		}

		output_audio.resize(output_port_count);

		// resize output audio
		for (auto& channel : output_audio)
			channel.resize(input_audio.front().size());

		// connect ports
		if (plugin) {
			size_t connected_ports = 0;
			for (size_t port = 0; port < plugin.output_ports.size(); ++port) {
				if (plugin.output_port_infos[port].type == Port::Type::audio) {
					plugin.output_ports[port] = output_audio[connected_ports].data();
					++connected_ports;
				}
			}
		}
	}

	if (plugin) {
		auto future_obj = std::async(std::launch::async, &Plugin::run, &plugin, input_audio.front().size(), info.sample_rate);
		size_t state = 0;
		std::cout << "Running plugin  ";
		do {
			std::cout << '\b';
			switch (state) {
				case 0:
					std::cout << '|';
					break;
				case 1:
					std::cout << '/';
					break;
				case 2:
					std::cout << '-';
					break;
				case 3:
					std::cout << '\\';
					break;
			}
			std::cout << std::flush;
			state = (state+1)%4;
		} while (future_obj.wait_for(std::chrono::milliseconds(250)) != std::future_status::ready);
		std::cout << std::endl;
	} else {
		output_audio = std::move(input_audio);
	}

	std::cout << "writing output to " << output_file << std::endl;
	write_audio_file(output_file, output_audio, info);

	return 0;
}
