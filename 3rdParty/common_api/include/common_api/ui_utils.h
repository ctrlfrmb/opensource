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
     * @brief Check and launch software update program
     *
     * Automatically detects if the updater has a backup version, performs self-update,
     * and then initiates the update check. Supports silent and interactive modes
     * with comprehensive error handling and permission management.
     *
     * @param isAutoCheck Whether in automatic check mode (silent mode, no error dialogs)
     * @return True if updater launched successfully, false otherwise
     *
     * @note In automatic check mode, no error dialogs are shown, suitable for background checks at program startup
     * @note In manual mode, detailed error information and user prompts are displayed
     * @note Supports the updater's self-update mechanism (via .bak backup files)
     */
    static bool checkForUpdates(bool isAutoCheck = false);

    //=============================================================================
    // Font Management and Localization
    //=============================================================================

    /**
     * @brief Get optimized font for Chinese character display
     *
     * Intelligently selects the most suitable Chinese font, prioritizing recommended
     * fonts already installed on the system. If system fonts are unavailable or a
     * custom font path is specified, loads the custom font file.
     *
     * @param fontPath Custom font file path (optional, when empty only system fonts are used)
     * @return Configured QFont object that ensures proper Chinese character display
     *
     * @note Automatically configures UTF-8 encoding to ensure correct text rendering
     * @note Tries recommended Chinese fonts in priority order
     * @note Supports cross-platform font selection (Windows/Linux/macOS)
     */
    static QFont getChineseFont(const QString& fontPath = QString());

    /**
     * @brief Get list of all available system fonts
     * @return List of system font family names
     */
    static QStringList getAvailableFonts();

    /**
     * @brief Get list of recommended Chinese fonts
     *
     * Returns a list of Chinese fonts sorted by priority, covering common
     * Chinese fonts across Windows, Linux, macOS and other platforms to
     * ensure optimal Chinese text display.
     *
     * @return List of recommended Chinese font names, sorted by display quality and compatibility
     */
    static QStringList getRecommendedChineseFonts();

    //=============================================================================
    // File and System Operations
    //=============================================================================

    /**
     * @brief Open help document using system default program
     *
     * Cross-platform file opening function that automatically uses the system's
     * default program associated with the specified file. Supports both relative
     * and absolute paths with detailed error feedback.
     *
     * @param helpFilePath Path to the help file (supports relative paths)
     * @param errorMsg Optional error message output parameter
     * @return True if file opened successfully, false otherwise
     *
     * @note Uses cmd command on Windows, xdg-open on Linux
     * @note Automatically checks if the file exists to avoid invalid file open attempts
     */
    static bool openHelpFile(const QString& helpFilePath, QString* errorMsg = nullptr);

    //=============================================================================
    // HTML Content Formatting
    //=============================================================================

    /**
     * @brief Format content as professional HTML
     *
     * Generates HTML content with modern CSS styles, suitable for tooltips,
     * status displays, and other UI elements. Supports title formatting,
     * multiple content sections, and various style classes including labels,
     * indentation, highlighting, and comments.
     *
     * @param title Title text, displayed at the top in bold blue
     * @param sections List of content sections, each displayed independently
     * @return Complete HTML string with embedded CSS styling
     *
     * @note Maximum width is limited to 400px to ensure good display in various environments
     * @note Supported CSS classes: .title, .section, .label, .indent, .highlight, .comment
     * @note Uses modern font stack to ensure consistency across different operating systems
     */
    static QString formatToHtml(const QString& title, const QStringList& sections);

    //=============================================================================
    // Qt Widget Styling
    //=============================================================================

    /**
     * @brief Get modern tab widget CSS stylesheet
     *
     * Provides professional tab appearance with blue bottom edge indicator for
     * selected state, hover effects, and clear visual hierarchy. Suitable for
     * QTabWidget and QTabBar controls.
     *
     * @return Complete CSS stylesheet string for tab widgets
     *
     * @note Selected tabs feature blue bottom edge and bold text visual feedback
     * @note Unselected tabs have light gray background, with medium gray on hover
     * @note Includes 5px padding to ensure comfortable text display
     */
    static QString getTabStyle();

    /**
     * @brief Get unified stylesheet for button container widgets
     *
     * Provides modern appearance for containers with multiple buttons,
     * including borderless button design, hover highlight effects, and
     * pressed state feedback. Suitable for toolbars and button groups.
     *
     * @return CSS stylesheet string for button container widgets
     *
     * @note Container has light gray background (#f0f0f0)
     * @note Buttons have 3px margin and padding, providing good visual spacing
     * @note Hovering shows light blue border, pressing shows light blue background
     */
    static QString getButtonWidgetStyle();

    //=============================================================================
    // Configuration and Data Support
    //=============================================================================

    /**
     * @brief Get list of supported file encoding formats
     *
     * Returns common file encoding formats suitable for encoding selection
     * in file import/export features. Includes Unicode standard encodings
     * and Chinese encoding formats, sorted by frequency of use.
     *
     * @return List of encoding names (UTF-8, GB2312, ASCII, GBK, etc.)
     */
    static QStringList getFileEncodings();

    //=============================================================================
    // Status Bar and Dialog Utilities
    //=============================================================================

    /**
     * @brief Set version information display in status bar
     *
     * Adds a version information label to the right side of the specified
     * status bar, providing persistent version information display. The version
     * information is added as a permanent widget that won't be overwritten by
     * temporary status messages.
     *
     * @param statusBar Target status bar pointer
     * @param versionInfo Version information text to display
     *
     * @note Version info is displayed on the right side of the status bar, not affecting status messages on the left
     * @note If statusBar is nullptr, function safely returns without performing any operations
     */
    static void setStatusBarVersionInfo(QStatusBar *statusBar, const QString& versionInfo);

    /**
     * @brief Create a generic input dialog
     *
     * Provides a simplified text input dialog with consistent appearance and behavior.
     * Automatically removes the help button and adjusts size to fit content.
     *
     * @param parent Parent window pointer
     * @param title Dialog title
     * @param label Input prompt text (optional)
     * @param defaultText Default input content (optional)
     * @param isPassword Whether it's a password field (optional)
     * @return User input text, returns empty string if canceled
     *
     * @note Dialog automatically adjusts size to avoid geometry warnings
     * @note Removes standard context help button for a cleaner interface
     */
    static QString getInputText(QWidget *parent, const QString &title,
                                const QString &label = QString(),
                                const QString &defaultText = QString(),
                                bool isPassword = false);

    /**
     * @brief Set regular expression validator for input field
     *
     * Configures input validation and placeholder text for QLineEdit control,
     * ensuring user input conforms to expected format. The validator will
     * restrict users to only enter content that matches the regular expression.
     *
     * @param lineEdit Target input field pointer
     * @param regexString Regular expression string
     * @param placeholderText Placeholder hint text
     *
     * @note Validator object lifecycle is managed by lineEdit, no manual release needed
     * @note Placeholder text will be displayed when input field is empty, providing input format hints
     */
    static void setupInputValidator(QLineEdit* lineEdit, const QString& regexString,
                                   const QString& placeholderText);

    //=============================================================================
    // Table Widget Operations
    //=============================================================================

    /**
     * @brief Check if table has any row selected
     *
     * Verifies if the table widget has any selected rows, with optional
     * warning dialog display. Includes safety checks for empty tables
     * and invalid table pointers.
     *
     * @param table Table widget pointer
     * @param isWarnning Whether to display warning dialog when no row is selected
     * @return True if a row is selected, false otherwise
     *
     * @note Empty tables or nullptr safely return false without crashing
     * @note Warning dialog uses English text, can be localized as needed
     */
    static bool isTableRowSelected(QTableWidget* table, bool isWarnning = true);

    /**
     * @brief Batch set checkbox state for specified column in table
     *
     * Iterates through all rows in the table and uniformly sets the checked
     * state of checkbox items in the specified column. Only operates on
     * existing table items, ignoring empty items.
     *
     * @param table Table widget pointer
     * @param col Target column index
     * @param isChecked Whether checked (true for checked, false for unchecked)
     *
     * @note Only affects QTableWidgetItems with checkbox state
     * @note Performs safety checks for empty tables or invalid pointers to avoid crashes
     */
    static void setTableItemCheckState(QTableWidget* table, int col, bool isChecked);

    /**
     * @brief Remove currently selected table row
     *
     * Deletes the currently selected row in the table. Internally calls
     * isTableRowSelected for selection state validation, ensuring deletion
     * operation is only performed when a row is selected.
     *
     * @param table Table widget pointer
     *
     * @note If no row is selected, displays warning dialog (if warnings are enabled)
     * @note Table row indices are automatically adjusted after deletion
     */
    static void removeTableRow(QTableWidget* table);

    /**
     * @brief Move selected row to the top of the table
     * @param table Table widget pointer
     * @note Automatically selects the row at the target position after moving
     */
    static void moveTableRowToTop(QTableWidget* table);

    /**
     * @brief Move selected row up one position
     * @param table Table widget pointer
     * @note If already at the first row, no operation is performed
     */
    static void moveTableRowUp(QTableWidget* table);

    /**
     * @brief Move selected row down one position
     * @param table Table widget pointer
     * @note If already at the last row, no operation is performed
     */
    static void moveTableRowDown(QTableWidget* table);

    /**
     * @brief Move selected row to the bottom of the table
     * @param table Table widget pointer
     * @note Automatically selects the row at the target position after moving
     */
    static void moveTableRowToBottom(QTableWidget* table);

    /**
     * @brief Move a row to a specified position in the table
     *
     * Core row movement functionality supporting movement of any row to any position.
     * Correctly handles transfer of cell contents and widgets, maintaining data integrity.
     *
     * @param table Table widget pointer
     * @param fromRow Source row index
     * @param toRow Target row index
     *
     * @note Automatically handles source row index adjustment to correctly delete source row after insertion
     * @note Supports mixed movement of cell widgets and regular items
     */
    static void moveTableRow(QTableWidget* table, int fromRow, int toRow);

    /**
     * @brief Clear table contents (with user confirmation)
     *
     * Removes all rows from the table, displaying a confirmation dialog before
     * execution to prevent accidental data loss. Clearing operation is only
     * performed after user confirmation.
     *
     * @param table Table widget pointer
     * @return True if user confirmed and table was successfully cleared, false if user canceled or table was empty
     *
     * @note Confirmation dialog uses English text, can be localized as needed
     * @note Clearing operation cannot be undone, ensure users understand the consequences
     */
    static bool clearTable(QTableWidget* table);

    //=============================================================================
    // Toolbar and Button Utilities
    //=============================================================================

    /**
     * @brief Create standardized tool button
     *
     * Creates a QToolButton with consistent appearance and behavior, supporting
     * configuration of text, icon and tooltip. Allows selection of button display
     * style (icon only, text only, icon + text, etc.).
     *
     * @param text Button display text
     * @param tooltip Tooltip text
     * @param iconPath Icon file path
     * @param buttonStyle Button style (Qt::ToolButtonStyle enumeration value)
     * @param parent Parent window pointer
     * @return Configured QToolButton pointer
     *
     * @note Default style is Qt::ToolButtonTextBesideIcon (icon beside text)
     * @note Button lifecycle is managed by parent, if parent is nullptr manual management is required
     */
    static QToolButton* createToolButton(const QString& text, const QString& tooltip,
                                        const QString& iconPath,
                                        int buttonStyle = Qt::ToolButtonTextBesideIcon,
                                        QWidget *parent = nullptr);
};

} // namespace Common

#endif // COMMON_UI_UTILS_H
