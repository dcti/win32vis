// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"

#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guiapp.cpp,v 1.4 1999/12/08 08:09:48 jlawson Exp $";
#endif


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#pragma argsused
int WINAPI WinMain(
    HINSTANCE hInstance,          // handle of current instance
    HINSTANCE hPrevInstance,      // handle of previous instance
    LPSTR lpszCmdLine,            // pointer to command line
    int nCmdShow                  // show state of window
   )
{
  WNDCLASSEX wcex;

  // initialize the common controls.
  InitCommonControls();
  if (!RegisterSliderRangeClass())
    MessageBox(NULL, "Slider Range class registration failed.", NULL, MB_OK);
  

  // Register the window class.
  memset(&wcex, 0, sizeof(wcex));
  wcex.cbSize = sizeof(wcex);
  wcex.lpfnWndProc = (WNDPROC) Main_WindowProc;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
  wcex.lpszMenuName = MAKEINTRESOURCE(IDM_MENU1);
  wcex.hIconSm = (HICON) LoadImage(hInstance,
      MAKEINTRESOURCE(IDI_ICON_MAIN), IMAGE_ICON,
      GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
      LR_SHARED);     //LR_LOADREALSIZE
  wcex.lpszClassName = "DnetLogVis";
  if (!RegisterClassEx(&wcex))
  {
    MessageBox(NULL, "Failed to register window class.", NULL, MB_OK | MB_ICONERROR);
    return 1;
  }

  // Create an instance of the window.
  HWND hwnd = NULL;
  if (!(hwnd = CreateWindowEx(0, wcex.lpszClassName, PROG_DESC_LONG,
      WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_THICKFRAME,
      CW_USEDEFAULT, CW_USEDEFAULT, 620, 370,
      NULL, NULL, hInstance, NULL)))
  {
    MessageBox(NULL, "Window creation failed.", NULL, MB_OK | MB_ICONERROR);
    return 1;
  }

  // display the open dialog file on startup
  Main_CmOpenLogfile(hwnd);

  // Run the message loop.
  MSG msg;
  while (GetMessage(&msg, NULL, NULL, NULL) == TRUE)
    DispatchMessage(&msg);


  return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


