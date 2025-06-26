#ifndef COMMON_GLOBAL_H
#define COMMON_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(BUILD_COMMON_API)
#  define COMMON_API_EXPORT Q_DECL_EXPORT
#else
#  define COMMON_API_EXPORT Q_DECL_IMPORT
#endif

#endif // COMMON_GLOBAL_H
