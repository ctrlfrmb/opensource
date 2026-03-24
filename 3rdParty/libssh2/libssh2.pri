# libssh2.pri - libssh2 库配置文件（包含 OpenSSL 和 zlib 依赖）
# 策略:
#   Windows - 使用 3rdParty 内的预编译静态库 (.lib)
#   Linux   - 使用 3rdParty 内从源码编译的静态库 (.a) + 系统 OpenSSL/zlib
#   两个平台共用同一份头文件（版本均为 1.11.1）
# Author: leiwei E-mail: ctrlfrmb@gmail.com

# libssh2 库根目录
LIBSSH2_ROOT = $$PWD

# --- 头文件路径（全平台统一，版本 1.11.1）---
INCLUDEPATH += $$LIBSSH2_ROOT/include

# --- 平台、架构检测 ---

win32 {
    PLATFORM_DIR = windows
} else:unix:!macx {
    PLATFORM_DIR = linux
} else:macx {
    PLATFORM_DIR = macos
} else {
    error("Unsupported platform for libssh2")
}

contains(QT_ARCH, x86_64) {
    ARCH_DIR = x64
    ARCH_NAME = x64
} else:contains(QT_ARCH, i386) {
    ARCH_DIR = x86
    ARCH_NAME = x86
} else {
    ARCH_DIR = x64
    ARCH_NAME = x64
    warning("Unknown architecture $$QT_ARCH, defaulting to x64")
}

# --- 平台分支配置 ---

win32 {
    # =================================================================
    # Windows: 预编译静态库 (.lib)  ← 与原始文件完全一致
    # =================================================================

    CONFIG(debug, debug|release) {
        BUILD_DIR = debug
        BUILD_TYPE = debug
        LIBSSH2_LIB_NAME = libssh2d.lib
        LIBSSL_LIB_NAME = libssld.lib
        LIBCRYPTO_LIB_NAME = libcryptod.lib
        ZLIB_LIB_NAME = zlibd.lib
    } else {
        BUILD_DIR = release
        BUILD_TYPE = release
        LIBSSH2_LIB_NAME = libssh2.lib
        LIBSSL_LIB_NAME = libssl.lib
        LIBCRYPTO_LIB_NAME = libcrypto.lib
        ZLIB_LIB_NAME = zlib.lib
    }

    LIBSSH2_LIBDIR = $$LIBSSH2_ROOT/lib/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR
    LIBS += -L$$LIBSSH2_LIBDIR

    DEFINES += "LIBSSH2_API="

    LIBS += $$LIBSSH2_LIB_NAME
    LIBS += $$LIBSSL_LIB_NAME
    LIBS += $$LIBCRYPTO_LIB_NAME
    LIBS += $$ZLIB_LIB_NAME

    LIBS += -lws2_32
    LIBS += -liphlpapi
    LIBS += -lcrypt32
    LIBS += -lbcrypt
    LIBS += -luser32
    LIBS += -ladvapi32

#    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:LIBCMT
#    QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:LIBCMTD

    !exists($$LIBSSH2_LIBDIR/$$LIBSSH2_LIB_NAME) {
        warning("libssh2 library not found at: $$LIBSSH2_LIBDIR/$$LIBSSH2_LIB_NAME")
    }
    !exists($$LIBSSH2_LIBDIR/$$LIBSSL_LIB_NAME) {
        warning("OpenSSL (ssl) library not found at: $$LIBSSH2_LIBDIR/$$LIBSSL_LIB_NAME")
    }
    !exists($$LIBSSH2_LIBDIR/$$LIBCRYPTO_LIB_NAME) {
        warning("OpenSSL (crypto) library not found at: $$LIBSSH2_LIBDIR/$$LIBCRYPTO_LIB_NAME")
    }
    !exists($$LIBSSH2_LIBDIR/$$ZLIB_LIB_NAME) {
        warning("zlib library not found at: $$LIBSSH2_LIBDIR/$$ZLIB_LIB_NAME")
    }

} else:unix:!macx {
    # =================================================================
    # Linux: 自编译静态库 (.a) + 系统 OpenSSL/zlib 动态库
    # =================================================================

    CONFIG(debug, debug|release) {
        BUILD_DIR = debug
    } else {
        BUILD_DIR = release
    }

    LIBSSH2_LIBDIR = $$LIBSSH2_ROOT/lib/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR
    LIBSSH2_STATIC = $$LIBSSH2_LIBDIR/libssh2.a

    # 直接指定 .a 文件路径，避免链接器去系统路径找旧的 libssh2.so
    LIBS += $$LIBSSH2_STATIC

    # OpenSSL 和 zlib 使用系统动态库
    LIBS += -lssl -lcrypto -lz -ldl -lpthread

    !exists($$LIBSSH2_STATIC) {
        error("Linux libssh2 static library not found at: $$LIBSSH2_STATIC")
    }

} else:macx {
    # =================================================================
    # macOS
    # =================================================================
    LIBS += -lssh2 -lssl -lcrypto -lz
}

# --- 调试与检查 ---
# message("Using libssh2 on: $$PLATFORM_DIR/$$ARCH_NAME")
