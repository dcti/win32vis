// Copyright distributed.net 1997-2004 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"

#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guigraph.cpp,v 1.18 2004/07/04 20:58:28 jlawson Exp $";
#endif


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MyGraphWindow::MyGraphWindow(void) : hLogThread(NULL)
{
  // set the default ranges
  rangestart = (time_t) -1;
  rangeend = (time_t) -1;
  viewedproject = PROJECT_RC5_72;

  // set the flags
  loggerstate = nologloaded;
  bStateChanged = true;
  InitializeCriticalSection(&loggerbusy);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MyGraphWindow::~MyGraphWindow(void)
{
  if (hLogThread != NULL &&
      hLogThread != INVALID_HANDLE_VALUE)
  {
    if (WaitForSingleObject(hLogThread, 15*1000) == WAIT_OBJECT_0) {
      TerminateThread(hLogThread, 0);
    }
    CloseHandle(hLogThread);
  }
  DeleteCriticalSection(&loggerbusy);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


static time_t __ParseTimestamp(const char *stamp)
{
  struct tm t;
  if (isdigit(*stamp))
  {
    if (sscanf(stamp, "%u/%u/%u %u:%u:%u",
      &t.tm_mon, &t.tm_mday, &t.tm_year,
      &t.tm_hour, &t.tm_min, &t.tm_sec) != 6) return 0;
  } else {
    char monthname[20];
    char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (sscanf(stamp, "%5s %u %u:%u:%u",
      monthname, &t.tm_mday,
      &t.tm_hour, &t.tm_min, &t.tm_sec) != 5) return 0;
    t.tm_mon = 0;
    for (int i = 0; i < 12; i++)
      if (strcmp(monthname, months[i]) == 0) { t.tm_mon = (i + 1); break; }
    t.tm_year = 1998;    // hard coded for now
  }

  // correct the date to a full 4-digit year
  if (t.tm_year < 0) return 0;
  else if (t.tm_year < 70) t.tm_year += 2000;
  else if (t.tm_year < 100) t.tm_year += 1900;

  // validate all fields
  if (t.tm_mon < 1 || t.tm_mon > 12 ||
      t.tm_mday < 1 || t.tm_mday > 31 ||
      t.tm_year < 1970 || t.tm_year >= 2038 ||
      t.tm_hour < 0 || t.tm_hour > 23 ||
      t.tm_min < 0 || t.tm_min > 59 ||
      t.tm_sec < 0 || t.tm_sec > 59) return 0;

  // convert the fields to the tm standard
  t.tm_mon--;
  t.tm_year -= 1900;
  time_t result = mktime(&t);
  if (result == -1) return 0;
  else return (result - _timezone);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static double __ParseDuration(const char *stamp)
{
  int days = 0, hours, mins;
  float secs;
  if (sscanf(stamp, "%d:%d:%f", &hours, &mins, &secs) != 3)
  {
    if (sscanf(stamp, "%d.%d:%d:%f", &days, &hours, &mins, &secs) != 4)
    {
      return 0;
    }
  }
  return (double) (24.0 * days + hours) * 3600.0 + (double) mins * 60.0 + (double) secs;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static MyGraphWindow::project_t __ParseProject(const char *stamp)
{
  //MessageBox(NULL, stamp, NULL, MB_OK | MB_ICONERROR);
  if (strncmp(stamp, "RC5-72", 6) == 0) return MyGraphWindow::PROJECT_RC5_72;
  else if (strncmp(stamp, "RC5", 3) == 0) return MyGraphWindow::PROJECT_RC5;
  else if (strncmp(stamp, "DES", 3) == 0) return MyGraphWindow::PROJECT_DES;
  else if (strncmp(stamp, "CSC", 3) == 0) return MyGraphWindow::PROJECT_CSC;
  else if (strncmp(stamp, "OGR-P2", 6) == 0) return MyGraphWindow::PROJECT_OGR_P2;
  else if (strncmp(stamp, "OGR", 3) == 0) return MyGraphWindow::PROJECT_OGR;
  else return MyGraphWindow::PROJECT_UNKNOWN;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Past logging formats of the client (older to newer):
//   [03/18/98 19:59:39 GMT] Completed block 00002752:E8100000 (2097152 keys)
//                          00:00:03.75 - [517825.30 keys/sec]
//  ---
//   [May 31 23:24:19 GMT] Completed RC5 block 687C9CC2:40000000 (1073741824 keys)
//                        0.00:19:29.52 - [918103.14 keys/sec]
//  ---
//   [Jul 18 03:00:57 GMT] Completed RC5 block 6DE46FD9:00000000 (2147483648 keys)
//   [Jul 18 03:00:57 GMT] 0.01:59:18.82 - [299,977.15 keys/sec]
//  ---
//   [Jan 06 19:19:06 UTC] Completed one DES block 001CFE87:B0000000 (8*2^28 keys)
//                         0.00:27:13.87 - [2,501,662.63 keys/sec]
//  ---
//   [Oct 13 18:32:18 UTC] Completed one RC5 packet CC302DB9:70000000 (32*2^28 keys)
//                         0.01:22:33.51 - [1,092,516.37 keys/sec]
//  ---
//   [Dec 16 03:25:59 UTC] Completed CSC packet 00205AE7:80000000 (4*2^28 keys)
//                         0.00:22:46.15 - [786,534.65 keys/sec]
//  ---
//   [Jul 21 10:01:55 UTC] Completed OGR stub 24/2-9-13-29-15 (9,743,881,734 nodes)
//                         0.00:45:22.11 - [3,579,527.43 nodes/sec]
//  ---
//   [Nov 09 03:38:02 UTC] RC5: Completed (1.00 stats units)
//                         0.00:02:43.43 - [176,486 keys/s]
//  ---
//   [Dec 22 09:07:32 UTC] RC5-72: Completed CA:407B9B7F:00000000 (1.00 stats units)
//                         0.01:25:53.63 - [833,398 keys/s]
//  ---
//   [Jul 04 09:42:34 UTC] OGR-P2: Completed 25/1-27-20-11-5-14 (38.59 stats units)
//                         0.00:59:05.85 - [10,884,020 nodes/s]

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Initiates immediate log reading.  Does not return until log
// reading is complete.

void MyGraphWindow::ReadLogData(void)
{
  EnterCriticalSection(&loggerbusy);

  loggerstate = loadinprogress;
  bStateChanged = true;

  // reset storage.
  logdata.erase(logdata.begin(), logdata.end());
  mintime = maxtime = 0;
  minrate = maxrate = 0;
  totalkeys = 0;
  for (int ii = 0; ii < PROJECT_NEXTUNUSED; ii++) {
    haveprojectdata[ii] = false;
  }

  // open up the log.
  FILE *fp = fopen(LogGetCurrentLogFilename(), "rt");
  if (fp != NULL)
  {
    // start parsing the log
    bool gotfirst = false;
    char linebuffer[500];
    time_t lasttimestamp = 0, timestampadd = 0;
    while (!feof(fp) && fgets(linebuffer, sizeof(linebuffer), fp) != NULL)
    {
      // parse the line
      if (linebuffer[0] == '[')
      {
        char *completedptr = strstr(linebuffer, "Completed ");
        if (completedptr != NULL)
        {
          MyGraphEntry ge;
          project_t geproject = PROJECT_UNKNOWN;
          bool bKeycountOrStatUnits;    // true=keycount, false=statunits.

          // parse timestamp from first line.
          // Unfortunately the log timestamps are all missing years, so
          // we just make a guess at what year things actually happened.
          ge.timestamp = __ParseTimestamp(linebuffer + 1);
          if (ge.timestamp == 0) continue;
          if (lasttimestamp != 0)
          {
            ge.timestamp += timestampadd;
            if (ge.timestamp + (6*30*24*60*60) < lasttimestamp)
            {
              ge.timestamp -= timestampadd;
              timestampadd += 365*24*60*60;
              ge.timestamp += timestampadd;
            }
          }
          lasttimestamp = ge.timestamp;

          // parse the project type from the first line.
          char compprech = *(completedptr - 2);
          if (compprech == ':') {
            // project name comes before colon, ie: "PRJ: Completed"
            bKeycountOrStatUnits = false;       // stat units.
            for (char *constart = completedptr - 3; ; constart--) {
              if (constart < linebuffer || *constart == ' ') {
                geproject = __ParseProject(constart + 1);
                break;
              }
            }
          } else if (compprech == ']') {
            // very old format.
            bKeycountOrStatUnits = true;        // keycount.
            geproject = __ParseProject(completedptr + 10);
            if (geproject == PROJECT_UNKNOWN) {
              if (strncmp(completedptr + 10, "block", 5) == 0) {
                // some very old clients did not indicate project ("Completed block")
                geproject = PROJECT_RC5;
              } else if (strncmp(completedptr + 10, "one", 3) == 0) {
                // some old RC5DES clients indicated "Completed one xxx block".
                geproject = __ParseProject(completedptr + 14);
              }
            }
          }
          haveprojectdata[geproject] = true;
          if (geproject == PROJECT_UNKNOWN || geproject != viewedproject) {
            // ignore blocks for projects that are not currently being viewed.
            continue;
          }

          // parse the keycount from the first line.
          char *p = strchr(linebuffer, '(');
          if (p != NULL) {
            if (!bKeycountOrStatUnits || strstr(p, "stats units") != NULL) {
              // reported count is already in "stats units" format.
              ge.statunits = (double) atof(p + 1);
            } else if (strchr(p, '*') != NULL) {
              // old style: reported count is like "32*2^28 keys"
              ge.statunits = (double) atoi(p + 1);
            } else if (strstr(p, "nodes") != NULL) {
              // old style: reported count is like "9,743,881,734 nodes"
              while (strchr(p,',') != NULL) // get rid of commas
              {
                char *s=strchr(p,',');
                memmove(s, s+1, strlen(s));
              }
              ge.statunits = ((double) atof(p)) / 1e9;   // 1 billion nodes per stats unit.
            } else {
              // old style: reported count is in keycounts, so divide by the workunit size (2**28).
              ge.statunits = ((double) atof(p + 1)) / 268435456.0;
            }
          } else {
            continue;
          }

          // parse the keyrate from the second line.
          if (!fgets(linebuffer, sizeof(linebuffer), fp)) break;
          char *q = strchr(linebuffer, '[');
          if (q == NULL) continue;
          char *r = strchr(q+1, '['); // with newer logs, we need the second [
          if (r != NULL) q=r;
          while (strchr(q+1,',') != NULL) // get rid of commas
          {
            char *s=strchr(q+1,',');
            memmove(s, s+1, strlen(s));
          }
          ge.rate = atof(q + 1);

          // parse the completion time duration from the second line.
          char *t=strchr(linebuffer,']'); // search for end of timestamp
          if (t < q) // new log format w/timestamp
            ge.duration = __ParseDuration(t+1);
          else if (t > q) // old log format w/o timestamp
            ge.duration = __ParseDuration(linebuffer);


          // add to our list
          logdata.push_back(ge);
          if (!gotfirst)
          {
            mintime = maxtime = ge.timestamp;
            minrate = maxrate = ge.rate;
            gotfirst = true;
          }
          else
          {
            if (ge.timestamp < mintime) mintime = ge.timestamp;
            if (ge.timestamp > maxtime) maxtime = ge.timestamp;
            if (ge.rate < minrate) minrate = ge.rate;
            if (ge.rate > maxrate) maxrate = ge.rate;
          }
        }
      }
    };

    // close the log
    fclose(fp);


    // try to use the last-modified timestamp of the log file to adjust
    // the years of all of the collected data points.
    struct stat st;
    if (stat(LogGetCurrentLogFilename(), &st) == 0) {
      // add 5 days of padding to allow for people with misadjusted clocks.
      if (lasttimestamp < st.st_mtime - (365*24*60*60) + (5*24*60*60)) {
        int delta = st.st_mtime - lasttimestamp + (5*24*60*60);
        delta -= (delta % (365*24*60*60));    // only adjust by a whole number of years.

        // adjust the data points and the time window.
        for (LogDataStorage_t::iterator pointiter = logdata.begin();
            pointiter != logdata.end(); pointiter++) {
          pointiter->timestamp += delta;
        }
        mintime += delta;
        maxtime += delta;
      }
    }


    loggerstate = logloaded;
    bStateChanged = true;
  }
  else
  {
    loggerstate = lognotfound;
    bStateChanged = true;
  }

  LeaveCriticalSection(&loggerbusy);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct threadstruct
{
  MyGraphWindow *that;
  HWND hwnd;
};

// Handler function for the background thread that loads and parses logs.
// Using a separate thread ensures that the event processing loop is
// not blocked during this long-duration process.

long MyGraphWindow::LogParseThread(long lParam)
{
  threadstruct *ts = (threadstruct*) lParam;

  // set the mouse cursor to busy.
  HCURSOR waitcursor = LoadCursor(NULL, IDC_WAIT);
  HCURSOR oldcursor = SetCursor(waitcursor);
  InvalidateRect(ts->hwnd, NULL, TRUE);

  // load the log file.
  ts->that->ReadLogData();

  // queue the update events.
  InvalidateRect(ts->hwnd, NULL, TRUE);
  //PostMessage(ts->hwnd, WM_INITMENU, (WPARAM) GetMenu(ts->hwnd), 0);

  // restore the mouse cursor.
  SetCursor(oldcursor);
  DestroyCursor(waitcursor);

  delete ts;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void MyGraphWindow::LogRereadNeeded(HWND hwnd)
{
  DWORD exitcode, threadid;

  // return immediately if a worker thread is still executing.
  // otherwise close the old thread handle if we still have one.
  if (hLogThread == NULL ||
      hLogThread == INVALID_HANDLE_VALUE) {} // nothing.
  else if (GetExitCodeThread(hLogThread, &exitcode) == FALSE ||
          exitcode != STILL_ACTIVE) CloseHandle(hLogThread);
  else return;

  // create the new worker thread.
  threadstruct *ts = new threadstruct;
  ts->that = this;
  ts->hwnd = hwnd;
  hLogThread = CreateThread(NULL, 0,
      (LPTHREAD_START_ROUTINE) LogParseThread,
      (LPVOID) ts, 0, &threadid);
  if (!hLogThread)
  {
    MessageBox(hwnd, "Failed to create log parsing thread.",
        NULL, MB_ICONERROR | MB_OK);
    delete ts;
  }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

const char *MyGraphWindow::GetStatusString(void)
{
  bStateChanged = false;
  switch (loggerstate) {
  case loadinprogress:
    return "Please wait, currently reloading log file...";
  case lognotfound:
    return "Could not load any data for graphing.  This may "
        "indicate that there was a problem opening the log file.";
  case nologloaded:
    return "You must specify a log file to be used for graph visualization.";
  case loginvalid:
    {
      for (int ii = 1; ii < PROJECT_NEXTUNUSED; ii++) {
        if (ii != viewedproject && haveprojectdata[ii]) 
          return "Could not load any data for graphing.  Pick another project to view.";
      }
      return "Could not load any data for graphing.  This may "
          "indicate that there was no graphable data inside of the "
          "specified log file.";
    }
  case logloaded:
    return "Log file successfully loaded.";
  default:
    return "Unknown state.";
  }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class MyPaintHelper
{
private:
  // Windows GDI graphics items.
  HDC dc;
  TEXTMETRIC tmet;

  // used for connecting between points.
  bool firstpoint;
  double lastrate;
  time_t lasttime;

  // defined window region for painting.
  double minrate, maxrate;
  time_t mintime, maxtime;
  RECT window;

  // width of the colored columns in the graph.
  double yinterval;
  time_t xinterval;


protected:
  POINT PointToClientCoords(time_t time, double rate) const;

public:
  MyPaintHelper(HDC _dc, LPCRECT _window,
                double _minrate, double _maxrate,
                time_t _mintime, time_t _maxtime);

  void FillBackgroundArea(HBRUSH back1, HBRUSH back2);
  void PaintGridLines(HPEN hpen);
  void DrawToDataPoint(const MyGraphEntry &datapoint);
  void DrawAxisLabelsAndTicks(void);
  void DisplayXAxisLabelDescription(const char *xlabel, const RECT &clientrect);
  void DisplayYAxisLabelDescription(const char *ylabel, const RECT &clientrect);
};


MyPaintHelper::MyPaintHelper(HDC _dc, LPCRECT _window,
              double _minrate, double _maxrate,
              time_t _mintime, time_t _maxtime) :
  dc(_dc), firstpoint(true),
  minrate(_minrate), maxrate(_maxrate),
  mintime(_mintime), maxtime(_maxtime)
{
  memcpy(&window, _window, sizeof(RECT));
  GetTextMetrics(dc, &tmet);
  yinterval = (maxrate - minrate) / (window.bottom - window.top) * (tmet.tmHeight * 4);
  xinterval = (maxtime - mintime) / (window.right - window.left) * (tmet.tmAveCharWidth * 10);
}


POINT MyPaintHelper::PointToClientCoords(time_t time, double rate) const
{
  double x = (double) (time - mintime) / (double) (maxtime - mintime);
  double y = (double) (rate - minrate) / (double) (maxrate - minrate);

  // force to within range
  if (x < 0) x = 0;
  else if (x > 1) x = 1;
  if (y < 0) y = 0;
  else if (y > 1) y = 1;

  // convert to screen coords
  POINT tmp;
  tmp.x = window.left + (int) ((window.right - window.left) * x);
  tmp.y = window.bottom - (int) ((window.bottom - window.top) * y);
  return tmp;
}

void MyPaintHelper::FillBackgroundArea(HBRUSH back1, HBRUSH back2)
{
  firstpoint = true;
  FillRect(dc, &window, back1);
  for (time_t t = mintime; t < maxtime; t += 2 * xinterval)
  {
    RECT shade;
    POINT point = PointToClientCoords(t, maxrate);
    shade.top = point.y;
    shade.left = point.x;
    point = PointToClientCoords(t + xinterval, minrate);
    shade.bottom = point.y;
    shade.right = point.x;
    FillRect(dc, &shade, back2);
  }
}


void MyPaintHelper::PaintGridLines(HPEN hpen)
{
  HGDIOBJ oldpen = SelectObject(dc, hpen);
  for (double r = minrate + yinterval; r < maxrate; r += yinterval)
  {
    POINT left = PointToClientCoords(mintime, r);
    POINT right = PointToClientCoords(maxtime, r);
    MoveToEx(dc, left.x, left.y, NULL);
    LineTo(dc, right.x, right.y);
  }
  SelectObject(dc, oldpen);
}


void MyPaintHelper::DrawToDataPoint(const MyGraphEntry &datapoint)
{
  if (!&datapoint) return;

  if ((datapoint.timestamp >= mintime) &&
      (datapoint.timestamp <= maxtime))
  {
    POINT point = PointToClientCoords(datapoint.timestamp, datapoint.rate);
    if (firstpoint)
    {
      // this is the first point being drawn.
      MoveToEx(dc, point.x, point.y, NULL);
      firstpoint = false;
    }
    else if (bShowIdleDrops && (datapoint.timestamp - lasttime) > 300 + 1.25 * datapoint.duration)
    {
      // There was a significant lapse in time since the last point,
      // which probably indicates that the client was turned off for
      // awhile, so draw a "drop" in the keyrate graph.
      POINT point1 = PointToClientCoords((long)(lasttime + 0.5 * datapoint.duration), 0);
      POINT point2 = PointToClientCoords((long)(datapoint.timestamp - 0.5 * datapoint.duration), 0);
      LineTo(dc, point1.x, point1.y);
      LineTo(dc, point2.x, point2.y);
      LineTo(dc, point.x, point.y);
    }
    else
    {
      // otherwise just connect the line from the last one.
      LineTo(dc, point.x, point.y);
    }
    lasttime = datapoint.timestamp;
    lastrate = datapoint.rate;
  }
}

void MyPaintHelper::DrawAxisLabelsAndTicks(void)
{
  // change to a small black pen for drawing the tick marks.
  HGDIOBJ oldbrush = SelectObject(dc, GetSysColorBrush(COLOR_WINDOWTEXT));
  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, RGB(0, 0, 0));
  SelectObject(dc, GetStockObject(BLACK_PEN));

  // draw the y-axis labels and ticks
  for (double y = minrate; y <= maxrate; y += yinterval)
  {
    // draw the tick mark
    POINT point = PointToClientCoords(0, y);
    MoveToEx(dc, point.x, point.y, NULL);
    LineTo(dc, point.x - 6, point.y);

    // draw the text
    char buffer[30];
#ifdef HAVE_SNPRINTF
    _snprintf(buffer, sizeof(buffer), "%.1f", y / 1000.0);
#else
    sprintf(buffer, "%.1f", y / 1000.0);
#endif
    RECT rect = {0, point.y - tmet.tmHeight,
        point.x - 6, point.y + tmet.tmHeight};
    DrawText(dc, buffer, -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
  }

  // draw the x-axis labels and tick marks
  for (time_t x = mintime; x <= maxtime; x += xinterval)
  {
    // draw the tick mark
    POINT point = PointToClientCoords(x, 0);
    MoveToEx(dc, point.x, point.y, NULL);
    LineTo(dc, point.x, point.y + 6);

    // draw the text
    char buffer[30];
    struct tm *gmt = gmtime(&x);
    strftime(buffer, sizeof(buffer), "%Y\n%b %d\n%H:%M", gmt);
    RECT rect = {point.x - tmet.tmAveCharWidth * 10, point.y + 6,
        point.x + tmet.tmAveCharWidth * 10, point.y + 6 + 3 * tmet.tmHeight};
    DrawText(dc, buffer, -1, &rect, DT_CENTER);
  }
  SelectObject(dc, oldbrush);
}


void MyPaintHelper::DisplayXAxisLabelDescription(const char *xlabel, const RECT &clientrect)
{
  HFONT unrotatedfont = CreateFont(tmet.tmHeight, 0, 0, 0,
      FW_DONTCARE, false, false, false, DEFAULT_CHARSET,
      OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
      DEFAULT_PITCH | FF_DONTCARE, "Arial");
  HFONT oldfont = (HFONT) SelectObject(dc, unrotatedfont);
  SIZE xlabelsize;
  GetTextExtentPoint32(dc, xlabel, strlen(xlabel), &xlabelsize);
  TextOut(dc, window.left + (window.right - window.left - xlabelsize.cx) / 2,
      clientrect.bottom - xlabelsize.cy, xlabel, strlen(xlabel));
  SelectObject(dc, oldfont);
  DeleteObject(unrotatedfont);
}

void MyPaintHelper::DisplayYAxisLabelDescription(const char *ylabel, const RECT &clientrect)
{
  // The rotated font size is at 9/10th normal size to compensate
  // for the perceived size increase on rotated fonts.
  HFONT rotatedfont = CreateFont(tmet.tmHeight * 9/10, 0, 900, 900,
      FW_DONTCARE, false, false, false, DEFAULT_CHARSET,
      OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
      DEFAULT_PITCH | FF_DONTCARE, "Arial");
  HFONT oldfont = (HFONT) SelectObject(dc, rotatedfont);
  SIZE ylabelsize;
  GetTextExtentPoint32(dc, ylabel, strlen(ylabel), &ylabelsize);
  TextOut(dc, clientrect.left, window.top + ylabelsize.cx +
    (window.bottom - window.top - ylabelsize.cx) / 2, ylabel, strlen(ylabel));
  SelectObject(dc, oldfont);
  DeleteObject(rotatedfont);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Window repaint handler, called in response to WM_PAINT handling.
int MyGraphWindow::DoRedraw(HDC dc, RECT clientrect)
{
  if (!TryEnterCriticalSection(&loggerbusy)) {
    return TRUE;
  }

  HCURSOR waitcursor = LoadCursor(NULL, IDC_WAIT);
  HCURSOR oldcursor = SetCursor(waitcursor);

  // select the font that we'll use and get the dimensions
#if defined(DEFAULT_GUI_FONT)
  HGDIOBJ oldfont = SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
#else
  HGDIOBJ oldfont = SelectObject(dc, GetStockObject(ANSI_VAR_FONT));
#endif


  TEXTMETRIC tmet;
  GetTextMetrics(dc, &tmet);

  // compute size of the rectangles
  RECT graphrect;
  graphrect.left = clientrect.left + tmet.tmHeight * 2 + tmet.tmAveCharWidth * 8;  // room on left for y-axis label and y-axis tickmarks
  graphrect.top = clientrect.top + 10;
  graphrect.right = clientrect.right - 20;
  graphrect.bottom = clientrect.bottom - tmet.tmHeight * 5;   // room on bottom for x-axis label and x-axis tick marks
  if (graphrect.right <= graphrect.left || graphrect.bottom <= graphrect.top) {
    SelectObject(dc, oldfont);
    SetCursor(oldcursor);
    DestroyCursor(waitcursor);
    LeaveCriticalSection(&loggerbusy);
    return TRUE;
  }

  if (loggerstate != logloaded) {
    SelectObject(dc, oldfont);
    SetCursor(oldcursor);
    DestroyCursor(waitcursor);
    LeaveCriticalSection(&loggerbusy);
    return TRUE;
  }

  if (logdata.empty() || minrate == maxrate || mintime == maxtime) {
    bStateChanged = true;
    loggerstate = loginvalid;
    SelectObject(dc, oldfont);
    SetCursor(oldcursor);
    DestroyCursor(waitcursor);
    LeaveCriticalSection(&loggerbusy);
    return TRUE;
  }


  // determine the range window that we will draw
  time_t timelo = (rangestart == (time_t) -1 ? mintime : rangestart);
  time_t timehi = (rangeend == (time_t) -1 ? maxtime : rangeend);
  if (timehi <= timelo) {
    SelectObject(dc, oldfont);
    SetCursor(oldcursor);
    DestroyCursor(waitcursor);
    LeaveCriticalSection(&loggerbusy);
    return TRUE;
  }



  // set up the graphing window structure and scaling
  MyPaintHelper paintstr(dc, &graphrect, 
                         (bShowIdleDrops ? 0 : minrate), maxrate, 
                         timelo, timehi);

  // draw the area for the graph
  HBRUSH back1 = (HBRUSH) GetStockObject(WHITE_BRUSH);
  HBRUSH back2 = CreateSolidBrush(RGB(0xEE, 0xEE, 0xEE));
  paintstr.FillBackgroundArea(back1, back2);
  DeleteObject(back2);

  // draw horizontal dashed lines
  HPEN dashedline = CreatePen(PS_DOT, 1, RGB(0, 0, 0));
  paintstr.PaintGridLines(dashedline);
  DeleteObject(dashedline);


  // start drawing the points
  HPEN graphline = CreatePen(PS_SOLID, 2, RGB(0x99, 0x33, 0x33));
  HPEN oldpen = (HPEN) SelectObject(dc, graphline);
  for (LogDataStorage_t::const_iterator pointiter = logdata.begin();
       pointiter != logdata.end(); pointiter++) {
    paintstr.DrawToDataPoint(*pointiter);
  }
  SelectObject(dc, oldpen);
  DeleteObject(graphline);


  // draw the X and Y axis ticks and tick labels.
  paintstr.DrawAxisLabelsAndTicks();
  SelectObject(dc, oldfont);


  // display the x-axis label
  paintstr.DisplayXAxisLabelDescription("Work Unit completion date", clientrect);


  // display the rotated y-axis label
  paintstr.DisplayYAxisLabelDescription(
            (viewedproject == PROJECT_OGR || viewedproject == PROJECT_OGR_P2 ?
                  "Work Unit noderate (nodes/sec)" :
                  "Work Unit keyrate (kkeys/sec)"), clientrect);

  SetCursor(oldcursor);
  DestroyCursor(waitcursor);
  LeaveCriticalSection(&loggerbusy);
  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Returns 0 on success, otherwise error.

int MyGraphWindow::ExportCSV(const char *outputfile)
{
  int retval = 0;
  EnterCriticalSection(&loggerbusy);

  FILE *fp = fopen(outputfile, "wt");
  if (fp != NULL) {
    fprintf(fp, "timestamp,rate,duration,statunits\n");

    for (LogDataStorage_t::const_iterator pointiter = logdata.begin();
        pointiter != logdata.end(); pointiter++)
    {
      char buffer[30];
      struct tm *gmt = gmtime(&pointiter->timestamp);
      if (gmt)
        strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", gmt);
      else
        strcpy(buffer, "unknown");

      fprintf(fp, "%s,%g,%g,%g\n", buffer, pointiter->rate, pointiter->duration, pointiter->statunits);
    }
    fclose(fp);
  } else {
    retval = -1;
  }

  LeaveCriticalSection(&loggerbusy);
  return retval;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

UINT MyGraphWindow::GetViewedProjectMenuId(void) const
{
  switch (viewedproject) {
    case PROJECT_RC5: return IDM_PROJECT_RC5;
    case PROJECT_RC5_72: return IDM_PROJECT_RC5_72;
    case PROJECT_DES: return IDM_PROJECT_DES;
    case PROJECT_CSC: return IDM_PROJECT_CSC;
    case PROJECT_OGR: return IDM_PROJECT_OGR;
    case PROJECT_OGR_P2: return IDM_PROJECT_OGR_P2;
  }
  return 0;
}

bool MyGraphWindow::SetViewedProjectByMenuId(UINT menuid)
{
  switch (menuid) {
    case IDM_PROJECT_RC5: viewedproject = PROJECT_RC5; return true;
    case IDM_PROJECT_RC5_72: viewedproject = PROJECT_RC5_72; return true;
    case IDM_PROJECT_DES: viewedproject = PROJECT_DES; return true;
    case IDM_PROJECT_CSC: viewedproject = PROJECT_CSC; return true;
    case IDM_PROJECT_OGR: viewedproject = PROJECT_OGR; return true;
    case IDM_PROJECT_OGR_P2: viewedproject = PROJECT_OGR_P2; return true;
  }
  return false;
}

