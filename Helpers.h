#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <vcruntime_exception.h>

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}