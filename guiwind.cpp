// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"


#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guiwind.cpp,v 1.1 1999/09/07 18:46:09 jlawson Exp $";
#endif


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static char __currentlogfilename[200] = {0};

const char *LogGetCurrentLogFilename(void)
{
  return __currentlogfilename;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL MyClientWindow::PreCreateWindow(LPCREATESTRUCT cs)
{
  // set default window dimensions
  cs->cx = 620;
  cs->cy = 370;
  return VMainWindow::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL MyClientWindow::PostCreateWindow()
{
  AutoMenuEnable(TRUE);

  // create the child window
  graphwin.Create(GetSafeWindow());
  graphwin.ClientInitDone();

  // allow the default initializer do its work too
  return VMainWindow::PostCreateWindow();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void MyClientWindow::RecalcLayout(LPRECT lpRect)
{
  graphwin.MoveWindow(lpRect);
  VMainWindow::RecalcLayout(lpRect);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LRESULT MyClientWindow::WindowProc(HWND hWnd, UINT nMessage,
    WPARAM wParam, LPARAM lParam)
{
  // Be on the lookout for message(s) we need to know about
  switch ( nMessage )
  {
    case WM_MENUSELECT:
      // don't let vMainWindow process this or it'll change the status bar
      return FALSE;

    case WM_INITMENU:
      OnInitMenu((HMENU)wParam);
      break;
  }
  return VMainWindow::WindowProc(hWnd, nMessage, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#pragma argsused
void MyClientWindow::OnInitMenu(HMENU hMenu)
{
  // we would enable or disable menu items just before the menu pops up here.
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int MyClientWindow::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndControl)
{
  if (wNotifyCode != 0)
  {
    return VMainWindow::OnCommand(wNotifyCode, wID, hWndControl);
  }
  if (wID == IDM_EXIT)
  {
    PostMessage(WM_CLOSE, 0, 0);
  }
  else if (wID == IDM_ABOUT)
  {
    CmAbout();
  }
  else if (wID == IDM_OPENLOGFILE)
  {
    CmOpenLogfile();
  }
  else
  {
    return VMainWindow::OnCommand(wNotifyCode, wID, hWndControl);
  }
  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void MyClientWindow::CmOpenLogfile(void)
{
  OPENFILENAME ofn;
  char filterarray[] = { "Log files (*.LOG)\0" "*.LOG\0"
          "All files (*.*)\0" "*.*\0" "\0\0" };
  
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = GetSafeWindow();
  ofn.lpstrFilter = filterarray;
  ofn.lpstrFile = __currentlogfilename;
  ofn.nMaxFile = sizeof(__currentlogfilename);
  ofn.lpstrTitle = "Select distributed.net logfile to load";
  ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  ofn.lpstrDefExt = "LOG";
  
  if (GetOpenFileName(&ofn))
  {
    graphwin.LogRereadNeeded();
  }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void MyClientWindow::CmAbout(void)
{
  const char *buffer =
      "Distributed Computing Log Visualizer, " PROG_VERSION_STR "\n"
      "Distributed Computing Technologies, Inc.\n"
      "http://www.distributed.net/\n\n"
      "Programmed in VWCL by Jeff Lawson (BovineOne)\n"
      "<bovine@distributed.net> http://www.bovine.net/\n\n"
      "\nPlease send questions about the client to <help@distributed.net>";

  #if (CLIENT_OS == OS_WIN32) || (CLIENT_OS == OS_WIN32S)
    MSGBOXPARAMS msgbox;
    msgbox.cbSize = sizeof(MSGBOXPARAMS);
    msgbox.hwndOwner = GetSafeWindow();
    msgbox.hInstance = VGetApp()->GetInstanceHandle();
    msgbox.lpszText = buffer;
    msgbox.lpszCaption = "About this Program";
    msgbox.dwStyle = MB_USERICON | MB_OK;
    msgbox.lpszIcon = MAKEINTRESOURCE(IDI_ICON_MAIN);
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    MessageBoxIndirect(&msgbox);
  #else
    MessageBox(buffer, "About this Program", MB_OK | MB_ICONINFORMATION);
  #endif
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

