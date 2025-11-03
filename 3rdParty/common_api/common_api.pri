# common_api.pri - 通用API库配置文件
# 使用方法:
#   include(../3rdParty/common_api/common_api.pri)               # 默认使用 common_api
#   或者在包含前定义: DEFINES += USE_COMMON_UI 然后再 include        # 使用 common_ui
#
# Author: leiwei E-mail: ctrlfrmb@gmail.com
# Version: 3.0.0

# 确保 UTF-8 支持
CONFIG += utf8_source
CODECFORTR = UTF-8

# 库根目录
COMMON_API_LIB_ROOT = $$PWD

# 检测用户是否需要 UI 支持
contains(DEFINES, USE_COMMON_UI) {
    COMMON_API_LIB_NAME = common_ui
    COMMON_API_LIB_TYPE = "Common UI Library"
} else {
    COMMON_API_LIB_NAME = common_api
    COMMON_API_LIB_TYPE = "Common API Library"
}

# 头文件路径
INCLUDEPATH += $$COMMON_API_LIB_ROOT/include
INCLUDEPATH += $$COMMON_API_LIB_ROOT/../

# 包含第三方库（如果存在）
exists($$COMMON_API_LIB_ROOT/../fmt-12.0.0/format.pri) {
    include($$COMMON_API_LIB_ROOT/../fmt-12.0.0/format.pri)
}

# 平台检测
win32 {
    PLATFORM_DIR = windows
} else:unix:!macx {
    PLATFORM_DIR = linux
} else:macx {
    PLATFORM_DIR = macos
} else {
    error("Unsupported platform: $$QMAKE_HOST.os")
}

# 架构检测
contains(QT_ARCH, x86_64) {
    ARCH_DIR = x64
} else:contains(QT_ARCH, i386) {
    ARCH_DIR = x86
} else {
    ARCH_DIR = x64
    warning("Unknown architecture $$QT_ARCH, defaulting to x64")
}

# 构建类型检测
CONFIG(debug, debug|release) {
    BUILD_DIR = debug
    LIB_SUFFIX = d
} else {
    BUILD_DIR = release
    LIB_SUFFIX =
}

# 设置库和二进制文件路径
COMMON_API_LIB_DIR = $$COMMON_API_LIB_ROOT/lib/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR
COMMON_API_BIN_DIR = $$COMMON_API_LIB_ROOT/bin/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR

# 验证库文件是否存在
win32 {
    LIB_FILE = $$COMMON_API_LIB_DIR/$${COMMON_API_LIB_NAME}$${LIB_SUFFIX}.lib
    DLL_FILE = $$COMMON_API_BIN_DIR/$${COMMON_API_LIB_NAME}$${LIB_SUFFIX}.dll
} else {
    LIB_FILE = $$COMMON_API_LIB_DIR/lib$${COMMON_API_LIB_NAME}$${LIB_SUFFIX}.so
    DLL_FILE = $$LIB_FILE
}

!exists($$LIB_FILE) {
    error("Library file not found: $$LIB_FILE")
}

win32:!exists($$DLL_FILE) {
    error("DLL file not found: $$DLL_FILE")
}

# 添加库路径并链接库
LIBS += -L$$COMMON_API_LIB_DIR
LIBS += -l$${COMMON_API_LIB_NAME}$${LIB_SUFFIX}

# 运行时库复制（Windows）
win32 {
    isEmpty(DESTDIR) {
        TARGET_DIR = $$OUT_PWD
    } else {
        TARGET_DIR = $$DESTDIR
    }

    DLL_SOURCE = $$shell_path($$DLL_FILE)
    DLL_TARGET = $$shell_path($$TARGET_DIR)

    QMAKE_POST_LINK += copy /Y $$shell_quote($$DLL_SOURCE) $$shell_quote($$DLL_TARGET) $$escape_expand(\\n\\t)
}

# 简化的配置信息输出（可选）
# message("Using $$COMMON_API_LIB_TYPE ($$PLATFORM_DIR/$$ARCH_DIR)")
