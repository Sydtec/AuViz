#include "FFT.h"
#define DJ_FFT_IMPLEMENTATION

namespace avfft {
	void init(uint32_t mlen) {
	}
	std::vector<dj::fft_arg<float>> getSpectrum(std::vector<float> in, uint32_t bands) {
		dj::fft_arg<float> data(in.begin(), in.end());
		data.resize(1 << (32 - std::countl_zero(data.size() - 1)));
		auto tfm = dj::fft1d(data, dj::fft_dir::DIR_FWD);
		std::vector<dj::fft_arg<float>> _out;
		_out.reserve(tfm.size() * bands);
		for (int i = 0; i < bands; ++i) {
			auto g = dj::fft_arg<float>(
				tfm.begin() + i * tfm.size() / bands,
				tfm.begin() + (i + 1) * tfm.size() / bands);
			_out.push_back(dj::fft1d(g, dj::fft_dir::DIR_BWD));
		}
		return _out;
	}
}