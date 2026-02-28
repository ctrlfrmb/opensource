# libbinlog.pri - binlog 库配置文件
# 使用方法:
#   include(../3rdParty/libbinlog/libbinlog.pri)     
#
# Author: leiwei E-mail: ctrlfrmb@gmail.com
# Version: 1.12

# 根据目标架构选择目录
contains(QMAKE_TARGET.arch, x86_64) {
    BINLOG_LIBDIR = $$PWD/lib/Win64
} else:contains(QMAKE_TARGET.arch, x86) {
    BINLOG_LIBDIR = $$PWD/lib/Win32
} else {
    error("Unknown QMAKE_TARGET.arch: $$QMAKE_TARGET.arch")
}

# 头文件路径
INCLUDEPATH += $$PWD/include

# 库文件路径
LIBS += -L$$BINLOG_LIBDIR

# 链接动态库（.lib），运行时需对应DLL
LIBS += -lbinlog

