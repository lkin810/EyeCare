// ============================================================================
//  EyeCare.cpp - EyeCare v6.4  ★ Pixel Paper Style
//  ★ 20-20-20 Break Strategy Preset
//  ★ Fullscreen Break Overlay (Pixel Flip Clock Style)
//  ★ Pixel Paper Style UI — Square Corners / 2px Border / Hard Offset Shadow / Grid Texture
//  ★ Double Buffer Drawing — Flicker-free
//  ★ DPI Aware — HiDPI Adaptive
//  ★ All Unicode (W) API
//  ★ Idle Detection / Continuous Use Warning / Data Statistics
//  ★ v6.3: Pixel Polish — shadow depth tiers / hover+active feedback / bevel contrast /
//           paper texture / unified color tier usage / bar chart bevel
//  ★ v6.4: Sleep/Idle Fix — WM_POWERBROADCAST sleep detection / break trigger respects
//           idle+sleep state / wall-clock idle compensation / future-time clamping
// ============================================================================

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <ctime>
#include <cstring>
#include <cmath>
#include <cstdio>

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

    // --- Pixel Hard Shadow (v6.3: three-tier depth) ---
    constexpr DWORD SHADOW_SM = 0xFF9A8A78;   // light shadow (small buttons, 2px offset)
    constexpr DWORD SHADOW    = 0xFF8A7A68;   // warm brown shadow (regular, 3px offset)
    constexpr DWORD SHADOW_DK = 0xFF5A4A38;   // deep brown shadow (primary buttons, 3px offset)
    constexpr DWORD SHADOW_LG = 0xFF706050;   // large shadow (cards/panels, 4px offset)

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

    // --- Button (v6.3: stronger hover contrast) ---
    constexpr DWORD BTN_GHOST = 0x00FFFFFF;   // transparent
    constexpr DWORD BTN_MINUS = 0xFFF5EEE4;   // +/- button background
    constexpr DWORD BTN_MINUS_H=0xFFFCE4D0;   // +/- hover (stronger: warm apricot)

    // --- Divider ---
    constexpr DWORD DIVIDER   = 0xFFD4C8B8;

    // --- Bevel / Depth (v6.3: stronger contrast for visible depth) ---
    constexpr DWORD BEVEL_LT  = 0xFFFFFFFF;   // card top-left highlight edge (pure white)
    constexpr DWORD BEVEL_DK  = 0xFFA89880;   // card bottom-right inner shadow edge (deeper)
    constexpr DWORD CLOSE_RED = 0xFFD04040;   // close button hover red

    // --- Break Overlay ---
    constexpr DWORD OVERLAY_BG= 0xFF1A1510;   // deep ink background
    constexpr DWORD OVERLAY_O = 0xFFE07840;   // orange pixel
    constexpr DWORD OVERLAY_DIM=0xFF2C1A10;   // dark orange ghost

    // --- Warning Colors ---
    constexpr DWORD WARN    = 0xFFE0A020;     // amber warning
    constexpr DWORD DANGER  = 0xFFD03030;     // red danger
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
#define BH  620

// Section heights
#define HEADER_H      44
#define TIME_PANEL_H  110
#define FILTER_SEC_H  155
#define BREAK_SEC_H   170
#define AUTOSTART_H   50

// Margins / spacing
#define MX            30
#define MY            12
#define SECTION_GAP   14

// Break rest duration (seconds) per strategy
#define REST_DURATION_202020  20
// Custom rest duration is now g_settings.breakRestSec (v6.3)

// ============================================================================
//  Settings
// ============================================================================

struct AppSettings {
    int  brightness;
    int  breakHours;
    int  breakMinutes;
    int  breakRestSec;      // v6.3: custom rest duration in seconds (10~300, default 30)
    int  filterMode;
    int  breakPreset;
    BOOL enableFilter;
    BOOL enableBreak;
    BOOL autoStart;
};

// ============================================================================
//  Daily Stats
// ============================================================================

