#include <windows.h>
#include <stdio.h>

#include "simulation.h"

#define TPS 10


HDC memoryDC;
Simulation simulation;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    const char CLASS_NAME[]  = " ";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.cbWndExtra = sizeof(HBITMAP);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "GFK 2022",
        WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1024,
        768,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    simulation.init();

    ShowWindow(hwnd, SW_SHOW);

    LARGE_INTEGER targetTime;
    LARGE_INTEGER currentTime;
    LARGE_INTEGER tickTime;

    QueryPerformanceFrequency(&tickTime);
    tickTime.QuadPart = (double)tickTime.QuadPart / TPS;

    QueryPerformanceCounter(&targetTime);

    while (true)
    {
        MSG msg;
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        QueryPerformanceCounter(&currentTime);
        if (currentTime.QuadPart > targetTime.QuadPart)
        {
            targetTime.QuadPart += tickTime.QuadPart;
            simulation.step();
            InvalidateRect(hwnd, NULL, false);
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
            RECT rect;
            GetClientRect(hwnd, &rect);
            HDC hdc = GetDC(hwnd);
            memoryDC = CreateCompatibleDC(hdc);
            HBITMAP bitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
            SelectObject(memoryDC, bitmap);
            ReleaseDC(hwnd, hdc);
        }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            simulation.draw(memoryDC, ps);

            BitBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, memoryDC, 0, 0, SRCCOPY);

            EndPaint(hwnd, &ps);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
