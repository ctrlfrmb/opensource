# format.pri - 开源Format库配置文件
# Author: leiwei E-mail: ctrlfrmb@gmail.com
# Copyright (c) 2021. All rights reserved.

# 判断如果是 Windows 且使用 MSVC 编译器
win32-msvc* {
    # 强制添加 /std:c++17 标志
    QMAKE_CXXFLAGS += /std:c++17
    # 如果还需要更激进的兼容性，可以用 /std:c++latest
} else {
    # 非 MSVC (如 MinGW/GCC/Clang) 使用标准配置
    CONFIG += c++17
}


INCLUDEPATH += $$PWD/include

HEADERS += \
    $$PWD/include/fmt/core.h \
    $$PWD/include/fmt/format.h \
    $$PWD/include/fmt/args.h \
    $$PWD/include/fmt/base.h \
    $$PWD/include/fmt/chrono.h \
    $$PWD/include/fmt/color.h \
    $$PWD/include/fmt/compile.h \
    $$PWD/include/fmt/format-inl.h \
    $$PWD/include/fmt/os.h \
    $$PWD/include/fmt/ostream.h \
    $$PWD/include/fmt/printf.h \
    $$PWD/include/fmt/ranges.h \
    $$PWD/include/fmt/std.h \
    $$PWD/include/fmt/xchar.h

SOURCES += \
    $$PWD/src/format.cc \
    $$PWD/src/os.cc