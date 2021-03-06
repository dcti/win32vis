// Copyright distributed.net 1997-2004 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"


#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guiwind.cpp,v 1.20 2004/07/04 20:58:28 jlawson Exp $";
#endif


bool bShowIdleDrops = true;
MyGraphWindow graphwin;
static char __currentlogfilename[200] = {0};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

const char *LogGetCurrentLogFilename(void)
{
  return __currentlogfilename;
}

void LogSetCurrentLogFilename(const char *filename, bool removeQuotes)
{
  // copy the string.
  lstrcpyn(__currentlogfilename, filename, sizeof(__currentlogfilename));
  __currentlogfilename[sizeof(__currentlogfilename) - 1] = 0;

  // remove leading and trailing quotes, if needed.
  int len = strlen(__currentlogfilename);
  if (removeQuotes && len >= 2 &&
      (__currentlogfilename[0] == '"') &&
      (__currentlogfilename[len - 1] == '"') )
  {
    memmove(__currentlogfilename, __currentlogfilename + 1, len - 2);
    __currentlogfilename[len - 2] = 0;
  }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK Main_WindowProc(
    HWND  hwnd, // handle of window
    UINT  uMsg, // message identifier
    WPARAM  wParam, // first message parameter
    LPARAM  lParam  // second message parameter
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

      // Accept drag-and-dropped files.
      DragAcceptFiles(hwnd, TRUE);

      return result;
    }

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    case WM_DROPFILES:
    {
      // get the filename of the dropped item.
      DragQueryFile((HDROP) wParam, 0,
          __currentlogfilename, sizeof(__currentlogfilename));
      DragFinish((HDROP) wParam);

      // trigger the reload to occur in the background.
      graphwin.LogRereadNeeded(hwnd);
      Main_UpdateTitlebar(hwnd);
      break;
    }

    case WM_INITMENU:
    {
      // This event is triggered each time just before a menu is going to be displayed.
      HMENU hPopup = (HMENU) wParam;

      // Enable or disable the menu items that are not valid without a logfile.
      DWORD dwEnabledWithLogAndData = (graphwin.IsDataAvailable() ? MF_ENABLED : MF_GRAYED);
      DWORD dwEnabledWithLog = (graphwin.GetStatusValue() != MyGraphWindow::nologloaded ? MF_ENABLED : MF_GRAYED);
      EnableMenuItem(hPopup, IDM_REFRESHLOGFILE, MF_BYCOMMAND | dwEnabledWithLog);
      EnableMenuItem(hPopup, IDM_GRAPHCONFIG, MF_BYCOMMAND | dwEnabledWithLogAndData);
      EnableMenuItem(hPopup, IDM_PROJECT_RC5, MF_BYCOMMAND | (graphwin.IsDataAvailable(MyGraphWindow::PROJECT_RC5) ? MF_ENABLED : MF_GRAYED));
      EnableMenuItem(hPopup, IDM_PROJECT_RC5_72, MF_BYCOMMAND | (graphwin.IsDataAvailable(MyGraphWindow::PROJECT_RC5_72) ? MF_ENABLED : MF_GRAYED));
      EnableMenuItem(hPopup, IDM_PROJECT_DES, MF_BYCOMMAND | (graphwin.IsDataAvailable(MyGraphWindow::PROJECT_DES) ? MF_ENABLED : MF_GRAYED));
      EnableMenuItem(hPopup, IDM_PROJECT_CSC, MF_BYCOMMAND | (graphwin.IsDataAvailable(MyGraphWindow::PROJECT_CSC) ? MF_ENABLED : MF_GRAYED));
      EnableMenuItem(hPopup, IDM_PROJECT_OGR, MF_BYCOMMAND | (graphwin.IsDataAvailable(MyGraphWindow::PROJECT_OGR) ? MF_ENABLED : MF_GRAYED));
      EnableMenuItem(hPopup, IDM_PROJECT_OGR_P2, MF_BYCOMMAND | (graphwin.IsDataAvailable(MyGraphWindow::PROJECT_OGR_P2) ? MF_ENABLED : MF_GRAYED));
      EnableMenuItem(hPopup, IDM_SHOWIDLE, MF_BYCOMMAND | dwEnabledWithLogAndData);
      EnableMenuItem(hPopup, IDM_EXPORT_CSV, MF_BYCOMMAND | dwEnabledWithLogAndData);

      // Enable the "full zoom" menu item only when there is data and zoomed in.
      time_t zoomleft, zoomright;
      graphwin.GetRange(zoomleft, zoomright);
      DWORD dwEnabledWithPartialZoom = (zoomleft != (time_t) -1 || zoomright != (time_t) -1) ? MF_ENABLED : MF_GRAYED;
      EnableMenuItem(hPopup, IDM_ZOOMFULL, MF_BYCOMMAND | dwEnabledWithLogAndData | dwEnabledWithPartialZoom);

      // Check or uncheck the drop menu item.
      CheckMenuItem(hPopup, IDM_SHOWIDLE, MF_BYCOMMAND |
                    (bShowIdleDrops ? MF_CHECKED : MF_UNCHECKED));
      // Set the radio button on whichever project is currently selected.
      UINT radioselect = graphwin.GetViewedProjectMenuId();
      CheckMenuRadioItem(hPopup, IDM_PROJECT_RC5, IDM_PROJECT_OGR_P2,
                         radioselect, MF_BYCOMMAND);
      return FALSE;
    }

    case WM_SIZE:
      SendMessage(GetDlgItem(hwnd, IDC_STATUSBAR), WM_SIZE, wParam, lParam);
      InvalidateRect(hwnd, NULL, TRUE);
      break;

    case WM_PAINT:
    {
      PAINTSTRUCT m_ps;
      RECT clientrect, statusrect;
      int savedcontext;

      // Repaint the window.
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

      // Update the status bar
      if (graphwin.HasStatusChanged())
        SendMessage(GetDlgItem(hwnd, IDC_STATUSBAR), SB_SETTEXT,
            0, (LPARAM) graphwin.GetStatusString());

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
        else if (wID == IDM_REFRESHLOGFILE)
        {
          graphwin.LogRereadNeeded(hwnd);
          return FALSE;
        }
        else if (wID == IDM_EXPORT_CSV)
        {
          Main_CmExportCSV(hwnd);
          return FALSE;
        }
        else if (wID == IDM_GRAPHCONFIG)
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
        else if (wID == IDM_PROJECT_RC5 ||
                 wID == IDM_PROJECT_RC5_72 ||
                 wID == IDM_PROJECT_DES ||
                 wID == IDM_PROJECT_CSC ||
                 wID == IDM_PROJECT_OGR ||
                 wID == IDM_PROJECT_OGR_P2)
        {
          if (graphwin.SetViewedProjectByMenuId(wID)) {
            // now trigger a reload from the log file.
            graphwin.LogRereadNeeded(hwnd);
          }
          return FALSE;
        }
        else if (wID == IDM_SHOWIDLE)
        {
          bShowIdleDrops = !bShowIdleDrops;

          // force a redraw
          InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (wID == IDM_ZOOMFULL)
        {
          // set zoom window to actual beginning and end of logfile.
          graphwin.SetRange((time_t) -1, (time_t) -1);

          // force a redraw
          InvalidateRect(hwnd, NULL, TRUE);
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
  {
    // trigger the reload to occur in the background.
    graphwin.LogRereadNeeded(hwnd);
    Main_UpdateTitlebar(hwnd);
  }
}

void Main_CmExportCSV(HWND hwnd)
{
  OPENFILENAME ofn;
  char filterarray[] = { "CSV (Comma delimited) (*.CSV)\0" "*.CSV\0"
          "All files (*.*)\0" "*.*\0" "\0\0" };

  char exportfilename[512] = { '\0' };

  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFilter = filterarray;
  ofn.lpstrFile = exportfilename;
  ofn.nMaxFile = sizeof(exportfilename);
  ofn.lpstrTitle = "Select new file to export to";
  ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
  ofn.lpstrDefExt = "CSV";

  if (GetSaveFileName(&ofn))
  {
    int retval = graphwin.ExportCSV(exportfilename);
    if (retval != 0) {
      MessageBox(hwnd, "Failed to write exported data to file.",
          NULL, MB_ICONERROR | MB_OK);
    }
  }         
}

void Main_UpdateTitlebar(HWND hwnd)
{
  char buff[sizeof(PROG_DESC_LONG) + sizeof(__currentlogfilename) + 10];

  // display the file name in the titlebar.
#ifdef HAVE_SNPRINTF
  _snprintf(buff, sizeof(buff),
#else
  sprintf(buff,
#endif
      "%s - [%s]", PROG_DESC_LONG, __currentlogfilename);
  SetWindowText(hwnd, buff);
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
      "Programmed by Jeff \"Bovine\" Lawson <bovine@distributed.net>\n"
      "Other work by Yang You <yang_you@yahoo.com>\n\n"
      "Please send questions about this program to <help@distributed.net>";

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

