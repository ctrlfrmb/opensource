# ===================================================================
# Vector XL Driver Library Configuration
# Author: leiwei E-mail: ctrlfrmb@gmail.com
# Copyright (c) 2025. All rights reserved.
# ===================================================================

VXL_ROOT = $$PWD

# 1. 头文件路径
INCLUDEPATH += $$VXL_ROOT/include

# 2. 架构判定 (映射为目录名 x64/x86 和 库文件名)
contains(QT_ARCH, x86_64) {
    ARCH_DIR = x64
    VXL_LIB_NAME = vxlapi64
} else {
    ARCH_DIR = x86
    VXL_LIB_NAME = vxlapi
}

# 3. 库文件路径配置
# 根据你的tree结构，只有 release 版本的库，所以无论编译 debug 还是 release，都链接 release 库
VXL_LIB_DIR = $$VXL_ROOT/lib/windows/$$ARCH_DIR/release
VXL_BIN_DIR = $$VXL_ROOT/bin/windows/$$ARCH_DIR/release

# 4. 链接库
LIBS += -L$$VXL_LIB_DIR -l$$VXL_LIB_NAME

# Windows 系统依赖库 (Vector驱动通常需要)
win32: LIBS += -luser32 -lkernel32 -ladvapi32

# 5. 定义一个拷贝函数，用于将 vxlapi64.dll 拷贝到构建目录，方便调试运行
defineReplace(copyVxlDll) {
    src = $$1
    dst = $$2
    return(copy /Y $$shell_quote($$shell_path($$src)) $$shell_quote($$shell_path($$dst)))
}

# 自动将 vxl 的 dll 拷贝到生成目录
VXL_DLL_PATH = $$VXL_BIN_DIR/$${VXL_LIB_NAME}.dll
exists($$VXL_DLL_PATH) {
    QMAKE_POST_LINK += $$copyVxlDll($$VXL_DLL_PATH, $$DESTDIR) $$escape_expand(\\n\\t)
}
