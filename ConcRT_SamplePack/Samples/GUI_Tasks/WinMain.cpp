// WinApp.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "WinMain.h"
#include "ppl.h"
#include "cowait.h"
#include "continue_with_on_ui_thread.h"
#include "..\..\concrtextras\ppltasks.h"
using namespace Concurrency::samples;

extern int fib(int n);

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
HFONT g_font;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_WINAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINAPP));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINAPP));
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WINAPP);
    wcex.lpszClassName	= szWindowClass;
    wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

HFONT FAR PASCAL CreateFont()
{
    LOGFONT lf;
    lf.lfHeight=-64;
    lf.lfWidth=0;
    lf.lfEscapement=0;
    lf.lfOrientation=0;
    lf.lfWeight=400;
    lf.lfItalic=0;
    lf.lfUnderline=0;
    lf.lfStrikeOut=0;
    lf.lfCharSet=0;
    lf.lfOutPrecision=3;
    lf.lfClipPrecision=2;
    lf.lfQuality=1;
    lf.lfPitchAndFamily=34;
    wcscpy_s(lf.lfFaceName,L"Arial");
    HFONT hfont = CreateFontIndirect(&lf); 
    return (hfont); 
}

void DrawTime(HDC hdc)
{
    HFONT hOldFont;

    SYSTEMTIME systemTime;

    GetLocalTime(&systemTime);

    const size_t bufSize = 1024;
    WCHAR buff[bufSize];
    swprintf(buff, bufSize, L"%02d:%02d:%02d:%01d", systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds/100);

    if (hOldFont = (HFONT)SelectObject(hdc, g_font)) 
    {
        TextOut(hdc, 10, 50, buff, wcslen(buff)); 
        SelectObject(hdc, hOldFont); 
    }
}

VOID CALLBACK TimerProc(HWND hWnd, UINT, UINT_PTR, DWORD)
{
    HDC hdc = GetDC(hWnd);
    DrawTime(hdc);
    ReleaseDC(hWnd,hdc);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance; // Store instance handle in our global variable

    g_font = CreateFont();

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    SetTimer(hWnd,0,100,&TimerProc);
    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    int n;

    switch (message)
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        switch (wmId)
        {

        // Schedule task execution as a regular continuation. Not blocking, but the continuation runs on a non-UI thread.
        case IDM_CONTINUATION:
            {
            clock_t ticks0 = clock();
            task<int> fibTask([=]()
            {
                return fib(40);
            });

            fibTask.continue_with([=](int n)
            {
                WCHAR elapsedTimeBuffer[256];
                swprintf( elapsedTimeBuffer, 256, L"Fib=%d\nElapsed time: %3.2f seconds\n", n, ((double)(clock()-ticks0))/CLOCKS_PER_SEC);
                MessageBox(hWnd, elapsedTimeBuffer, L"Continuation", MB_OK);
            });

            break;
            }

        // Schedule task execution, and then wait blocking the UI thread. The result is displayed on the UI thread
        case IDM_WAIT:
            {
            clock_t ticks0 = clock();
            task<int> fibTask([=]()
            {
                return fib(40);
            });

            fibTask.wait();
            n = fibTask.get();

            WCHAR elapsedTimeBuffer[256];
            swprintf( elapsedTimeBuffer, 256, L"Fib=%d\nElapsed time: %3.2f seconds\n", n, ((double)(clock()-ticks0))/CLOCKS_PER_SEC);
            MessageBox(hWnd, elapsedTimeBuffer, L"Waiting on UI thread", MB_OK);

            break;
            }

        // Schedule task execution, and then start a pumping wait that does not block the UI thread. The result is displayed on the UI thread
        case IDM_PUMPINGWAIT:

            {
            clock_t ticks0 = clock();
            task<int> fibTask([=]()
            {
                return fib(40);
            });

            cowait(fibTask);
            n = fibTask.get();

            WCHAR elapsedTimeBuffer[256];
            swprintf( elapsedTimeBuffer, 256, L"Fib=%d\nElapsed time: %3.2f seconds\n", n, ((double)(clock()-ticks0))/CLOCKS_PER_SEC);
            MessageBox(hWnd, elapsedTimeBuffer, L"Co-waiting on UI thread", MB_OK);

            break;
            }

        // Schedule the continuation on the UI thread
        case IDM_CONTINUATIONONUITHREAD:

            {
            clock_t ticks0 = clock();
            task<int> fibTask([=]()
            {
                return fib(40);
            });

            continue_with_on_ui_thread(fibTask, [=](int n)
            {
                WCHAR elapsedTimeBuffer[256];
                swprintf( elapsedTimeBuffer, 256, L"Fib=%d\nElapsed time: %3.2f seconds\n", n, ((double)(clock()-ticks0))/CLOCKS_PER_SEC);
                MessageBox(hWnd, elapsedTimeBuffer, L"continue_with_on_ui_thread", MB_OK);
            });

            break;
            }

        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        {
            hdc = BeginPaint(hWnd, &ps);
            DrawTime(hdc);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
