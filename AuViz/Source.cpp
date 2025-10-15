#include <iostream>
#include <iomanip>
#include <numeric>
#include <list>
#include <ranges>
#include "Render.h"
#include "WinAudioCapture.h"
#include "FFT.h"

#include <dwmapi.h>
#pragma comment (lib, "Dwmapi.lib")

int main() {
	uint32_t len = 0;
	std::vector<float> ndata = {};
	uint32_t bands = 256;
	int32_t tme = 0, atme = 0;
	std::vector<float> abands = {}, smbands = {}, svec = {};
	std::list<std::vector<float>> bnbr;
	auto wn = avRender::init(bands / 4);
	float tot = 0;
	float maxval = 0;
	float amax = 0;
	float smooth = 300;

	smbands.assign(bands, 0);
	abands.assign(bands, 0);

	sf::Clock clk;
	int idx = 0;

	winAC::initAC();

#if defined(_WIN64) || defined(__CYGWIN)
	MARGINS mrgn;
	mrgn.cxLeftWidth = -1;
	SetWindowLongPtr(wn->getNativeHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
	DwmExtendFrameIntoClientArea(wn->getNativeHandle(), &mrgn);
	SetWindowLongPtr(avRender::settingswin->getNativeHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
	DwmExtendFrameIntoClientArea(avRender::settingswin->getNativeHandle(), &mrgn);
#endif

	while (!avRender::close) {
		
		abands.reserve(bands);
		winAC::getData(len, ndata);
		
		idx = 0;
		if (ndata.size()) {
			auto data = avfft::getSpectrum(ndata, bands);
			for (auto& v : data) {
				maxval = std::abs(*std::max_element(v.begin(), v.end(), [](std::complex<float>& a, std::complex<float>& b) {
					return std::abs(a) < std::abs(b);
					}));
				abands.push_back(std::accumulate(v.begin(), v.end(), 0.f, [](float a, std::complex<float>& b) {
					return a + std::abs(b);
					}) * bands * ++idx / v.size());
			}
		}
		else {
			std::for_each(smbands.begin(), smbands.end(), [](float k) {return k -= 0.1f;});
			avRender::render(tan(M_PI / 3), clk.restart().asMilliseconds(), smbands);
			abands.clear();
			ndata.clear();
			continue;
		}
		if (bnbr.size() > smooth) {
			for (unsigned int i = 0; i < bands; ++i)
				smbands[i] -= bnbr.back()[i] / smooth;
			bnbr.erase(bnbr.begin());
			for (unsigned int l = 0; l < bands; ++l)
				smbands[l] += abands[l] / smooth;
		}
		bnbr.emplace_back(abands);
		svec = std::vector<float>(smbands.begin(), smbands.begin() + smbands.size());
		tme = clk.restart().asMilliseconds();
		atme += tme;
		if (atme / 2.f >= 800 || amax < maxval) {
			amax = maxval;
			maxval = 0;
			atme = 0;
		}

		for (float& a : svec) {
			if (a > maxval) 
				maxval = a;
			a /= amax > 0 ? amax : 1;
			a *= 2;
		}

		avRender::render(tan(M_PI / 3.f), tme, svec);
		abands.clear();
		ndata.clear();
	}
	winAC::stop();
	avRender::exit();
}