// DriverVision.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "DriverVision.h"
#include "AxisCamera.h"
#include "Target.h"
#include "BitmapImage.h"

#define MAX_LOADSTRING 100

#define CAMERA1_IP "10.14.25.11"
#define	CAMERA2_IP "10.14.25.12"
#define	THRESHOLD 240

#define	IMAGE1_X	0
#define	IMAGE1_WIDTH	640
#define	IMAGE1_Y	0
#define	IMAGE1_HEIGHT	480

#define	IMAGE2_X	640
#define	IMAGE2_WIDTH	640
#define	IMAGE2_Y	0
#define	IMAGE2_HEIGHT	480

#define	TEXT_X		0
#define	TEXT_WIDTH	640
#define	TEXT_Y		0
#define	TEXT_HEIGHT	64

#define	WINDOW_WIDTH	1280
#define	WINDOW_HEIGHT	480


// Global Variables:
HINSTANCE hInst;				// current instance
HWND hAppWnd;
TCHAR szTitle[MAX_LOADSTRING];			// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];		// the main window class name

HWND hCollectorWnd;
AxisCamera *pCollectorCamera = NULL;
Image *collectorImage = NULL;
HANDLE collectorMutex = NULL;

HWND hTargetWnd, hTxtWnd;
AxisCamera *pTargetCamera = NULL;
Target *pTarget = NULL;
Image *targetImage = NULL;
HANDLE targetMutex = NULL;

bool targetValid = false;
Target::TargetLocation tlTop, tlLeft, tlRight, tlBottom, tlCenter;

// Forward declarations of functions included in this code module:
ATOM		    MyRegisterClass(HINSTANCE hInstance);
BOOL		    InitInstance(HINSTANCE, LPTSTR, int);
DWORD WINAPI	    CollectorApp(LPVOID param);
DWORD WINAPI	    TargetApp(LPVOID param);
LRESULT CALLBACK    AppWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ImgWndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI        CollectorPaint();
DWORD WINAPI        TargetPaint();
DWORD WINAPI        TxtPaint();

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

#ifdef DEBUG_CONSOLE
    AllocConsole();
    freopen("CONIN$", "rb", stdin);
    freopen("CONOUT$", "wb", stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
    freopen("CONOUT$", "wb", stderr);
    setvbuf(stderr, NULL, _IONBF, 0);
#endif

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_DRIVERVISION, szWindowClass, MAX_LOADSTRING);
    if (!MyRegisterClass(hInstance))
    {
	return FALSE;
    }

    // Perform application initialization:
    if (!InitInstance (hInstance, lpCmdLine, nCmdShow))
    {
	return FALSE;
    }

    // Main message loop:
    MSG msg;
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DRIVERVISION));
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
    WNDCLASSEX wcexApp, wcexImg;

    wcexApp.cbSize = sizeof(WNDCLASSEX);

    wcexApp.style		= CS_HREDRAW | CS_VREDRAW;
    wcexApp.lpfnWndProc		= AppWndProc;
    wcexApp.cbClsExtra		= 0;
    wcexApp.cbWndExtra		= 0;
    wcexApp.hInstance		= hInstance;
    wcexApp.hIcon		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DRIVERVISION));
    wcexApp.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcexApp.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcexApp.lpszMenuName	= MAKEINTRESOURCE(IDC_DRIVERVISION);
    wcexApp.lpszClassName	= szWindowClass;
    wcexApp.hIconSm		= LoadIcon(wcexApp.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    ATOM aApp = RegisterClassEx(&wcexApp);
    if (!aApp) {
	return NULL;
    }

    wcexImg.cbSize = sizeof(WNDCLASSEX);

    wcexImg.style		= CS_HREDRAW | CS_VREDRAW;
    wcexImg.lpfnWndProc		= ImgWndProc;
    wcexImg.cbClsExtra		= 0;
    wcexImg.cbWndExtra		= 0;
    wcexImg.hInstance		= hInstance;
    wcexImg.hIcon		= NULL;
    wcexImg.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcexImg.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcexImg.lpszMenuName	= NULL;
    wcexImg.lpszClassName	= TEXT("DriverVisionImage");
    wcexImg.hIconSm		= NULL;

    ATOM aImg = RegisterClassEx(&wcexImg);
    if (!aImg) {
	return NULL;
    }

    return aApp;
}

