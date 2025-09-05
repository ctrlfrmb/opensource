# libssh2.pri - libssh2 库配置文件（包含 OpenSSL 和 zlib 依赖）
# 使用方法: 在你的主 .pro 文件中添加 include(../3rdParty/libssh2/libssh2.pri)
# Author: leiwei E-mail: ctrlfrmb@gmail.com

# libssh2 库根目录
LIBSSH2_ROOT = $$PWD

# 头文件路径
INCLUDEPATH += $$LIBSSH2_ROOT/include

# --- 平台、架构和构建类型检测 ---

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
    # 默认使用 x64，并给出警告
    ARCH_DIR = x64
    ARCH_NAME = x64
    warning("Unknown architecture $$QT_ARCH, defaulting to x64")
}

# 根据 Debug/Release 配置选择不同的库文件名
CONFIG(debug, debug|release) {
    BUILD_DIR = debug
    BUILD_TYPE = debug
    # Debug 库通常有 'd' 后缀
    LIBSSH2_LIB_NAME = libssh2d.lib
    LIBSSL_LIB_NAME = libssld.lib
    LIBCRYPTO_LIB_NAME = libcryptod.lib
    ZLIB_LIB_NAME = zlibd.lib
} else {
    BUILD_DIR = release
    BUILD_TYPE = release
    # Release 库
    LIBSSH2_LIB_NAME = libssh2.lib
    LIBSSL_LIB_NAME = libssl.lib
    LIBCRYPTO_LIB_NAME = libcrypto.lib
    ZLIB_LIB_NAME = zlib.lib
}

# --- 库路径和链接配置 ---

# 设置库文件路径
LIBSSH2_LIBDIR = $$LIBSSH2_ROOT/lib/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR

# 添加库路径
LIBS += -L$$LIBSSH2_LIBDIR

# 链接库文件
win32 {
    # 强制将 LIBSSH2_API 定义为空。
    # libssh2.h 的逻辑会在我们构建 DLL (ssh_core.dll) 时，错误地将 LIBSSH2_API
    # 设置为 __declspec(dllimport)，导致链接错误。
    # 通过在这里预先定义，可以跳过头文件中的逻辑，确保静态链接。
    DEFINES += "LIBSSH2_API="

    # 1. 链接我们自己的静态库 (使用变量以支持Debug/Release)
    LIBS += $$LIBSSH2_LIB_NAME
    LIBS += $$LIBSSL_LIB_NAME
    LIBS += $$LIBCRYPTO_LIB_NAME
    LIBS += $$ZLIB_LIB_NAME

    # 2. 链接所有必需的 Windows 系统库
    LIBS += -lws2_32
    LIBS += -lcrypt32
    LIBS += -lbcrypt
    LIBS += -luser32
    LIBS += -ladvapi32

    # 3. 解决静态/动态运行时库冲突
    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:LIBCMT
    QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:LIBCMTD
} else:unix {
    # Unix/Linux/macOS 平台链接库
    LIBS += -lssh2 -lssl -lcrypto -lz
    QMAKE_RPATHDIR += $$LIBSSH2_LIBDIR
}

# --- 调试与检查 ---

# message("Using libssh2: $$ARCH_NAME/$$BUILD_TYPE")
# message("Library path: $$LIBSSH2_LIBDIR")

# 库文件存在性检查
win32 {
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
}
