#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <GL/gl.h>
#include <tchar.h>
#include "clockwidget.h"

// Data stored per platform window

struct WGL_WindowData { HDC hDC; };
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;

static WNDPROC g_pOriginalWndProc = nullptr;
static HWND g_hDesktopWnd = nullptr;


bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
LRESULT WINAPI DesktopWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    HWND p = FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", NULL);
    HWND* ret = (HWND*)lParam;

    if (p)
    {
        *ret = FindWindowEx(NULL, hwnd, L"WorkerW", NULL);
    }

    return true;
}


HWND GetDesktopWindowHandle()
{
    HWND progman = FindWindow(L"Progman", NULL);

    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);

    HWND desktop_hwnd = nullptr;
    EnumWindows(EnumWindowsProc, (LPARAM)&desktop_hwnd);

    if (desktop_hwnd == NULL) {
        desktop_hwnd = FindWindowEx(progman, NULL, L"SHELLDLL_DefView", NULL);
        desktop_hwnd = FindWindowEx(desktop_hwnd, NULL, L"SysListView32", NULL);
    }

    return desktop_hwnd;
}


int main(int, char**)
{
    g_hDesktopWnd = GetDesktopWindowHandle();

    if (g_hDesktopWnd == nullptr) {
        MessageBox(NULL, L"Could not find desktop window handle. \nPlease ensure explorer.exe is running.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_pOriginalWndProc = (WNDPROC)SetWindowLongPtr(g_hDesktopWnd, GWLP_WNDPROC, (LONG_PTR)DesktopWndProc);

    if (!CreateDeviceWGL(g_hDesktopWnd, &g_MainWindow))
    {
        CleanupDeviceWGL(g_hDesktopWnd, &g_MainWindow);
        SetWindowLongPtr(g_hDesktopWnd, GWLP_WNDPROC, (LONG_PTR)g_pOriginalWndProc);

        return 1;

    }

    wglMakeCurrent(g_MainWindow.hDC, g_hRC);


    RECT r;
    GetClientRect(g_hDesktopWnd, &r);
    g_Width = r.right - r.left;
    g_Height = r.bottom - r.top;


    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    io.IniFilename = NULL;

    io.Fonts->AddFontDefault();
    g_FontLarge = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\seguisb.ttf", 80.f);
    g_FontMedium = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 40.f);

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui_ImplWin32_InitForOpenGL(g_hDesktopWnd);
    ImGui_ImplOpenGL3_Init();


    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    bool done = false;

    while (!done)
    {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::Begin("DesktopWindow", NULL,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoInputs

        );

        ImGui::End();

        RenderClockWidget();

        ImGui::Render();

        glViewport(0, 0, g_Width, g_Height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        HDC backup_current_context = wglGetCurrentDC();
        HGLRC backup_current_hrc = wglGetCurrentContext();
        wglMakeCurrent(backup_current_context, backup_current_hrc);

        ::SwapBuffers(g_MainWindow.hDC);

        Sleep(1);

    }


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();


    CleanupDeviceWGL(g_hDesktopWnd, &g_MainWindow);
    wglDeleteContext(g_hRC);

    SetWindowLongPtr(g_hDesktopWnd, GWLP_WNDPROC, (LONG_PTR)g_pOriginalWndProc);


    return 0;

}


bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);

    PIXELFORMATDESCRIPTOR pfd = { 0 };

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SUPPORT_COMPOSITION;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);

    if (pf == 0)
        return false;

    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;

    ::ReleaseDC(hWnd, hDc);


    data->hDC = ::GetDC(hWnd);

    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);

    return true;
}


void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


LRESULT WINAPI DesktopWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;


    switch (msg)
    {
        case WM_SIZE:

            if (wParam != SIZE_MINIMIZED)
            {
                g_Width = LOWORD(lParam);
                g_Height = HIWORD(lParam);
            }

            return 0;
    }

    return ::CallWindowProc(g_pOriginalWndProc, hWnd, msg, wParam, lParam);
}