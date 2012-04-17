// DriverVision.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "DriverVision.h"
#include "AxisCamera.h"
#include "Target.h"
#include "BitmapImage.h"

#define MAX_LOADSTRING 100

#define CAMERA_IP "10.14.25.11"
#define	CAMERA2_IP "10.14.25.12"
#define	THRESHOLD 240

#define	IMAGE1_X	660
#define	IMAGE1_WIDTH	640
#define	IMAGE1_Y	0
#define	IMAGE1_HEIGHT	480

#define	IMAGE2_X	0
#define	IMAGE2_WIDTH	640
#define	IMAGE2_Y	0
#define	IMAGE2_HEIGHT	480

#define	TEXT_X		660
#define	TEXT_WIDTH	640
#define	TEXT_Y		490
#define	TEXT_HEIGHT	60

#define	WINDOW_WIDTH	1300
#define	WINDOW_HEIGHT	610


// Global Variables:
HINSTANCE hInst;				// current instance
HWND hAppWnd, hTxtWnd;
TCHAR targetStr[400];
TCHAR szTitle[MAX_LOADSTRING];			// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];		// the main window class name
AxisCamera *pCamera = NULL;
AxisCamera *pCamera2 = NULL;
Target *pTarget = NULL;
Image *processedImage = NULL;
Image *secondImage = NULL;
HANDLE appSync;
bool targetValid = false;
Target::TargetLocation tlTop, tlLeft, tlRight, tlBottom, tlCenter;

// Forward declarations of functions included in this code module:
ATOM		    MyRegisterClass(HINSTANCE hInstance);
BOOL		    InitInstance(HINSTANCE, LPTSTR, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

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
    LoadString(hInstance, IDC_DRIVERVISION, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, lpCmdLine, nCmdShow))
    {
	return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DRIVERVISION));

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

    wcex.style		= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= WndProc;
    wcex.cbClsExtra	= 0;
    wcex.cbWndExtra	= 0;
    wcex.hInstance	= hInstance;
    wcex.hIcon		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DRIVERVISION));
    wcex.hCursor	= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_DRIVERVISION);
    wcex.lpszClassName	= szWindowClass;
    wcex.hIconSm	= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

DWORD WINAPI RunApp(LPVOID param)
{
    HWND hWnd = (HWND) param;
    HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, TEXT("DriverVision"));
    Image *cameraImage = imaqCreateImage(IMAQ_IMAGE_RGB, 3);
    for (;;) {
	bool update = false;
	if (pCamera) {
	    if (pCamera->GetImage(cameraImage)) {
		WaitForSingleObject(hMutex, INFINITE);
		if (targetValid = pTarget->ProcessImage(cameraImage, THRESHOLD)) {
		    // printf("ProcessImage returns true\n");
		    tlCenter = pTarget->GetTargetLocation(Target::kCenter);
		    tlTop    = pTarget->GetTargetLocation(Target::kTop);
		    tlBottom = pTarget->GetTargetLocation(Target::kBottom);
		    tlLeft   = pTarget->GetTargetLocation(Target::kLeft);
		    tlRight  = pTarget->GetTargetLocation(Target::kRight);
		}
		ReleaseMutex(hMutex);
		update = true;
	    } else {
		fprintf(stderr, "Camera 1 GetImage failed\n");
	    }
	}
	if (pCamera2) {
	    if (pCamera2->GetImage(cameraImage)) {
		WaitForSingleObject(hMutex, INFINITE);
		if (imaqDuplicate(secondImage, cameraImage) != 0) {
		    update = true;
		}
		ReleaseMutex(hMutex);
	    } else {
		fprintf(stderr, "Camera 2 GetImage failed\n");
	    }
	}
	if (update) {
	    InvalidateRect(hWnd, NULL, false);
	}
    }

    return 0;
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

    if (!hAppWnd)
    {
	return FALSE;
    }

    hTxtWnd = CreateWindow(TEXT("STATIC"), NULL, WS_CHILD | WS_VISIBLE,
			    TEXT_X, TEXT_Y, TEXT_WIDTH, TEXT_HEIGHT,
			    hAppWnd, NULL, hInst, NULL);

    if (lpCmdLine && _tcslen(lpCmdLine) > 0) {
//	pCamera = new AxisCamera(lpCmdLine);
	pCamera2 = NULL;
	secondImage = NULL;
    } else {
//	pCamera = new AxisCamera(TEXT(CAMERA_IP));
	pCamera2 = new AxisCamera(TEXT(CAMERA2_IP));
	secondImage = imaqCreateImage(IMAQ_IMAGE_RGB, 3);
    }
    pTarget = new Target();
    processedImage = imaqCreateImage(IMAQ_IMAGE_RGB, 3);
    appSync = CreateMutex(NULL, FALSE, TEXT("DriverVision"));

    CreateThread(NULL, 0, RunApp, hAppWnd, 0, NULL);

    ShowWindow(hAppWnd, nCmdShow);
    UpdateWindow(hAppWnd);

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

