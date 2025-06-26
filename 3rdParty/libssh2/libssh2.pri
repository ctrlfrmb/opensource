# libssh2.pri - libssh2 库配置文件（包含 OpenSSL 和 zlib 依赖）
# 使用方法: include(../3rdParty/libssh2/libssh2.pri)

# libssh2 库根目录
LIBSSH2_ROOT = $$PWD

# 头文件路径
INCLUDEPATH += $$LIBSSH2_ROOT/include

# 平台检测
win32 {
    PLATFORM_DIR = windows
} else:unix:!macx {
    PLATFORM_DIR = linux
} else:macx {
    PLATFORM_DIR = macos
} else {
    error("Unsupported platform for libssh2")
}

# 架构检测
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

# 构建类型检测
CONFIG(debug, debug|release) {
    BUILD_DIR = debug
    BUILD_TYPE = debug
    LIB_SUFFIX = d
} else {
    BUILD_DIR = release
    BUILD_TYPE = release
    LIB_SUFFIX =
}

# 设置库文件路径
LIBSSH2_LIBDIR = $$LIBSSH2_ROOT/lib/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR
LIBSSH2_BINDIR = $$LIBSSH2_ROOT/bin/$$PLATFORM_DIR/$$ARCH_DIR/$$BUILD_DIR

# 添加库路径
LIBS += -L$$LIBSSH2_LIBDIR

# 链接库文件
win32 {
    # Windows 平台链接库
    LIBS += -llibssh2$${LIB_SUFFIX}
    LIBS += -llibssl$${LIB_SUFFIX}
    LIBS += -llibcrypto$${LIB_SUFFIX}
    LIBS += -lzlib$${LIB_SUFFIX}
    
    # Windows 系统库
    LIBS += -lws2_32 -ladvapi32 -luser32 -lcrypt32
    
} else:unix {
    # Unix/Linux 平台链接库
    LIBS += -lssh2$${LIB_SUFFIX}
    LIBS += -lssl$${LIB_SUFFIX}
    LIBS += -lcrypto$${LIB_SUFFIX}
    LIBS += -lz$${LIB_SUFFIX}
    
    # 添加 rpath 用于运行时库搜索
    QMAKE_RPATHDIR += $$LIBSSH2_LIBDIR
}

# 定义宏
# DEFINES += USE_LIBSSH2

# 可选：添加运行时 DLL 路径到环境变量（Windows 开发时有用）
# win32 {
#     # 这在某些开发环境中有助于找到 DLL
#     message("DLL path: $$LIBSSH2_BINDIR")
#     # 注意：这不会影响最终部署，只是开发时的提示
# }

# 调试信息输出
# message("Using libssh2: $$ARCH_NAME/$$BUILD_TYPE")
# message("Include path: $$LIBSSH2_ROOT/include")
# message("Library path: $$LIBSSH2_LIBDIR")
# message("Linking libraries: libssh2$${LIB_SUFFIX}, libssl$${LIB_SUFFIX}, libcrypto$${LIB_SUFFIX}, zlib$${LIB_SUFFIX}")

# 库文件存在性检查（可选，用于调试）
win32 {
    !exists($$LIBSSH2_LIBDIR/libssh2$${LIB_SUFFIX}.lib) {
        warning("libssh2 library not found at: $$LIBSSH2_LIBDIR/libssh2$${LIB_SUFFIX}.lib")
    }
    !exists($$LIBSSH2_LIBDIR/libssl$${LIB_SUFFIX}.lib) {
        warning("OpenSSL library not found at: $$LIBSSH2_LIBDIR/libssl$${LIB_SUFFIX}.lib")
    }
}
