# ===================================================================
# Microsoft WebView2 Loader Library Configuration (DLL 动态链接)
# DLL Version: Use WebView2Loader.dll.lib (import lib) + copy WebView2Loader.dll
# Runtime Note: Also needs Microsoft.WebView2.Core.dll (system or manual deploy)
# Author: leiwei E-mail: ctrlfrmb@gmail.com
# Copyright (c) 2025. All rights reserved.
# ===================================================================

WEBVIEW2_ROOT = $$PWD

# 1. 头文件路径
INCLUDEPATH += $$WEBVIEW2_ROOT/include

# 2. 架构判定 (x64/x86 目录)
contains(QT_ARCH, x86_64) {
    ARCH_DIR = x64
} else {
    ARCH_DIR = x86
}

# 3. 库/DLL 文件路径配置
WEBVIEW2_LIB_DIR = $$WEBVIEW2_ROOT/$$ARCH_DIR
WEBVIEW2_BIN_DIR = $$WEBVIEW2_ROOT/$$ARCH_DIR

# 4. 链接库 - 使用完整库文件名（关键修改）
# 方法一：直接指定完整的 .lib 文件路径
LIBS += $$WEBVIEW2_LIB_DIR/WebView2Loader.dll.lib

# 或者 方法二：使用静态库
# LIBS += $$WEBVIEW2_LIB_DIR/WebView2LoaderStatic.lib

# Windows COM 系统依赖库 (WebView2 必需)
win32: LIBS += -lole32 -loleaut32 -luser32

# 5. 定义拷贝函数，用于将 WebView2Loader.dll 拷贝到构建目录
defineReplace(copyWebView2Dll) {
    src = $$1
    dst = $$2
    return(copy /Y $$shell_quote($$shell_path($$src)) $$shell_quote($$shell_path($$dst)))
}

# 自动将 WebView2Loader.dll 拷贝到生成目录
WEBVIEW2_DLL_PATH = $$WEBVIEW2_BIN_DIR/WebView2Loader.dll
exists($$WEBVIEW2_DLL_PATH) {
    QMAKE_POST_LINK += $$copyWebView2Dll($$WEBVIEW2_DLL_PATH, $$DESTDIR) $$escape_expand(\\n\\t)
    message(WebView2: Auto-copying DLL from $$WEBVIEW2_DLL_PATH to $$DESTDIR)
} else {
    warning(WebView2 DLL not found: $$WEBVIEW2_DLL_PATH - Check path!)
}
