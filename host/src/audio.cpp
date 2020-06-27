#include <algorithm>
#include <fstream>

#include "audio.hpp"

struct Wav_Header {
	// riff header
	char chunk_id[4] = {'R', 'I', 'F', 'F'};
	uint32_t chunk_size;
	char wave_id[4] = {'W', 'A', 'V', 'E'};
	// fmt
	char subchunk_1_id[4] = {'f', 'm', 't', ' '};
	uint32_t subchunk_1_size = 16;
	uint16_t audio_format = 3;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample = 32;
	// fact
	char subchunk_2_id[4] = {'f', 'a', 'c', 't'};
	uint32_t subchunk_2_size = 4;
	uint32_t sample_length;
	// data
	char subchunk_3_id[4] = {'d', 'a', 't', 'a'};
	uint32_t subchunk_3_size;
};

static void write_wav(const std::filesystem::path& path, const std::vector<std::vector<float>>& audio, const Audio_Info& info) {
	std::ofstream wav_file(path, std::ios::out | std::ios::binary);

	Wav_Header header;
	header.num_channels = audio.size();
	header.sample_rate = info.sample_rate;
	header.byte_rate = header.sample_rate*header.num_channels*sizeof(float);
	header.block_align = header.num_channels*sizeof(float);
	header.bits_per_sample = 8*sizeof(float);

	header.sample_length = audio.front().size()*header.num_channels;

	header.subchunk_3_size = header.sample_length*sizeof(float);

	header.chunk_size = 4 +
	                    8 + header.subchunk_1_size +
	                    8 + header.subchunk_2_size +
	                    8 + header.subchunk_3_size +
	                    (header.subchunk_3_size & 1);

	wav_file.write(reinterpret_cast<char*>(&header), sizeof(header));

	std::vector<float> interleaved_audio;
	interleaved_audio.reserve(header.sample_length);
	for (size_t i = 0; i < header.sample_length/header.num_channels; ++i)
		for (const auto& channel : audio)
			interleaved_audio.push_back(channel[i]);

	wav_file.write(reinterpret_cast<char*>(interleaved_audio.data()), interleaved_audio.size()*sizeof(float));

	wav_file.close();
}

struct Fmt_Chunk {
	char chunk_id[4];
	uint32_t chunk_size;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
};

static std::vector<std::vector<float>> read_wav(const std::filesystem::path& path, Audio_Info& info) {
	std::ifstream wav_file(path, std::ios::in | std::ios::binary);

	char riff_header[8];
	wav_file.read(riff_header, sizeof(riff_header));
	if (riff_header[0] != 'R' || riff_header[1] != 'I' || riff_header[2] != 'F' || riff_header[3] != 'F')
		throw std::runtime_error("input file is not a valid wav file: incorrect chunk id");

	uint32_t chunk_size = *reinterpret_cast<uint32_t*>(riff_header+4);
	char* data = new char[chunk_size];
	wav_file.read(data, chunk_size);
	wav_file.close();

	if (data[0] != 'W' || data[1] != 'A' || data[2] != 'V' || data[3] != 'E')
		throw std::runtime_error("input file is not a valid wav file: incorrect wave id");


	// read fmt chunk
	const char* fmt_chunk_id = "fmt ";

	char* p_fmt_chunk = std::search(data+4, data+chunk_size, fmt_chunk_id, fmt_chunk_id+4);
	if (p_fmt_chunk == data+chunk_size)
		throw std::runtime_error("wav file is missing fmt chunk");

	Fmt_Chunk fmt_chunk = *reinterpret_cast<Fmt_Chunk*>(p_fmt_chunk);
	if ((fmt_chunk.audio_format != 1 || fmt_chunk.bits_per_sample != 32)
		&& !(fmt_chunk.audio_format != 3 || fmt_chunk.bits_per_sample != 16))
		throw std::runtime_error("only wav files with 32 bit IEEE floating point or 16 bit pcm data are supported!");

	info.sample_rate = fmt_chunk.sample_rate;

	std::vector<std::vector<float>> audio(fmt_chunk.num_channels);

	// get audio data
	const char* data_chunk_id = "data";
	char* data_chunk = std::search(p_fmt_chunk+fmt_chunk.chunk_size, data+chunk_size, data_chunk_id, data_chunk_id+4);

	uint32_t data_chunk_size = *reinterpret_cast<uint32_t*>(data_chunk+4);

	for (auto& channel : audio)
		channel.reserve(8*data_chunk_size/(fmt_chunk.num_channels*fmt_chunk.bits_per_sample));

	switch (fmt_chunk.audio_format) {
		case 1: {
			int16_t* audio_data = reinterpret_cast<int16_t*>(data_chunk+8);

			for (size_t sample = 0; sample < 8*data_chunk_size/fmt_chunk.bits_per_sample; sample += fmt_chunk.num_channels)
				for (size_t channel = 0; channel < fmt_chunk.num_channels; ++channel)
					audio[channel].push_back(static_cast<float>(audio_data[sample + channel])/32768.f);
			break;
		}
		case 3: {
			float* audio_data = reinterpret_cast<float*>(data_chunk+8);

			for (size_t sample = 0; sample < 8*data_chunk_size/fmt_chunk.bits_per_sample; sample += fmt_chunk.num_channels)
				for (size_t channel = 0; channel < fmt_chunk.num_channels; ++channel)
					audio[channel].push_back(audio_data[sample + channel]);
			break;
		}
	}

	delete[] data;

	return audio;
}

std::vector<std::vector<float>> read_audio_file(const std::filesystem::path& path, Audio_Info& info) {
	if (path.extension() == ".wav") return read_wav(path, info);
	else throw std::invalid_argument("input file type is not supported!");
}

void write_audio_file(const std::filesystem::path& path, const std::vector<std::vector<float>>& data, const Audio_Info& info) {
	if (path.extension() == ".wav") write_wav(path, data, info);
	else throw std::invalid_argument("output file type is not supported!");
}
