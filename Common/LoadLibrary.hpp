#pragma once

#include "define.h"

#if PSS_PLATFORM == PLATFORM_WIN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifndef RTLD_NOW
#define RTLD_NOW 2
#endif

//实现不同OS的动态库加载
//add by freeeyes

#if PSS_PLATFORM == PLATFORM_WIN
using Pss_Library_Handler = HINSTANCE;
#else
using Pss_Library_Handler = void*;
#endif

class CLoadLibrary
{
public:
    static Pss_Library_Handler PSS_dlopen(const char* pFilePath, const int nMode)
    {
#if PSS_PLATFORM == PLATFORM_WIN
        PSS_UNUSED_ARG(nMode);

        //Unicode字符集转换
        WCHAR wszFilePath[512] = {'\0'};
        memset(wszFilePath, 0, sizeof(wszFilePath));
        MultiByteToWideChar(CP_ACP, 0, pFilePath, (int)strlen(pFilePath) + 1, wszFilePath,
            (int)(sizeof(wszFilePath) / sizeof(wszFilePath[0])));

        return ::LoadLibraryW(wszFilePath);
#else
        return dlopen(pFilePath, nMode);
#endif
    }

    static void* PSS_dlsym(Pss_Library_Handler h, const char* pFuncName)
    {
#if PSS_PLATFORM == PLATFORM_WIN
        return ::GetProcAddress(h, pFuncName);
#else
        return dlsym(h, pFuncName);
#endif
    }

    static char* PSS_dlerror()
    {
#if PSS_PLATFORM == PLATFORM_WIN
        static char buf[128] = { '\0' };
        ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
            0,
            ::GetLastError(),
            0,
            buf,
            sizeof buf / sizeof buf[0],
            0);

        return buf;
#else
        return dlerror();
#endif
    }

    static void PSS_dlClose(Pss_Library_Handler h)
    {
#if PSS_PLATFORM == PLATFORM_WIN
        ::FreeLibrary(h);
#else
        dlclose(h);
#endif
    }
};