//
//   FUNCTION: InitInstance(HINSTANCE, LPTSTR, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    hAppWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
	NULL, NULL, hInst, NULL);
    printf("hAppWnd = %p\n", hAppWnd);
    if (!hAppWnd)
    {
	return FALSE;
    }

    hCollectorWnd = CreateWindow(TEXT("DriverVisionImage"), NULL, WS_CHILD | WS_VISIBLE,
			    IMAGE1_X, IMAGE1_Y, IMAGE1_WIDTH, IMAGE1_HEIGHT,
			    hAppWnd, NULL, hInst, NULL);
    printf("hCollectorWnd = %p\n", hCollectorWnd);
    if (!hCollectorWnd)
    {
	return FALSE;
    }

    hTargetWnd = CreateWindow(TEXT("DriverVisionImage"), NULL, WS_CHILD | WS_VISIBLE,
			    IMAGE2_X, IMAGE2_Y, IMAGE2_WIDTH, IMAGE2_HEIGHT,
			    hAppWnd, NULL, hInst, NULL);
    printf("hTargetWnd = %p\n", hTargetWnd);
    if (!hTargetWnd)
    {
	return FALSE;
    }

    // Initialize Winsock library.
    WSADATA WsaDat;
    if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != NO_ERROR) {
	int error = WSAGetLastError();
	printf("AxisCamera: winsock error %d\n", error);
	ExitProcess(1);
    }

    LPTSTR ip1 = TEXT(CAMERA1_IP);
    LPTSTR ct1 = TEXT("a");
    LPTSTR ip2 = TEXT(CAMERA2_IP);
    LPTSTR ct2 = TEXT("a");
    if (lpCmdLine && _tcslen(lpCmdLine) > 0) {
	LPTSTR context = NULL;
	LPCTSTR whitespace = TEXT(" \t\r\n");
	ip1 = _tcstok_s(lpCmdLine, whitespace, &context);
	ct1 = _tcstok_s(NULL, whitespace, &context);
	ip2 = _tcstok_s(NULL, whitespace, &context);
	ct2 = _tcstok_s(NULL, whitespace, &context);
    }

    if (ip1 && _tcslen(ip1) > 0) {
	AxisCamera::CameraType t =
	    (ct1 && ct1[0] == 't') ? AxisCamera::kTrendNet : AxisCamera::kAxis;
	pTargetCamera = new AxisCamera(ip1, t);
	printf("pTargetCamera = %p\n", pTargetCamera);
	pTarget = new Target();
	printf("pTarget = %p\n", pTarget);
	targetImage = imaqCreateImage(IMAQ_IMAGE_RGB, 3);
	targetMutex = CreateMutex(NULL, FALSE, NULL);
	CreateThread(NULL, 0, TargetApp, NULL, 0, NULL);
    }

    if (ip2 && _tcslen(ip2) > 0) {
	AxisCamera::CameraType t =
	    (ct2 && ct2[0] == 't') ? AxisCamera::kTrendNet : AxisCamera::kAxis;
	pCollectorCamera = new AxisCamera(ip2, t);
	printf("pCollectorCamera = %p\n", pTargetCamera);
	collectorImage = imaqCreateImage(IMAQ_IMAGE_RGB, 3);
	collectorMutex = CreateMutex(NULL, FALSE, NULL);
	CreateThread(NULL, 0, CollectorApp, NULL, 0, NULL);
    }

    ShowWindow(hAppWnd, nCmdShow);
    UpdateWindow(hAppWnd);

    return TRUE;
}

DWORD WINAPI CollectorApp(LPVOID param)
{
    Image *cameraImage = imaqCreateImage(IMAQ_IMAGE_RGB, 3);
    while (1) {
	pCollectorCamera->StartCamera();
	while (pCollectorCamera->GetImage(cameraImage)) {
	    WaitForSingleObject(collectorMutex, INFINITE);
	    (void) imaqDuplicate(collectorImage, cameraImage);
	    ReleaseMutex(collectorMutex);
	    InvalidateRect(hCollectorWnd, NULL, false);
	}
    }
    return 0;
}

DWORD WINAPI TargetApp(LPVOID param)
{
    Image *cameraImage = imaqCreateImage(IMAQ_IMAGE_RGB, 3);
    while (1) {
	pTargetCamera->StartCamera();
	while (pTargetCamera->GetImage(cameraImage)) {
	    WaitForSingleObject(targetMutex, INFINITE);
	    if (targetValid = pTarget->ProcessImage(cameraImage, THRESHOLD)) {
		// printf("ProcessImage returns true\n");
		tlCenter = pTarget->GetTargetLocation(Target::kCenter);
		tlTop    = pTarget->GetTargetLocation(Target::kTop);
		tlBottom = pTarget->GetTargetLocation(Target::kBottom);
		tlLeft   = pTarget->GetTargetLocation(Target::kLeft);
		tlRight  = pTarget->GetTargetLocation(Target::kRight);
	    }
	    (void) pTarget->GetProcessedImage(targetImage);
	    ReleaseMutex(targetMutex);
	    InvalidateRect(hTargetWnd, NULL, false);
	}
    }
    return 0;
}

