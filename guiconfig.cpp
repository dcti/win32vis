// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"

#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guiconfig.cpp,v 1.1 1999/09/09 09:02:09 jlawson Exp $";
#endif

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


int MyGraphConfig::DoModal(HWND parent)
{
  if (datastart == dataend) return FALSE;
  return VDialog::DoModal(parent, IDD_GRAPHCFG);
}

int MyGraphConfig::OnInitDialog(HWND hWndFocus, LPARAM lParam)
{
  // attach the range slider to the control
#if 0
  HWND placeholder = GetDlgItem(IDC_SLIDERRANGE);
  RECT placerect, parentrect;
  ::GetWindowRect(GetSafeWindow(), &parentrect);
  ::GetWindowRect(placeholder, &placerect);
  ::DestroyWindow(placeholder);
  placerect.left -= parentrect.left;
  placerect.right -= parentrect.left;
  placerect.top -= parentrect.top;
  placerect.bottom -= parentrect.top;
  rangeslider.Create(&placerect, GetSafeWindow(), IDC_SLIDERRANGE);
#else
  rangeslider.Attach(GetDlgItem(IDC_SLIDERRANGE));
#endif

  // validate ranges.
  while (datastart > dataend) dataend += 365*24*60*60;
  if (starttime != (time_t) -1 && (starttime < datastart || starttime > dataend))
    starttime = (time_t) -1;
  if (endtime != (time_t) -1 && (endtime > dataend || endtime < datastart))
    endtime = (time_t) -1;
  if (endtime != (time_t) -1 && starttime != (time_t) -1 && endtime < starttime)
    endtime = (time_t) -1;


  // combo start box
  combostart.Attach(GetDlgItem(IDC_COMBOSTART));
  combostart.AddString("Start of logfile");
  combostart.AddString("User specified date");
  if (starttime == (time_t) -1 || dataend == datastart)
  {
    userstart = 0;
    combostart.SetCurSel(0);
    rangeslider.SetLeft(0);
    SetDlgItemGMT(IDS_STARTDATE, datastart);
  } else {
    userstart = starttime;
    combostart.SetCurSel(1);
    double percent = (double) (starttime - datastart) /
        (double) (dataend - datastart);
    rangeslider.SetLeft( (int) (percent * rangeslider.GetMaxVal()) );
    SetDlgItemGMT(IDS_STARTDATE, userstart);
  }
  for (int i = 0; graphevents[i].text; i++)
    if (graphevents[i].timestamp >= datastart &&
        graphevents[i].timestamp <= dataend)
    {
      int newindex = combostart.AddString(graphevents[i].text);
      if (graphevents[i].timestamp == starttime)
        combostart.SetCurSel(newindex);
    }


  // combo end box
  comboend.Attach(GetDlgItem(IDC_COMBOEND));
  comboend.AddString("End of logfile");
  comboend.AddString("User specified date");
  if (endtime == (time_t) -1 || dataend == datastart)
  {
    userend = 0;
    comboend.SetCurSel(0);
    rangeslider.SetRight(rangeslider.GetMaxVal());
    SetDlgItemGMT(IDS_ENDDATE, dataend);
  } else {
    userend = endtime;
    comboend.SetCurSel(1);
    double percent = (double) (userend - datastart) / (double) (dataend - datastart);
    rangeslider.SetRight( (int) (percent * rangeslider.GetMaxVal()) );
    SetDlgItemGMT(IDS_ENDDATE, userend);
  }
  for (int j = 0; graphevents[j].text; j++)
    if (graphevents[j].timestamp >= datastart &&
        graphevents[j].timestamp <= dataend)
    {
      int newindex = comboend.AddString(graphevents[j].text);
      if (graphevents[j].timestamp == endtime)
        comboend.SetCurSel(newindex);
    }


  // process standard initialization
  return VDialog::OnInitDialog(hWndFocus, lParam);
}

void MyGraphConfig::SetDlgItemGMT(int nID, time_t timestamp)
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
  SetDlgItemText(nID, buffer);
}

