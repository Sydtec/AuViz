#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define _ALLOCA_S_THRESHOLD 1000
#include <objbase.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <combaseapi.h>
#include <thread>
#include <vector>
#include <iostream>

#define REFTIMES_PER_SEC  1000

namespace winAC {
	extern uint32_t psize;

	void initAC();
	void exitAC();
	void stop();
	void exitOnError(HRESULT r);
	void getData(uint32_t &len, std::vector<float> &audioBuf);
	template<class... Ts>
	void safeRelease(Ts...);
	template<class D>
	void freeAC(D);
}