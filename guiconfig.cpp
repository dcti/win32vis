// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"

#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guiconfig.cpp,v 1.2 1999/09/10 09:36:00 jlawson Exp $";
#endif


#define UNUSEDMBZ     0       // Unused, must be zero.

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct EventType
{
  char *text;
  time_t timestamp;
};

static EventType graphevents [] = {
  {"End of RC5-56", 854438400L},        // January 28, 1997 13:25 GMT
  {"Start of DESII-1", 884674800L},     // January 13, 1998 at 09:00 PST
  {"End of DESII-1", 888197160L},       // February 23, 1998 at 02:26 PST
  {"Start of DESII-2", 877260300L},     // Mon Jul 13 09:00:00 1998 PST
  {NULL, 0} };


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static void SetDlgItemGMT(HWND hwndParent, int nID, time_t timestamp)
{
  char buffer[50];
  if (timestamp < 0)
  {
    buffer[0] = 0;
  }
  else
  {
    struct tm *gmt = gmtime(&timestamp);
    if (gmt)
      strftime(buffer, sizeof(buffer), "%b %d %H:%M", gmt);
    else
      buffer[0] = 0;
  }
  SetDlgItemText(hwndParent, nID, buffer);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int MyGraphConfig::DoModal(HWND parent)
{
  if (datastart == dataend) return FALSE;

  return DialogBoxParam(
    (HINSTANCE) GetWindowLong(parent, GWL_HINSTANCE),
    MAKEINTRESOURCE(IDD_GRAPHCFG), parent,
    (DLGPROC) MyGraphConfig::DialogProc, (LPARAM) this);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK MyGraphConfig::DialogProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
  static MyGraphConfig *mgc = NULL;
  
  switch (uMsg)
  {
    case WM_INITDIALOG:
      mgc = (MyGraphConfig *) lParam;
      return mgc->OnInitDialog(hwndDlg, (HWND) wParam, lParam);

    case WM_COMMAND:
      return mgc->OnCommand(hwndDlg, HIWORD(wParam),
          LOWORD(wParam), (HWND) lParam);
  };
  return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#pragma argsused
int MyGraphConfig::OnInitDialog(HWND hwndDlg, HWND hWndFocus, LPARAM lParam)
{
  // attach the range slider to the control
  HWND rangeslider = GetDlgItem(hwndDlg, IDC_SLIDERRANGE);

  // validate ranges.
  while (datastart > dataend) dataend += 365*24*60*60;
  if (starttime != (time_t) -1 && (starttime < datastart || starttime > dataend))
    starttime = (time_t) -1;
  if (endtime != (time_t) -1 && (endtime > dataend || endtime < datastart))
    endtime = (time_t) -1;
  if (endtime != (time_t) -1 && starttime != (time_t) -1 && endtime < starttime)
    endtime = (time_t) -1;


  // combo start box
  HWND combostart = GetDlgItem(hwndDlg, IDC_COMBOSTART);
  SendMessage(combostart, CB_ADDSTRING, UNUSEDMBZ, (LPARAM) "Start of logfile");
  SendMessage(combostart, CB_ADDSTRING, UNUSEDMBZ, (LPARAM) "User specified date");
  if (starttime == (time_t) -1 || dataend == datastart)
  {
    userstart = 0;
    SendMessage(combostart, CB_SETCURSEL, 0, UNUSEDMBZ);
    SendMessage(rangeslider, SRM_SETLEFT, UNUSEDMBZ, 0);
    SetDlgItemGMT(hwndDlg, IDS_STARTDATE, datastart);
  } else {
    userstart = starttime;
    SendMessage(combostart, CB_SETCURSEL, 1, UNUSEDMBZ);
    double percent = (double) (starttime - datastart) /
        (double) (dataend - datastart);
    SendMessage(rangeslider, SRM_SETLEFT, UNUSEDMBZ, (int) (percent *
        SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ)) );
    SetDlgItemGMT(hwndDlg, IDS_STARTDATE, userstart);
  }
  for (int i = 0; graphevents[i].text; i++)
    if (graphevents[i].timestamp >= datastart &&
        graphevents[i].timestamp <= dataend)
  {
    int newindex = SendMessage(combostart, CB_ADDSTRING, UNUSEDMBZ,
        (LPARAM) graphevents[i].text);
    if (graphevents[i].timestamp == starttime)
      SendMessage(combostart, CB_SETCURSEL, newindex, UNUSEDMBZ);
  }


  // combo end box
  HWND comboend = GetDlgItem(hwndDlg, IDC_COMBOEND);
  SendMessage(comboend, CB_ADDSTRING, UNUSEDMBZ, (LPARAM) "End of logfile");
  SendMessage(comboend, CB_ADDSTRING, UNUSEDMBZ, (LPARAM) "User specified date");
  if (endtime == (time_t) -1 || dataend == datastart)
  {
    userend = 0;
    SendMessage(comboend, CB_SETCURSEL, 0, UNUSEDMBZ);
    SendMessage(rangeslider, SRM_SETRIGHT, UNUSEDMBZ, (int)
        SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ) );
    SetDlgItemGMT(hwndDlg, IDS_ENDDATE, dataend);
  } else {
    userend = endtime;
    SendMessage(comboend, CB_SETCURSEL, 1, UNUSEDMBZ);
    double percent = (double) (userend - datastart) / (double) (dataend - datastart);
    SendMessage(rangeslider, SRM_SETRIGHT, UNUSEDMBZ, (int) (percent *
        SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ)) );
    SetDlgItemGMT(hwndDlg, IDS_ENDDATE, userend);
  }
  for (int j = 0; graphevents[j].text; j++)
    if (graphevents[j].timestamp >= datastart &&
        graphevents[j].timestamp <= dataend)
  {
    int newindex = SendMessage(comboend, CB_ADDSTRING, UNUSEDMBZ,
      (LPARAM) graphevents[j].text);
    if (graphevents[j].timestamp == endtime)
      SendMessage(comboend, CB_SETCURSEL, newindex, UNUSEDMBZ);
  }


  return TRUE;        // allow focus to go to first control.
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#pragma argsused 
int MyGraphConfig::OnCommand(
    HWND hwndDlg,
    WORD wNotifyCode,
    WORD wID,
    HWND hWndControl)
{
  HWND combostart = GetDlgItem(hwndDlg, IDC_COMBOSTART);
  HWND comboend = GetDlgItem(hwndDlg, IDC_COMBOEND);
  HWND rangeslider = GetDlgItem(hwndDlg, IDC_SLIDERRANGE);

  if (wID == IDOK)
  {
    if (!OnOK(hwndDlg))
      EndDialog(hwndDlg, IDOK);
    return FALSE;
  }
  else if (wID == IDCANCEL)
  {
    EndDialog(hwndDlg, IDCANCEL);
    return FALSE;
  }
  else if (wID == IDC_SLIDERRANGE)
  {
    // compute the new user-entered values
    double maxval = (double) SendMessage(rangeslider,
        SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ);
    if (maxval == 0) return FALSE;
    if (wNotifyCode == SRN_LEFTCHANGED)
    {
      if (maxval)
      {
        double percent = (double) SendMessage(rangeslider, SRM_GETLEFT,
            UNUSEDMBZ, UNUSEDMBZ) / maxval;
        userstart = (time_t) (percent * (dataend - datastart) + datastart);
        SendMessage(combostart, CB_SETCURSEL, 1, UNUSEDMBZ);
        SetDlgItemGMT(hwndDlg, IDS_STARTDATE, userstart);
        SetFocus(combostart);
      }
    }
    else if (wNotifyCode == SRN_RIGHTCHANGED)
    {
      if (maxval)
      {
        double percent = (double) SendMessage(rangeslider, SRM_GETRIGHT,
            UNUSEDMBZ, UNUSEDMBZ) / maxval;
        userend = (time_t) (percent * (dataend - datastart) + datastart);
        SendMessage(comboend, CB_SETCURSEL, 1, UNUSEDMBZ);
        SetDlgItemGMT(hwndDlg, IDS_ENDDATE, userend);
        SetFocus(comboend);
      }
    }
    return FALSE;
  }
  // ---------------------
  else if (wID == IDC_COMBOSTART && wNotifyCode == CBN_SELCHANGE)
  {
    int newchoice = (int) SendMessage(combostart, CB_GETCURSEL,
        UNUSEDMBZ, UNUSEDMBZ);
    if (dataend == datastart) return FALSE;
    if (newchoice == 0)   // start of log
    {
      SetDlgItemGMT(hwndDlg, IDS_STARTDATE, datastart);
      SendMessage(rangeslider, SRM_SETLEFT, UNUSEDMBZ, 0);
      return FALSE;
    }
    else if (newchoice == 1)  // user specified
    {
      if (userstart == 0) userstart = datastart;
      SetDlgItemGMT(hwndDlg, IDS_STARTDATE, userstart);
      double percent = (double) (userstart - datastart) / (double) (dataend - datastart);
      SendMessage(rangeslider, SRM_SETLEFT, UNUSEDMBZ, (int) (percent *
          SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ)) );
      return FALSE;
    }
    else for (int i = 0; graphevents[i].text; i++)  // drop down choices
    {
      char buffer[50];
      SendMessage(combostart, CB_GETLBTEXT, i, (LPARAM) buffer);
      if (strcmp(buffer, graphevents[i].text) == 0)
      {
        SetDlgItemGMT(hwndDlg, IDS_STARTDATE, graphevents[i].timestamp);
        double percent = (double) (graphevents[i].timestamp - datastart) / (double) (dataend - datastart);
        SendMessage(rangeslider, SRM_SETLEFT, UNUSEDMBZ, (int) (percent *
            SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ)) );
        return FALSE;
      }
    }
    SetDlgItemText(hwndDlg, IDS_STARTDATE, "unknown");
  }
  // ---------------------
  else if (wID == IDC_COMBOEND && wNotifyCode == CBN_SELCHANGE)
  {
    int newchoice = SendMessage(comboend, CB_GETCURSEL, UNUSEDMBZ, UNUSEDMBZ);
    if (dataend == datastart) return FALSE;
    if (newchoice == 0)   // end of log
    {
      SetDlgItemGMT(hwndDlg, IDS_ENDDATE, dataend);
      SendMessage(rangeslider, SRM_SETRIGHT, UNUSEDMBZ,
          SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ));
      return FALSE;
    }
    else if (newchoice == 1)    // user specified
    {
      if (userend == 0) userend = dataend;
      SetDlgItemGMT(hwndDlg, IDS_ENDDATE, userend);
      double percent = (double) (userend - datastart) / (double) (dataend - datastart);
      SendMessage(rangeslider, SRM_SETRIGHT, UNUSEDMBZ, (int) (percent *
          SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ)) );
      return FALSE;
    }
    else for (int i = 0; graphevents[i].text; i++)    // drop down choice
    {
      char buffer[50];
      SendMessage(combostart, CB_GETLBTEXT, i, (LPARAM) buffer);
      if (strcmp(buffer, graphevents[i].text) == 0)
      {
        SetDlgItemGMT(hwndDlg, IDS_ENDDATE, graphevents[i].timestamp);
        double percent = (double) (graphevents[i].timestamp - datastart) /
            (double) (dataend - datastart);
        SendMessage(rangeslider, SRM_SETRIGHT, UNUSEDMBZ, (int) (percent *
            SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ)) );
        return FALSE;
      }
    }
    SetDlgItemText(hwndDlg, IDS_ENDDATE, "unknown");
  }
  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#pragma argsused
