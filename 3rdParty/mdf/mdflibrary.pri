# MDF库路径
MDF_ROOT = $$PWD

INCLUDEPATH += $${MDF_ROOT}/include
DEPENDPATH += $${MDF_ROOT}/include

win32:CONFIG(release, debug|release): LIBS += -L$${MDF_ROOT}/lib/Winx86/ -lmdflib
else:win32:CONFIG(debug, debug|release): LIBS += -L$${MDF_ROOT}/lib/Winx86/ -lmdflib
else:unix: LIBS += -L$${MDF_ROOT}/lib/ -lmdflib
