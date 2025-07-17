/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2022 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei
* Version: v1.0.0
* Date: 2022-09-15
*----------------------------------------------------------------------------*/

/**
* @file ui_utils.h
* @brief User Interface utility functions for Qt-based applications
*
* The UiUtils class provides a comprehensive set of utility functions for
* Qt-based user interface development, focusing on common UI styling and
* formatting tasks.
*
* Features:
* - HTML formatting utilities for rich text display
* - Standardized UI styling functions
* - Tab widget styling with modern design
* - Tooltip and help text formatting
* - Cross-platform UI consistency helpers
*
* This class is designed to be extensible and will grow to include more
* UI-related utility functions as the application develops.
*
*/

#ifndef COMMON_UI_UTILS_H
#define COMMON_UI_UTILS_H

#include <QStringList>
#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT UiUtils {
public:
    //=============================================================================
    // UI Style
    //=============================================================================

    static QString formatToHtml(const QString& title, const QStringList& sections);

    static QString getTabStyle();
};

}

#endif // COMMON_UI_UTILS_H
