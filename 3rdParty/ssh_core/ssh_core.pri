# ssh_core.pri - SSH 核心库配置文件
# 使用方法:
#   include(../3rdParty/ssh_core/ssh_core.pri)                    # 默认使用 ssh_core
#   或者在包含前定义: DEFINES += USE_SSH_UI 然后再 include            # 使用 ssh_ui
#
# 注意：SSH 库内嵌了 Common 库源码，因此可以直接使用 Common 库的功能
#
# Author: leiwei E-mail: ctrlfrmb@gmail.com
# Version: 2.1.0

# 确保 UTF-8 支持
CONFIG += utf8_source
CODECFORTR = UTF-8

# ===================================================================
# 库根目录
# ===================================================================
SSH_CORE_LIB_ROOT = $$PWD

# ===================================================================
# 根据 USE_SSH_UI 选择库
# ===================================================================
contains(DEFINES, USE_SSH_UI) {
    SSH_CORE_LIB_NAME = ssh_ui
    SSH_CORE_LIB_TYPE = "SSH UI Library"
} else {
    SSH_CORE_LIB_NAME = ssh_core
    SSH_CORE_LIB_TYPE = "SSH Core Library"
}

# ===================================================================
# 头文件路径
# ===================================================================
# SSH 头文件
INCLUDEPATH += $$SSH_CORE_LIB_ROOT/include

# Common 库头文件（SSH 库内嵌了 Common 源码，头文件单独部署在 common_api 下）
COMMON_API_INCLUDE_DIR = $$SSH_CORE_LIB_ROOT/../common_api/include
exists($$COMMON_API_INCLUDE_DIR) {
    INCLUDEPATH += $$COMMON_API_INCLUDE_DIR
} else {
    warning("Common API headers not found at: $$COMMON_API_INCLUDE_DIR")
}

# 第三方库路径
INCLUDEPATH += $$SSH_CORE_LIB_ROOT/../

# 包含第三方库（如果存在）
exists($$SSH_CORE_LIB_ROOT/../fmt-12.0.0/format.pri) {
    include($$SSH_CORE_LIB_ROOT/../fmt-12.0.0/format.pri)
}

# ===================================================================
# 平台检测
# ===================================================================
win32 {
    PLATFORM_DIR = windows
} else:unix:!macx {
    PLATFORM_DIR = linux
} else:macx {
    PLATFORM_DIR = macos
} else {
    error("Unsupported platform: $$QMAKE_HOST.os")
}

# ===================================================================
# 架构检测
# ===================================================================
contains(QT_ARCH, x86_64) {
    ARCH_DIR = x64
} else:contains(QT_ARCH, i386) {
    ARCH_DIR = x86
} else {
    ARCH_DIR = x64
    warning("Unknown architecture $$QT_ARCH, defaulting to x64")
}

# ===================================================================
# 构建类型检测
# ===================================================================
CONFIG(debug, debug|release) {
    BUILD_DIR = debug
    LIB_SUFFIX = d
} else {
    BUILD_DIR = release
    LIB_SUFFIX =
}

# ===================================================================
# 库路径
# ===================================================================
SSH_CORE_LIB_DIR = $$SSH_CORE_LIB_ROOT/lib/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR
SSH_CORE_BIN_DIR = $$SSH_CORE_LIB_ROOT/bin/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR

# ===================================================================
# 验证库文件是否存在
# ===================================================================
win32 {
    LIB_FILE = $$SSH_CORE_LIB_DIR/$${SSH_CORE_LIB_NAME}$${LIB_SUFFIX}.lib
    DLL_FILE = $$SSH_CORE_BIN_DIR/$${SSH_CORE_LIB_NAME}$${LIB_SUFFIX}.dll

    !exists($$LIB_FILE) {
        error("SSH library not found: $$LIB_FILE")
    }
    !exists($$DLL_FILE) {
        error("SSH DLL not found: $$DLL_FILE")
    }
} else:unix:!macx {
    LIB_FILE = $$SSH_CORE_LIB_DIR/lib$${SSH_CORE_LIB_NAME}$${LIB_SUFFIX}.so

    !exists($$LIB_FILE) {
        error("SSH library not found: $$LIB_FILE (did you deploy the library first?)")
    }
} else:macx {
    LIB_FILE = $$SSH_CORE_LIB_DIR/lib$${SSH_CORE_LIB_NAME}$${LIB_SUFFIX}.dylib

    !exists($$LIB_FILE) {
        error("SSH library not found: $$LIB_FILE")
    }
}

# ===================================================================
# 链接库
# ===================================================================
LIBS += -L$$SSH_CORE_LIB_DIR
LIBS += -l$${SSH_CORE_LIB_NAME}$${LIB_SUFFIX}

# ===================================================================
# 运行时库查找
# ===================================================================
win32 {
    # Windows: 复制 DLL 到输出目录
    isEmpty(DESTDIR) {
        TARGET_DIR = $$OUT_PWD
    } else {
        TARGET_DIR = $$DESTDIR
    }

    DLL_SOURCE = $$shell_path($$DLL_FILE)
    DLL_TARGET = $$shell_path($$TARGET_DIR)

    QMAKE_POST_LINK += copy /Y $$shell_quote($$DLL_SOURCE) $$shell_quote($$DLL_TARGET) $$escape_expand(\\n\\t)
}

unix:!macx {
    # =================================================================
    # Linux 传递依赖
    # =================================================================
    # libssh_uid.so 内部已静态链接了 libssh2 1.11.1 (.a)
    # ★ 不要链接系统的 libssh2（apt 版本 1.10.0 太旧，且已内嵌）
    # ★ 只需要链接 libssh2 的底层运行时依赖：OpenSSL、zlib 等
    LIBS += -lssl -lcrypto -lz -ldl -lpthread
}

macx {
    # macOS: 设置 RPATH
    QMAKE_RPATHDIR += $$SSH_CORE_LIB_DIR
}
