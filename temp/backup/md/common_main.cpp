#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#else
#include <unistd.h>
#endif

static void CleanupResources() {
    try {
#ifdef _WIN32
		// TODO DEL
        timeEndPeriod(1);
#endif
        printf("Common api dll cleanup.\n");
    } catch (...) {
        // 防止清理过程中再次崩溃
    }
}

static void InitializeLibaray() {
#ifdef _WIN32
	// TODO DEL
    timeBeginPeriod(1);
    printf("Common api dll loaded.\n");
#endif
}

// Windows 平台下的 DLL 入口点
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        InitializeLibaray();
        break;
    case DLL_PROCESS_DETACH:
        CleanupResources();
        break;
    default:
        break;
    }
    return TRUE;
}
// Linux 平台下的库初始化和清理
#elif __linux__
__attribute__((constructor))
static void library_init() {
    // 库加载时执行
    InitializeLibaray();
}

__attribute__((destructor))
static void library_fini() {
    CleanupResources();
}
#endif


