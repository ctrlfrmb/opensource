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
* Date: 2022-09-20
*----------------------------------------------------------------------------*/

/**
* @file ui_utils.h
* @brief Universal UI utilities for Qt applications with comprehensive widget management
*
* The UiUtils class provides a comprehensive set of static utility functions for
* Qt application user interface operations. This class follows the utility pattern,
* providing common UI functionality without requiring instantiation.
*
* Key Features:
* - Cross-platform software update management with backup support
* - Multi-language font handling with Chinese font optimization
* - Professional HTML content formatting for tooltips and displays
* - Modern CSS styling for tabs and button containers
* - Comprehensive table widget operations (selection, movement, validation)
* - Input validation and dialog creation utilities
* - Status bar and toolbar component management
* - File encoding detection and configuration support
*
* Font Management:
* - Automatic system font detection for optimal Chinese character display
* - Fallback font loading from custom font files
* - Comprehensive font family recommendations for different platforms
* - UTF-8 encoding configuration for proper text rendering
*
* HTML Formatting:
* - Professional tooltip formatting with embedded CSS styling
* - Multi-section content organization with visual hierarchy
* - Responsive design with maximum width constraints
* - Support for labels, indentation, highlighting, and comments
*
* Widget Styling:
* - Modern tab control appearance with selection indicators
* - Professional button container styling with hover effects
* - Consistent visual feedback across different widget states
*
* Table Operations:
* - Row selection validation with optional warning messages
* - Batch checkbox state management across table columns
* - Row movement operations (top, bottom, up, down, custom position)
* - Safe table clearing with user confirmation dialogs
* - Comprehensive error handling for edge cases
*
* Usage example:
*   // Setup Chinese font support
*   QFont chineseFont = Common::UiUtils::getChineseFont("./fonts/chinese.ttf");
*   QApplication::setFont(chineseFont);
*
*   // Apply modern tab styling
*   tabWidget->setStyleSheet(Common::UiUtils::getTabStyle());
*
*   // Format professional tooltip
*   QString tooltip = Common::UiUtils::formatToHtml("Configuration",
*       {"Server: 192.168.1.100", "Port: 8080", "Status: Connected"});
*   widget->setToolTip(tooltip);
*
*   // Table row operations with validation
*   if (Common::UiUtils::isTableRowSelected(tableWidget)) {
*       Common::UiUtils::moveTableRowUp(tableWidget);
*   }
*/

#ifndef COMMON_UI_UTILS_H
#define COMMON_UI_UTILS_H

#include "common_global.h"
#include <QFont>
#include <QStringList>
#include <QWidget>

// Forward declarations
class QStatusBar;
class QTableWidget;
class QToolButton;
class QLineEdit;

namespace Common {

class COMMON_API_EXPORT UiUtils : public QWidget
{
    Q_OBJECT

public:
    //=============================================================================
    // Software Update Management
    //=============================================================================

    /**
     * @brief 检查并启动软件更新程序
     *
     * 自动检测更新程序是否存在备份版本并进行自我更新，然后启动更新检查。
     * 支持静默模式和交互模式，提供完整的错误处理和权限管理。
     *
     * @param isAutoCheck 是否为自动检查模式（静默模式，不显示错误对话框）
     * @return 更新程序启动成功返回true，失败返回false
     *
     * @note 自动检查模式下不会显示错误对话框，适用于程序启动时的后台检查
     * @note 手动模式下会显示详细的错误信息和用户提示
     * @note 支持更新程序的自我更新机制（通过.bak备份文件）
     */
    static bool checkForUpdates(bool isAutoCheck = false);

    //=============================================================================
    // Font Management and Localization
    //=============================================================================

    /**
     * @brief 获取支持中文显示的优化字体
     *
     * 智能选择最适合的中文字体，优先使用系统已安装的推荐字体，
     * 如果系统字体不可用或指定了自定义字体路径，则加载自定义字体文件。
     *
     * @param fontPath 自定义字体文件路径（可选，为空时仅使用系统字体）
     * @return 配置好的QFont对象，保证中文字符正确显示
     *
     * @note 自动配置UTF-8编码以确保文本正确渲染
     * @note 按优先级顺序尝试推荐的中文字体列表
     * @note 支持跨平台字体选择（Windows/Linux/macOS）
     */
    static QFont getChineseFont(const QString& fontPath = QString());

    /**
     * @brief 获取系统所有可用字体列表
     * @return 系统字体家族名称的字符串列表
     */
    static QStringList getAvailableFonts();

    /**
     * @brief 获取推荐的中文字体列表
     *
     * 返回按优先级排序的中文字体列表，涵盖Windows、Linux、macOS
     * 等不同平台的常见中文字体，确保最佳的中文显示效果。
     *
     * @return 推荐中文字体名称列表，按显示质量和兼容性排序
     */
    static QStringList getRecommendedChineseFonts();

    //=============================================================================
    // File and System Operations
    //=============================================================================

