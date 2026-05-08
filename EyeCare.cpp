// ============================================================================
//  EyeCare.cpp - EyeCare v6.0  ★ Pixel Paper Style
//  ★ 20-20-20 Break Strategy Preset
//  ★ Fullscreen Break Overlay (Pixel Flip Clock Style)
//  ★ Pixel Paper Style UI — Square Corners / 2px Border / Hard Offset Shadow / Grid Texture
//  ★ Double Buffer Drawing — Flicker-free
//  ★ DPI Aware — HiDPI Adaptive
//  ★ All Unicode (W) API
// ============================================================================

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <ctime>
#include <cstring>
#include <cmath>

#include "resource.h"

// ============================================================================
//  Colors — Pixel Paper Style (Warm Orange Series)
// ============================================================================

namespace C {
    // --- Warm Orange Primary ---
    constexpr DWORD O         = 0xFFE07840;   // primary warm orange
    constexpr DWORD O2        = 0xFFD46830;   // deep orange border
    constexpr DWORD O3        = 0xFFC05828;   // deeper orange shadow
    constexpr DWORD O4        = 0xFFF0A070;   // light orange
    constexpr DWORD O5        = 0xFFFFF0E8;   // very light orange hover background

    // --- Auxiliary Colors ---
    constexpr DWORD LAV       = 0xFFB088C0;   // lavender
    constexpr DWORD SAGE      = 0xFF70A080;   // sage green

    // --- Ink Colors (Text) ---
    constexpr DWORD INK       = 0xFF2C2416;   // ink body text
    constexpr DWORD INK_CORE  = 0xFF1A1510;   // deep ink
    constexpr DWORD I2        = 0xFF4A3F2E;   // medium ink
    constexpr DWORD I3        = 0xFF7A6F5A;   // light ink secondary text

    // --- Background Colors ---
    constexpr DWORD BG        = 0xFFFCF7F0;   // cream white main background
    constexpr DWORD BG2       = 0xFFF5EEE4;   // slightly deeper cream white
    constexpr DWORD SURFACE   = 0xFFFFFFFF;   // white card/panel

    // --- Border / Separator ---
    constexpr DWORD BORDER    = 0xFFD4C8B8;   // primary border color
    constexpr DWORD GRID      = 0x18D4C8B8;   // grid line (low opacity)

    // --- Pixel Hard Shadow ---
    constexpr DWORD SHADOW    = 0xFF8A7A68;   // warm brown shadow (card)
    constexpr DWORD SHADOW_DK = 0xFF5A4A38;   // deep brown shadow (button/emphasis)

    // --- Text Colors ---
    constexpr DWORD TEXT_MAIN = 0xFF2C2416;   // main text
    constexpr DWORD TEXT_DIM  = 0xFF9A8A78;   // secondary text
    constexpr DWORD TEXT_DARK = 0xFF7A6F5A;   // dimmer secondary text
    constexpr DWORD TEXT_ON_O = 0xFFFFFFFF;   // white on orange

    // --- Switch ---
    constexpr DWORD SWITCH_ON = 0xFFE07840;   // orange on
    constexpr DWORD SWITCH_OFF= 0xFFD4C8B8;   // border color off
    constexpr DWORD SWITCH_KNOB=0xFFFFFFFF;   // white knob

    // --- Slider ---
    constexpr DWORD TRACK_BG  = 0xFFF5EEE4;   // slider background
    constexpr DWORD TRACK_FILL= 0xFFE07840;   // slider fill

    // --- Button ---
    constexpr DWORD BTN_GHOST = 0x00FFFFFF;   // transparent
    constexpr DWORD BTN_MINUS = 0xFFF5EEE4;   // +/- button background
    constexpr DWORD BTN_MINUS_H=0xFFFFF0E8;   // +/- hover

    // --- Divider ---
    constexpr DWORD DIVIDER   = 0xFFD4C8B8;

    // --- Break Overlay ---
    constexpr DWORD OVERLAY_BG= 0xFF1A1510;   // deep ink background
    constexpr DWORD OVERLAY_O = 0xFFE07840;   // orange pixel
    constexpr DWORD OVERLAY_DIM=0xFF2C1A10;   // dark orange ghost
}

static Gdiplus::Color Argb(DWORD c) { return Gdiplus::Color(c); }

// ============================================================================
//  Layout Constants (96 DPI base)
// ============================================================================

#define TIMER_BREAK_CHECK   1
#define TIMER_BREAK_COUNT   2
#define TIMER_SCREEN_TIME   3
#define TIMER_COUNTDOWN     4

#define BREAK_CHECK_INTERVAL  60000
#define BREAK_COUNT_INTERVAL  1000
#define SCREEN_TIME_INTERVAL  60000
#define COUNTDOWN_INTERVAL    1000

// Window base size
#define BW  720
#define BH  580

// Section heights
#define HEADER_H      44
#define TIME_PANEL_H  110
#define FILTER_SEC_H  155
#define BREAK_SEC_H   148
#define AUTOSTART_H   50

// Margins / spacing
#define MX            30
#define MY            12
#define SECTION_GAP   14

// Break rest duration (seconds) per strategy
#define REST_DURATION_CUSTOM  30
#define REST_DURATION_202020  20

// ============================================================================
//  Settings
// ============================================================================

struct AppSettings {
    int  brightness;
    int  breakHours;
    int  breakMinutes;
    int  filterMode;
    int  breakPreset;
    BOOL enableFilter;
    BOOL enableBreak;
    BOOL autoStart;
};

// ============================================================================
//  Global State
// ============================================================================

static AppSettings      g_settings;
static HWND             g_hWndMain     = NULL;
static HWND             g_hWndBreak    = NULL;
static HICON            g_hIconApp     = NULL;
static NOTIFYICONDATAW  g_nid          = {};
static BOOL             g_filterActive = FALSE;
static WORD             g_originalRamp[3][256];
static BOOL             g_rampSaved    = FALSE;
static time_t           g_lastBreakEnd = 0;
static int              g_breakCountdown = 0;
static int              g_breakTotalTime = 0;
static int              g_screenMinutes = 0;
static int              g_nextBreakSec  = 0;
static WCHAR            g_exePath[MAX_PATH] = {};
static HINSTANCE        g_hInst        = NULL;
static ULONG_PTR        g_gdiplusToken = 0;
static float            g_dpiScale     = 1.0f;

// Double-buffer (main window)
static HBITMAP          g_hBufferBmp   = NULL;
static HDC              g_hBufferDC    = NULL;
static int              g_bufW = 0, g_bufH = 0;

// Double-buffer (break overlay)
static HBITMAP          g_hBreakBmp    = NULL;
static HDC              g_hBreakDC     = NULL;
static int              g_breakBufW = 0, g_breakBufH = 0;

// Interaction state
static BOOL             g_draggingSlider = FALSE;
static int              g_hoverBtn     = 0;
static int              g_hoverBreakBtn= 0;

// Single-instance mutex (defined later, forward-declared for WM_DESTROY)
static HANDLE           g_hMutex = NULL;

// DPI helper — round to nearest pixel for cleaner rendering
static int S(int v) { return (int)(v * g_dpiScale + 0.5f); }
static float SF(float v) { return v * g_dpiScale; }

// ============================================================================
//  Forward declarations
// ============================================================================

void LoadSettings();
void SaveSettings();
BOOL SetAutoStart(BOOL enable);
BOOL CheckAutoStart();
void ApplyFilter();
void RemoveFilter();
void UpdateTrayTooltip();
void ShowTrayBalloon(const WCHAR* title, const WCHAR* msg);
void ShowBreakOverlay();
void CloseBreakOverlay();
void InitTrayIcon();
void RemoveTrayIcon();
void PaintDoubleBuffer(HWND hWnd);
void DrawToBuffer(int W, int H);
void PaintBreakOverlay(HWND hWnd);
void DrawBreakToBuffer(int W, int H);
int  HitTest(int mx, int my);
void HandleSlider(int mx);
static int CalcSliderVal(int mx, int sliderX, int sliderW, int minVal, int maxVal);
void UpdateCountdown();
int  GetBreakIntervalSec();
int  GetBreakRestSec();

// ============================================================================
//  Double Buffer helpers
// ============================================================================

static void CreateBuffer(HDC screenDC, int w, int h, HBITMAP& hBmp, HDC& hDC, int& bw, int& bh) {
    if (hBmp) { DeleteObject(hBmp); hBmp = NULL; }
    if (hDC)  { DeleteDC(hDC);      hDC  = NULL; }
    hDC  = CreateCompatibleDC(screenDC);
    hBmp = CreateCompatibleBitmap(screenDC, w, h);
    SelectObject(hDC, hBmp);
    bw = w; bh = h;
}

static void FreeBuffer(HBITMAP& hBmp, HDC& hDC, int& bw, int& bh) {
    if (hBmp) { DeleteObject(hBmp); hBmp = NULL; }
    if (hDC)  { DeleteDC(hDC);      hDC  = NULL; }
    bw = 0; bh = 0;
}

// ============================================================================
//  Settings — Registry
// ============================================================================

static const WCHAR* REG_PATH     = L"Software\\EyeCare";
static const WCHAR* AUTORUN_KEY  = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const WCHAR* AUTORUN_NAME = L"EyeCare";

