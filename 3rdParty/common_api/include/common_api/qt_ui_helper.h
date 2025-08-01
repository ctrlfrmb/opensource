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
* Date: 2022-10-15
*----------------------------------------------------------------------------*/

/**
 * @file qt_ui_helper.h
 * @brief Qt UI utility functions for common interface styling and formatting
 *
 * The QtUiHelper class provides static utility functions for Qt-based user interfaces,
 * focusing on consistent styling and content formatting across applications.
 *
 * Key features:
 *
 * 1. HTML Content Formatting
 *    - Rich HTML tooltip generation with professional styling
 *    - Structured content presentation with customizable sections
 *    - Responsive layout with maximum width constraints
 *
 * 2. Qt Widget Styling
 *    - Tab widget styling with modern appearance
 *    - Button widget styling with hover and pressed states
 *    - Consistent color schemes and typography
 *
 * 3. Configuration Support
 *    - Common file encoding options for international support
 *    - Standardized encoding list for file operations
 *
 * Usage example:
 * @code
 *   // Create formatted HTML tooltip
 *   QStringList sections = {"Type: CAN Message", "ID: 0x123", "Data Length: 8 bytes"};
 *   QString html = Common::QtUiHelper::formatToHtml("Message Info", sections);
 *   widget->setToolTip(html);
 *
 *   // Apply tab styling
 *   tabWidget->setStyleSheet(Common::QtUiHelper::getTabStyle());
 * @endcode
 */

#ifndef COMMON_QT_UI_HELPER_H
#define COMMON_QT_UI_HELPER_H

#include "common_global.h"
#include <QStringList>

namespace Common {

class COMMON_API_EXPORT QtUiHelper {
public:

    //=============================================================================
    // HTML Content Formatting
    //=============================================================================

    /**
     * @brief 格式化内容为HTML格式（用于工具提示等）
     *
     * 生成具有专业外观的HTML内容，包含标题和多个节段。使用现代CSS样式，
     * 支持多种内容类型的格式化显示，包括标签、缩进、高亮和注释样式。
     *
     * @param title 标题文本，将以粗体蓝色显示
     * @param sections 内容节段列表，每个节段独立显示
     * @return 格式化后的HTML字符串，可直接用于setToolTip()等
     *
     * @note HTML包含内嵌CSS样式，最大宽度限制为400px
     * @note 支持的CSS类：.title, .section, .label, .indent, .highlight, .comment
     */
    static QString formatToHtml(const QString& title, const QStringList& sections);

    //=============================================================================
    // Qt Widget Styling
    //=============================================================================

    /**
     * @brief 获取标签页控件的样式表
     *
     * 提供现代化的标签页外观，包含未选中、选中和悬停三种状态的样式。
     * 选中标签具有蓝色底边和粗体文字，未选中标签为浅灰色背景。
     *
     * @return 标签页控件的CSS样式表字符串
     *
     * @note 样式包含hover效果和选中状态的视觉反馈
     * @note 兼容QTabWidget和QTabBar控件
     */
    static QString getTabStyle();

    /**
     * @brief 获取按钮容器控件的样式表
     *
     * 为包含多个按钮的容器控件提供统一的外观样式。
     * 包含浅灰色背景、无边框按钮、悬停高亮和按下效果。
     *
     * @return 按钮容器控件的CSS样式表字符串
     *
     * @note 适用于工具栏、按钮组等包含多个QPushButton的QWidget
     * @note 按钮具有3px的边距和内边距，提供良好的视觉间距
     */
    static QString getButtonWidgetStyle();

    //=============================================================================
    // Configuration Support
    //=============================================================================

    /**
     * @brief 获取支持的文件编码列表
     *
     * 返回常用的文件编码格式列表，适用于文件导入/导出操作中的编码选择。
     * 包含Unicode、中文和国际标准编码格式。
     *
     * @return 编码名称的字符串列表，按使用频率排序
     */
    static QStringList getFileEncodings();
};

}

#endif // COMMON_QT_UI_HELPER_H