    /**
     * @brief 使用系统默认程序打开帮助文档
     *
     * 跨平台的文件打开功能，自动使用系统关联的默认程序打开指定文件。
     * 支持相对路径和绝对路径，提供详细的错误信息反馈。
     *
     * @param helpFilePath 帮助文件的路径（支持相对路径）
     * @param errorMsg 可选的错误信息输出参数
     * @return 文件打开成功返回true，失败返回false
     *
     * @note Windows下使用cmd命令，Linux下使用xdg-open
     * @note 自动检查文件存在性，避免无效的文件打开尝试
     */
    static bool openHelpFile(const QString& helpFilePath, QString* errorMsg = nullptr);

    //=============================================================================
    // HTML Content Formatting
    //=============================================================================

    /**
     * @brief 格式化内容为专业的HTML格式
     *
     * 生成具有现代CSS样式的HTML内容，适用于工具提示、状态显示等场景。
     * 支持标题、多个内容节段的格式化，提供标签、缩进、高亮、注释等样式类。
     *
     * @param title 标题文本，以粗体蓝色显示在顶部
     * @param sections 内容节段列表，每个节段独立显示
     * @return 包含内嵌CSS样式的完整HTML字符串
     *
     * @note 最大宽度限制为400px，确保在各种显示环境下的良好表现
     * @note 支持的CSS类：.title, .section, .label, .indent, .highlight, .comment
     * @note 使用现代字体栈，确保在不同操作系统下的一致性
     */
    static QString formatToHtml(const QString& title, const QStringList& sections);

    //=============================================================================
    // Qt Widget Styling
    //=============================================================================

    /**
     * @brief 获取现代化标签页控件的CSS样式表
     *
     * 提供专业的标签页外观，包含选中状态的蓝色底边指示器、
     * 悬停效果和清晰的视觉层次。适用于QTabWidget和QTabBar控件。
     *
     * @return 标签页控件的完整CSS样式表字符串
     *
     * @note 选中标签具有蓝色底边和粗体文字的视觉反馈
     * @note 未选中标签为浅灰色背景，悬停时显示中等灰色
     * @note 包含5px的内边距，确保文字显示的舒适度
     */
    static QString getTabStyle();

    /**
     * @brief 获取按钮容器控件的统一样式表
     *
     * 为包含多个按钮的容器提供现代化外观，包含无边框按钮设计、
     * 悬停高亮效果和按下状态反馈。适用于工具栏和按钮组。
     *
     * @return 按钮容器控件的CSS样式表字符串
     *
     * @note 容器具有浅灰色背景（#f0f0f0）
     * @note 按钮具有3px边距和内边距，提供良好的视觉间距
     * @note 悬停时显示淡蓝色边框，按下时显示淡蓝色背景
     */
    static QString getButtonWidgetStyle();

    //=============================================================================
    // Configuration and Data Support
    //=============================================================================

    /**
     * @brief 获取支持的文件编码格式列表
     *
     * 返回常用的文件编码格式，适用于文件导入/导出功能中的编码选择。
     * 包含Unicode标准编码和中文编码格式，按使用频率排序。
     *
     * @return 编码名称的字符串列表（UTF-8, GB2312, ASCII, GBK等）
     */
    static QStringList getFileEncodings();

    //=============================================================================
    // Status Bar and Dialog Utilities
    //=============================================================================

    /**
     * @brief 在状态栏中设置版本信息显示
     *
     * 在指定状态栏的右侧添加版本信息标签，提供持久的版本信息显示。
     * 版本信息将作为永久部件添加，不会被临时状态消息覆盖。
     *
     * @param statusBar 目标状态栏指针
     * @param versionInfo 要显示的版本信息文本
     *
     * @note 版本信息显示在状态栏右侧，不会影响左侧的状态消息
     * @note 如果statusBar为nullptr，函数将安全返回，不执行任何操作
     */
    static void setStatusBarVersionInfo(QStatusBar *statusBar, const QString& versionInfo);

    /**
     * @brief 创建通用输入对话框
     *
     * 提供一个简化的文本输入对话框，具有一致的外观和行为。
     * 自动移除帮助按钮，并调整大小以适应内容。
     *
     * @param parent 父窗口指针
     * @param title 对话框标题
     * @param label 输入提示文本（可选）
     * @param defaultText 默认输入内容（可选）
     * @param isPassword 是否为密码框（可选）
     * @return 用户输入的文本，取消时返回空字符串
     *
     * @note 对话框会自动调整大小以避免几何警告
     * @note 移除了标准的上下文帮助按钮，提供更简洁的界面
     */
    static QString getInputText(QWidget *parent, const QString &title,
                                const QString &label = QString(),
                                const QString &defaultText = QString(),
                                bool isPassword = false);

    /**
     * @brief 为输入框设置正则表达式验证器
     *
     * 为QLineEdit控件配置输入验证和占位符文本，确保用户输入符合预期格式。
     * 验证器将限制用户只能输入匹配正则表达式的内容。
     *
     * @param lineEdit 目标输入框指针
     * @param regexString 正则表达式字符串
     * @param placeholderText 占位符提示文本
     *
     * @note 验证器对象的生命周期由lineEdit管理，无需手动释放
     * @note 占位符文本将在输入框为空时显示，提供输入格式提示
     */
    static void setupInputValidator(QLineEdit* lineEdit, const QString& regexString,
                                   const QString& placeholderText);

