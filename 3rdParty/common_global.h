/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2021 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei E-mail: ctrlfrmb@gmail.com
* Version: v1.0.0
* Date: 2021-12-27
*----------------------------------------------------------------------------*/

#ifndef COMMON_GLOBAL_H
#define COMMON_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(BUILD_COMMON_API)
#  define COMMON_API_EXPORT Q_DECL_EXPORT
#else
#  define COMMON_API_EXPORT Q_DECL_IMPORT
#endif

#endif // COMMON_GLOBAL_H
