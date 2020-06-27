#ifndef API_H
#define API_H

typedef struct _Global_Parameters {
	double sample_rate;
	const char* plugin_path;
} Global_Parameters;

typedef void (*Process_Function)(const Global_Parameters* global,
				                 const float* const* input_ports,
                                 float* const* output_ports,
                                 size_t n_samples);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#define SYMBOL_EXPORT extern "C" __declspec(dllexport)
#else
	#ifdef __GNUC__
		#define SYMBOL_EXPORT extern "C" __attribute__ ((visibility("default")))
	#else
		#define SYMBOL_EXPORT extern "C"
	#endif
#endif


#endif