bool GetProcessedImage( Image *pImage )
{
    if (pTarget) {
	int result = pTarget->GetProcessedImage(pImage);
	return (result != 0);
    }
    return false;
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

void Paint(HWND hWnd)
{
    WaitForSingleObject(appSync, INFINITE);

    // note: repaint all windows rather than just the one indicated by hWnd
    RECT rect;
    PAINTSTRUCT ps;
    GetClientRect(hAppWnd, &rect);
    HDC hdc = BeginPaint(hAppWnd, &ps);
    if (GetProcessedImage(processedImage)) {
	BitmapImage* bmpImage = new BitmapImage(processedImage);
	HBITMAP hbmp = bmpImage->GetBitmap();
	BITMAP bmp;
	GetObject(hbmp, sizeof(BITMAP), &bmp);
	HDC hdcMem = CreateCompatibleDC(hdc);
	SelectObject(hdcMem, hbmp);
	// FillRect(hdc, &rect, GetSysColorBrush(COLOR_BACKGROUND));
	BitBlt(hdc, IMAGE1_X, IMAGE1_Y, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);
	DeleteDC(hdcMem);
	delete bmpImage;
    } else {
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    }
    if (secondImage) {
	BitmapImage* bmpImage = new BitmapImage(secondImage);
	HBITMAP hbmp = bmpImage->GetBitmap();
	BITMAP bmp;
	GetObject(hbmp, sizeof(BITMAP), &bmp);
	HDC hdcMem = CreateCompatibleDC(hdc);
	SelectObject(hdcMem, hbmp);
	BitBlt(hdc, IMAGE2_X, IMAGE2_Y, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);
	DeleteDC(hdcMem);
	delete bmpImage;
    }
    EndPaint(hAppWnd, &ps);

targetValid = true;
    if (targetValid) {
	// TBD: use a dynamic String object here
	// TBD: proportionally-spaced fonts mess up this formatting
	_stprintf_s(targetStr,
	    TEXT("top   visible %d angle %6.1f distance %6.1f speed %5.0f\n")
	    TEXT("left   visible %d angle %6.1f distance %6.1f speed %5.0f\n")
	    TEXT("right visible %d angle %6.1f distance %6.1f speed %5.0f\n") ,
	    tlTop.valid,    tlTop.angle,    tlTop.distance,   Ballistics(2, tlTop.distance),
	    tlLeft.valid,   tlLeft.angle,   tlLeft.distance,  Ballistics(1, tlLeft.distance),
	    tlRight.valid,  tlRight.angle,  tlRight.distance, Ballistics(1, tlRight.distance) );
    } else {
	targetStr[0] = TEXT('\0');
    }
    SetWindowText(hTxtWnd, targetStr);

    ReleaseMutex(appSync);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
    case WM_PAINT:
	Paint(hWnd);
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