void LoadSettings() {
    g_settings.brightness    = 50;
    g_settings.breakHours    = 1;
    g_settings.breakMinutes  = 0;
    g_settings.filterMode    = 0;
    g_settings.breakPreset   = 0;
    g_settings.enableFilter  = TRUE;
    g_settings.enableBreak   = TRUE;
    g_settings.autoStart     = FALSE;

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;
    DWORD size = sizeof(DWORD), val;
    if (RegQueryValueExW(hKey, L"Brightness", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
        g_settings.brightness = (int)val;
    if (RegQueryValueExW(hKey, L"BreakHours", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
        g_settings.breakHours = (int)val;
    if (RegQueryValueExW(hKey, L"BreakMinutes", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
        g_settings.breakMinutes = (int)val;
    if (RegQueryValueExW(hKey, L"FilterMode", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
        g_settings.filterMode = (int)val;
    if (RegQueryValueExW(hKey, L"BreakPreset", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
        g_settings.breakPreset = (int)val;
    size = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"EnableFilter", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
        g_settings.enableFilter = val ? TRUE : FALSE;
    if (RegQueryValueExW(hKey, L"EnableBreak", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
        g_settings.enableBreak = val ? TRUE : FALSE;
    g_settings.autoStart = CheckAutoStart();
    RegCloseKey(hKey);

    // Boundary validation — protect against corrupted registry values
    if (g_settings.brightness < 0 || g_settings.brightness > 100) g_settings.brightness = 50;
    if (g_settings.breakHours < 0 || g_settings.breakHours > 6) g_settings.breakHours = 1;
    if (g_settings.breakMinutes < 0 || g_settings.breakMinutes > 55) g_settings.breakMinutes = 0;
    if (g_settings.filterMode < 0 || g_settings.filterMode > 1) g_settings.filterMode = 0;
    if (g_settings.breakPreset < 0 || g_settings.breakPreset > 1) g_settings.breakPreset = 0;
}

void SaveSettings() {
    HKEY hKey; DWORD disp;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disp) != ERROR_SUCCESS)
        return;
    DWORD val;
    val = (DWORD)g_settings.brightness;   RegSetValueExW(hKey, L"Brightness", 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
    val = (DWORD)g_settings.breakHours;   RegSetValueExW(hKey, L"BreakHours", 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
    val = (DWORD)g_settings.breakMinutes; RegSetValueExW(hKey, L"BreakMinutes", 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
    val = (DWORD)g_settings.filterMode;   RegSetValueExW(hKey, L"FilterMode", 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
    val = (DWORD)g_settings.breakPreset;  RegSetValueExW(hKey, L"BreakPreset", 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
    val = g_settings.enableFilter ? 1 : 0; RegSetValueExW(hKey, L"EnableFilter", 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
    val = g_settings.enableBreak ? 1 : 0;  RegSetValueExW(hKey, L"EnableBreak", 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
    RegCloseKey(hKey);
    SetAutoStart(g_settings.autoStart);
}

BOOL SetAutoStart(BOOL enable) {
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, AUTORUN_KEY, 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS) return FALSE;
    if (enable) {
        WCHAR quoted[MAX_PATH + 4] = {};
        wsprintfW(quoted, L"\"%s\"", g_exePath);
        result = RegSetValueExW(hKey, AUTORUN_NAME, 0, REG_SZ,
            (const BYTE*)quoted, (DWORD)((wcslen(quoted) + 1) * sizeof(WCHAR)));
        RegCloseKey(hKey); return (result == ERROR_SUCCESS);
    } else {
        result = RegDeleteValueW(hKey, AUTORUN_NAME);
        RegCloseKey(hKey); return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
    }
}

BOOL CheckAutoStart() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, AUTORUN_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return FALSE;
    WCHAR val[MAX_PATH + 4] = {};
    DWORD size = sizeof(val);
    LONG result = RegQueryValueExW(hKey, AUTORUN_NAME, NULL, NULL, (LPBYTE)val, &size);
    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) return FALSE;
    WCHAR expected[MAX_PATH + 4] = {};
    wsprintfW(expected, L"\"%s\"", g_exePath);
    return (wcscmp(val, expected) == 0);
}

// ============================================================================
//  Break interval/rest helpers
// ============================================================================

int GetBreakIntervalSec() {
    if (g_settings.breakPreset == 1) return 20 * 60;
    return (g_settings.breakHours * 60 + g_settings.breakMinutes) * 60;
}

int GetBreakRestSec() {
    if (g_settings.breakPreset == 1) return REST_DURATION_202020;
    return REST_DURATION_CUSTOM;
}

// ============================================================================
//  Blue Light Filter — Gamma Ramp
// ============================================================================

static void GetFilterParams(int mode, double& redS, double& greenS, double& blueS) {
    double strength = g_settings.brightness / 100.0;
    switch (mode) {
    case 0: redS = 1.0+strength*0.08; greenS = 1.0-strength*0.04; blueS = 1.0-strength*0.50; break;
    case 1: redS = 1.0+strength*0.03; greenS = 1.0; blueS = 1.0-strength*0.30; break;
    default: redS = 1.0; greenS = 1.0; blueS = 1.0-strength*0.40;
    }
}

void ApplyFilter() {
    if (!g_settings.enableFilter) { RemoveFilter(); return; }
    HDC hDC = GetDC(NULL);
    if (!hDC) return;
    if (!g_rampSaved) {
        if (GetDeviceGammaRamp(hDC, g_originalRamp)) g_rampSaved = TRUE;
        else { ReleaseDC(NULL, hDC); return; }
    }
    double redS, greenS, blueS;
    GetFilterParams(g_settings.filterMode, redS, greenS, blueS);
    WORD ramp[3][256];
    for (int i = 0; i < 256; i++) {
        double r = g_originalRamp[0][i] * redS;
        double g = g_originalRamp[1][i] * greenS;
        double b = g_originalRamp[2][i] * blueS;
        ramp[0][i] = (WORD)(r > 65535 ? 65535 : r < 0 ? 0 : r);
        ramp[1][i] = (WORD)(g > 65535 ? 65535 : g < 0 ? 0 : g);
        ramp[2][i] = (WORD)(b > 65535 ? 65535 : b < 0 ? 0 : b);
    }
    if (SetDeviceGammaRamp(hDC, ramp)) g_filterActive = TRUE;
    ReleaseDC(NULL, hDC);
    UpdateTrayTooltip();
}

void RemoveFilter() {
    if (!g_rampSaved) return;
    HDC hDC = GetDC(NULL);
    if (!hDC) return;
    SetDeviceGammaRamp(hDC, g_originalRamp);
    g_filterActive = FALSE;
    ReleaseDC(NULL, hDC);
    UpdateTrayTooltip();
}

// ============================================================================
//  Countdown
// ============================================================================

void UpdateCountdown() {
    if (!g_settings.enableBreak) { g_nextBreakSec = 0; return; }
    int intervalSec = GetBreakIntervalSec();
    if (intervalSec <= 0) { g_nextBreakSec = 0; return; }
    time_t now = time(NULL);
    double elapsed = difftime(now, g_lastBreakEnd);
    g_nextBreakSec = intervalSec - (int)elapsed;
    if (g_nextBreakSec < 0) g_nextBreakSec = 0;
}

// ============================================================================
//  Tray Icon
// ============================================================================

void InitTrayIcon() {
    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd   = g_hWndMain;
    g_nid.uID    = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_hIconApp = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APP));
    if (!g_hIconApp) g_hIconApp = (HICON)LoadImageW(NULL, MAKEINTRESOURCEW(32512), IMAGE_ICON, 0, 0, LR_SHARED);
    g_nid.hIcon = g_hIconApp;
    wcscpy(g_nid.szTip, L"\x62a4\x773c\x52a9\x624b");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void UpdateTrayTooltip() {
    WCHAR tip[128];
    wsprintfW(tip, L"\x62a4\x773c\x52a9\x624b | \x84dd\x5149:%s | \x4eae\x5ea6:%d%%",
              g_settings.enableFilter ? L"\x5f00" : L"\x5173", g_settings.brightness);
    wcsncpy(g_nid.szTip, tip, 63); g_nid.szTip[63] = L'\0';
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

void ShowTrayBalloon(const WCHAR* title, const WCHAR* msg) {
    g_nid.uFlags = NIF_INFO;
    wcsncpy(g_nid.szInfoTitle, title, 63); g_nid.szInfoTitle[63] = L'\0';
    wcsncpy(g_nid.szInfo, msg, 255); g_nid.szInfo[255] = L'\0';
    g_nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
}

void RemoveTrayIcon() { Shell_NotifyIconW(NIM_DELETE, &g_nid); }

// ============================================================================
//  Fullscreen Break Overlay
// ============================================================================

static const WCHAR* BREAK_WND_CLASS = L"EyeCareBreakOverlay";

LRESULT CALLBACK BreakOverlayProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        PaintBreakOverlay(hWnd);
        return 0;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right, H = rc.bottom;
        int btnW = 200, btnH = 48;
        int btnX = (W - btnW) / 2;
        int btnY = H - 120;
        if (mx >= btnX && mx <= btnX+btnW && my >= btnY && my <= btnY+btnH)
            CloseBreakOverlay();
        return 0;
    }
    case WM_MOUSEMOVE: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right, H = rc.bottom;
        int btnW = 200, btnH = 48;
        int btnX = (W - btnW) / 2;
        int btnY = H - 120;
        int nh = 0;
        if (mx >= btnX && mx <= btnX+btnW && my >= btnY && my <= btnY+btnH) nh = 1;
        if (nh != g_hoverBreakBtn) {
            g_hoverBreakBtn = nh;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        SetCursor(LoadCursorW(NULL, nh ? MAKEINTRESOURCEW(32649) : MAKEINTRESOURCEW(32512)));
        return 0;
    }
    case WM_TIMER:
        if (wParam == TIMER_BREAK_COUNT) {
            g_breakCountdown--;
            if (g_breakCountdown <= 0) {
                CloseBreakOverlay();
                ShowTrayBalloon(
                    L"\x4f11\x606f\x7ed3\x675f",
                    L"\x4f11\x606f\x5b8c\x6bd5\uff0c\x7ee7\x7eed\x52a0\x6cb9\uff01");
                MessageBeep(MB_ICONINFORMATION);
                return 0;
            }
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) { CloseBreakOverlay(); return 0; }
        break;
    case WM_CLOSE:
        CloseBreakOverlay();
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void ShowBreakOverlay() {
    if (g_hWndBreak && IsWindow(g_hWndBreak)) { SetForegroundWindow(g_hWndBreak); return; }
    g_breakTotalTime = GetBreakRestSec();
    g_breakCountdown = g_breakTotalTime;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    g_hWndBreak = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW, BREAK_WND_CLASS,
        L"\x62a4\x773c\x52a9\x624b - \x4f11\x606f",
        WS_POPUP | WS_VISIBLE, 0, 0, screenW, screenH,
        NULL, NULL, g_hInst, NULL);
    if (g_hWndBreak) {
        ShowWindow(g_hWndBreak, SW_SHOW);
        SetForegroundWindow(g_hWndBreak);
        MessageBeep(MB_ICONEXCLAMATION);
        SetTimer(g_hWndBreak, TIMER_BREAK_COUNT, BREAK_COUNT_INTERVAL, NULL);
    }
}

void CloseBreakOverlay() {
    if (g_hWndBreak && IsWindow(g_hWndBreak)) {
        KillTimer(g_hWndBreak, TIMER_BREAK_COUNT);
        DestroyWindow(g_hWndBreak);
        g_hWndBreak = NULL;
    }
    FreeBuffer(g_hBreakBmp, g_hBreakDC, g_breakBufW, g_breakBufH);
    g_lastBreakEnd = time(NULL);
    UpdateCountdown();
    if (g_hWndMain) InvalidateRect(g_hWndMain, NULL, FALSE);
}

void PaintBreakOverlay(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT rc; GetClientRect(hWnd, &rc);
    int W = rc.right - rc.left;
    int H = rc.bottom - rc.top;
    if (!g_hBreakDC || g_breakBufW != W || g_breakBufH != H)
        CreateBuffer(hdc, W, H, g_hBreakBmp, g_hBreakDC, g_breakBufW, g_breakBufH);
    DrawBreakToBuffer(W, H);
    BitBlt(hdc, 0, 0, W, H, g_hBreakDC, 0, 0, SRCCOPY);
    EndPaint(hWnd, &ps);
}

// ============================================================================
//  Pixel Digit Drawing — 5x7 pixel font for flip-clock display
// ============================================================================

static const BYTE g_pixelDigits[10][7] = {
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x0E,0x11,0x01,0x0E,0x10,0x10,0x1F},
    {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E},
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
};

static void DrawPixelDigit(Gdiplus::Graphics* g, int ox, int oy,
                           int digit, int pxSize, int gap,
                           Gdiplus::Brush* onBrush, Gdiplus::Brush* offBrush) {
    if (digit < 0 || digit > 9) return;
    const BYTE* rows = g_pixelDigits[digit];
    int cellSize = pxSize + gap;
    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 5; c++) {
            int px = ox + c * cellSize;
            int py = oy + r * cellSize;
            BOOL on = (rows[r] >> (4 - c)) & 1;
            g->FillRectangle(on ? onBrush : offBrush, px, py, pxSize, pxSize);
        }
    }
}

static void DrawPixelColon(Gdiplus::Graphics* g, int ox, int oy, int pxSize, int gap,
                           BOOL visible, Gdiplus::Brush* onBrush, Gdiplus::Brush* offBrush) {
    int cellSize = pxSize + gap;
    int dotX = ox;
    int dotY1 = oy + 2 * cellSize;
    int dotY2 = oy + 4 * cellSize;
    Gdiplus::Brush* brush = visible ? onBrush : offBrush;
    g->FillRectangle(brush, dotX, dotY1, pxSize, pxSize);
    g->FillRectangle(brush, dotX, dotY2, pxSize, pxSize);
}

// ============================================================================
//  Break Overlay Drawing — Pixel Flip Clock Style (Orange Series)
// ============================================================================

void DrawBreakToBuffer(int W, int H) {
    if (!g_hBreakDC) return;
    Gdiplus::Graphics graphics(g_hBreakDC);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);

    // === Deep dark background ===
    Gdiplus::SolidBrush bgBrush(Argb(C::OVERLAY_BG));
    graphics.FillRectangle(&bgBrush, 0, 0, W, H);

    // === Subtle grid dots ===
    {
        Gdiplus::SolidBrush dotBrush(Argb(0xFF2A2018));
        int gridSpacing = 32;
        for (int y = gridSpacing; y < H; y += gridSpacing)
            for (int x = gridSpacing; x < W; x += gridSpacing)
                graphics.FillRectangle(&dotBrush, x, y, 2, 2);
    }

    // === Scanlines ===
    {
        Gdiplus::SolidBrush scanBrush(Argb(0x06000000));
        for (int y = 0; y < H; y += 4)
            graphics.FillRectangle(&scanBrush, 0, y, W, 1);
    }

    int cx = W / 2;
    int cy = H / 2 - 40;

    int pxSize = H / 55;
    if (pxSize < 6) pxSize = 6;
    if (pxSize > 20) pxSize = 20;
    int gap = (pxSize + 2) / 4;
    if (gap < 1) gap = 1;
    int cellSize = pxSize + gap;

    int digitW = 5 * cellSize;
    int digitH = 7 * cellSize;
    int colonW = 1 * cellSize;
    int digitGap = cellSize;

    int totalSec = g_breakCountdown;
    if (totalSec < 0) totalSec = 0;
    int m1 = (totalSec / 600) % 10;
    int m2 = (totalSec / 60) % 10;
    int s1 = (totalSec % 60) / 10;
    int s2 = totalSec % 10;

    int clockW = 4 * digitW + 3 * digitGap + colonW;
    int clockX = cx - clockW / 2;
    int clockY = cy - digitH / 2;

    // === Flip-clock card background (behind each digit pair) ===
    {
        Gdiplus::SolidBrush cardBg(Argb(0xFF2C2018));
        int cardPad = pxSize * 2;
        // Left card (minutes)
        graphics.FillRectangle(&cardBg,
            clockX - cardPad, clockY - cardPad,
            2*digitW + digitGap + cardPad*2, digitH + cardPad*2);
        // Right card (seconds)
        int rightX = clockX + 2*digitW + 2*digitGap + colonW;
        graphics.FillRectangle(&cardBg,
            rightX - cardPad, clockY - cardPad,
            2*digitW + digitGap + cardPad*2, digitH + cardPad*2);

        // Card borders (2px hard border)
        Gdiplus::Pen cardBorder(Argb(C::OVERLAY_O), 2);
        graphics.DrawRectangle(&cardBorder,
            clockX - cardPad, clockY - cardPad,
            2*digitW + digitGap + cardPad*2, digitH + cardPad*2);
        graphics.DrawRectangle(&cardBorder,
            rightX - cardPad, clockY - cardPad,
            2*digitW + digitGap + cardPad*2, digitH + cardPad*2);

        // Flip line (horizontal center line on each card)
        Gdiplus::Pen flipLine(Argb(0xFF1A1510), 2);
        int midY = clockY + digitH / 2;
        graphics.DrawLine(&flipLine,
            clockX - cardPad, midY,
            clockX + 2*digitW + digitGap + cardPad, midY);
        graphics.DrawLine(&flipLine,
            rightX - cardPad, midY,
            rightX + 2*digitW + digitGap + cardPad, midY);

        // Hard offset shadow (3px right-down)
        Gdiplus::SolidBrush shadow(Argb(0xFF0A0804));
        int sOff = 3;
        // Left card shadow
        graphics.FillRectangle(&shadow,
            clockX - cardPad + sOff, clockY - cardPad + digitH + cardPad*2,
            2*digitW + digitGap + cardPad*2, sOff);
        graphics.FillRectangle(&shadow,
            clockX - cardPad + 2*digitW + digitGap + cardPad*2, clockY - cardPad + sOff,
            sOff, digitH + cardPad*2);
        // Right card shadow
        graphics.FillRectangle(&shadow,
            rightX - cardPad + sOff, clockY - cardPad + digitH + cardPad*2,
            2*digitW + digitGap + cardPad*2, sOff);
        graphics.FillRectangle(&shadow,
            rightX - cardPad + 2*digitW + digitGap + cardPad*2, clockY - cardPad + sOff,
            sOff, digitH + cardPad*2);
    }

    // Orange on / dark ghost off
    Gdiplus::SolidBrush onBrush(Argb(C::OVERLAY_O));
    Gdiplus::SolidBrush offBrush(Argb(C::OVERLAY_DIM));
    Gdiplus::SolidBrush colonOnBrush(Argb(C::OVERLAY_O));

    // Draw digits
    int dx = clockX;
    DrawPixelDigit(&graphics, dx, clockY, m1, pxSize, gap, &onBrush, &offBrush);
    dx += digitW + digitGap;
    DrawPixelDigit(&graphics, dx, clockY, m2, pxSize, gap, &onBrush, &offBrush);
    dx += digitW + digitGap;

    // Colon (blinks)
    {
        BOOL colonVisible = (totalSec % 2 == 0);
        DrawPixelColon(&graphics, dx + (colonW - pxSize) / 2, clockY, pxSize, gap,
                       colonVisible, &colonOnBrush, &offBrush);
        dx += colonW + digitGap;
    }

    DrawPixelDigit(&graphics, dx, clockY, s1, pxSize, gap, &onBrush, &offBrush);
    dx += digitW + digitGap;
    DrawPixelDigit(&graphics, dx, clockY, s2, pxSize, gap, &onBrush, &offBrush);

    // === Progress bar ===
    {
        int barY = clockY + digitH + pxSize * 4;
        int barH = 4;
        int barW = clockW;
        int barX = clockX;
        Gdiplus::SolidBrush barBg(Argb(0xFF2C2018));
        graphics.FillRectangle(&barBg, barX, barY, barW, barH);
        if (g_breakTotalTime > 0) {
            float progress = (float)g_breakCountdown / g_breakTotalTime;
            int fillW = (int)(barW * progress);
            if (fillW > 0) {
                Gdiplus::SolidBrush barFill(Argb(C::OVERLAY_O));
                graphics.FillRectangle(&barFill, barX, barY, fillW, barH);
            }
        }
    }

    // === "Take a Break" title ===
    {
        Gdiplus::Font titleFont(L"Microsoft YaHei", (float)(pxSize * 2.5), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush titleBrush(Argb(C::OVERLAY_O));
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::TextRenderingHint prev = graphics.GetTextRenderingHint();
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        graphics.DrawString(L"\x4f11\x606f\x4e00\x4e0b", -1, &titleFont,
            Gdiplus::RectF((float)(cx-200), (float)(clockY - pxSize*7), 400.0f, 50.0f), &sf, &titleBrush);
        graphics.SetTextRenderingHint(prev);

        // Decorative hard line
        Gdiplus::Pen linePen(Argb(0xFF4A3F2E), 2);
        int lineY = clockY - pxSize;
        graphics.DrawLine(&linePen, cx - clockW/2, lineY, cx + clockW/2, lineY);
    }

    // === Tip text ===
    {
        const WCHAR* tips[] = {
            L"\x8fdc\x770b\x7a97\x5916" L" 20 " L"\x82f1\x5c3a\x7684\x5730\x65b9",
            L"\x8ba9\x773c\x775b\x5f97\x5230\x5145\x5206\x4f11\x606f",
            L"\x8d77\x8eab\x6d3b\x52a8\xff0c\x62c9\x4f38\x8eab\x4f53",
        };
        int tipIdx = (g_breakTotalTime - g_breakCountdown) % 3;
        Gdiplus::Font tipFont(L"Microsoft YaHei", (float)(pxSize * 1.2), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush tipBrush(Argb(0xFF7A6F5A));
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        int tipY = clockY + digitH + pxSize * 6;
        Gdiplus::TextRenderingHint prev = graphics.GetTextRenderingHint();
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        graphics.DrawString(tips[tipIdx], -1, &tipFont,
            Gdiplus::RectF((float)(cx-300), (float)tipY, 600.0f, 30.0f), &sf, &tipBrush);

        // Blinking cursor
        Gdiplus::Font curFont(L"Consolas", (float)(pxSize * 1.0), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush curBrush(Argb((totalSec % 2) ? C::OVERLAY_O : 0x00E07840));
        graphics.DrawString(L"\x25ae", -1, &curFont,
            Gdiplus::RectF((float)(cx+280), (float)(tipY+2), 20.0f, 20.0f), &sf, &curBrush);
    }

    // === Skip button (pixel card style) ===
    {
        int btnW = 200, btnH = 48;
        int btnX = (W - btnW) / 2;
        int btnY = H - 120;
        BOOL hovered = (g_hoverBreakBtn == 1);

        // Hard offset shadow
        Gdiplus::SolidBrush btnShadow(Argb(0xFF0A0804));
        graphics.FillRectangle(&btnShadow, btnX+3, btnY+3, btnW, btnH);

        // Button background
        Gdiplus::SolidBrush btnBg(Argb(hovered ? C::OVERLAY_O : 0xFF2C2018));
        graphics.FillRectangle(&btnBg, btnX, btnY, btnW, btnH);

        // Border
        Gdiplus::Pen borderPen(Argb(C::OVERLAY_O), 2);
        graphics.DrawRectangle(&borderPen, btnX, btnY, btnW, btnH);

        // Corner pixels
        Gdiplus::SolidBrush cornerBrush(Argb(C::OVERLAY_O));
        int cs = 4;
        graphics.FillRectangle(&cornerBrush, btnX-cs, btnY-cs, cs, cs);
        graphics.FillRectangle(&cornerBrush, btnX+btnW, btnY-cs, cs, cs);
        graphics.FillRectangle(&cornerBrush, btnX-cs, btnY+btnH, cs, cs);
        graphics.FillRectangle(&cornerBrush, btnX+btnW, btnY+btnH, cs, cs);

        Gdiplus::Font btnFont(L"Microsoft YaHei", 16, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush btnText(Argb(hovered ? 0xFF1A1510 : C::OVERLAY_O));
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        graphics.DrawString(L"[ ESC ] \x8df3\x8fc7", -1, &btnFont,
            Gdiplus::RectF((float)btnX, (float)btnY, (float)btnW, (float)btnH), &sf, &btnText);
    }

    // === Bottom info ===
    {
        WCHAR info[64];
        wsprintfW(info, L"\x62a4\x773c\x52a9\x624b v6  |  \x5269\x4f59 %d \x79d2", totalSec);
        Gdiplus::Font infoFont(L"Microsoft YaHei", 11, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush infoBrush(Argb(0xFF4A3F2E));
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        graphics.DrawString(info, -1, &infoFont,
            Gdiplus::RectF((float)(cx-200), (float)(H-50), 400.0f, 20.0f), &sf, &infoBrush);
    }
}

// ============================================================================
//  GDI+ Pixel-Style Drawing Helpers
// ============================================================================

// CJK detection — must be before all drawing helpers that use it
static BOOL HasCJK(const WCHAR* text) {
    for (int i = 0; text[i]; i++) {
        WCHAR c = text[i];
        if ((c >= 0x4E00 && c <= 0x9FFF) ||   // CJK Unified
            (c >= 0x3400 && c <= 0x4DBF) ||   // CJK Ext-A
            (c >= 0x2E80 && c <= 0x2FDF) ||   // CJK Radicals
            (c >= 0x3000 && c <= 0x303F) ||   // CJK Symbols
            (c >= 0xFF00 && c <= 0xFFEF) ||   // Fullwidth
            (c >= 0xF900 && c <= 0xFAFF))      // CJK Compat
            return TRUE;
    }
    return FALSE;
}

// Pixel card: square rectangle + 2px border + 3px hard offset shadow
static void DrawPixelCard(Gdiplus::Graphics* g, int x, int y, int w, int h,
                          Gdiplus::Brush* fillBrush, Gdiplus::Brush* shadowBrush = NULL) {
    // Hard offset shadow (3px right, 3px down)
    if (shadowBrush) {
        g->FillRectangle(shadowBrush, x+3, y+3, w, h);
    }
    // Fill
    g->FillRectangle(fillBrush, x, y, w, h);
}

// Pixel border: 2px hard border on a rectangle
static void DrawPixelBorder(Gdiplus::Graphics* g, int x, int y, int w, int h, DWORD borderColor) {
    Gdiplus::Pen pen(Argb(borderColor), 2);
    g->DrawRectangle(&pen, x, y, w, h);
}

// Pixel switch: square track + square knob (no rounded corners)
static void DrawPixelSwitch(Gdiplus::Graphics* g, int x, int y, int w, int h, BOOL isOn) {
    // Track
    Gdiplus::SolidBrush track(Argb(isOn ? C::SWITCH_ON : C::SWITCH_OFF));
    Gdiplus::SolidBrush shadow(Argb(C::SHADOW));
    DrawPixelCard(g, x, y, w, h, &track, &shadow);
    DrawPixelBorder(g, x, y, w, h, isOn ? C::O2 : C::BORDER);

    // Knob (square)
    int knobSize = h - 8;
    int knobX = isOn ? (x + w - knobSize - 4) : (x + 4);
    int knobY = y + 4;
    Gdiplus::SolidBrush knob(Argb(C::SWITCH_KNOB));
    g->FillRectangle(&knob, knobX, knobY, knobSize, knobSize);
    Gdiplus::Pen knobBorder(Argb(C::BORDER), 1);
    g->DrawRectangle(&knobBorder, knobX, knobY, knobSize, knobSize);
}

// Pixel slider: square track + square thumb
static void DrawPixelSlider(Gdiplus::Graphics* g, int x, int y, int w, int h, int val, int maxVal) {
    float ratio = (float)val / maxVal;
    int fillW = (int)(w * ratio);

    // Track background
    Gdiplus::SolidBrush trackBg(Argb(C::TRACK_BG));
    Gdiplus::SolidBrush trackShadow(Argb(C::SHADOW));
    DrawPixelCard(g, x, y, w, h, &trackBg, &trackShadow);
    DrawPixelBorder(g, x, y, w, h, C::BORDER);

    // Fill
    if (fillW > 0) {
        Gdiplus::SolidBrush fill(Argb(C::TRACK_FILL));
        g->FillRectangle(&fill, x+1, y+1, fillW-1, h-2);
    }

    // Thumb (square, bigger than track height)
    int thumbSize = h + 10;
    int thumbX = x + fillW - thumbSize/2;
    if (thumbX < x) thumbX = x;
    if (thumbX > x + w - thumbSize) thumbX = x + w - thumbSize;
    int thumbY = y + h/2 - thumbSize/2;
    Gdiplus::SolidBrush thumbShadow(Argb(C::SHADOW_DK));
    g->FillRectangle(&thumbShadow, thumbX+2, thumbY+2, thumbSize, thumbSize);
    Gdiplus::SolidBrush thumb(Argb(C::O));
    g->FillRectangle(&thumb, thumbX, thumbY, thumbSize, thumbSize);
    DrawPixelBorder(g, thumbX, thumbY, thumbSize, thumbSize, C::O2);
    // Inner square (hollow look)
    Gdiplus::SolidBrush thumbInner(Argb(C::BG));
    g->FillRectangle(&thumbInner, thumbX+3, thumbY+3, thumbSize-6, thumbSize-6);
}

// Pixel small button (+/-)
static void DrawPixelSmallBtn(Gdiplus::Graphics* g, int x, int y, int w, int h,
                               const WCHAR* text, int hoverId, int myId) {
    BOOL hovered = (hoverId == myId);
    Gdiplus::SolidBrush bg(Argb(hovered ? C::O5 : C::BTN_MINUS));
    Gdiplus::SolidBrush shadow(Argb(C::SHADOW));
    DrawPixelCard(g, x, y, w, h, &bg, &shadow);
    DrawPixelBorder(g, x, y, w, h, hovered ? C::O2 : C::BORDER);

    Gdiplus::Font font(L"Consolas", SF(14), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush fg(Argb(hovered ? C::O : C::INK));
    Gdiplus::StringFormat sf;
    sf.SetAlignment(Gdiplus::StringAlignmentCenter);
    sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    g->DrawString(text, -1, &font, Gdiplus::RectF((float)x,(float)y,(float)w,(float)h), &sf, &fg);
}

// Pixel preset button (two-line label)
static void DrawPixelPresetBtn(Gdiplus::Graphics* g, int x, int y, int w, int h,
                                const WCHAR* label, const WCHAR* sub,
                                BOOL selected, int hoverId, int myId) {
    BOOL hovered = (hoverId == myId);

    if (selected) {
        // Selected: orange filled
        Gdiplus::SolidBrush fill(Argb(C::O));
        Gdiplus::SolidBrush shadow(Argb(C::SHADOW_DK));
        DrawPixelCard(g, x, y, w, h, &fill, &shadow);
        DrawPixelBorder(g, x, y, w, h, C::O2);
    } else {
        // Normal: white card
        Gdiplus::SolidBrush fill(Argb(hovered ? C::O5 : C::SURFACE));
        Gdiplus::SolidBrush shadow(Argb(C::SHADOW));
        DrawPixelCard(g, x, y, w, h, &fill, &shadow);
        DrawPixelBorder(g, x, y, w, h, C::BORDER);
    }

    DWORD labelColor = selected ? 0xFFFFFFFF : C::INK;
    BOOL labelCN = HasCJK(label);
    Gdiplus::Font lfont(labelCN ? L"Microsoft YaHei" : L"Consolas", SF(13), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush lbr(Argb(labelColor));
    Gdiplus::StringFormat sf;
    sf.SetAlignment(Gdiplus::StringAlignmentCenter);
    sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::TextRenderingHint prev = g->GetTextRenderingHint();
    g->SetTextRenderingHint(labelCN ? Gdiplus::TextRenderingHintAntiAliasGridFit : Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);
    g->DrawString(label, -1, &lfont, Gdiplus::RectF((float)x,(float)(y+S(2)),(float)w,(float)h/2), &sf, &lbr);

    DWORD subColor = selected ? 0xFFFFF0E8 : C::TEXT_DIM;
    BOOL subCN = HasCJK(sub);
    Gdiplus::Font sfont(subCN ? L"Microsoft YaHei" : L"Consolas", SF(10), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush sbr(Argb(subColor));
    g->SetTextRenderingHint(subCN ? Gdiplus::TextRenderingHintAntiAliasGridFit : Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);
    g->DrawString(sub, -1, &sfont, Gdiplus::RectF((float)x,(float)(y+h/2-S(4)),(float)w,(float)h/2), &sf, &sbr);
    g->SetTextRenderingHint(prev);
}

// Text helpers — auto CJK: Chinese→YaHei+AntiAlias, English→Consolas+Monochrome Pixel
static void DrawPxText(Gdiplus::Graphics* g, const WCHAR* text, float x, float y,
                       float size, DWORD color, BOOL bold = FALSE) {
    BOOL cn = HasCJK(text);
    Gdiplus::Font font(cn ? L"Microsoft YaHei" : L"Consolas", size,
                       bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular,
                       Gdiplus::UnitPixel);
    Gdiplus::TextRenderingHint prev = g->GetTextRenderingHint();
    g->SetTextRenderingHint(cn ? Gdiplus::TextRenderingHintAntiAliasGridFit
                               : Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);
    Gdiplus::SolidBrush brush(Argb(color));
    g->DrawString(text, -1, &font, Gdiplus::PointF(x, y), &brush);
    g->SetTextRenderingHint(prev);
}

static void DrawPxTextCenter(Gdiplus::Graphics* g, const WCHAR* text, int cx, int cy,
                              float size, DWORD color, BOOL bold = FALSE) {
    BOOL cn = HasCJK(text);
    Gdiplus::Font font(cn ? L"Microsoft YaHei" : L"Consolas", size,
                       bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular,
                       Gdiplus::UnitPixel);
    Gdiplus::TextRenderingHint prev = g->GetTextRenderingHint();
    g->SetTextRenderingHint(cn ? Gdiplus::TextRenderingHintAntiAliasGridFit
                               : Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);
    Gdiplus::SolidBrush brush(Argb(color));
    Gdiplus::RectF box;
    g->MeasureString(text, -1, &font, Gdiplus::PointF(0,0), &box);
    g->DrawString(text, -1, &font, Gdiplus::PointF((float)(cx - box.Width/2), (float)(cy - box.Height/2)), &brush);
    g->SetTextRenderingHint(prev);
}

// ============================================================================
//  Draw to buffer — Main Window (Pixel Paper Style)
// ============================================================================
//
//  Layout (720x580 base):
//    Header          44px   title + minimize + close
//    Time Panel     110px   used time | countdown | rest button
//    Filter Section 155px   title + mode tabs + switch + brightness slider
//    Break Section  148px   title + switch + presets + time adjuster
//    Auto-start      50px   title + switch
//

void DrawToBuffer(int W, int H) {
    if (!g_hBufferDC) return;
    Gdiplus::Graphics graphics(g_hBufferDC);
    // Pixel-perfect: no anti-aliasing
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);

    int M = S(MX);
    int curY = 0;

    // === Background: warm paper + grid lines ===
    {
        Gdiplus::SolidBrush bgBrush(Argb(C::BG));
        graphics.FillRectangle(&bgBrush, 0, 0, W, H);

        // 16px grid lines
        Gdiplus::Pen gridPen(Argb(C::GRID), 1);
        int gridStep = S(16);
        for (int x = gridStep; x < W; x += gridStep)
            graphics.DrawLine(&gridPen, x, 0, x, H);
        for (int y = gridStep; y < H; y += gridStep)
            graphics.DrawLine(&gridPen, 0, y, W, y);

        // Paper texture: very subtle horizontal lines every 19px
        Gdiplus::Pen paperPen(Argb(0x04000000), 1);
        int paperStep = S(19);
        for (int y = paperStep; y < H; y += paperStep)
            graphics.DrawLine(&paperPen, 0, y, W, y);
    }

    // ====================================================================
    //  Header (44px)
    // ====================================================================
    {
        int HH = S(HEADER_H);
        DrawPxText(&graphics, L"\x62a4\x773c\x52a9\x624b", (float)(M+S(4)), (float)S(12), SF(17), C::INK, TRUE);

        // Minimize button (pixel style)
        {
            int bx = W - S(90), by = S(10), bw = S(28), bh = S(28);
            BOOL hovered = (g_hoverBtn == IDC_BTN_MINIMIZE);
            if (hovered) {
                Gdiplus::SolidBrush hbg(Argb(C::O5));
                graphics.FillRectangle(&hbg, bx, by, bw, bh);
                DrawPixelBorder(&graphics, bx, by, bw, bh, C::O2);
            }
            Gdiplus::Pen minPen(Argb(hovered ? C::O : C::I3), 2);
            graphics.DrawLine(&minPen, bx+7, by+bh/2, bx+bw-7, by+bh/2);
        }
        // Close button (pixel style)
        {
            int bx = W - S(50), by = S(10), bw = S(28), bh = S(28);
            BOOL hovered = (g_hoverBtn == IDC_BTN_CLOSE);
            if (hovered) {
                Gdiplus::SolidBrush hbg(Argb(C::O));
                graphics.FillRectangle(&hbg, bx, by, bw, bh);
                DrawPixelBorder(&graphics, bx, by, bw, bh, C::O2);
            }
            Gdiplus::Pen closePen(Argb(hovered ? 0xFFFFFFFF : C::I3), 2);
            graphics.DrawLine(&closePen, bx+7, by+7, bx+bw-7, by+bh-7);
            graphics.DrawLine(&closePen, bx+bw-7, by+7, bx+7, by+bh-7);
        }
        curY = HH;
    }

    // ====================================================================
    //  Time Panel (110px) — pixel card style
    // ====================================================================
    {
        int TPH = S(TIME_PANEL_H);
        int panelX = M;
        int panelY = curY + S(8);
        int panelW = W - M*2;

        // White card with hard shadow
        Gdiplus::SolidBrush panelFill(Argb(C::SURFACE));
        Gdiplus::SolidBrush panelShadow(Argb(C::SHADOW));
        DrawPixelCard(&graphics, panelX, panelY, panelW, TPH, &panelFill, &panelShadow);
        DrawPixelBorder(&graphics, panelX, panelY, panelW, TPH, C::BORDER);

        int innerM = panelX + S(24);

        // Left: "Screen Time"
        DrawPxText(&graphics, L"\x5df2\x4f7f\x7528\x65f6\x95f4", (float)(innerM), (float)(panelY+S(14)), SF(12), C::TEXT_DIM);
        int hours = g_screenMinutes / 60;
        int mins  = g_screenMinutes % 60;
        WCHAR hStr[8], mStr[8];
        wsprintfW(hStr, L"%d", hours);
        wsprintfW(mStr, L"%02d", mins);
        DrawPxTextCenter(&graphics, hStr, innerM+S(40), panelY+S(62), SF(40), C::INK, TRUE);
        DrawPxTextCenter(&graphics, L":", innerM+S(90), panelY+S(58), SF(34), C::INK, TRUE);
        DrawPxTextCenter(&graphics, mStr, innerM+S(140), panelY+S(62), SF(40), C::INK, TRUE);
        DrawPxText(&graphics, L"\x5c0f\x65f6", (float)(innerM+S(58)), (float)(panelY+S(80)), SF(10), C::TEXT_DARK);
        DrawPxText(&graphics, L"\x5206\x949f", (float)(innerM+S(118)), (float)(panelY+S(80)), SF(10), C::TEXT_DARK);

        // Divider (dashed pixel line)
        int divX = panelX + S(310);
        Gdiplus::Pen divPen(Argb(C::BORDER), 2);
        graphics.DrawLine(&divPen, divX, panelY+S(18), divX, panelY+TPH-S(18));

        // Right: "Next Break"
        int rightX = divX + S(24);
        DrawPxText(&graphics, L"\x8ddd\x4e0b\x6b21\x4f11\x606f", (float)rightX, (float)(panelY+S(14)), SF(12), C::TEXT_DIM);

        if (g_settings.enableBreak && g_nextBreakSec > 0) {
            int nh = g_nextBreakSec / 3600;
            int nm = (g_nextBreakSec % 3600) / 60;
            int ns = g_nextBreakSec % 60;
            WCHAR t1[8], t2[8], t3[8];
            wsprintfW(t1, L"%d", nh); wsprintfW(t2, L"%02d", nm); wsprintfW(t3, L"%02d", ns);
            DrawPxTextCenter(&graphics, t1, rightX+S(20), panelY+S(50), SF(26), C::O, TRUE);
            DrawPxTextCenter(&graphics, L":", rightX+S(46), panelY+S(47), SF(20), C::O, TRUE);
            DrawPxTextCenter(&graphics, t2, rightX+S(72), panelY+S(50), SF(26), C::O, TRUE);
            DrawPxTextCenter(&graphics, L":", rightX+S(98), panelY+S(47), SF(20), C::O, TRUE);
            DrawPxTextCenter(&graphics, t3, rightX+S(124), panelY+S(50), SF(26), C::O, TRUE);
        } else {
            DrawPxText(&graphics, L"--:--:--", (float)(rightX+S(10)), (float)(panelY+S(42)), SF(22), C::TEXT_DARK, TRUE);
        }

        // "Break Now" button (pixel primary button)
        {
            int bx = W - M - S(115), by = panelY + S(32), bw = S(95), bh = S(46);
            BOOL hovered = (g_hoverBtn == IDC_BTN_RESET);
            Gdiplus::SolidBrush btnFill(Argb(hovered ? C::O2 : C::O));
            Gdiplus::SolidBrush btnShadow(Argb(C::SHADOW_DK));
            DrawPixelCard(&graphics, bx, by, bw, bh, &btnFill, &btnShadow);
            DrawPixelBorder(&graphics, bx, by, bw, bh, C::O2);
            Gdiplus::Font btnFont(L"Microsoft YaHei", SF(14), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::SolidBrush btnText(Argb(C::TEXT_ON_O));
            Gdiplus::StringFormat sf;
            sf.SetAlignment(Gdiplus::StringAlignmentCenter);
            sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::TextRenderingHint prev = graphics.GetTextRenderingHint();
            graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
            graphics.DrawString(L"\x7acb\x5373\x4f11\x606f", -1, &btnFont,
                Gdiplus::RectF((float)bx,(float)by,(float)bw,(float)bh), &sf, &btnText);
            graphics.SetTextRenderingHint(prev);
        }

        curY = panelY + TPH + S(SECTION_GAP);
    }

    // ====================================================================
    //  Blue Light Filter Section (155px) — pixel style
    // ====================================================================
    {
        int secH = S(FILTER_SEC_H);
        int secY = curY;
        int innerM = M + S(4);

        DrawPxText(&graphics, L"\x84dd\x5149\x8fc7\x6ee4", (float)innerM, (float)(secY+S(2)), SF(15), C::O, TRUE);
        DrawPixelSwitch(&graphics, W - M - S(84), secY, S(84), S(30), g_settings.enableFilter);

        // Mode tabs (pixel button style)
        int tabY = secY + S(42);
        int tabW = S(90), tabH = S(34), tabGap = S(12);
        int tabStartX = innerM;
        const WCHAR* modeNames[] = {L"\x6e29\x548c", L"\x529e\x516c"};
        for (int i = 0; i < 2; i++) {
            int tx = tabStartX + i * (tabW + tabGap);
            BOOL sel = (g_settings.filterMode == i);
            if (sel) {
                Gdiplus::SolidBrush fill(Argb(C::O));
                Gdiplus::SolidBrush shadow(Argb(C::SHADOW_DK));
                DrawPixelCard(&graphics, tx, tabY, tabW, tabH, &fill, &shadow);
                DrawPixelBorder(&graphics, tx, tabY, tabW, tabH, C::O2);
            } else {
                Gdiplus::SolidBrush fill(Argb(C::SURFACE));
                Gdiplus::SolidBrush shadow(Argb(C::SHADOW));
                DrawPixelCard(&graphics, tx, tabY, tabW, tabH, &fill, &shadow);
                DrawPixelBorder(&graphics, tx, tabY, tabW, tabH, C::BORDER);
            }
            Gdiplus::Font tabFont(L"Microsoft YaHei", SF(13), sel ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::SolidBrush tabFg(Argb(sel ? C::TEXT_ON_O : C::INK));
            Gdiplus::StringFormat sf;
            sf.SetAlignment(Gdiplus::StringAlignmentCenter);
            sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::TextRenderingHint prev = graphics.GetTextRenderingHint();
            graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
            graphics.DrawString(modeNames[i], -1, &tabFont, Gdiplus::RectF((float)tx,(float)tabY,(float)tabW,(float)tabH), &sf, &tabFg);
            graphics.SetTextRenderingHint(prev);
        }

        // Mode description
        const WCHAR* modeDesc[] = {
            L"\x51cf\x5c11\x84dd\x5149\x8f83\x591a\xff0c\x9002\x5408\x665a\x95f4",
            L"\x8f7b\x5ea6\x6ee4\x5149\xff0c\x4fdd\x6301\x8272\x5f69"
        };
        DrawPxText(&graphics, modeDesc[g_settings.filterMode],
                 (float)(tabStartX + 2*(tabW+tabGap) + S(8)), (float)(tabY+S(8)), SF(11), C::TEXT_DIM);

        // Brightness slider
        int sliderY = tabY + tabH + S(22);
        WCHAR pctText[16]; wsprintfW(pctText, L"%d%%", g_settings.brightness);
        DrawPxText(&graphics, L"\x4eae\x5ea6", (float)innerM, (float)sliderY, SF(13), C::O);
        DrawPxText(&graphics, pctText, (float)(W - M - S(40)), (float)sliderY, SF(20), C::O, TRUE);

        sliderY += S(30);
        int sliderW = W - M*2 - S(20);
        int sliderX = M + S(10);
        DrawPixelSlider(&graphics, sliderX, sliderY, sliderW, S(10), g_settings.brightness, 100);

        curY = secY + secH + S(4);
    }

    // ====================================================================
    //  Break Settings Section (148px) — pixel style
    // ====================================================================
    {
        // Divider
        Gdiplus::Pen divPen(Argb(C::BORDER), 2);
        graphics.DrawLine(&divPen, M, curY, W - M, curY);
        curY += S(SECTION_GAP);

        int secH = S(BREAK_SEC_H);
        int secY = curY;
        int innerM = M + S(4);

        DrawPxText(&graphics, L"\x5b9a\x65f6\x4f11\x606f", (float)innerM, (float)(secY+S(2)), SF(15), C::O, TRUE);
        DrawPixelSwitch(&graphics, W - M - S(84), secY, S(84), S(30), g_settings.enableBreak);

        // Preset buttons
        int presetY = secY + S(40);
        int presetW = S(120), presetH = S(44), presetGap = S(12);
        int presetX = innerM;

        DrawPixelPresetBtn(&graphics, presetX, presetY, presetW, presetH,
            L"\x81ea\x5b9a\x4e49",
            L"\x624b\x52a8\x8bbe\x7f6e",
            g_settings.breakPreset == 0, g_hoverBtn, IDC_BTN_PRESET_CUSTOM);

        DrawPixelPresetBtn(&graphics, presetX + presetW + presetGap, presetY, presetW, presetH,
            L"20-20-20",
            L"\x6bcf" L"20" L"\x5206\x949f\x4f11" L"20" L"\x79d2",
            g_settings.breakPreset == 1, g_hoverBtn, IDC_BTN_PRESET_202020);

        // Time adjuster (custom mode)
        int rowY = presetY + presetH + S(14);
        if (g_settings.breakPreset == 0) {
            DrawPxText(&graphics, L"\x4f11\x606f\x95f4\x9694", (float)innerM, (float)(rowY+S(4)), SF(13), C::O);

            // Hours adjuster
            {
                int adjX = innerM + S(90);
                int btnW = S(30), btnH = S(30), valW = S(36);
                DrawPixelSmallBtn(&graphics, adjX, rowY, btnW, btnH, L"-", g_hoverBtn, IDC_BTN_HOUR_DEC);
                WCHAR hVal[8]; wsprintfW(hVal, L"%d", g_settings.breakHours);
                DrawPxTextCenter(&graphics, hVal, adjX + btnW + valW/2, rowY + btnH/2, SF(16), C::INK, TRUE);
                DrawPixelSmallBtn(&graphics, adjX + btnW + valW, rowY, btnW, btnH, L"+", g_hoverBtn, IDC_BTN_HOUR_INC);
                DrawPxText(&graphics, L"\x5c0f\x65f6", (float)(adjX + 2*btnW + valW + S(6)), (float)(rowY+S(6)), SF(13), C::TEXT_DIM);
            }

            // Minutes adjuster
            {
                int adjX = innerM + S(90) + S(180);
                int btnW = S(30), btnH = S(30), valW = S(36);
                DrawPixelSmallBtn(&graphics, adjX, rowY, btnW, btnH, L"-", g_hoverBtn, IDC_BTN_MIN_DEC);
                WCHAR mVal[8]; wsprintfW(mVal, L"%d", g_settings.breakMinutes);
                DrawPxTextCenter(&graphics, mVal, adjX + btnW + valW/2, rowY + btnH/2, SF(16), C::INK, TRUE);
                DrawPixelSmallBtn(&graphics, adjX + btnW + valW, rowY, btnW, btnH, L"+", g_hoverBtn, IDC_BTN_MIN_INC);
                DrawPxText(&graphics, L"\x5206\x949f", (float)(adjX + 2*btnW + valW + S(6)), (float)(rowY+S(6)), SF(13), C::TEXT_DIM);
            }

            DrawPxText(&graphics, L"\x5efa\x8bae\x6bcf 45~60 \x5206\x949f\x4f11\x606f\x4e00\x6b21",
                     (float)innerM, (float)(rowY+S(40)), SF(11), C::TEXT_DARK);
        } else {
            DrawPxText(&graphics,
                L"\x6bcf\x4f7f\x7528" L" 20 " L"\x5206\x949f\x5c4f\x5e55\xef0c\x4f11\x606f" L" 20 " L"\x79d2\xef0c\x8fdc\x770b" L" 20 " L"\x82f1\x5c3a",
                (float)(innerM+S(4)), (float)(rowY+S(6)), SF(12), C::TEXT_DIM);
            DrawPxText(&graphics,
                L"\x7f8e\x56fd\x773c\x79d1\x5b66\x4f1a\x63a8\x8350\x7684\x62a4\x773c\x65b9\x6cd5",
                (float)(innerM+S(4)), (float)(rowY+S(28)), SF(11), C::TEXT_DARK);
        }

        curY = secY + secH + S(4);
    }

    // ====================================================================
    //  Auto-start Section (50px) — pixel style
    // ====================================================================
    {
        Gdiplus::Pen divPen(Argb(C::BORDER), 2);
        graphics.DrawLine(&divPen, M, curY, W - M, curY);
        curY += S(SECTION_GAP);

        int innerM = M + S(4);
        DrawPxText(&graphics, L"\x5f00\x673a\x81ea\x542f", (float)innerM, (float)(curY+S(4)), SF(15), C::O, TRUE);
        DrawPxText(&graphics, L"\x7cfb\x7edf\x542f\x52a8\x65f6\x81ea\x52a8\x8fd0\x884c",
                 (float)(innerM+S(100)), (float)(curY+S(8)), SF(11), C::TEXT_DIM);
        DrawPixelSwitch(&graphics, W - M - S(84), curY, S(84), S(30), g_settings.autoStart);
    }
}

void PaintDoubleBuffer(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT rc; GetClientRect(hWnd, &rc);
    int W = rc.right - rc.left;
    int H = rc.bottom - rc.top;
    if (!g_hBufferDC || g_bufW != W || g_bufH != H)
        CreateBuffer(hdc, W, H, g_hBufferBmp, g_hBufferDC, g_bufW, g_bufH);
    DrawToBuffer(W, H);
    BitBlt(hdc, 0, 0, W, H, g_hBufferDC, 0, 0, SRCCOPY);
    EndPaint(hWnd, &ps);
}

// ============================================================================
//  Hit Testing — Main Window (unchanged logic)
// ============================================================================

static int CalcSliderVal(int mx, int sliderX, int sliderW, int minVal, int maxVal) {
    int relX = mx - sliderX;
    if (relX < 0) relX = 0;
    if (relX > sliderW) relX = sliderW;
    float ratio = (float)relX / sliderW;
    return minVal + (int)(ratio * (maxVal - minVal));
}

int HitTest(int mx, int my) {
    int M = S(MX);
    RECT rc; GetClientRect(g_hWndMain, &rc);
    int W = rc.right;
    int curY = 0;

    // --- Header ---
    {
        int HH = S(HEADER_H);
        if (mx >= W-S(50) && mx <= W-S(22) && my >= S(10) && my <= S(38))
            return IDC_BTN_CLOSE;
        if (mx >= W-S(90) && mx <= W-S(62) && my >= S(10) && my <= S(38))
            return IDC_BTN_MINIMIZE;
        if (my < HH) return -1;
        curY = HH;
    }

    // --- Time Panel ---
    {
        int TPH = S(TIME_PANEL_H);
        int panelY = curY + S(8);
        int bx = W - M - S(115), by = panelY + S(32), bw = S(95), bh = S(46);
        if (mx >= bx && mx <= bx+bw && my >= by && my <= by+bh)
            return IDC_BTN_RESET;
        curY = panelY + TPH + S(SECTION_GAP);
    }

    // --- Filter Section ---
    {
        int secY = curY;
        int innerM = M + S(4);
        if (mx >= W-M-S(84) && mx <= W-M && my >= secY && my <= secY+S(30))
            return IDC_BTN_FILTER_SWITCH;

        int tabY = secY + S(42);
        int tabW = S(90), tabH = S(34), tabGap = S(12);
        for (int i = 0; i < 2; i++) {
            int tx = innerM + i * (tabW + tabGap);
            if (mx >= tx && mx <= tx+tabW && my >= tabY && my <= tabY+tabH)
                return IDC_BTN_FILTER + i;
        }

        int sliderY = tabY + tabH + S(52);
        int sliderW = W - M*2 - S(20);
        int sliderX = M + S(10);
        if (mx >= sliderX-S(5) && mx <= sliderX+sliderW+S(5) && my >= sliderY-S(12) && my <= sliderY+S(22))
            return IDC_SLIDER_FILTER;

        curY = secY + S(FILTER_SEC_H) + S(4);
    }

    // --- Break Section ---
    {
        curY += S(SECTION_GAP);
        int secY = curY;
        int innerM = M + S(4);

        if (mx >= W-M-S(84) && mx <= W-M && my >= secY && my <= secY+S(30))
            return IDC_BTN_BREAK_SWITCH;

        int presetY = secY + S(40);
        int presetW = S(120), presetH = S(44), presetGap = S(12);
        int presetX = innerM;
        if (mx >= presetX && mx <= presetX+presetW && my >= presetY && my <= presetY+presetH)
            return IDC_BTN_PRESET_CUSTOM;
        if (mx >= presetX+presetW+presetGap && mx <= presetX+2*presetW+presetGap && my >= presetY && my <= presetY+presetH)
            return IDC_BTN_PRESET_202020;

        if (g_settings.breakPreset == 0) {
            int rowY = presetY + presetH + S(14);
            int btnW = S(30), btnH = S(30), valW = S(36);
            {
                int adjX = innerM + S(90);
                if (mx >= adjX && mx <= adjX+btnW && my >= rowY && my <= rowY+btnH)
                    return IDC_BTN_HOUR_DEC;
                if (mx >= adjX+btnW+valW && mx <= adjX+2*btnW+valW && my >= rowY && my <= rowY+btnH)
                    return IDC_BTN_HOUR_INC;
            }
            {
                int adjX = innerM + S(90) + S(180);
                if (mx >= adjX && mx <= adjX+btnW && my >= rowY && my <= rowY+btnH)
                    return IDC_BTN_MIN_DEC;
                if (mx >= adjX+btnW+valW && mx <= adjX+2*btnW+valW && my >= rowY && my <= rowY+btnH)
                    return IDC_BTN_MIN_INC;
            }
        }

        curY = secY + S(BREAK_SEC_H) + S(4);
    }

    // --- Auto-start Section ---
    {
        curY += S(SECTION_GAP);
        int secY = curY;
        if (mx >= W-M-S(84) && mx <= W-M && my >= secY && my <= secY+S(30))
            return IDC_BTN_AUTOSTART_SWITCH;
    }

    return 0;
}

void HandleSlider(int mx) {
    int M = S(MX);
    int sliderW = S(BW) - M*2 - S(20);
    int sliderX = M + S(10);
    g_settings.brightness = CalcSliderVal(mx, sliderX, sliderW, 0, 100);
    ApplyFilter();
}

// ============================================================================
//  Main Window Procedure
// ============================================================================

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE: {
        HDC hDC = GetDC(hWnd);
        g_dpiScale = GetDeviceCaps(hDC, LOGPIXELSX) / 96.0f;
        ReleaseDC(hWnd, hDC);
        if (g_dpiScale < 1.0f) g_dpiScale = 1.0f;
        SetTimer(hWnd, TIMER_BREAK_CHECK, BREAK_CHECK_INTERVAL, NULL);
        SetTimer(hWnd, TIMER_SCREEN_TIME, SCREEN_TIME_INTERVAL, NULL);
        SetTimer(hWnd, TIMER_COUNTDOWN, COUNTDOWN_INTERVAL, NULL);
        g_lastBreakEnd = time(NULL);
        return 0;
    }

    case WM_PAINT: PaintDoubleBuffer(hWnd); return 0;
    case WM_ERASEBKGND: return TRUE;

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        int hit = HitTest(mx, my);
        switch (hit) {
        case IDC_BTN_CLOSE:
            DestroyWindow(hWnd);
            break;
        case IDC_BTN_MINIMIZE:
            ShowWindow(hWnd, SW_MINIMIZE);
            break;
        case IDC_BTN_RESET:
            ShowBreakOverlay();
            break;
        case IDC_BTN_FILTER:
        case IDC_BTN_AUTOSTART:
            g_settings.filterMode = hit - IDC_BTN_FILTER;
            ApplyFilter(); SaveSettings();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_FILTER_SWITCH:
            g_settings.enableFilter = !g_settings.enableFilter;
            if (g_settings.enableFilter) ApplyFilter(); else RemoveFilter();
            SaveSettings(); InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_BREAK_SWITCH:
            g_settings.enableBreak = !g_settings.enableBreak;
            if (g_settings.enableBreak) g_lastBreakEnd = time(NULL);
            SaveSettings(); UpdateCountdown();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_AUTOSTART_SWITCH:
            g_settings.autoStart = !g_settings.autoStart;
            SaveSettings(); InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_PRESET_CUSTOM:
            g_settings.breakPreset = 0;
            SaveSettings(); UpdateCountdown();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_PRESET_202020:
            g_settings.breakPreset = 1;
            SaveSettings(); UpdateCountdown();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_HOUR_DEC:
            if (g_settings.breakHours > 0) { g_settings.breakHours--; SaveSettings(); UpdateCountdown(); }
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_HOUR_INC:
            if (g_settings.breakHours < 6) { g_settings.breakHours++; SaveSettings(); UpdateCountdown(); }
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_MIN_DEC:
            if (g_settings.breakMinutes >= 5) { g_settings.breakMinutes -= 5; }
            else if (g_settings.breakMinutes > 0) { g_settings.breakMinutes = 0; }
            SaveSettings(); UpdateCountdown();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_MIN_INC:
            if (g_settings.breakMinutes < 55) { g_settings.breakMinutes += 5; }
            SaveSettings(); UpdateCountdown();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_SLIDER_FILTER:
            g_draggingSlider = TRUE;
            HandleSlider(mx);
            InvalidateRect(hWnd, NULL, FALSE); SetCapture(hWnd);
            break;
        case -1:
            if (!g_draggingSlider) {
                ReleaseCapture();
                SendMessageW(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            }
            break;
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        if (g_draggingSlider) {
            HandleSlider(mx); InvalidateRect(hWnd, NULL, FALSE);
        } else {
            int nh = HitTest(mx, my);
            if (nh != g_hoverBtn) {
                g_hoverBtn = nh;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            if (nh == -1) SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32646)));
            else if (nh > 0) SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32649)));
            else SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32512)));
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (g_draggingSlider) {
            g_draggingSlider = FALSE; ReleaseCapture(); SaveSettings(); InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_TIMER:
        if (wParam == TIMER_BREAK_CHECK) {
            if (g_settings.enableBreak && !g_hWndBreak) {
                int intervalSec = GetBreakIntervalSec();
                if (intervalSec > 0) {
                    time_t now = time(NULL);
                    if (difftime(now, g_lastBreakEnd) >= intervalSec) ShowBreakOverlay();
                }
            }
        } else if (wParam == TIMER_SCREEN_TIME) {
            g_screenMinutes++; UpdateTrayTooltip();
        } else if (wParam == TIMER_COUNTDOWN) {
            UpdateCountdown();
            // Instant trigger — no 60s delay when countdown reaches zero
            if (g_nextBreakSec <= 0 && g_settings.enableBreak && !g_hWndBreak) {
                ShowBreakOverlay();
            }
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;

    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING|(g_settings.enableFilter?MF_CHECKED:MF_UNCHECKED),
                IDM_TOGGLE_FILTER, L"\x84dd\x5149\x8fc7\x6ee4");
            AppendMenuW(hMenu, MF_STRING|(g_settings.enableBreak?MF_CHECKED:MF_UNCHECKED),
                IDM_TOGGLE_BREAK, L"\x4f11\x606f\x63d0\x9192");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, IDM_SHOW, L"\x663e\x793a\x4e3b\x7a97\x53e3");
            AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"\x5173\x4e8e");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"\x9000\x51fa");
            POINT pt; GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            ShowWindow(hWnd, SW_SHOW); SetForegroundWindow(hWnd);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_TOGGLE_FILTER:
            g_settings.enableFilter = !g_settings.enableFilter;
            if (g_settings.enableFilter) ApplyFilter(); else RemoveFilter();
            SaveSettings(); InvalidateRect(hWnd, NULL, FALSE); return 0;
        case IDM_TOGGLE_BREAK:
            g_settings.enableBreak = !g_settings.enableBreak;
            SaveSettings(); UpdateCountdown(); InvalidateRect(hWnd, NULL, FALSE); return 0;
        case IDM_SHOW: ShowWindow(hWnd, SW_SHOW); SetForegroundWindow(hWnd); return 0;
        case IDM_ABOUT:
            MessageBoxW(hWnd,
                L"\x62a4\x773c\x52a9\x624b v6.0\n\n"
                L"\x50cf\x7d20\x7eb8\x9762\x98ce  |  \x7eaf Win32 + GDI+\n"
                L"\x652f\x6301 20-20-20 \x4f11\x606f\x7b56\x7565\n"
                L"\x65e0\x7b2c\x4e09\x65b9\x4f9d\x8d56",
                L"\x5173\x4e8e", MB_OK|MB_ICONINFORMATION);
            return 0;
        case IDM_EXIT: DestroyWindow(hWnd); return 0;
        }
        break;

    case WM_QUERYENDSESSION:
        CloseBreakOverlay();
        RemoveFilter();
        return TRUE;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        CloseBreakOverlay();
        RemoveFilter(); RemoveTrayIcon();
        if (g_hMutex) { CloseHandle(g_hMutex); g_hMutex = NULL; }
        FreeBuffer(g_hBufferBmp, g_hBufferDC, g_bufW, g_bufH);
        KillTimer(hWnd, TIMER_BREAK_CHECK);
        KillTimer(hWnd, TIMER_SCREEN_TIME);
        KillTimer(hWnd, TIMER_COUNTDOWN);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================================
//  Single Instance
// ============================================================================

static BOOL IsAlreadyRunning() {
    g_hMutex = CreateMutexW(NULL, TRUE, L"EyeCare_SingleInstance_8F3A2D");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hwndExisting = FindWindowW(L"EyeCareMain", NULL);
        if (hwndExisting) {
            ShowWindow(hwndExisting, SW_SHOW);
            SetForegroundWindow(hwndExisting);
        }
        CloseHandle(g_hMutex); g_hMutex = NULL;
        return TRUE;
    }
    return FALSE;
}

// ============================================================================
//  WinMain
// ============================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    if (IsAlreadyRunning()) return 0;
    g_hInst = hInstance;
    GetModuleFileNameW(NULL, g_exePath, MAX_PATH);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES | ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);

    LoadSettings();

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    wc.hIcon          = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APP));
    if (!wc.hIcon) wc.hIcon = (HICON)LoadImageW(NULL, MAKEINTRESOURCEW(32512), IMAGE_ICON, 0, 0, LR_SHARED);
    wc.lpszClassName  = L"EyeCareMain";
    RegisterClassW(&wc);

    WNDCLASSW bw = {};
    bw.lpfnWndProc   = BreakOverlayProc;
    bw.hInstance      = hInstance;
    bw.hCursor        = NULL;
    bw.hIcon          = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APP));
    if (!bw.hIcon) bw.hIcon = (HICON)LoadImageW(NULL, MAKEINTRESOURCEW(32512), IMAGE_ICON, 0, 0, LR_SHARED);
    bw.lpszClassName  = BREAK_WND_CLASS;
    RegisterClassW(&bw);

    HDC screenDC = GetDC(NULL);
    g_dpiScale = GetDeviceCaps(screenDC, LOGPIXELSX) / 96.0f;
    ReleaseDC(NULL, screenDC);
    if (g_dpiScale < 1.0f) g_dpiScale = 1.0f;

    int winW = S(BW), winH = S(BH);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winX = (screenW - winW) / 2;
    int winY = (screenH - winH) / 2;

    // v6: NO rounded window — pixel style uses square corners
    g_hWndMain = CreateWindowExW(0, L"EyeCareMain",
        L"\x62a4\x773c\x52a9\x624b",
        WS_POPUP | WS_MINIMIZEBOX,
        winX, winY, winW, winH, NULL, NULL, hInstance, NULL);

    if (!g_hWndMain) {
        MessageBoxW(NULL, L"\x521b\x5efa\x7a97\x53e3\x5931\x8d25", L"\x62a4\x773c\x52a9\x624b", MB_OK|MB_ICONERROR);
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        return 1;
    }

    // v6: Square window region (no rounding)
    HRGN hRgn = CreateRectRgn(0, 0, winW, winH);
    SetWindowRgn(g_hWndMain, hRgn, TRUE);

    InitTrayIcon();
    if (g_settings.enableFilter) ApplyFilter();
    if (g_settings.autoStart && !CheckAutoStart()) SetAutoStart(TRUE);
    UpdateCountdown();

    ShowWindow(g_hWndMain, nCmdShow);
    UpdateWindow(g_hWndMain);
    ShowTrayBalloon(L"\x62a4\x773c\x52a9\x624b", L"\x5df2\x542f\x52a8");

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }

    Gdiplus::GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}