struct DailyStats {
    int year, month, day;
    int screenMinutes;
    int breakTotal, breakComplete, breakSkip;
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
static BOOL             g_hoverStatsClose = FALSE;   // v6.3: stats popup X hover

// Single-instance mutex (defined later, forward-declared for WM_DESTROY)
static HANDLE           g_hMutex = NULL;

// --- v6.1: Stats / Idle / Continuous ---
static DailyStats       g_todayStats;
static BOOL             g_isIdle = FALSE;
static time_t           g_idleStartWall = 0;  // v6.4: wall-clock idle start time
static int              g_continuousMinutes = 0;
static int              g_lastWarnBalloonMin = 0;

// --- v6.4: Sleep / Hibernate detection ---
static BOOL             g_isSleeping = FALSE;
static time_t           g_sleepStartTime = 0;
static time_t           g_wakeTime = 0;        // wall-clock of last wake (grace period)
static HWND             g_hWndStats = NULL;
static WCHAR            g_statsPath[MAX_PATH] = {};

// Double-buffer (stats popup)
static HBITMAP          g_hStatsBmp = NULL;
static HDC              g_hStatsDC  = NULL;
static int              g_statsBufW = 0, g_statsBufH = 0;

// DPI helper — round to nearest pixel for cleaner rendering
static int S(int v) { return (int)(v * g_dpiScale + 0.5f); }
static float SF(float v) { return v * g_dpiScale; }

// Cross-platform localtime (MSVC: localtime_s, MinGW: localtime_r)
static void SafeLocalTime(time_t t, struct tm* out) {
#ifdef _MSC_VER
    localtime_s(out, &t);
#else
    struct tm* tmp = localtime(&t);
    if (tmp) *out = *tmp;
    else memset(out, 0, sizeof(struct tm));
#endif
}

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

// v6.1 stats
void ShowStatsPopup();
void CloseStatsPopup();
void PaintStatsPopup(HWND hWnd);
void DrawStatsToBuffer(int W, int H);
void GetStatsPath();
void LoadTodayStats();
void SaveStats();
void PruneOldStats();

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
    g_settings.breakRestSec  = 30;
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
    if (RegQueryValueExW(hKey, L"BreakRestSec", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
        g_settings.breakRestSec = (int)val;
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
    if (g_settings.breakRestSec < 10 || g_settings.breakRestSec > 300) g_settings.breakRestSec = 30;
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
    val = (DWORD)g_settings.breakRestSec; RegSetValueExW(hKey, L"BreakRestSec", 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
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
//  Data Statistics — v6.1
// ============================================================================

void GetStatsPath() {
    WCHAR appData[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        wsprintfW(g_statsPath, L"%s\\EyeCare", appData);
        CreateDirectoryW(g_statsPath, NULL);
        wcscat(g_statsPath, L"\\stats.dat");
    } else {
        wsprintfW(g_statsPath, L"stats.dat");
    }
}

void LoadTodayStats() {
    time_t now = time(NULL);
    struct tm tmNow;
    SafeLocalTime(now, &tmNow);
    g_todayStats.year  = tmNow.tm_year + 1900;
    g_todayStats.month = tmNow.tm_mon + 1;
    g_todayStats.day   = tmNow.tm_mday;
    g_todayStats.screenMinutes  = 0;
    g_todayStats.breakTotal     = 0;
    g_todayStats.breakComplete  = 0;
    g_todayStats.breakSkip      = 0;

    GetStatsPath();

    FILE* f = _wfopen(g_statsPath, L"r");
    if (!f) return;

    WCHAR line[128];
    while (fgetws(line, 128, f)) {
        int y, m, d, sm, bt, bc, bs;
        if (swscanf(line, L"%d-%d-%d %d %d %d %d", &y, &m, &d, &sm, &bt, &bc, &bs) == 7) {
            if (y == g_todayStats.year && m == g_todayStats.month && d == g_todayStats.day) {
                g_todayStats.screenMinutes  = sm;
                g_todayStats.breakTotal     = bt;
                g_todayStats.breakComplete  = bc;
                g_todayStats.breakSkip      = bs;
                break;
            }
        }
    }
    fclose(f);
}

void SaveStats() {
    struct StatEntry { int y, m, d, sm, bt, bc, bs; };
    StatEntry entries[200];
    int count = 0;

    FILE* f = _wfopen(g_statsPath, L"r");
    if (f) {
        WCHAR line[128];
        while (fgetws(line, 128, f) && count < 200) {
            int y, m, d, sm, bt, bc, bs;
            if (swscanf(line, L"%d-%d-%d %d %d %d %d", &y, &m, &d, &sm, &bt, &bc, &bs) == 7) {
                entries[count] = { y, m, d, sm, bt, bc, bs };
                count++;
            }
        }
        fclose(f);
    }

    // Update or add today's entry
    BOOL found = FALSE;
    for (int i = 0; i < count; i++) {
        if (entries[i].y == g_todayStats.year && entries[i].m == g_todayStats.month && entries[i].d == g_todayStats.day) {
            entries[i].sm = g_todayStats.screenMinutes;
            entries[i].bt = g_todayStats.breakTotal;
            entries[i].bc = g_todayStats.breakComplete;
            entries[i].bs = g_todayStats.breakSkip;
            found = TRUE;
            break;
        }
    }
    if (!found && count < 200) {
        entries[count++] = { g_todayStats.year, g_todayStats.month, g_todayStats.day,
                             g_todayStats.screenMinutes, g_todayStats.breakTotal,
                             g_todayStats.breakComplete, g_todayStats.breakSkip };
    }

    // Write back
    f = _wfopen(g_statsPath, L"w");
    if (f) {
        for (int i = 0; i < count; i++) {
            fwprintf(f, L"%04d-%02d-%02d %d %d %d %d\n",
                     entries[i].y, entries[i].m, entries[i].d,
                     entries[i].sm, entries[i].bt, entries[i].bc, entries[i].bs);
        }
        fclose(f);
    }
}

void PruneOldStats() {
    struct StatEntry { int y, m, d, sm, bt, bc, bs; };
    StatEntry entries[200];
    int count = 0;

    FILE* f = _wfopen(g_statsPath, L"r");
    if (!f) return;

    WCHAR line[128];
    while (fgetws(line, 128, f) && count < 200) {
        int y, m, d, sm, bt, bc, bs;
        if (swscanf(line, L"%d-%d-%d %d %d %d %d", &y, &m, &d, &sm, &bt, &bc, &bs) == 7) {
            entries[count++] = { y, m, d, sm, bt, bc, bs };
        }
    }
    fclose(f);

    // Cutoff: 365 days ago (stats data is tiny, ~9KB/year)
    time_t now = time(NULL);
    time_t cutoff = now - 365 * 24 * 3600;
    struct tm tmCutoff;
    SafeLocalTime(cutoff, &tmCutoff);
    int cutoffVal = (tmCutoff.tm_year + 1900) * 10000 + (tmCutoff.tm_mon + 1) * 100 + tmCutoff.tm_mday;

    f = _wfopen(g_statsPath, L"w");
    if (f) {
        for (int i = 0; i < count; i++) {
            int entryVal = entries[i].y * 10000 + entries[i].m * 100 + entries[i].d;
            if (entryVal >= cutoffVal) {
                fwprintf(f, L"%04d-%02d-%02d %d %d %d %d\n",
                         entries[i].y, entries[i].m, entries[i].d,
                         entries[i].sm, entries[i].bt, entries[i].bc, entries[i].bs);
            }
        }
        fclose(f);
    }
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
    return g_settings.breakRestSec;
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
    // If g_lastBreakEnd is in the future, clamp elapsed to 0.
    // This can happen if compensation overshot; treat as fresh cycle.
    if (elapsed < 0) elapsed = 0;
    g_nextBreakSec = intervalSec - (int)elapsed;
    if (g_nextBreakSec < 0) g_nextBreakSec = 0;
}

// ============================================================================
//  Break Timer Compensation (v6.4)
//
//  DESIGN: When the user returns from idle/sleep, we need to adjust the break
//  timer to account for the time they were away. The key insight is:
//
//    workedBefore = time away started - last break end
//    remainingWork = intervalSec - workedBefore
//
//  If the away duration >= remainingWork, the user was away long enough that
//  their eyes have "rested through" the remaining work period → reset to a
//  fresh cycle. Otherwise, preserve the work already done.
//
//  INVARIANTS maintained:
//    I1: g_lastBreakEnd <= time(NULL) after compensation
//    I2: g_nextBreakSec <= intervalSec after compensation
//    I6: User never waits more than intervalSec after a valid away period
// ============================================================================

static void CompensateBreakTime(time_t awayStart, time_t awayEnd) {
    if (!g_settings.enableBreak) return;
    int intervalSec = GetBreakIntervalSec();
    if (intervalSec <= 0) return;

    // How long the user worked before going away
    double workedBefore = difftime(awayStart, g_lastBreakEnd);
    if (workedBefore < 0) workedBefore = 0;

    // How much more work was needed before a break
    double remainingWork = intervalSec - workedBefore;
    if (remainingWork < 0) remainingWork = 0;

    double awayDuration = difftime(awayEnd, awayStart);
    if (awayDuration < 0) awayDuration = 0;
    // Cap to 24h to prevent extreme values from clock changes
    if (awayDuration > 86400) awayDuration = 86400;

    if (awayDuration >= remainingWork) {
        // Away long enough to count as a "break" → fresh cycle from now
        g_lastBreakEnd = awayEnd;
    } else {
        // Preserve work already done, skip the away period
        g_lastBreakEnd += (time_t)awayDuration;
        // Clamp: never let g_lastBreakEnd exceed now (Invariant I1)
        time_t now = time(NULL);
        if (g_lastBreakEnd > now) g_lastBreakEnd = now;
    }

    UpdateCountdown();
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
    BOOL wasActive = (g_hWndBreak && IsWindow(g_hWndBreak));
    if (wasActive) {
        KillTimer(g_hWndBreak, TIMER_BREAK_COUNT);
        DestroyWindow(g_hWndBreak);
        g_hWndBreak = NULL;
    }
    FreeBuffer(g_hBreakBmp, g_hBreakDC, g_breakBufW, g_breakBufH);

    // v6.1: Stats tracking
    if (wasActive) {
        g_todayStats.breakTotal++;
        if (g_breakCountdown > 0) {
            g_todayStats.breakSkip++;
        } else {
            g_todayStats.breakComplete++;
            g_continuousMinutes = 0;  // Reset on completed break
        }
        SaveStats();
    }

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

    // === Tip text — 8 tips (v6.1) ===
    {
        const WCHAR* tips[] = {
            L"\x8fdc\x770b\x7a97\x5916" L" 20 " L"\x82f1\x5c3a\x7684\x5730\x65b9",
            L"\x8ba9\x773c\x775b\x5f97\x5230\x5145\x5206\x4f11\x606f",
            L"\x8d77\x8eab\x6d3b\x52a8\xff0c\x62c9\x4f38\x8eab\x4f53",
            L"\x95ed\x773c\x6df1\x547c\x5438\xff0c\x653e\x677e\x80a9\x9888",
            L"\x53cc\x624b\x6413\x70ed\xff0c\x8f7b\x6342\x53cc\x773c",
            L"\x4e0a\x4e0b\x5de6\x53f3\x8f6c\x52a8\x773c\x7403\x5404" L"5\x6b21",
            L"\x8fdc\x8fd1\x4ea4\x66ff\x6ce8\x89c6\xff0c\x953b\x70bc\x776b\x72b6\x808c",
            L"\x63c9\x6309\x592a\x9633\x7a74\x548c\x98ce\x6c60\x7a74",
        };
        int tipIdx = (g_breakTotalTime - g_breakCountdown) % 8;
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

        // v6.2: Tip pagination dots (8 dots, current highlighted)
        int dotSize = 4, dotGap = 4;
        int dotsW = 8 * dotSize + 7 * dotGap;
        int dotStartX = cx - dotsW / 2;
        int dotY = tipY + S(26);
        Gdiplus::SolidBrush dotOn(Argb(C::OVERLAY_O));
        Gdiplus::SolidBrush dotOff(Argb(C::OVERLAY_DIM));
        for (int di = 0; di < 8; di++) {
            Gdiplus::SolidBrush* b = (di == tipIdx) ? &dotOn : &dotOff;
            graphics.FillRectangle(b, dotStartX + di * (dotSize + dotGap), dotY, dotSize, dotSize);
        }
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
        wsprintfW(info, L"\x62a4\x773c\x52a9\x624b v6.2  |  \x5269\x4f59 %d \x79d2", totalSec);
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

// Pixel card: square rectangle + hard offset shadow + inner bevel edges (v6.3: shadow offset param)
// bevelLight=0 / bevelDark=0 means no bevel (flat card)
// shadowOff: 2=small btn, 3=regular, 4=card/panel
static void DrawPixelCard(Gdiplus::Graphics* g, int x, int y, int w, int h,
                          Gdiplus::Brush* fillBrush, Gdiplus::Brush* shadowBrush = NULL,
                          DWORD bevelLight = 0, DWORD bevelDark = 0, int shadowOff = 3) {
    // Hard offset shadow
    if (shadowBrush) {
        g->FillRectangle(shadowBrush, x+shadowOff, y+shadowOff, w, h);
    }
    // Fill
    g->FillRectangle(fillBrush, x, y, w, h);
    // Inner bevel — 1px highlight top-left + 1px shadow bottom-right
    if (bevelLight && bevelDark) {
        Gdiplus::SolidBrush lt(Argb(bevelLight));
        Gdiplus::SolidBrush dk(Argb(bevelDark));
        g->FillRectangle(&lt, x+1, y+1, w-2, 1);      // top highlight
        g->FillRectangle(&lt, x+1, y+2, 1, h-3);       // left highlight
        g->FillRectangle(&dk, x+1, y+h-2, w-2, 1);    // bottom shadow
        g->FillRectangle(&dk, x+w-2, y+2, 1, h-3);     // right shadow
    }
}

// Pixel border: 2px hard border on a rectangle
static void DrawPixelBorder(Gdiplus::Graphics* g, int x, int y, int w, int h, DWORD borderColor) {
    Gdiplus::Pen pen(Argb(borderColor), 2);
    g->DrawRectangle(&pen, x, y, w, h);
}

// Pixel switch: square track + square knob (v6.3: bevel + 2px shadow)
static void DrawPixelSwitch(Gdiplus::Graphics* g, int x, int y, int w, int h, BOOL isOn) {
    // Track
    Gdiplus::SolidBrush track(Argb(isOn ? C::SWITCH_ON : C::SWITCH_OFF));
    Gdiplus::SolidBrush shadow(Argb(C::SHADOW_SM));
    DWORD trackBL = isOn ? C::O4 : 0xFFEEE8E0;   // bevel light
    DWORD trackBD = isOn ? C::O3 : C::BEVEL_DK;   // bevel dark
    DrawPixelCard(g, x, y, w, h, &track, &shadow, trackBL, trackBD, 2);
    DrawPixelBorder(g, x, y, w, h, isOn ? C::O2 : C::BORDER);

    // Knob (square) with bevel
    int knobSize = h - 8;
    int knobX = isOn ? (x + w - knobSize - 4) : (x + 4);
    int knobY = y + 4;
    Gdiplus::SolidBrush knob(Argb(C::SWITCH_KNOB));
    g->FillRectangle(&knob, knobX, knobY, knobSize, knobSize);
    Gdiplus::Pen knobBorder(Argb(isOn ? C::O2 : C::BORDER), 1);
    g->DrawRectangle(&knobBorder, knobX, knobY, knobSize, knobSize);
    // Knob inner bevel
    Gdiplus::SolidBrush knobHL(Argb(0xFFFFFFFF));   // top-left highlight
    Gdiplus::SolidBrush knobSH(Argb(C::BEVEL_DK));   // bottom-right shadow
    g->FillRectangle(&knobHL, knobX+1, knobY+1, knobSize-2, 1);
    g->FillRectangle(&knobHL, knobX+1, knobY+2, 1, knobSize-3);
    g->FillRectangle(&knobSH, knobX+1, knobY+knobSize-2, knobSize-2, 1);
    g->FillRectangle(&knobSH, knobX+knobSize-2, knobY+2, 1, knobSize-3);
}

// Pixel slider: square track + square thumb (v6.3: bevel on track + thumb)
static void DrawPixelSlider(Gdiplus::Graphics* g, int x, int y, int w, int h, int val, int maxVal) {
    float ratio = (float)val / maxVal;
    int fillW = (int)(w * ratio);

    // Track background with bevel
    Gdiplus::SolidBrush trackBg(Argb(C::TRACK_BG));
    Gdiplus::SolidBrush trackShadow(Argb(C::SHADOW_SM));
    DrawPixelCard(g, x, y, w, h, &trackBg, &trackShadow, C::BEVEL_LT, C::BEVEL_DK, 2);
    DrawPixelBorder(g, x, y, w, h, C::BORDER);

    // Fill
    if (fillW > 0) {
        Gdiplus::SolidBrush fill(Argb(C::TRACK_FILL));
        g->FillRectangle(&fill, x+1, y+1, fillW-1, h-2);
    }

    // Thumb (square, bigger than track height) with bevel + shadow
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
    // Inner square (hollow look) with bevel
    Gdiplus::SolidBrush thumbInner(Argb(C::O5));
    g->FillRectangle(&thumbInner, thumbX+3, thumbY+3, thumbSize-6, thumbSize-6);
    Gdiplus::SolidBrush thumbHL(Argb(0xFFFFFFFF));
    g->FillRectangle(&thumbHL, thumbX+3, thumbY+3, thumbSize-6, 1);
    g->FillRectangle(&thumbHL, thumbX+3, thumbY+4, 1, thumbSize-8);
}

// Pixel small button (+/-) (v6.3: 2px shadow + stronger hover)
static void DrawPixelSmallBtn(Gdiplus::Graphics* g, int x, int y, int w, int h,
                               const WCHAR* text, int hoverId, int myId) {
    BOOL hovered = (hoverId == myId);
    Gdiplus::SolidBrush bg(Argb(hovered ? C::BTN_MINUS_H : C::BTN_MINUS));
    Gdiplus::SolidBrush shadow(Argb(C::SHADOW_SM));
    DrawPixelCard(g, x, y, w, h, &bg, &shadow, C::BEVEL_LT, C::BEVEL_DK, 2);
    DrawPixelBorder(g, x, y, w, h, hovered ? C::O2 : C::BORDER);

    Gdiplus::Font font(L"Consolas", SF(14), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush fg(Argb(hovered ? C::O : C::INK));
    Gdiplus::StringFormat sf;
    sf.SetAlignment(Gdiplus::StringAlignmentCenter);
    sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    g->DrawString(text, -1, &font, Gdiplus::RectF((float)x,(float)y,(float)w,(float)h), &sf, &fg);
}

// Pixel preset button (two-line label) (v6.3: stronger hover + bevel on unselected)
static void DrawPixelPresetBtn(Gdiplus::Graphics* g, int x, int y, int w, int h,
                                const WCHAR* label, const WCHAR* sub,
                                BOOL selected, int hoverId, int myId) {
    BOOL hovered = (hoverId == myId);

    if (selected) {
        // Selected: orange filled + bevel
        Gdiplus::SolidBrush fill(Argb(C::O));
        Gdiplus::SolidBrush shadow(Argb(C::SHADOW_DK));
        DrawPixelCard(g, x, y, w, h, &fill, &shadow, C::O4, C::O3, 3);
        DrawPixelBorder(g, x, y, w, h, C::O2);
    } else {
        // Normal: white card + bevel (v6.3: stronger hover with warm tint)
        Gdiplus::SolidBrush fill(Argb(hovered ? C::O5 : C::SURFACE));
        Gdiplus::SolidBrush shadow(Argb(C::SHADOW));
        DrawPixelCard(g, x, y, w, h, &fill, &shadow, C::BEVEL_LT, C::BEVEL_DK, 3);
        DrawPixelBorder(g, x, y, w, h, hovered ? C::O2 : C::BORDER);
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
//  Layout (720x620 base):
//    Header          44px   title + stats + minimize + close
//    Time Panel     110px   used time | countdown | rest button + today stats + warning
//    Filter Section 155px   title + mode tabs + switch + brightness slider
//    Break Section  170px   title + switch + presets + work/rest duration adjuster
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

        // Paper texture: subtle horizontal lines every 19px (v6.3: increased from 4% to 6%)
        Gdiplus::Pen paperPen(Argb(0x0A000000), 1);
        int paperStep = S(19);
        for (int y = paperStep; y < H; y += paperStep)
            graphics.DrawLine(&paperPen, 0, y, W, y);
    }

    // ====================================================================
    //  Header (44px) — v6.1: added stats button
    // ====================================================================
    {
        int HH = S(HEADER_H);
        DrawPxText(&graphics, L"\x62a4\x773c\x52a9\x624b", (float)(M+S(4)), (float)S(12), SF(17), C::INK, TRUE);

        // Stats button (bar chart icon) — v6.3: stronger hover (O4 tint background)
        {
            int bx = W - S(86), by = S(10), bw = S(28), bh = S(28);
            BOOL hovered = (g_hoverBtn == IDC_BTN_STATS);
            if (hovered) {
                Gdiplus::SolidBrush hbg(Argb(C::O4));
                graphics.FillRectangle(&hbg, bx, by, bw, bh);
                DrawPixelBorder(&graphics, bx, by, bw, bh, C::O2);
            }
            // Draw 3 bars of different heights (bar chart icon)
            Gdiplus::SolidBrush barBrush(Argb(hovered ? C::INK_CORE : C::I3));
            int barW = 4, barGap = 3;
            int baseY = by + bh - 7;
            // Short bar
            graphics.FillRectangle(&barBrush, bx + 5, baseY - 6, barW, 6);
            // Medium bar
            graphics.FillRectangle(&barBrush, bx + 5 + barW + barGap, baseY - 10, barW, 10);
            // Tall bar
            graphics.FillRectangle(&barBrush, bx + 5 + 2*(barW + barGap), baseY - 14, barW, 14);
        }
        // Minimize button (pixel style) — v6.3: stronger hover
        {
            int bx = W - S(120), by = S(10), bw = S(28), bh = S(28);
            BOOL hovered = (g_hoverBtn == IDC_BTN_MINIMIZE);
            if (hovered) {
                Gdiplus::SolidBrush hbg(Argb(C::O4));
                graphics.FillRectangle(&hbg, bx, by, bw, bh);
                DrawPixelBorder(&graphics, bx, by, bw, bh, C::O2);
            }
            Gdiplus::Pen minPen(Argb(hovered ? C::INK_CORE : C::I3), 2);
            graphics.DrawLine(&minPen, bx+7, by+bh/2, bx+bw-7, by+bh/2);
        }
        // Close button (pixel style)
        {
            int bx = W - S(50), by = S(10), bw = S(28), bh = S(28);
            BOOL hovered = (g_hoverBtn == IDC_BTN_CLOSE);
            if (hovered) {
                Gdiplus::SolidBrush hbg(Argb(C::CLOSE_RED));
                graphics.FillRectangle(&hbg, bx, by, bw, bh);
                DrawPixelBorder(&graphics, bx, by, bw, bh, C::CLOSE_RED);
            }
            Gdiplus::Pen closePen(Argb(hovered ? 0xFFFFFFFF : C::I3), 2);
            graphics.DrawLine(&closePen, bx+7, by+7, bx+bw-7, by+bh-7);
            graphics.DrawLine(&closePen, bx+bw-7, by+7, bx+7, by+bh-7);
        }
        curY = HH;
    }

    // ====================================================================
    //  Time Panel (110px) — pixel card style — v6.1: +today stats + warning
    // ====================================================================
    {
        int TPH = S(TIME_PANEL_H);
        int panelX = M;
        int panelY = curY + S(8);
        int panelW = W - M*2;

        // White card with hard shadow + bevel (v6.3: 4px shadow for card depth)
        Gdiplus::SolidBrush panelFill(Argb(C::SURFACE));
        Gdiplus::SolidBrush panelShadow(Argb(C::SHADOW_LG));
        DrawPixelCard(&graphics, panelX, panelY, panelW, TPH, &panelFill, &panelShadow, C::BEVEL_LT, C::BEVEL_DK, 4);
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

        // Right: "Next Break" + idle indicator (v6.2)
        int rightX = divX + S(24);
        DrawPxText(&graphics, L"\x8ddd\x4e0b\x6b21\x4f11\x606f", (float)rightX, (float)(panelY+S(14)), SF(12), C::TEXT_DIM);

        if (g_isIdle) {
            // v6.2: Idle indicator — pixel pause icon + "空闲中"
            Gdiplus::SolidBrush pauseBrush(Argb(C::SAGE));
            int px = rightX + S(6), py = panelY + S(42);
            graphics.FillRectangle(&pauseBrush, px, py, S(4), S(18));
            graphics.FillRectangle(&pauseBrush, px + S(8), py, S(4), S(18));
            DrawPxText(&graphics, L"\x7a7a\x95f2\x4e2d", (float)(rightX + S(22)), (float)(panelY+S(46)), SF(14), C::SAGE, TRUE);
        } else if (g_settings.enableBreak && g_nextBreakSec > 0) {
            // v6.2: Urgency color — normal / amber / red
            DWORD countColor = C::O;
            if (g_nextBreakSec <= 60) countColor = C::DANGER;
            else if (g_nextBreakSec <= 300) countColor = C::WARN;
            int nh = g_nextBreakSec / 3600;
            int nm = (g_nextBreakSec % 3600) / 60;
            int ns = g_nextBreakSec % 60;
            WCHAR t1[8], t2[8], t3[8];
            wsprintfW(t1, L"%d", nh); wsprintfW(t2, L"%02d", nm); wsprintfW(t3, L"%02d", ns);
            DrawPxTextCenter(&graphics, t1, rightX+S(20), panelY+S(50), SF(26), countColor, TRUE);
            DrawPxTextCenter(&graphics, L":", rightX+S(46), panelY+S(47), SF(20), countColor, TRUE);
            DrawPxTextCenter(&graphics, t2, rightX+S(72), panelY+S(50), SF(26), countColor, TRUE);
            DrawPxTextCenter(&graphics, L":", rightX+S(98), panelY+S(47), SF(20), countColor, TRUE);
            DrawPxTextCenter(&graphics, t3, rightX+S(124), panelY+S(50), SF(26), countColor, TRUE);
        } else {
            DrawPxText(&graphics, L"--:--:--", (float)(rightX+S(10)), (float)(panelY+S(42)), SF(22), C::TEXT_DARK, TRUE);
        }

        // v6.2: Today's stats line + mini progress bar
        {
            int totalB = g_todayStats.breakTotal;
            int compB  = g_todayStats.breakComplete;
            DWORD statsColor = C::TEXT_DIM;
            int rate = (totalB > 0) ? (compB * 100 / totalB) : 0;
            if (totalB > 0 && rate < 60) statsColor = C::WARN;
            WCHAR statsLine[64];
            wsprintfW(statsLine, L"\x4eca\x65e5 %d/%d \x5b8c\x6210", compB, totalB);
            DrawPxText(&graphics, statsLine, (float)rightX, (float)(panelY+S(76)), SF(11), statsColor);

            // Mini progress bar
            int barX = rightX;
            int barY = panelY + S(92);
            int barW = S(140);
            int barH = 4;
            Gdiplus::SolidBrush barBg(Argb(C::BG2));
            graphics.FillRectangle(&barBg, barX, barY, barW, barH);
            if (totalB > 0 && compB > 0) {
                int fillW = barW * compB / totalB;
                if (fillW > 0) {
                    DWORD barColor = (rate < 60) ? C::WARN : C::SAGE;
                    Gdiplus::SolidBrush barFill(Argb(barColor));
                    graphics.FillRectangle(&barFill, barX, barY, fillW, barH);
                }
            }
            Gdiplus::Pen barBorder(Argb(C::BORDER), 1);
            graphics.DrawRectangle(&barBorder, barX, barY, barW, barH);
        }

        // "Break Now" button (pixel primary button) (v6.3: hover O→O2, 3px shadow)
        {
            int bx = W - M - S(115), by = panelY + S(32), bw = S(95), bh = S(46);
            BOOL hovered = (g_hoverBtn == IDC_BTN_RESET);
            Gdiplus::SolidBrush btnFill(Argb(hovered ? C::O2 : C::O));
            Gdiplus::SolidBrush btnShadow(Argb(C::SHADOW_DK));
            DrawPixelCard(&graphics, bx, by, bw, bh, &btnFill, &btnShadow, C::O4, C::O3, 3);
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

    // v6.1: Continuous use warning (below time panel)
    if (g_continuousMinutes >= 90) {
        int warnY = curY - S(SECTION_GAP) + S(2);
        if (g_continuousMinutes >= 120) {
            DrawPxText(&graphics,
                L"\x8fde\x7eed\x7528\x773c\x8d85\x8fc7" L"2\x5c0f\x65f6\xff01\x8bf7\x7acb\x5373\x4f11\x606f",
                (float)(M + S(4)), (float)warnY, SF(13), C::DANGER, TRUE);
        } else {
            DrawPxText(&graphics,
                L"\x8fde\x7eed\x7528\x773c\x8d85\x8fc7" L"90\x5206\x949f",
                (float)(M + S(4)), (float)warnY, SF(13), C::WARN, TRUE);
        }
    }

    // ====================================================================
    //  Blue Light Filter Section (155px) — pixel style
    // ====================================================================
    {
        int secH = S(FILTER_SEC_H);
        int secY = curY;
        int innerM = M + S(4);

        // v6.2: Section title with pixel dot
        Gdiplus::SolidBrush dotBrush(Argb(C::O));
        graphics.FillRectangle(&dotBrush, innerM, secY+S(6), S(8), S(8));
        DrawPxText(&graphics, L"\x84dd\x5149\x8fc7\x6ee4", (float)(innerM+S(14)), (float)(secY+S(2)), SF(15), C::O, TRUE);
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
                DrawPixelCard(&graphics, tx, tabY, tabW, tabH, &fill, &shadow, C::O4, C::O3, 3);
                DrawPixelBorder(&graphics, tx, tabY, tabW, tabH, C::O2);
            } else {
                Gdiplus::SolidBrush fill(Argb(C::SURFACE));
                Gdiplus::SolidBrush shadow(Argb(C::SHADOW));
                DrawPixelCard(&graphics, tx, tabY, tabW, tabH, &fill, &shadow, C::BEVEL_LT, C::BEVEL_DK, 3);
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

        // v6.2: Section title with pixel dot
        Gdiplus::SolidBrush bdotBrush(Argb(C::O));
        graphics.FillRectangle(&bdotBrush, innerM, secY+S(6), S(8), S(8));
        DrawPxText(&graphics, L"\x5b9a\x65f6\x4f11\x606f", (float)(innerM+S(14)), (float)(secY+S(2)), SF(15), C::O, TRUE);
        DrawPixelSwitch(&graphics, W - M - S(84), secY, S(84), S(30), g_settings.enableBreak);

        // Preset buttons
        int presetY = secY + S(40);
        int presetW = S(120), presetH = S(44), presetGap = S(12);
        int presetX = innerM;

        DrawPixelPresetBtn(&graphics, presetX, presetY, presetW, presetH,
            L"\x81ea\x5b9a\x4e49",
            L"\x81ea\x8bbe\x5de5\x4f5c/\x4f11\x606f",
            g_settings.breakPreset == 0, g_hoverBtn, IDC_BTN_PRESET_CUSTOM);

        DrawPixelPresetBtn(&graphics, presetX + presetW + presetGap, presetY, presetW, presetH,
            L"20-20-20",
            L"\x6bcf" L"20" L"\x5206\x949f\x4f11" L"20" L"\x79d2",
            g_settings.breakPreset == 1, g_hoverBtn, IDC_BTN_PRESET_202020);

        // Time adjuster (custom mode) — v6.3: work duration + rest duration
        int rowY = presetY + presetH + S(14);
        if (g_settings.breakPreset == 0) {
            // Row 1: Work duration
            DrawPxText(&graphics, L"\x5de5\x4f5c\x65f6\x957f", (float)innerM, (float)(rowY+S(4)), SF(13), C::O);

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

            // Row 2: Rest duration
            int row2Y = rowY + S(40);
            DrawPxText(&graphics, L"\x4f11\x606f\x65f6\x957f", (float)innerM, (float)(row2Y+S(4)), SF(13), C::O);
            {
                int adjX = innerM + S(90);
                int btnW = S(30), btnH = S(30), valW = S(36);
                DrawPixelSmallBtn(&graphics, adjX, row2Y, btnW, btnH, L"-", g_hoverBtn, IDC_BTN_REST_SEC_DEC);
                // Display: if >= 60 show Xm Ys, else show Xs
                WCHAR rVal[16];
                int rm = g_settings.breakRestSec / 60;
                int rs = g_settings.breakRestSec % 60;
                if (rm > 0 && rs > 0)
                    wsprintfW(rVal, L"%d:%02d", rm, rs);
                else if (rm > 0)
                    wsprintfW(rVal, L"%d\x5206", rm);
                else
                    wsprintfW(rVal, L"%d\x79d2", rs);
                DrawPxTextCenter(&graphics, rVal, adjX + btnW + valW/2, row2Y + btnH/2, SF(14), C::INK, TRUE);
                DrawPixelSmallBtn(&graphics, adjX + btnW + valW, row2Y, btnW, btnH, L"+", g_hoverBtn, IDC_BTN_REST_SEC_INC);
                DrawPxText(&graphics, L"\x5206:\x79d2", (float)(adjX + 2*btnW + valW + S(6)), (float)(row2Y+S(6)), SF(13), C::TEXT_DIM);
            }

            DrawPxText(&graphics, L"\x5efa\x8bae\x6bcf 45~60 \x5206\x949f\x4f11\x606f\x4e00\x6b21",
                     (float)innerM, (float)(row2Y+S(40)), SF(11), C::TEXT_DARK);
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

        // v6.2: Section title with pixel dot
        int innerM = M + S(4);
        Gdiplus::SolidBrush adotBrush(Argb(C::O));
        graphics.FillRectangle(&adotBrush, innerM, curY+S(6), S(8), S(8));
        DrawPxText(&graphics, L"\x5f00\x673a\x81ea\x542f", (float)(innerM+S(14)), (float)(curY+S(4)), SF(15), C::O, TRUE);
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
//  Hit Testing — Main Window — v6.1: +stats button
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
        if (mx >= W-S(86) && mx <= W-S(58) && my >= S(10) && my <= S(38))
            return IDC_BTN_STATS;
        if (mx >= W-S(120) && mx <= W-S(92) && my >= S(10) && my <= S(38))
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
            // Row 2: Rest duration adjuster (v6.3)
            int row2Y = rowY + S(40);
            {
                int adjX = innerM + S(90);
                if (mx >= adjX && mx <= adjX+btnW && my >= row2Y && my <= row2Y+btnH)
                    return IDC_BTN_REST_SEC_DEC;
                if (mx >= adjX+btnW+valW && mx <= adjX+2*btnW+valW && my >= row2Y && my <= row2Y+btnH)
                    return IDC_BTN_REST_SEC_INC;
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
//  Stats Popup — v6.1
// ============================================================================

static const WCHAR* STATS_WND_CLASS = L"EyeCareStatsPopup";

LRESULT CALLBACK StatsPopupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        PaintStatsPopup(hWnd);
        return 0;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_LBUTTONDOWN: {
        // Only close when clicking the X button area (top-right corner)
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right;
        int xBx = W - S(40), xBy = S(14), xBw = S(24), xBh = S(24);
        if (mx >= xBx && mx <= xBx+xBw && my >= xBy && my <= xBy+xBh)
            CloseStatsPopup();
        return 0;
    }
    case WM_MOUSEMOVE: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right;
        int xBx = W - S(40), xBy = S(14), xBw = S(24), xBh = S(24);
        BOOL onX = (mx >= xBx-2 && mx <= xBx+xBw+2 && my >= xBy-2 && my <= xBy+xBh+2);
        SetCursor(LoadCursorW(NULL, onX ? MAKEINTRESOURCEW(32649) : MAKEINTRESOURCEW(32512)));
        // v6.3: update hover state and repaint for X button hover effect
        if (g_hoverStatsClose != onX) {
            g_hoverStatsClose = onX;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) { CloseStatsPopup(); return 0; }
        break;
    case WM_CLOSE:
        CloseStatsPopup();
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void ShowStatsPopup() {
    if (g_hWndStats && IsWindow(g_hWndStats)) { SetForegroundWindow(g_hWndStats); return; }
    int popupW = S(520), popupH = S(560);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - popupW) / 2;
    int y = (screenH - popupH) / 2;
    g_hWndStats = CreateWindowExW(WS_EX_TOPMOST, STATS_WND_CLASS,
        L"\x7528\x773c\x7edf\x8ba1", WS_POPUP | WS_VISIBLE,
        x, y, popupW, popupH, NULL, NULL, g_hInst, NULL);
    if (g_hWndStats) {
        // Square region
        HRGN hRgn = CreateRectRgn(0, 0, popupW, popupH);
        SetWindowRgn(g_hWndStats, hRgn, TRUE);
        ShowWindow(g_hWndStats, SW_SHOW);
        SetForegroundWindow(g_hWndStats);
    }
}

void CloseStatsPopup() {
    if (g_hWndStats && IsWindow(g_hWndStats)) {
        DestroyWindow(g_hWndStats);
        g_hWndStats = NULL;
    }
    FreeBuffer(g_hStatsBmp, g_hStatsDC, g_statsBufW, g_statsBufH);
}

void PaintStatsPopup(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT rc; GetClientRect(hWnd, &rc);
    int W = rc.right - rc.left;
    int H = rc.bottom - rc.top;
    if (!g_hStatsDC || g_statsBufW != W || g_statsBufH != H)
        CreateBuffer(hdc, W, H, g_hStatsBmp, g_hStatsDC, g_statsBufW, g_statsBufH);
    DrawStatsToBuffer(W, H);
    BitBlt(hdc, 0, 0, W, H, g_hStatsDC, 0, 0, SRCCOPY);
    EndPaint(hWnd, &ps);
}

void DrawStatsToBuffer(int W, int H) {
    if (!g_hStatsDC) return;
    Gdiplus::Graphics graphics(g_hStatsDC);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);

    int M = S(16);

    // === Background: warm paper + grid ===
    {
        Gdiplus::SolidBrush bgBrush(Argb(C::BG));
        graphics.FillRectangle(&bgBrush, 0, 0, W, H);
        Gdiplus::Pen gridPen(Argb(C::GRID), 1);
        int gridStep = S(16);
        for (int x = gridStep; x < W; x += gridStep)
            graphics.DrawLine(&gridPen, x, 0, x, H);
        for (int y = gridStep; y < H; y += gridStep)
            graphics.DrawLine(&gridPen, 0, y, W, y);
    }

    // === Card background (v6.3: 4px shadow + bevel) ===
    {
        Gdiplus::SolidBrush cardFill(Argb(C::SURFACE));
        Gdiplus::SolidBrush cardShadow(Argb(C::SHADOW_LG));
        DrawPixelCard(&graphics, S(6), S(6), W - S(12), H - S(12), &cardFill, &cardShadow, C::BEVEL_LT, C::BEVEL_DK, 4);
        DrawPixelBorder(&graphics, S(6), S(6), W - S(12), H - S(12), C::BORDER);
    }

    // === Title bar (40px) ===
    {
        int titleY = S(10);
        DrawPxText(&graphics,
            L"\x7528\x773c\x7edf\x8ba1 \x2014 \x8fd1" L"7" L"\x5929",
            (float)(M + S(8)), (float)(titleY + S(8)), SF(16), C::INK, TRUE);

        // Close X (v6.3: hover red background + red X)
        int xBx = W - S(40), xBy = S(14), xBw = S(24), xBh = S(24);
        BOOL xHovered = (g_hoverStatsClose);
        if (xHovered) {
            Gdiplus::SolidBrush xbg(Argb(C::CLOSE_RED));
            graphics.FillRectangle(&xbg, xBx-2, xBy-2, xBw+4, xBh+4);
        }
        Gdiplus::Pen closePen(Argb(xHovered ? 0xFFFFFFFF : C::I3), 2);
        graphics.DrawLine(&closePen, xBx+5, xBy+5, xBx+xBw-5, xBy+xBh-5);
        graphics.DrawLine(&closePen, xBx+xBw-5, xBy+5, xBx+5, xBy+xBh-5);

        // Divider
        Gdiplus::Pen divPen(Argb(C::BORDER), 2);
        graphics.DrawLine(&divPen, M, titleY + S(40), W - M, titleY + S(40));
    }

    // === Load 7-day stats ===
    struct StatRow { int y, m, d, sm, bt, bc, bs; };
    StatRow rows[7] = {};
    int rowCount = 0;

    {
        // Get today's date
        time_t now = time(NULL);
        struct tm tmNow;
        SafeLocalTime(now, &tmNow);
        int todayVal = (tmNow.tm_year + 1900) * 10000 + (tmNow.tm_mon + 1) * 100 + tmNow.tm_mday;

        // Read all entries from file
        struct StatEntry { int y, m, d, sm, bt, bc, bs; };
        StatEntry entries[200];
        int entryCount = 0;

        FILE* f = _wfopen(g_statsPath, L"r");
        if (f) {
            WCHAR line[128];
            while (fgetws(line, 128, f) && entryCount < 200) {
                int y, m, d, sm, bt, bc, bs;
                if (swscanf(line, L"%d-%d-%d %d %d %d %d", &y, &m, &d, &sm, &bt, &bc, &bs) == 7) {
                    entries[entryCount++] = { y, m, d, sm, bt, bc, bs };
                }
            }
            fclose(f);
        }

        // Collect last 7 days (including today)
        for (int dayOff = 6; dayOff >= 0 && rowCount < 7; dayOff--) {
            time_t dayT = now - dayOff * 24 * 3600;
            struct tm tmDay;
            SafeLocalTime(dayT, &tmDay);
            int dy = tmDay.tm_year + 1900;
            int dm = tmDay.tm_mon + 1;
            int dd = tmDay.tm_mday;

            // Find matching entry
            BOOL found = FALSE;
            for (int i = 0; i < entryCount; i++) {
                if (entries[i].y == dy && entries[i].m == dm && entries[i].d == dd) {
                    rows[rowCount++] = { dy, dm, dd, entries[i].sm, entries[i].bt, entries[i].bc, entries[i].bs };
                    found = TRUE;
                    break;
                }
            }
            if (!found) {
                rows[rowCount++] = { dy, dm, dd, 0, 0, 0, 0 };
            }
        }

        // Ensure today's live stats override the file data for today
        if (rowCount >= 1) {
            StatRow* last = &rows[rowCount - 1];
            if (last->y == g_todayStats.year && last->m == g_todayStats.month && last->d == g_todayStats.day) {
                last->sm = g_todayStats.screenMinutes;
                last->bt = g_todayStats.breakTotal;
                last->bc = g_todayStats.breakComplete;
                last->bs = g_todayStats.breakSkip;
            }
        }
    }

    // === Table header (30px) ===
    {
        int headerY = S(56);
        const WCHAR* colNames[] = {
            L"\x65e5\x671f",        // 日期
            L"\x4f7f\x7528\x65f6\x957f",  // 使用时长
            L"\x4f11\x606f\x6b21\x6570",  // 休息次数
            L"\x5b8c\x6210",        // 完成
            L"\x8df3\x8fc7",        // 跳过
            L"\x5b8c\x6210\x7387"   // 完成率
        };
        int colX[] = { M + S(8), M + S(90), M + S(170), M + S(250), M + S(310), M + S(370) };
        for (int i = 0; i < 6; i++) {
            DrawPxText(&graphics, colNames[i], (float)colX[i], (float)(headerY + S(4)), SF(11), C::O, TRUE);
        }

        // Header bottom line
        Gdiplus::Pen divPen(Argb(C::BORDER), 2);
        graphics.DrawLine(&divPen, M, headerY + S(26), W - M, headerY + S(26));
    }

    // === Data rows (36px each) ===
    {
        int rowStartY = S(86);
        int rowH = S(36);
        int colX[] = { M + S(8), M + S(90), M + S(170), M + S(250), M + S(310), M + S(370) };

        time_t nowT = time(NULL);
        struct tm tmNowR;
        SafeLocalTime(nowT, &tmNowR);
        int todayDV = (tmNowR.tm_year + 1900) * 10000 + (tmNowR.tm_mon + 1) * 100 + tmNowR.tm_mday;

        for (int i = 0; i < rowCount; i++) {
            int ry = rowStartY + i * rowH;

            // Alternating background
            DWORD rowBgColor = (i % 2 == 0) ? C::SURFACE : C::BG2;
            BOOL isToday = (rows[i].y * 10000 + rows[i].m * 100 + rows[i].d == todayDV);
            if (isToday) rowBgColor = C::O5;  // Orange highlight for today

            Gdiplus::SolidBrush rowBg(Argb(rowBgColor));
            graphics.FillRectangle(&rowBg, M, ry, W - M * 2, rowH);

            // Date (MM-DD)
            WCHAR dateStr[16];
            wsprintfW(dateStr, L"%02d-%02d", rows[i].m, rows[i].d);
            DrawPxText(&graphics, dateStr, (float)colX[0], (float)(ry + S(8)), SF(12), isToday ? C::O : C::INK, TRUE);

            // Usage time (Xh Xm)
            int uh = rows[i].sm / 60;
            int um = rows[i].sm % 60;
            WCHAR useStr[32];
            wsprintfW(useStr, L"%dh %dm", uh, um);
            DrawPxText(&graphics, useStr, (float)colX[1], (float)(ry + S(8)), SF(12), C::INK);

            // Break count
            WCHAR btStr[16];
            wsprintfW(btStr, L"%d", rows[i].bt);
            DrawPxText(&graphics, btStr, (float)colX[2], (float)(ry + S(8)), SF(12), C::INK);

            // Completed
            WCHAR bcStr[16];
            wsprintfW(bcStr, L"%d", rows[i].bc);
            DrawPxText(&graphics, bcStr, (float)colX[3], (float)(ry + S(8)), SF(12), C::SAGE);

            // Skipped
            WCHAR bsStr[16];
            wsprintfW(bsStr, L"%d", rows[i].bs);
            DrawPxText(&graphics, bsStr, (float)colX[4], (float)(ry + S(8)), SF(12), C::TEXT_DIM);

            // Completion rate
            WCHAR rateStr[16];
            if (rows[i].bt > 0) {
                int rate = rows[i].bc * 100 / rows[i].bt;
                wsprintfW(rateStr, L"%d%%", rate);
                DWORD rateColor = (rate < 60) ? C::WARN : C::SAGE;
                DrawPxText(&graphics, rateStr, (float)colX[5], (float)(ry + S(8)), SF(12), rateColor, TRUE);
            } else {
                wcscpy(rateStr, L"--");
                DrawPxText(&graphics, rateStr, (float)colX[5], (float)(ry + S(8)), SF(12), C::TEXT_DIM);
            }

            // Row separator line
            Gdiplus::Pen rowPen(Argb(C::BORDER), 1);
            graphics.DrawLine(&rowPen, M, ry + rowH, W - M, ry + rowH);
        }
    }

    // === Summary row (40px) ===
    {
        int sumY = S(86) + rowCount * S(36) + S(8);
        Gdiplus::Pen divPen(Argb(C::O), 2);
        graphics.DrawLine(&divPen, M, sumY, W - M, sumY);

        // Calculate averages
        int totalScreen = 0, totalBt = 0, totalBc = 0, totalBs = 0;
        int validDays = 0;
        for (int i = 0; i < rowCount; i++) {
            totalScreen += rows[i].sm;
            totalBt += rows[i].bt;
            totalBc += rows[i].bc;
            totalBs += rows[i].bs;
            if (rows[i].sm > 0 || rows[i].bt > 0) validDays++;
        }
        if (validDays == 0) validDays = 1;
        int avgScreen = totalScreen / validDays;
        int avgRate = (totalBt > 0) ? (totalBc * 100 / totalBt) : 0;

        int ah = avgScreen / 60;
        int am = avgScreen % 60;

        WCHAR sumStr[128];
        wsprintfW(sumStr,
            L"7\x65e5\x5e73\x5747\x7528\x773c %dh %dm | \x5e73\x5747\x5b8c\x6210\x7387 %d%%",
            ah, am, avgRate);
        DrawPxText(&graphics, sumStr, (float)(M + S(8)), (float)(sumY + S(10)), SF(12), C::O, TRUE);
    }

    // v6.2: 7-day completion rate pixel bar chart
    {
        int chartY = S(86) + rowCount * S(36) + S(42);
        DrawPxText(&graphics, L"\x5b8c\x6210\x7387\x8d8b\x52bf", (float)(M + S(8)), (float)(chartY), SF(11), C::O, TRUE);
        chartY += S(18);

        int barW = S(40), barGap = S(12), maxBarH = S(48);
        int chartW = 7 * barW + 6 * barGap;
        int chartStartX = (W - chartW) / 2;
        int baseY = chartY + maxBarH;

        for (int i = 0; i < rowCount; i++) {
            int bx = chartStartX + i * (barW + barGap);
            int rate = (rows[i].bt > 0) ? (rows[i].bc * 100 / rows[i].bt) : 0;
            int fillH = (maxBarH * rate) / 100;

            BOOL isToday = FALSE;
            time_t nowT2 = time(NULL);
            struct tm tmNow2;
            SafeLocalTime(nowT2, &tmNow2);
            if (rows[i].y * 10000 + rows[i].m * 100 + rows[i].d ==
                (tmNow2.tm_year + 1900) * 10000 + (tmNow2.tm_mon + 1) * 100 + tmNow2.tm_mday)
                isToday = TRUE;

            // Bar background (v6.3: with bevel)
            Gdiplus::SolidBrush barBg(Argb(C::BG2));
            Gdiplus::SolidBrush barBgShadow(Argb(C::SHADOW_SM));
            DrawPixelCard(&graphics, bx, chartY, barW, maxBarH, &barBg, &barBgShadow, C::BEVEL_LT, C::BEVEL_DK, 2);
            DrawPixelBorder(&graphics, bx, chartY, barW, maxBarH, C::BORDER);

            // Bar fill (v6.3: with bevel highlight)
            if (fillH > 0) {
                DWORD barColor = (rate < 60) ? C::WARN : C::SAGE;
                if (isToday) barColor = C::O;
                int fY = baseY - fillH;
                int fH = fillH;
                int fX = bx + 2;
                int fW = barW - 4;
                Gdiplus::SolidBrush barFill(Argb(barColor));
                graphics.FillRectangle(&barFill, fX, fY, fW, fH);
                // Top highlight
                DWORD hlColor = (barColor == C::O) ? C::O4 : (barColor == C::WARN) ? 0xFFF0C040 : 0xFF90C0A0;
                Gdiplus::SolidBrush barHL(Argb(hlColor));
                graphics.FillRectangle(&barHL, fX, fY, fW, 1);
                // Bottom shadow
                DWORD shColor = (barColor == C::O) ? C::O3 : (barColor == C::WARN) ? 0xFFC08010 : 0xFF508060;
                Gdiplus::SolidBrush barSH(Argb(shColor));
                graphics.FillRectangle(&barSH, fX, fY + fH - 1, fW, 1);
            }

            // Date label below bar
            WCHAR dStr[8];
            wsprintfW(dStr, L"%d", rows[i].d);
            DrawPxTextCenter(&graphics, dStr, bx + barW/2, baseY + S(10), SF(10), isToday ? C::O : C::TEXT_DIM);

            // Rate label above bar
            if (rows[i].bt > 0) {
                WCHAR rStr[8];
                wsprintfW(rStr, L"%d%%", rate);
                DrawPxTextCenter(&graphics, rStr, bx + barW/2, chartY - S(4), SF(9), (rate < 60) ? C::WARN : C::SAGE);
            }
        }
    }

    // === Close hint at bottom ===
    {
        DrawPxText(&graphics,
            L"ESC \x5173\x95ed",
            (float)(W / 2 - S(30)), (float)(H - S(30)), SF(11), C::TEXT_DARK);
    }
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

        // v6.1: Load today's stats
        LoadTodayStats();

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
        case IDC_BTN_STATS:
            ShowStatsPopup();
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
        case IDC_BTN_REST_SEC_DEC:
            if (g_settings.breakRestSec > 10) {
                if (g_settings.breakRestSec <= 60) g_settings.breakRestSec -= 10;
                else if (g_settings.breakRestSec <= 120) g_settings.breakRestSec -= 15;
                else g_settings.breakRestSec -= 30;
                if (g_settings.breakRestSec < 10) g_settings.breakRestSec = 10;
            }
            SaveSettings(); UpdateCountdown();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case IDC_BTN_REST_SEC_INC:
            if (g_settings.breakRestSec < 300) {
                if (g_settings.breakRestSec < 60) g_settings.breakRestSec += 10;
                else if (g_settings.breakRestSec < 120) g_settings.breakRestSec += 15;
                else g_settings.breakRestSec += 30;
                if (g_settings.breakRestSec > 300) g_settings.breakRestSec = 300;
            }
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
            // v6.4: Don't trigger break while idle or sleeping — user is away
            if (g_settings.enableBreak && !g_hWndBreak && !g_isIdle && !g_isSleeping) {
                int intervalSec = GetBreakIntervalSec();
                if (intervalSec > 0) {
                    time_t now = time(NULL);
                    if (difftime(now, g_lastBreakEnd) >= intervalSec) ShowBreakOverlay();
                }
            }
        } else if (wParam == TIMER_SCREEN_TIME) {
            // v6.1: Idle detection for screen time
            // Also skip during break overlay (user is resting eyes)
            // v6.4: Also skip during sleep (user is away)
            if (!g_isIdle && !g_hWndBreak && !g_isSleeping) {
                g_screenMinutes++;
                g_continuousMinutes++;
                g_todayStats.screenMinutes++;
                // Save stats periodically (every 5 minutes)
                if (g_screenMinutes % 5 == 0) SaveStats();
            }
            UpdateTrayTooltip();
        } else if (wParam == TIMER_COUNTDOWN) {
            // v6.4: Idle detection with wall-clock compensation + wake grace period
            {
                LASTINPUTINFO lii = { sizeof(lii) };
                if (GetLastInputInfo(&lii)) {
                    DWORD idleMs = GetTickCount() - lii.dwTime;
                    if (idleMs > 180000) { // 3 minutes
                        if (!g_isIdle) {
                            // Grace period: skip idle detection for 30s after wake
                            // (GetLastInputInfo needs time to update after sleep)
                            if (g_wakeTime == 0 || difftime(time(NULL), g_wakeTime) > 30) {
                                g_isIdle = TRUE;
                                g_idleStartWall = time(NULL);
                            }
                        }
                    } else {
                        if (g_isIdle) {
                            // Returning from idle — compensate using smart logic
                            g_isIdle = FALSE;
                            g_continuousMinutes = 0;
                            // Only compensate if break overlay is NOT showing
                            // (during break, g_lastBreakEnd is set when overlay closes)
                            if (!g_hWndBreak) {
                                time_t now = time(NULL);
                                CompensateBreakTime(g_idleStartWall, now);
                            }
                        }
                        // Clear wake grace period once we see real input
                        g_wakeTime = 0;
                    }
                }
            }
            UpdateCountdown();
            // Instant trigger — no 60s delay when countdown reaches zero
            // Don't trigger while idle or sleeping
            if (g_nextBreakSec <= 0 && g_settings.enableBreak && !g_hWndBreak && !g_isIdle && !g_isSleeping) {
                ShowBreakOverlay();
            }

            // v6.1: Continuous use balloon warning
            // 90 min: one-time amber reminder
            if (g_continuousMinutes == 90) {
                ShowTrayBalloon(
                    L"\x8fde\x7eed\x7528\x773c\x63d0\x9192",
                    L"\x8fde\x7eed\x7528\x773c\x8d85\x8fc7" L"90\x5206\x949f\xff0c\x8bf7\x6ce8\x610f\x4f11\x606f");
            }
            // >= 120 min: repeat every 10 min
            if (g_continuousMinutes >= 120 && (g_continuousMinutes - g_lastWarnBalloonMin) >= 10) {
                g_lastWarnBalloonMin = g_continuousMinutes;
                ShowTrayBalloon(
                    L"\x8fde\x7eed\x7528\x773c\x63d0\x9192",
                    L"\x8fde\x7eed\x7528\x773c\x8d85\x8fc7" L"2\x5c0f\x65f6\xff01\x8bf7\x7acb\x5373\x4f11\x606f");
            }
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;

    // v6.4: Handle system sleep/wake — pause timer during sleep, resume on wake
    case WM_POWERBROADCAST:
        if (wParam == PBT_APMSUSPEND) {
            // System is going to sleep — record wall-clock time
            g_isSleeping = TRUE;
            g_sleepStartTime = time(NULL);
            // Close break overlay properly (via CloseBreakOverlay for stats + g_lastBreakEnd)
            // Invariant I5: always use CloseBreakOverlay to ensure consistent stats
            if (g_hWndBreak && IsWindow(g_hWndBreak)) {
                CloseBreakOverlay();
            }
        } else if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND) {
            // System woke from sleep — compensate timer
            if (g_isSleeping && g_sleepStartTime > 0) {
                // Use idleStartWall if the user was idle before sleeping,
                // to capture the FULL away period (not just sleep portion)
                time_t awayStart = (g_idleStartWall > 0 && g_idleStartWall < g_sleepStartTime)
                                   ? g_idleStartWall : g_sleepStartTime;
                time_t now = time(NULL);
                CompensateBreakTime(awayStart, now);
                g_continuousMinutes = 0;  // Sleep = eyes rested
            }
            g_isSleeping = FALSE;
            g_sleepStartTime = 0;
            // Clear idle state so idle-return handler doesn't double-compensate
            g_isIdle = FALSE;
            g_idleStartWall = 0;
            // Set wake grace period (GetLastInputInfo needs time to update)
            g_wakeTime = time(NULL);
            UpdateCountdown();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return TRUE;

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
            // Invariant I7: reset timer start when enabling, prevent instant trigger
            if (g_settings.enableBreak) g_lastBreakEnd = time(NULL);
            SaveSettings(); UpdateCountdown(); InvalidateRect(hWnd, NULL, FALSE); return 0;
        case IDM_SHOW: ShowWindow(hWnd, SW_SHOW); SetForegroundWindow(hWnd); return 0;
        case IDM_ABOUT:
            MessageBoxW(hWnd,
                L"\x62a4\x773c\x52a9\x624b v6.4\n\n"
                L"\x50cf\x7d20\x7eb8\x9762\x98ce  |  \x7eaf Win32 + GDI+\n"
                L"\x652f\x6301 20-20-20 \x4f11\x606f\x7b56\x7565\n"
                L"\x7a7a\x95f2\x68c0\x6d4b \x00b7 \x8fde\x7eed\x7528\x773c\x8b66\x544a \x00b7 \x7528\x773c\x7edf\x8ba1\n"
                L"\x65e0\x7b2c\x4e09\x65b9\x4f9d\x8d56",
                L"\x5173\x4e8e", MB_OK|MB_ICONINFORMATION);
            return 0;
        case IDM_EXIT: DestroyWindow(hWnd); return 0;
        }
        break;

    case WM_QUERYENDSESSION:
        SaveStats();
        CloseBreakOverlay();
        RemoveFilter();
        return TRUE;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        SaveStats();
        PruneOldStats();
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

    // v6.1: Register stats popup window class
    WNDCLASSW sw = {};
    sw.lpfnWndProc   = StatsPopupProc;
    sw.hInstance      = hInstance;
    sw.hCursor        = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    sw.hIcon          = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APP));
    if (!sw.hIcon) sw.hIcon = (HICON)LoadImageW(NULL, MAKEINTRESOURCEW(32512), IMAGE_ICON, 0, 0, LR_SHARED);
    sw.lpszClassName  = STATS_WND_CLASS;
    RegisterClassW(&sw);

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
