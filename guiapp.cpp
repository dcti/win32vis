// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"

#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guiapp.cpp,v 1.1 1999/09/07 18:46:09 jlawson Exp $";
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#if (CLIENT_OS == OS_WIN16) || (CLIENT_OS == OS_WIN32S)
static bool ewpifound = false;

BOOL CALLBACK ewpi(HWND hwnd, LPARAM)
{
  char title[100];
  ::GetWindowText(hwnd, title, sizeof(title));
  if (strncmpi(title, PROG_DESC_LONG, sizeof(PROG_DESC_LONG)) == 0)
    ewpifound = true;
  return !ewpifound;
}
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#pragma argsused
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPSTR lpszCmdLine, int nCmdShow)
{
#if (CLIENT_OS == OS_WIN16) || (CLIENT_OS == OS_WIN32S)
  // verify that ntohl/ntohl work properly (mozock.dll doesn't)
  if (ntohl(1) == 0 || htonl(1) == 0)
  {
    MessageBox(NULL, "Your Winsock implementation is unusable with this program.",
      NULL, MB_ICONHAND | MB_OK);
    return 0;
  }
#endif
  
  {
    // Initialize the Virtual Windows Class Library
    if ( VGetApp()->Initialize(hInstance, nCmdShow, IDM_MENU1, IDI_ICON_MAIN) )
    {
      // Complete app initialization
      VGetApp()->AppTitle(PROG_DESC_LONG);

      // Create and show main dialog window
#if (CLIENT_OS == OS_WIN32)
      // on full win32, we want a status bar at the bottom of our window
      MyClientWindow().Create(PROG_DESC_LONG, WS_OVERLAPPEDWINDOW, NULL, true);
#else
      MyClientWindow().Create(PROG_DESC_LONG);
#endif
    }
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


