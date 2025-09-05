# libssh.pri - libssh 库配置文件（包含 OpenSSL 和 zlib 依赖）
# 使用方法: 在你的主 .pro 文件中添加 include(../3rdParty/libssh/libssh.pri)
# Author: leiwei E-mail: ctrlfrmb@gmail.com

# libssh 库根目录
LIBSSH_ROOT = $$PWD

# 头文件路径
INCLUDEPATH += $$LIBSSH_ROOT/include

# --- 平台、架构和构建类型检测 ---

win32 {
    PLATFORM_DIR = windows
} else:unix:!macx {
    PLATFORM_DIR = linux
} else:macx {
    PLATFORM_DIR = macos
} else {
    error("Unsupported platform for libssh")
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

CONFIG(debug, debug|release) {
    BUILD_DIR = debug
    BUILD_TYPE = debug
} else {
    BUILD_DIR = release
    BUILD_TYPE = release
}

# --- 库路径和链接配置 (最终修正版) ---

# 设置库文件路径
LIBSSH_LIBDIR = $$LIBSSH_ROOT/lib/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR

# 添加库路径
LIBS += -L$$LIBSSH_LIBDIR

# 链接库文件
win32 {
    # 1. 链接我们自己的静态库 (使用完整文件名)
    LIBS += ssh.lib
    LIBS += libssl.lib
    LIBS += libcrypto.lib
    LIBS += zlib.lib

    # 2. 链接所有必需的 Windows 系统库
    #    这些库提供了 libssh 和 OpenSSL 调用的操作系统函数
    LIBS += -lws2_32     # Windows Sockets
    LIBS += -lcrypt32    # 加密 API
    LIBS += -lbcrypt     # 新版加密 API
    LIBS += -ladvapi32   # 高级服务 (用户、注册表、事件日志)
    LIBS += -lshell32    # Windows Shell (例如获取用户目录)
    LIBS += -liphlpapi   # IP 助手 API
    LIBS += -luser32     # 用户界面 API (例如消息框)

    # 3. 解决静态/动态运行时库冲突 (LNK4098 警告)
    #    vcpkg 静态库使用 /MT，Qt 默认使用 /MD。
    #    此设置告诉链接器忽略 Qt 的默认库，以避免冲突。
    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:LIBCMT
    QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:LIBCMTD
} else:unix {
    # Unix/Linux 平台链接库
    LIBS += -lssh -lssl -lcrypto -lz
    QMAKE_RPATHDIR += $$LIBSSH_LIBDIR
}

# 定义一个宏
DEFINES += USE_LIBSSH

# --- 调试与检查 ---

# message("Using libssh: $$ARCH_NAME/$$BUILD_TYPE")
# message("Library path: $$LIBSSH_LIBDIR")

# 库文件存在性检查
win32 {
    !exists($$LIBSSH_LIBDIR/ssh.lib) {
        warning("libssh library not found at: $$LIBSSH_LIBDIR/ssh.lib")
    }
    !exists($$LIBSSH_LIBDIR/libssl.lib) {
        warning("OpenSSL (ssl) library not found at: $$LIBSSH_LIBDIR/libssl.lib")
    }
    !exists($$LIBSSH_LIBDIR/libcrypto.lib) {
        warning("OpenSSL (crypto) library not found at: $$LIBSSH_LIBDIR/libcrypto.lib")
    }
    !exists($$LIBSSH_LIBDIR/zlib.lib) {
        warning("zlib library not found at: $$LIBSSH_LIBDIR/zlib.lib")
    }
}
