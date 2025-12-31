# ===================================================================
# Vector XL Driver Library Configuration
# Author: leiwei E-mail: ctrlfrmb@gmail.com
# Copyright (c) 2025. All rights reserved.
# ===================================================================

# 确保 UTF-8 支持
CONFIG += utf8_source

# 库根目录 (即 vxl_api.pri 所在目录)
VXL_LIB_ROOT = $$PWD

# 头文件路径
INCLUDEPATH += $$VXL_LIB_ROOT/include

# 平台检测 (Vector 驱动主要在 Windows 下使用)
win32 {
    PLATFORM_DIR = windows
} else {
    error("Vector XL Driver is only supported on Windows.")
}

# 架构检测 (自动区分 x86 和 x64)
# Vector 的库命名规则：x86为 vxlapi.lib, x64为 vxlapi64.lib
contains(QT_ARCH, x86_64) {
    ARCH_DIR = x64
    VXL_LIB_NAME = vxlapi64
} else:contains(QT_ARCH, i386) {
    ARCH_DIR = x86
    VXL_LIB_NAME = vxlapi
} else {
    ARCH_DIR = x64
    VXL_LIB_NAME = vxlapi64
    warning("Unknown architecture $$QT_ARCH, defaulting to x64 and vxlapi64")
}

# 构建类型检测 (Debug/Release)
# 注意：通常第三方提供的 DLL 只有 Release 版或混合版。
# 根据你的 tree 结构，你区分了 debug 和 release 目录，这里做相应适配。
# CONFIG(debug, debug|release) {
#     BUILD_DIR = debug
# } else {
#     BUILD_DIR = release
# }
BUILD_DIR = release

# 设置库文件(.lib) 和 二进制文件(.dll) 的路径
VXL_LIB_DIR = $$VXL_LIB_ROOT/lib/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR
VXL_BIN_DIR = $$VXL_LIB_ROOT/bin/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR

# 具体的库文件路径
LIB_FILE = $$VXL_LIB_DIR/$${VXL_LIB_NAME}.lib
DLL_FILE = $$VXL_BIN_DIR/$${VXL_LIB_NAME}.dll

# 检查库文件是否存在
!exists($$LIB_FILE) {
    error("Vector Library file not found: $$LIB_FILE")
}

# 链接库
LIBS += -L$$VXL_LIB_DIR -l$$VXL_LIB_NAME

# Post-Link: 自动将 DLL 复制到生成目录
win32 {
    isEmpty(DESTDIR) {
        TARGET_DIR = $$OUT_PWD
    } else {
        TARGET_DIR = $$DESTDIR
    }

    DLL_SOURCE = $$shell_path($$DLL_FILE)
    DLL_TARGET = $$shell_path($$TARGET_DIR)

    # 使用 copy /Y 命令覆盖复制
    QMAKE_POST_LINK += copy /Y $$shell_quote($$DLL_SOURCE) $$shell_quote($$DLL_TARGET) $$escape_expand(\\n\\t)
}

message("Vector XL Driver configured: $$ARCH_DIR | $$BUILD_DIR")
