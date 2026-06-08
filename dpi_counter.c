/* dpi_counter.c - minimal raw-input mouse count tester
 *
 * One big window. Press ENTER to start counting raw mouse X/Y counts,
 * press ENTER again to stop (the session gets logged). Press ENTER again
 * to start a fresh count. Past counts stay listed on screen.
 *
 *   ENTER : start / stop a count
 *   C     : clear the log (only when stopped)
 *   ESC   : quit
 *
 * Build (MSVC):  cl /W4 /O2 dpi_counter.c user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS
 * Build (MinGW): gcc -O2 -Wall -o dpi_counter.exe dpi_counter.c -luser32 -lgdi32 -mwindows
 */

#include <windows.h>

#define MAX_LOG 4096

typedef struct {
    long          x;
    long          y;
    unsigned long reports;   /* number of WM_INPUT mouse reports this session */
} Session;

static int           g_counting = 0;
static long          g_accX = 0, g_accY = 0;
static unsigned long g_reports = 0;

static Session       g_log[MAX_LOG];
static int           g_logCount = 0;

static HFONT         g_font    = NULL;   /* normal monospace          */
static HFONT         g_fontBig = NULL;   /* big bold for live counter */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

    case WM_CREATE: {
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01;   /* generic desktop controls */
        rid.usUsage     = 0x02;   /* mouse                    */
        rid.dwFlags     = 0;      /* receive while foreground */
        rid.hwndTarget  = hwnd;
        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
            MessageBoxA(hwnd, "RegisterRawInputDevices failed", "Error", MB_OK | MB_ICONERROR);
        SetTimer(hwnd, 1, 16, NULL);   /* ~60 Hz repaint while counting */
        return 0;
    }

    case WM_INPUT: {
        RAWINPUT raw;
        UINT size = sizeof(raw);
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size,
                            sizeof(RAWINPUTHEADER)) == (UINT)-1)
            return 0;
        if (raw.header.dwType == RIM_TYPEMOUSE &&
            (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0) {
            if (g_counting) {
                g_accX += raw.data.mouse.lLastX;
                g_accY += raw.data.mouse.lLastY;
                g_reports++;
            }
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            if (lParam & (1 << 30)) return 0;   /* ignore key auto-repeat */
            if (!g_counting) {
                g_accX = g_accY = 0;
                g_reports = 0;
                g_counting = 1;
            } else {
                g_counting = 0;
                if (g_logCount < MAX_LOG) {
                    g_log[g_logCount].x       = g_accX;
                    g_log[g_logCount].y       = g_accY;
                    g_log[g_logCount].reports = g_reports;
                    g_logCount++;
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        } else if (wParam == 'C') {
            if (!g_counting) { g_logCount = 0; InvalidateRect(hwnd, NULL, FALSE); }
        }
        return 0;

    case WM_TIMER:
        if (g_counting) InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_ERASEBKGND:
        return 1;   /* we paint everything ourselves (double buffered) */

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        /* double buffer to avoid flicker */
        HDC     mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

        HBRUSH bg = CreateSolidBrush(RGB(18, 18, 18));
        FillRect(mem, &rc, bg);
        DeleteObject(bg);
        SetBkMode(mem, TRANSPARENT);

        char line[256];
        int  y = 20;

        SelectObject(mem, g_font);
        SetTextColor(mem, RGB(150, 150, 150));
        lstrcpyA(line, "Raw Input Mouse Count Tester    ENTER start/stop    C clear log    ESC quit");
        TextOutA(mem, 24, y, line, lstrlenA(line));
        y += 44;

        SelectObject(mem, g_fontBig);
        if (g_counting) {
            SetTextColor(mem, RGB(80, 220, 120));
            wsprintfA(line, "COUNTING    X = %ld    Y = %ld    (%lu reports)",
                      g_accX, g_accY, g_reports);
        } else {
            SetTextColor(mem, RGB(220, 210, 80));
            if (g_logCount > 0)
                wsprintfA(line, "STOPPED    last  X = %ld  Y = %ld  (%lu reports)    ENTER to count again",
                          g_log[g_logCount - 1].x, g_log[g_logCount - 1].y,
                          g_log[g_logCount - 1].reports);
            else
                lstrcpyA(line, "READY    press ENTER to start counting");
        }
        TextOutA(mem, 24, y, line, lstrlenA(line));
        y += 64;

        SelectObject(mem, g_font);
        SetTextColor(mem, RGB(150, 150, 150));
        lstrcpyA(line, "Past counts (newest first):");
        TextOutA(mem, 24, y, line, lstrlenA(line));
        y += 32;

        SetTextColor(mem, RGB(210, 210, 210));
        for (int i = g_logCount - 1; i >= 0 && y < rc.bottom - 16; --i) {
            wsprintfA(line, "#%-4d   X = %-9ld   Y = %-9ld   reports = %lu",
                      i + 1, g_log[i].x, g_log[i].y, g_log[i].reports);
            TextOutA(mem, 36, y, line, lstrlenA(line));
            y += 26;
        }

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    (void)hPrev; (void)lpCmd;

    g_font = CreateFontA(-18, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                         FIXED_PITCH | FF_MODERN, "Consolas");
    g_fontBig = CreateFontA(-32, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            FIXED_PITCH | FF_MODERN, "Consolas");

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = "RawCountWnd";
    RegisterClassA(&wc);

    int W = 1000, H = 720;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - W) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - H) / 2;

    HWND hwnd = CreateWindowA("RawCountWnd", "Raw Input Mouse Count Tester",
                              WS_OVERLAPPEDWINDOW, sx, sy, W, H,
                              NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(g_font);
    DeleteObject(g_fontBig);
    return (int)msg.wParam;
}
