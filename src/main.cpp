#include <windows.h>
#include <stdio.h>
#include <ShellScalingApi.h>

#include "constants.h"
#include "fpscounter.h"
#include "random.h"
#include "simulation.h"


HDC memoryDC;
HBITMAP memoryDCBitmap;
unsigned char *bitmapData;
Simulation simulation;
FpsCounter fpsCounter;
int ticks = 0;
int scaleFactorExponent = 14;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    const char CLASS_NAME[]  = " ";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.cbWndExtra = sizeof(HBITMAP);
    RegisterClassA(&wc);

    RECT rect = {};
    rect.right = WINDOW_WIDTH;
    rect.bottom = WINDOW_HEIGHT;
    AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, false);

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "",
        WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    SetWindowLong(hwnd, GWL_STYLE, WS_CAPTION | WS_SYSMENU);

    if (hwnd == NULL)
    {
        return 0;
    }

    Random::init();
    simulation.init();
    bitmapData = (unsigned char *)calloc(1024 * 1024 * 10, 1);

    ShowWindow(hwnd, SW_SHOW);

    LARGE_INTEGER targetTime;
    LARGE_INTEGER previousTime;
    LARGE_INTEGER currentTime;
    LARGE_INTEGER tickTime;

    QueryPerformanceFrequency(&tickTime);
    tickTime.QuadPart = (double)tickTime.QuadPart / TPS;

    QueryPerformanceCounter(&targetTime);
    currentTime = targetTime;
    previousTime = targetTime;

    while (true)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        QueryPerformanceCounter(&currentTime);

        if (currentTime.QuadPart > targetTime.QuadPart)
        {
            targetTime.QuadPart += tickTime.QuadPart;
            LONGLONG threshold = currentTime.QuadPart - 10 * tickTime.QuadPart;
            if (targetTime.QuadPart < threshold)
            {
                targetTime.QuadPart = threshold;
            }

            simulation.step(pow(SCALE_FACTOR_BASE, scaleFactorExponent));
            InvalidateRect(hwnd, NULL, false);

            fpsCounter.addFrameTime((double)(currentTime.QuadPart - previousTime.QuadPart) / (tickTime.QuadPart * TPS));
            previousTime = currentTime;

            char windowTitle[1024];
            _snprintf(windowTitle, sizeof(windowTitle), "GFK 2020 - %.1f fps", fpsCounter.getFps());
            SetWindowTextA(hwnd, windowTitle);

            ticks++;
        }
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        {
            HDC hdc = GetDC(hwnd);
            memoryDC = CreateCompatibleDC(hdc);
            memoryDCBitmap = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
            SelectObject(memoryDC, memoryDCBitmap);
            ReleaseDC(hwnd, hdc);
        }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            BITMAPINFO bitmapInfo = {};
            bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
            bitmapInfo.bmiHeader.biWidth = MAX_SIMULATION_WIDTH;
            bitmapInfo.bmiHeader.biHeight = -MAX_SIMULATION_HEIGHT;
            bitmapInfo.bmiHeader.biPlanes = 1;
            bitmapInfo.bmiHeader.biBitCount = 24;
            bitmapInfo.bmiHeader.biCompression = BI_RGB;

            double scaleFactor = pow(SCALE_FACTOR_BASE, scaleFactorExponent);
            simulation.draw(scaleFactor, bitmapData);

            SetDIBits(memoryDC, memoryDCBitmap, 0, MAX_SIMULATION_HEIGHT, bitmapData, &bitmapInfo, DIB_RGB_COLORS);
            StretchBlt(
                hdc,
                0, 0,
                WINDOW_WIDTH, WINDOW_HEIGHT,
                memoryDC,
                1, 1,
                WINDOW_WIDTH / scaleFactor - 2, WINDOW_HEIGHT / scaleFactor - 2,
                SRCCOPY
            );

            if (ticks < 120)
            {
                RECT textRect;
                textRect.left = 0;
                textRect.top = 0;
                textRect.right = WINDOW_WIDTH;
                textRect.bottom = WINDOW_HEIGHT;
                SetBkMode(hdc, TRANSPARENT);
                DrawTextA(hdc, "Maya Jelonkiewicz", -1, &textRect, 0);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

    case WM_KEYDOWN:
        if (lParam & (1 << 30)) return 0; // skip autorepeat messages

        switch (wParam)
        {
        case 'B':
            simulation.drawBorders ^= true;
            break;

        case VK_OEM_PLUS:
            scaleFactorExponent++;
            break;

        case VK_OEM_MINUS:
            scaleFactorExponent--;
            if (scaleFactorExponent < 0)
            {
                scaleFactorExponent = 0;
            }
            break;

        }
        return 0;

    case WM_CLOSE:
        ExitProcess(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