int MyGraphConfig::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndControl)
{
  if (wID == IDC_SLIDERRANGE)
  {
    // compute the new user-entered values
    double maxval = (double) rangeslider.GetMaxVal();
    if (maxval == 0) return FALSE;
    if (wNotifyCode == SRN_LEFTCHANGED)
    {
      if (maxval)
      {
        double percent = (double) rangeslider.GetLeft() / maxval;
        userstart = (time_t) (percent * (dataend - datastart) + datastart);
        combostart.SetCurSel(1);
        SetDlgItemGMT(IDS_STARTDATE, userstart);
        combostart.SetFocus();
      }
    }
    else if (wNotifyCode == SRN_RIGHTCHANGED)
    {
      if (maxval)
      {
        double percent = (double) rangeslider.GetRight() / maxval;
        userend = (time_t) (percent * (dataend - datastart) + datastart);
        comboend.SetCurSel(1);
        SetDlgItemGMT(IDS_ENDDATE, userend);
        comboend.SetFocus();
      }
    }
    return FALSE;
  }
  // ---------------------
  else if (wID == IDC_COMBOSTART && wNotifyCode == CBN_SELCHANGE)
  {
    int newchoice = combostart.GetCurSel();
    if (dataend == datastart) return FALSE;
    if (newchoice == 0)   // start of log
    {
      SetDlgItemGMT(IDS_STARTDATE, datastart);
      rangeslider.SetLeft(0);
      return FALSE;
    }
    else if (newchoice == 1)  // user specified
    {
      if (userstart == 0) userstart = datastart;
      SetDlgItemGMT(IDS_STARTDATE, userstart);
      double percent = (double) (userstart - datastart) / (double) (dataend - datastart);
      rangeslider.SetLeft( (int) (percent * rangeslider.GetMaxVal()) );
      return FALSE;
    }
    else for (int i = 0; graphevents[i].text; i++)  // drop down choices
    {
      char buffer[50];
      combostart.GetLBText(i, buffer);
      if (strcmp(buffer, graphevents[i].text) == 0)
      {
        SetDlgItemGMT(IDS_STARTDATE, graphevents[i].timestamp);
        double percent = (double) (graphevents[i].timestamp - datastart) / (double) (dataend - datastart);
        rangeslider.SetLeft( (int) (percent * rangeslider.GetMaxVal()) );
        return FALSE;
      }
    }
    SetDlgItemText(IDS_STARTDATE, "unknown");
  }
  // ---------------------
  else if (wID == IDC_COMBOEND && wNotifyCode == CBN_SELCHANGE)
  {
    int newchoice = comboend.GetCurSel();
    if (dataend == datastart) return FALSE;
    if (newchoice == 0)   // end of log
    {
      SetDlgItemGMT(IDS_ENDDATE, dataend);
      rangeslider.SetRight(rangeslider.GetMaxVal());
      return FALSE;
    }
    else if (newchoice == 1)    // user specified
    {
      if (userend == 0) userend = dataend;
      SetDlgItemGMT(IDS_ENDDATE, userend);
      double percent = (double) (userend - datastart) / (double) (dataend - datastart);
      rangeslider.SetRight( (int) (percent * rangeslider.GetMaxVal()) );
      return FALSE;
    }
    else for (int i = 0; graphevents[i].text; i++)    // drop down choice
    {
      char buffer[50];
      combostart.GetLBText(i, buffer);
      if (strcmp(buffer, graphevents[i].text) == 0)
      {
        SetDlgItemGMT(IDS_ENDDATE, graphevents[i].timestamp);
        double percent = (double) (graphevents[i].timestamp - datastart) / (double) (dataend - datastart);
        rangeslider.SetRight( (int) (percent * rangeslider.GetMaxVal()) );
        return FALSE;
      }
    }
    SetDlgItemText(IDS_ENDDATE, "unknown");
  }
  return VDialog::OnCommand(wNotifyCode, wID, hWndControl);
}

BOOL MyGraphConfig::OnOK(void)
{
  // starting time
  int newchoice = combostart.GetCurSel();
  if (newchoice == 0)   // start of log
  {
    starttime = (time_t) -1;
  }
  else                  // user specified or drop down
  {
    int maxval = rangeslider.GetMaxVal();
    if (maxval != 0) {
      double percent = (double) rangeslider.GetLeft() / maxval;
      starttime = datastart + (time_t) (percent * (dataend - datastart));
    } else starttime = (time_t) -1;
  }

  // ending time
  newchoice = comboend.GetCurSel();
  if (newchoice == 0)   // end of log
  {
    endtime = (time_t) -1;
  }
  else                  // user specified or drop down
  {
    int maxval = rangeslider.GetMaxVal();
    if (maxval != 0) {
      double percent = (double) rangeslider.GetRight() / maxval;
      endtime = datastart + (time_t) (percent * (dataend - datastart));
    } else endtime = (time_t) -1;
  }

  return VDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


