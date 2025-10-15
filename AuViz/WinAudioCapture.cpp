#include "WinAudioCapture.h"

namespace winAC {
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	const IID IID_IAudioClient = __uuidof(IAudioClient);
	const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

	uint8_t frameSize; //size of a frame in bytes
	uint32_t psize = 1000; //possible size of any buffer

	BYTE* data = NULL;

	IMMDevice* device = NULL;
	IAudioClient* client = NULL;
	IAudioCaptureClient* captureClient = NULL;
	WAVEFORMATEX* format = NULL;
	uint32_t bfc = 0;
	uint32_t* bufferFrameCount = &bfc;
	uint32_t packetLength = 0;
	uint32_t availableFrames = 0;
	DWORD flags;
	IMMDeviceEnumerator* Ntor = NULL;

	bool onHeap = false;
}

void winAC::initAC() {
	HRESULT result = CoInitialize(NULL);

	exitOnError(result);

	result = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&Ntor);

	if (result != S_OK) {
		printf("Unable to create an instance of IMMDeviceEnumerator: %i", result);
		switch (result) {
		case E_POINTER:
			std::cout << "e_pointer";
			return;
		case REGDB_E_CLASSNOTREG:
			std::cout << "classnotreg";
			return;
		case CLASS_E_NOAGGREGATION:
			std::cout << "noaggregation";
			return;
		case E_NOINTERFACE:
			std::cout << "nointerface";
			return;
		default:
			break;
		}
		exitAC();
		CoUninitialize();
		exit(-1);
	}

	result = Ntor->GetDefaultAudioEndpoint(eRender, eMultimedia , &device);
	if (result != S_OK) {
		printf("Unable to get a default audio endpoint: %i", result);
		exitAC();
		CoUninitialize();
		exit(-1);
	}

	result = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&client);
	if (result != S_OK) {
		printf("Unable to activate audio client: %i", result);
		exitAC();
		CoUninitialize();
		exit(-1);
	}

	result = client->GetMixFormat(&format);
	if (result != S_OK) {
		printf("unable to get mix format from client: %i", result);
		exitAC();
		CoUninitialize();
		exit(-1);
	}

	result = client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, REFTIMES_PER_SEC, 0, format, NULL);
	if (result != S_OK) {
		printf("Unable to initialize client: %i", result);
		exitAC();
		CoUninitialize();
		exit(-1);
	}

	result = client->GetBufferSize(bufferFrameCount);
	if (result != S_OK) {
		printf("Failed to get buffer size: %i", result);
		exitAC();
		CoUninitialize();
		exit(-1);
	}

	result = client->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
	if (result != S_OK) {
		printf("Unable to get service from client: %i", result);
		exitAC();
		CoUninitialize();
		exit(-1);
	}

	result = client->Start();
	if (result != S_OK) {
		printf("Unable to start client: %i", result);
		exitAC();
		CoUninitialize();
		exit(-1);
	}

	frameSize = format->nChannels * format->wBitsPerSample / 8;
}


void winAC::getData(uint32_t& len, std::vector<float> &audioBuf) {
	HRESULT res;
	float* fbuf;
	len = 0;
	res = captureClient->GetNextPacketSize(&packetLength);
	onHeap = packetLength * frameSize > _ALLOCA_S_THRESHOLD;
	exitOnError(res);
	uint32_t mlen = 0;
	mlen = 0;
	while (packetLength != 0) {
		res = captureClient->GetBuffer(&data, &availableFrames, &flags, NULL, NULL);
		if (availableFrames * frameSize == 0)
			continue;
		exitOnError(res);
		fbuf = (float*)data;
		audioBuf.insert(audioBuf.end(), fbuf, fbuf + format->nChannels * availableFrames);
		res = captureClient->ReleaseBuffer(availableFrames);
		exitOnError(res);

		res = captureClient->GetNextPacketSize(&packetLength);
		exitOnError(res);
	}
	len = mlen;
}

void winAC::stop() {
	client->Stop();
	exitAC();
}

void winAC::exitOnError(HRESULT r) {
	if (FAILED(r)) {
		std::cout << "AC failed";
		exitAC();
	}
}

void winAC::exitAC() {
	CoTaskMemFree(format);
	safeRelease(Ntor, device, client, captureClient);
}

template<class... Ts>
void winAC::safeRelease(Ts... args) {
	(freeAC(args), ...);
}

template<class D>
void winAC::freeAC(D arg) {
	if ((arg) != NULL) {
		(arg)->Release();
		(arg) = NULL;
	}
}