BOOL MyGraphConfig::OnOK(HWND hwndDlg)
{
  HWND combostart = GetDlgItem(hwndDlg, IDC_COMBOSTART);
  HWND comboend = GetDlgItem(hwndDlg, IDC_COMBOEND);
  HWND rangeslider = GetDlgItem(hwndDlg, IDC_SLIDERRANGE);

  // starting time
  int newchoice = SendMessage(combostart, CB_GETCURSEL, UNUSEDMBZ, UNUSEDMBZ);
  if (newchoice == 0)   // start of log
  {
    starttime = (time_t) -1;
  }
  else                  // user specified or drop down
  {
    int maxval = SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ);
    if (maxval != 0) {
      double percent = (double) SendMessage(rangeslider, SRM_GETLEFT,
          UNUSEDMBZ, UNUSEDMBZ) / maxval;
      starttime = datastart + (time_t) (percent * (dataend - datastart));
    } else starttime = (time_t) -1;
  }

  // ending time
  newchoice = SendMessage(comboend, CB_GETCURSEL, UNUSEDMBZ, UNUSEDMBZ);
  if (newchoice == 0)   // end of log
  {
    endtime = (time_t) -1;
  }
  else                  // user specified or drop down
  {
    int maxval = SendMessage(rangeslider, SRM_GETMAXRANGE, UNUSEDMBZ, UNUSEDMBZ);
    if (maxval != 0) {
      double percent = (double) SendMessage(rangeslider, SRM_GETRIGHT,
          UNUSEDMBZ, UNUSEDMBZ) / maxval;
      endtime = datastart + (time_t) (percent * (dataend - datastart));
    } else endtime = (time_t) -1;
  }

  return FALSE;     // allow closure.
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


