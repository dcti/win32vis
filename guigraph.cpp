// Copyright distributed.net 1997-2001 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"

#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guigraph.cpp,v 1.7 2001/01/15 08:25:05 jlawson Exp $";
#endif


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MyGraphWindow::MyGraphWindow(void) : hLogThread(NULL)
{
  // set the default ranges
  rangestart = (time_t) -1;
  rangeend = (time_t) -1;
  viewedcontest = CONTEST_RC5;

  // set the flags
  loggerstate = nologloaded;
  bStateChanged = true;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MyGraphWindow::~MyGraphWindow(void)
{
  if (hLogThread != NULL &&
      hLogThread != INVALID_HANDLE_VALUE)
  {
    WaitForSingleObject(hLogThread, INFINITE);
    CloseHandle(hLogThread);
  }
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

static MyGraphWindow::contest_t __ParseContest(const char *stamp)
{
  if (strncmp(stamp, "RC5", 3) == 0) return MyGraphWindow::CONTEST_RC5;
  else if (strncmp(stamp, "DES", 3) == 0) return MyGraphWindow::CONTEST_DES;
  else if (strncmp(stamp, "CSC", 3) == 0) return MyGraphWindow::CONTEST_CSC;
  else if (strncmp(stamp, "OGR", 3) == 0) return MyGraphWindow::CONTEST_OGR;
  else return MyGraphWindow::CONTEST_UNKNOWN;
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
//   [Dec 16 03:25:59 UTC] Completed CSC packet 00205AE7:80000000 (4*2^28 keys)
//                         0.00:22:46.15 - [786,534.65 keys/sec]
//  ---
//   [Jul 21 10:01:55 UTC] Completed OGR stub 24/2-9-13-29-15 (9,743,881,734 nodes)
//                         0.00:45:22.11 - [3,579,527.43 nodes/sec]
//  ---
//   [Nov 09 03:38:02 UTC] RC5: Completed (1.00 stats units)
//                         0.00:02:43.43 - [176,486 keys/s]

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Initiates immediate log reading.  Does not return until log
// reading is complete.

void MyGraphWindow::ReadLogData(void)
{
  // reset storage.
  logdata.erase(logdata.begin(), logdata.end());
  mintime = maxtime = 0;
  minrate = maxrate = 0;
  totalkeys = 0;

  // open up the log.
  FILE *fp = fopen(LogGetCurrentLogFilename(), "rt");
  if (fp != NULL)
  {
    // start parsing the log
    bool gotfirst = false;
    char linebuffer[500];
    time_t lasttimestamp = 0, timestampadd = 0;
    while (!feof(fp))
    {
      // parse the line
      if (!fgets(linebuffer, sizeof(linebuffer), fp)) break;
      if (linebuffer[0] == '[')
      {
        char *completedptr = strstr(linebuffer, "Completed ");
        if (completedptr != NULL)
        {
          MyGraphEntry ge;
          contest_t gecontest;
          bool bKeycountOrStatUnits;    // true=keycount, false=statunits.

          // parse timestamp from first line.
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
            bKeycountOrStatUnits = false;       // stat units.
            gecontest = __ParseContest(completedptr - 5);
            if (gecontest == CONTEST_UNKNOWN) continue;
          } else if (compprech == ']') {
            bKeycountOrStatUnits = true;        // keycount.
            gecontest = __ParseContest(completedptr + 10);
            if (gecontest == CONTEST_UNKNOWN) {
              if (strncmp(completedptr + 10, "block", 5) == 0) {
                gecontest = CONTEST_RC5;
              } else {
                continue;
              }
            }
          } else {
            continue;
          }
          if (gecontest != viewedcontest) {
            // ignore blocks for contests that are not currently being viewed.
            continue;
          }

          // parse the keycount from the first line.
          char *p = strchr(linebuffer, '(');
          if (p != NULL) {
            if (!bKeycountOrStatUnits || gecontest == CONTEST_OGR) {
              // reported count is already in "stats units" format.
              ge.statunits = (double) atol(p + 1);
            } else {
              // reported count is in keycounts, so divide by the workunit size (2**28).
              ge.statunits = ((double) atol(p + 1)) / 268435456.0;
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
            strcpy(s,s+1);      // assume strcpy always copies forward.
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
    loggerstate = logloaded;
    bStateChanged = true;
  }
  else
  {
    loggerstate = lognotfound;
    bStateChanged = true;
  }
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

  ts->that->ReadLogData();
  InvalidateRect(ts->hwnd, NULL, TRUE);
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
    MessageBox(NULL, "Failed to create log parsing thread.",
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
  if (loggerstate == loadinprogress)
  {
    return "Please wait, currently reloading log file.";
  }
  else if (loggerstate == lognotfound)
  {
    return "Could not load any data for graphing.  This may "
        "indicate that there was a problem opening the log file.";
  }
  else if (loggerstate == nologloaded)
  {
    return "You must specify a log file to be used for graph visualization.";
  }
  else if (loggerstate == loginvalid)
  {
    return "Could not load any data for graphing.  This may "
        "indicate that there was no graphable data inside of the "
        "specified log file.";
  }
  else if (loggerstate == logloaded)
  {
    return "Log file successfully loaded.";
  }
  return "Unknown state.";
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
      POINT point1 = PointToClientCoords((long)(lasttime + 0.25 * datapoint.duration), 0);
      POINT point2 = PointToClientCoords((long)(datapoint.timestamp - 0.25 * datapoint.duration), 0);
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
    sprintf(buffer, "%.1f", y / 1000.0);
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
    strftime(buffer, sizeof(buffer), "%b %d\n%H:%M", gmt);
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
  HFONT rotatedfont = CreateFont(tmet.tmHeight, 0, 900, 900,
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
  RECT graphrect;

  // compute size of the rectangles
  graphrect.left = clientrect.left + 55;
  graphrect.top = clientrect.top + 10;
  graphrect.right = clientrect.right - 20;
  graphrect.bottom = clientrect.bottom - 45;
  if (graphrect.right <= graphrect.left ||
    graphrect.bottom <= graphrect.top) return TRUE;

  if (loggerstate != logloaded) {
    return TRUE;
  }

  if (logdata.empty() || minrate == maxrate || mintime == maxtime) {
    bStateChanged = true;
    loggerstate = loginvalid;
    return TRUE;
  }


  // determine the range window that we will draw
  time_t timelo = (rangestart == (time_t) -1 ? mintime : rangestart);
  time_t timehi = (rangeend == (time_t) -1 ? maxtime : rangeend);
  if (timehi <= timelo) return TRUE;


  // select the font that we'll use and get the dimensions
#if defined(DEFAULT_GUI_FONT)
  HGDIOBJ oldfont = SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
#else
  HGDIOBJ oldfont = SelectObject(dc, GetStockObject(ANSI_VAR_FONT));
#endif


  // set up the graphing window structure and scaling
  MyPaintHelper paintstr(dc, &graphrect, minrate, maxrate, timelo, timehi);

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
  for (LogDataStorage_t::iterator pointiter = logdata.begin();
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
            (viewedcontest == CONTEST_OGR ?
                  "Work Unit noderate (nodes/sec)" :
                  "Work Unit keyrate (kkeys/sec)"), clientrect);

  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

UINT MyGraphWindow::GetViewedContestMenuId(void) const
{
  switch (viewedcontest) {
    case CONTEST_RC5: return IDM_CONTEST_RC5;
    case CONTEST_DES: return IDM_CONTEST_DES;
    case CONTEST_CSC: return IDM_CONTEST_CSC;
    case CONTEST_OGR: return IDM_CONTEST_OGR;
  }
  return 0;
}

bool MyGraphWindow::SetViewedContestByMenuId(UINT menuid)
{
  switch (menuid) {
    case IDM_CONTEST_RC5: viewedcontest = CONTEST_RC5; return true;
    case IDM_CONTEST_DES: viewedcontest = CONTEST_DES; return true;
    case IDM_CONTEST_CSC: viewedcontest = CONTEST_CSC; return true;
    case IDM_CONTEST_OGR: viewedcontest = CONTEST_OGR; return true;
  }
  return false;
}