    //=============================================================================
    // Table Widget Operations
    //=============================================================================

    /**
     * @brief 检查表格是否有行被选中
     *
     * 验证表格控件是否有选中的行，可选择是否显示警告对话框。
     * 包含空表格和无效表格指针的安全检查。
     *
     * @param table 表格控件指针
     * @param isWarnning 是否在无选中行时显示警告对话框
     * @return 有行被选中返回true，否则返回false
     *
     * @note 空表格或nullptr会安全返回false，不会崩溃
     * @note 警告对话框使用英文文本，可根据需要本地化
     */
    static bool isTableRowSelected(QTableWidget* table, bool isWarnning = true);

    /**
     * @brief 批量设置表格指定列的复选框状态
     *
     * 遍历表格的所有行，统一设置指定列中复选框项目的选中状态。
     * 只对已存在的表格项目进行操作，忽略空项目。
     *
     * @param table 表格控件指针
     * @param col 目标列索引
     * @param isChecked 是否选中（true为选中，false为未选中）
     *
     * @note 只影响具有复选框状态的QTableWidgetItem
     * @note 对空表格或无效指针进行安全检查，避免崩溃
     */
    static void setTableItemCheckState(QTableWidget* table, int col, bool isChecked);

    /**
     * @brief 删除当前选中的表格行
     *
     * 删除表格中当前选中的行。内部调用isTableRowSelected进行选中状态验证，
     * 确保只有在有行被选中时才执行删除操作。
     *
     * @param table 表格控件指针
     *
     * @note 如果没有行被选中，会显示警告对话框（如果启用警告）
     * @note 删除后表格的行索引会自动调整
     */
    static void removeTableRow(QTableWidget* table);

    /**
     * @brief 将选中行移动到表格顶部
     * @param table 表格控件指针
     * @note 移动后会自动选中目标位置的行
     */
    static void moveTableRowToTop(QTableWidget* table);

    /**
     * @brief 将选中行向上移动一位
     * @param table 表格控件指针
     * @note 如果已经是第一行，则不执行任何操作
     */
    static void moveTableRowUp(QTableWidget* table);

    /**
     * @brief 将选中行向下移动一位
     * @param table 表格控件指针
     * @note 如果已经是最后一行，则不执行任何操作
     */
    static void moveTableRowDown(QTableWidget* table);

    /**
     * @brief 将选中行移动到表格底部
     * @param table 表格控件指针
     * @note 移动后会自动选中目标位置的行
     */
    static void moveTableRowToBottom(QTableWidget* table);

    /**
     * @brief 在表格中移动行到指定位置
     *
     * 核心的行移动功能，支持将任意行移动到任意位置。
     * 正确处理单元格内容和控件的转移，保持数据完整性。
     *
     * @param table 表格控件指针
     * @param fromRow 源行索引
     * @param toRow 目标行索引
     *
     * @note 自动处理源行索引的调整，确保在插入后正确删除源行
     * @note 支持单元格控件和普通项目的混合移动
     */
    static void moveTableRow(QTableWidget* table, int fromRow, int toRow);

    /**
     * @brief 清空表格内容（带用户确认）
     *
     * 清除表格中的所有行，在执行前显示确认对话框，避免意外的数据丢失。
     * 只有用户确认后才会执行清空操作。
     *
     * @param table 表格控件指针
     * @return 用户确认并成功清空返回true，用户取消或表格为空返回false
     *
     * @note 确认对话框使用英文文本，可根据需要本地化
     * @note 清空操作不可撤销，请确保用户了解操作后果
     */
    static bool clearTable(QTableWidget* table);

    //=============================================================================
    // Toolbar and Button Utilities
    //=============================================================================

    /**
     * @brief 创建标准化的工具按钮
     *
     * 创建具有一致外观和行为的QToolButton，支持文本、图标和工具提示的配置。
     * 可选择按钮的显示样式（仅图标、仅文本、图标+文本等）。
     *
     * @param text 按钮显示文本
     * @param tooltip 工具提示文本
     * @param iconPath 图标文件路径
     * @param buttonStyle 按钮样式（Qt::ToolButtonStyle枚举值）
     * @param parent 父窗口指针
     * @return 配置完成的QToolButton指针
     *
     * @note 默认样式为Qt::ToolButtonTextBesideIcon（图标在文本旁边）
     * @note 按钮的生命周期由parent管理，如果parent为nullptr则需手动管理
     */
    static QToolButton* createToolButton(const QString& text, const QString& tooltip,
                                        const QString& iconPath,
                                        int buttonStyle = Qt::ToolButtonTextBesideIcon,
                                        QWidget *parent = nullptr);
};

} // namespace Common

#endif // COMMON_UI_UTILS_H
