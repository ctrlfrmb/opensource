# common_ui.pri - 通用UI库配置文件
# 使用方法: include(../3rdParty/common_ui/common_ui.pri)

# 确保 UTF-8 支持
CONFIG += utf8_source
CODECFORTR = UTF-8

# 包含第三方库
include(../fmt-11.1.4/format.pri)

# 库根目录
COMMON_UI_LIB_ROOT = $$PWD
# 库名称
COMMON_UI_LIB_NAME = common_ui

# 头文件路径
INCLUDEPATH += $$COMMON_UI_LIB_ROOT/../common_api/include
INCLUDEPATH += $$COMMON_UI_LIB_ROOT/../

# 平台检测
win32: PLATFORM_DIR = windows
else:unix:!macx: PLATFORM_DIR = linux
else:macx: PLATFORM_DIR = macos
else: error("Unsupported platform")

# 架构检测
contains(QT_ARCH, x86_64): ARCH_DIR = x64
else:contains(QT_ARCH, i386): ARCH_DIR = x86
else: ARCH_DIR = x64

# 构建类型检测
CONFIG(debug, debug|release) {
    BUILD_DIR = debug
    LIB_SUFFIX = d
    BUILD_TYPE = debug
} else {
    BUILD_DIR = release
    LIB_SUFFIX =
    BUILD_TYPE = release
}

# 设置库路径
COMMON_UI_LIB_DIR = $$COMMON_UI_LIB_ROOT/lib/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR
COMMON_UI_BIN_DIR = $$COMMON_UI_LIB_ROOT/bin/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR

# 添加库路径
LIBS += -L$$COMMON_UI_LIB_DIR

# 链接库
win32 {
    LIBS += -l$${COMMON_UI_LIB_NAME}$${LIB_SUFFIX}
} else {
    LIBS += -l$${COMMON_UI_LIB_NAME}$${LIB_SUFFIX}
}

# 定义宏
# DEFINES += USE_COMMON_UI

# 调试信息
# message("Using common_ui: $$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_TYPE")
# message("Include path: $$COMMON_UI_LIB_ROOT")
# message("Library path: $$COMMON_UI_LIB_DIR")
# message("Linking: common_ui$${LIB_SUFFIX}")
