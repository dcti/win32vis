// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"


#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guiwind.cpp,v 1.4 1999/09/10 09:59:09 jlawson Exp $";
#endif


MyGraphWindow graphwin;
static char __currentlogfilename[200] = {0};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

const char *LogGetCurrentLogFilename(void)
{
  return __currentlogfilename;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK Main_WindowProc(
    HWND  hwnd,	// handle of window
    UINT  uMsg,	// message identifier
    WPARAM  wParam,	// first message parameter
    LPARAM  lParam 	// second message parameter
   )
{
  switch (uMsg)
  {
    case WM_CREATE:
    {
      // Allow normal window creation to finish.
      LRESULT result = DefWindowProc(hwnd, uMsg, wParam, lParam);

      // Create the status window.
      CreateStatusWindow((SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE),
          "Ready", hwnd, IDC_STATUSBAR);

      return result;
    }

    case WM_DESTROY:
      PostQuitMessage(0);
      break;      

//
//SendMessage(GetDlgItem(hwnd, IDC_STATUSBAR), SB_SETTEXT, 0, "Blah");

    case WM_INITMENU:
      break;

    case WM_SIZE:
      SendMessage(GetDlgItem(hwnd, IDC_STATUSBAR), WM_SIZE, wParam, lParam);
      InvalidateRect(hwnd, NULL, TRUE);
      break;

    case WM_PAINT:
    {
      PAINTSTRUCT m_ps;
      RECT clientrect, statusrect;
      int savedcontext;

      if (BeginPaint(hwnd, &m_ps))
      {      
        if ((savedcontext = SaveDC(m_ps.hdc)) != 0)
        {
          // Compute the rectangle of the client paint area,
          // not including the status bar and the window borders.
          // This is probably not the best way to do it, but this
          // is the easiest way I could manage it.
          GetClientRect(hwnd, &clientrect);
          GetClientRect(GetDlgItem(hwnd, IDC_STATUSBAR), &statusrect);
          clientrect.bottom -= (statusrect.bottom - statusrect.top);
          clientrect.left += GetSystemMetrics(SM_CXFRAME);
          clientrect.right -= GetSystemMetrics(SM_CXFRAME);

          // actually perform the repaint operation.
          graphwin.DoRedraw(m_ps.hdc, clientrect);
        }
        RestoreDC(m_ps.hdc, savedcontext);
        EndPaint(hwnd, &m_ps);
      }
      return FALSE;
    }

    case WM_COMMAND:
    {
      WORD wNotifyCode = HIWORD(wParam); // notification code 
      WORD wID = LOWORD(wParam);         // control identifier 
      
      if (wNotifyCode == 0)
      {
        if (wID == IDM_EXIT)
        {
          PostMessage(hwnd, WM_CLOSE, 0, 0);
          return FALSE;
        }
        else if (wID == IDM_ABOUT)
        {
          Main_CmAbout(hwnd);
          return FALSE;
        }
        else if (wID == IDM_OPENLOGFILE)
        {
          Main_CmOpenLogfile(hwnd);
          return FALSE;
        }
      }
      break;
    }

    case WM_RBUTTONDOWN:
    {
      // require that a logfile is already loaded.
      if (graphwin.IsDataAvailable())
      {      
        // bring up the configuration dialog
        MyGraphConfig config;
        graphwin.GetDataRange(config.datastart, config.dataend);
        graphwin.GetRange(config.starttime, config.endtime);
        config.DoModal(hwnd);
        graphwin.SetRange(config.starttime, config.endtime);

        // force a redraw
        InvalidateRect(hwnd, NULL, TRUE);
      }
      return FALSE;
    }

  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void Main_CmOpenLogfile(HWND hwnd)
{
  OPENFILENAME ofn;
  char filterarray[] = { "Log files (*.LOG)\0" "*.LOG\0"
          "All files (*.*)\0" "*.*\0" "\0\0" };
  
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFilter = filterarray;
  ofn.lpstrFile = __currentlogfilename;
  ofn.nMaxFile = sizeof(__currentlogfilename);
  ofn.lpstrTitle = "Select distributed.net logfile to load";
  ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  ofn.lpstrDefExt = "LOG";
  
  if (GetOpenFileName(&ofn))
    graphwin.LogRereadNeeded(hwnd);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void Main_CmAbout(HWND hwnd)
{
  const char *buffer =
      PROG_DESC_LONG ", " PROG_VERSION_STR "\n"
      "Distributed Computing Technologies, Inc.\n"
      "http://www.distributed.net/\n\n"
      "Programmed by Jeff \"Bovine\" Lawson <bovine@distributed.net>\n\n"
      "\nPlease send questions about this program to <help@distributed.net>";

  MSGBOXPARAMS msgbox;
  msgbox.cbSize = sizeof(MSGBOXPARAMS);
  msgbox.hwndOwner = hwnd;
  msgbox.hInstance = (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE);
  msgbox.lpszText = buffer;
  msgbox.lpszCaption = "About this Program";
  msgbox.dwStyle = MB_USERICON | MB_OK;
  msgbox.lpszIcon = MAKEINTRESOURCE(IDI_ICON_MAIN);
  msgbox.dwContextHelpId = 0;
  msgbox.lpfnMsgBoxCallback = NULL;
  msgbox.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
  MessageBoxIndirect(&msgbox);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

