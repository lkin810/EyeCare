// resource.h - EyeCare Resource Definitions v6.3
#pragma once

#ifndef IDC_STATIC
#define IDC_STATIC            -1
#endif

// Icon
#define IDI_APP               101

// Break reminder dialog (legacy, kept for compat)
#define IDD_BREAK             203

// Main window custom-drawn control IDs
#define IDC_BTN_FILTER        2001   // mode tab 0 (Gentle)
#define IDC_BTN_AUTOSTART     2002   // mode tab 1 (Office) — reused ID
#define IDC_SLIDER_FILTER     2003   // brightness slider
#define IDC_BTN_MINIMIZE      2009
#define IDC_BTN_CLOSE         2010
#define IDC_BTN_RESET         2011   // Break Now button

// Switch IDs
#define IDC_BTN_FILTER_SWITCH  2020  // Blue Light Filter switch
#define IDC_BTN_BREAK_SWITCH   2021  // Timed Break switch
#define IDC_BTN_AUTOSTART_SWITCH 2022 // Auto-start switch

// Break time adjuster buttons
#define IDC_BTN_HOUR_DEC      2030   // Hour -
#define IDC_BTN_HOUR_INC      2031   // Hour +
#define IDC_BTN_MIN_DEC       2032   // Minute -
#define IDC_BTN_MIN_INC       2033   // Minute +

// Break rest duration adjuster buttons (v6.3)
#define IDC_BTN_REST_SEC_DEC  2034   // Rest seconds -
#define IDC_BTN_REST_SEC_INC  2035   // Rest seconds +

// Preset buttons
#define IDC_BTN_PRESET_CUSTOM  2040  // Custom Strategy
#define IDC_BTN_PRESET_202020  2041  // 20-20-20 Strategy

// Stats button
#define IDC_BTN_STATS          2050  // Show stats popup

// Break dialog controls (legacy)
#define IDC_BREAK_TEXT        1101
#define IDC_BREAK_SKIP        1102
#define IDC_BREAK_PROGRESS    1103
#define IDC_BREAK_TIME        1104

// Tray menu commands
#define IDM_TOGGLE_FILTER     4001
#define IDM_TOGGLE_BREAK      4002
#define IDM_SHOW              4003
#define IDM_ABOUT             4004
#define IDM_EXIT              4005

// Custom message
#define WM_TRAYICON           (WM_USER + 100)