double Ballistics( int height, double distance )
{
    // these constants for distances in inches, speed in PPS
    const double swish_low[3] = { 3.702E+02, -1.293E+00, 2.145E-02 };
    const double backboard_low[3] = { 3.773E+02, 1.867E+00, 0.000E+00 };
//  const double swish_mid[3] = { 1.296E+02, 4.956E+00, -8.586E-03 };
    const double backboard_mid[3] = { 2.755E+02, 3.556E+00, -3.864E-03 };
    const double backboard_high[3] = { 5.412E+02, 1.768E+00, -1.383E-03 };

    const double *coeff;

    switch (height) {
    case 0:
	coeff = (distance < 133.) ? swish_low : backboard_low;
	break;
    case 1:
	coeff = backboard_mid;
	break;
    case 2:
	coeff = backboard_high;
	break;
    default:
	// invalid
	return 0.;
    }

    return coeff[0] + distance * (coeff[1] + (distance * coeff[2]));
}

//
//  FUNCTION: AppWndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT CALLBACK AppWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;

    switch (message)
    {
    case WM_COMMAND:
	wmId    = LOWORD(wParam);
	wmEvent = HIWORD(wParam);
	// Parse the menu selections:
	switch (wmId)
	{
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

LRESULT CALLBACK ImgWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_PAINT) {
	if (hWnd == hCollectorWnd) {
	    return CollectorPaint();
	}
	if (hWnd == hTargetWnd) {
	    return TargetPaint();
	}
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

DWORD WINAPI CollectorPaint()
{
    WaitForSingleObject(collectorMutex, INFINITE);

    BitmapImage* bmpImage = new BitmapImage(collectorImage);

    HBITMAP hbmp = bmpImage->GetBitmap();
    BITMAP bmp;
    GetObject(hbmp, sizeof(BITMAP), &bmp);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hCollectorWnd, &ps);
    HDC hdcMem = CreateCompatibleDC(hdc);
    SelectObject(hdcMem, hbmp);
    BitBlt(hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);
    DeleteDC(hdcMem);
    EndPaint(hCollectorWnd, &ps);

    delete bmpImage;

    ReleaseMutex(collectorMutex);

    return 0;
}

DWORD WINAPI TargetPaint()
{
    WaitForSingleObject(targetMutex, INFINITE);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hTargetWnd, &ps);

    BitmapImage* bmpImage = new BitmapImage(targetImage);
    HBITMAP hbmp = bmpImage->GetBitmap();
    BITMAP bmp;
    GetObject(hbmp, sizeof(BITMAP), &bmp);
    HDC hdcMem = CreateCompatibleDC(hdc);
    SelectObject(hdcMem, hbmp);
    BitBlt(hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);
    DeleteDC(hdcMem);
    delete bmpImage;

    // TBD: use a dynamic String object here
    // TBD: change to fixed-width font for alignment?
    TCHAR targetStr[400];
    if (targetValid) {
	_stprintf_s(targetStr,
	    TEXT("top\t%s\t%6.1f\260\t%6.1f\"\tset %5.0f\n")
	    TEXT("left\t%s\t%6.1f\260\t%6.1f\"\tset %5.0f\n")
	    TEXT("right\t%s\t%6.1f\260\t%6.1f\"\tset %5.0f\n") ,
	    tlTop.valid ? TEXT("OK") : TEXT("clip"),
	      tlTop.angle,
	      tlTop.distance,
	      Ballistics(2, tlTop.distance),
	    tlLeft.valid ? TEXT("OK") : TEXT("clip"),
	      tlLeft.angle,
	      tlLeft.distance,
	      Ballistics(1, tlLeft.distance),
	    tlRight.valid ? TEXT("OK") : TEXT("clip"),
	      tlRight.angle,
	      tlRight.distance,
	      Ballistics(1, tlRight.distance));
    } else {
	_stprintf_s(targetStr, TEXT("no targets"));
    }

    RECT rect;
    rect.left   = TEXT_X;
    rect.top    = TEXT_Y;
    rect.right  = TEXT_X + TEXT_WIDTH;
    rect.bottom = TEXT_Y + TEXT_HEIGHT;

    DrawText(hdc, targetStr, -1, &rect, DT_TOP | DT_LEFT | DT_EXPANDTABS | DT_TABSTOP | (7 << 8));

    EndPaint(hTargetWnd, &ps);

    ReleaseMutex(targetMutex);
    return 0;
}
#define WPI_ERRORS_DEFINE_STRINGS
#include "WPIErrors.h"

void wpi_setWPIError_print( const char *msg, const char *context ) {
    if (context) {
	printf("WPI error: %s in %s\n", msg, context);
    } else {
	printf("WPI error: %s", msg);
    }
}

void wpi_setImaqErrorWithContext( int success, const char *msg ) {
    printf("Imaq error (%d): %s\n", success, msg);
}

