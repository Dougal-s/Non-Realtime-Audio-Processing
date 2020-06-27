#pragma once

#include <vector>
#include <filesystem>

struct Audio_Info {
	double sample_rate;
};

std::vector<std::vector<float>> read_audio_file(const std::filesystem::path& path, Audio_Info& info);
void write_audio_file(const std::filesystem::path& path, const std::vector<std::vector<float>>& data, const Audio_Info& info);
