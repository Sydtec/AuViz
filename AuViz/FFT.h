#pragma once
#include <stdint.h>
#include <algorithm>
#include <memory>
#include <vector>
#include <bit>
#include <iostream>
#include "dj_fft.h"

namespace avfft{
	void init(uint32_t mlen);
	std::vector<dj::fft_arg<float>> getSpectrum(std::vector<float>, uint32_t bands = 5);
};