CONFIG += c++17

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
