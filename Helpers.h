#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <vcruntime_exception.h>

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// The number of swap chain back buffers.
const uint8_t g_numFrames = 3;

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}