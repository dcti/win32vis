// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#include "guiwin.h"

#if (!defined(lint) && defined(__showids__))
static char *id="@(#)$Id: guigraph.cpp,v 1.4 1999/12/08 08:09:48 jlawson Exp $";
#endif


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MyGraphWindow::MyGraphWindow(void) : hLogThread(NULL)
{
  // set the default ranges
  rangestart = (time_t) -1;
  rangeend = (time_t) -1;
  
  // set the flags
  loggerstate = nologloaded;
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


time_t MyGraphWindow::ParseTimestamp(char *stamp)
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

double MyGraphWindow::ParseDuration(char *stamp)
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

// Initiates immediate log reading.  Does not return until log
// reading is complete.

//   [03/18/98 19:59:39 GMT] Completed block 00002752:E8100000 (2097152 keys)
//                          00:00:03.75 - [517825.30 keys/sec]
//   [May 31 23:24:19 GMT] Completed RC5 block 687C9CC2:40000000 (1073741824 keys)
//                        0.00:19:29.52 - [918103.14 keys/sec]
//   [Jul 18 03:00:57 GMT] Completed RC5 block 6DE46FD9:00000000 (2147483648 keys)
//   [Jul 18 03:00:57 GMT] 0.01:59:18.82 - [299,977.15 keys/sec]

void MyGraphWindow::ReadLogData(void)
{
  // reset storage.
  logdata.Flush();
  mintime = maxtime = 0;
  minrate = maxrate = 0;
  totalkeys = 0;

  // loading now in progress.
  //UpdateWindow();

  // open up the log.
  FILE *fp = fopen(LogGetCurrentLogFilename(), "rt");
  if (fp)
  {
    // start parsing the log
    bool gotfirst = false;
    char linebuffer[500];
    time_t lasttimestamp = 0, timestampadd = 0;
    while (!feof(fp))
    {
      // parse the line
      if (!fgets(linebuffer, sizeof(linebuffer), fp)) break;
      if (linebuffer[0] == '[' && strstr(linebuffer, "] Completed "))
      {
        // parse first line
        MyGraphEntry ge;
        ge.timestamp = ParseTimestamp(linebuffer + 1);
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
        char *p = strchr(linebuffer, '(');
        if (p == NULL) continue;
        ge.keycount = (double) atol(p + 1);

        // parse second line
        if (!fgets(linebuffer, sizeof(linebuffer), fp)) break;
        char *q = strchr(linebuffer, '['); 
        if (q == NULL) continue;
        char *r = strchr(q+1, '['); // with newer logs, we need the second [
        if (r != NULL) q=r;
        while (strchr(q+1,',') != NULL) // get rid of commas
        {
          char *s=strchr(q+1,',');
          strcpy(s,s+1);
        }
        ge.rate = atof(q + 1);

        char *t=strchr(linebuffer,']'); // search for end of timestamp
          if (t < q) // new log format w/timestamp
            ge.duration = ParseDuration(t+1);
          else if (t > q) // old log format w/o timestamp
            ge.duration = ParseDuration(linebuffer);


        // add to our list
        logdata.AddAtTail(ge);
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
    };

    // close the log
    fclose(fp);
    loggerstate = logloaded;
  }
  else
  {
    loggerstate = lognotfound;
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

struct MyPaintStruct
{
  HDC dc;

  bool firstpoint;
  double lastrate;
  time_t lasttime;

  double minrate, maxrate;
  time_t mintime, maxtime;
  RECT *window;

  POINT PointToClientCoords(time_t time, double rate) const;
};

POINT MyPaintStruct::PointToClientCoords(time_t time, double rate) const
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
  tmp.x = window->left + (int) ((window->right - window->left) * x);
  tmp.y = window->bottom - (int) ((window->bottom - window->top) * y);
  return tmp;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void MyGraphWindow::IterDrawFuncRate(MyGraphEntry &datapoint, void *vptr)
{
  MyPaintStruct *paintstr = (MyPaintStruct*)vptr;
  if ((datapoint.timestamp >= paintstr->mintime) &&
      (datapoint.timestamp <= paintstr->maxtime))
  { 
    POINT point = paintstr->PointToClientCoords(datapoint.timestamp, datapoint.rate);
    if (paintstr->firstpoint)
    {
      // this is the first point being drawn.
      MoveToEx(paintstr->dc, point.x, point.y, NULL);
      paintstr->firstpoint = false;
    }
    else if ((datapoint.timestamp - paintstr->lasttime) > 300 + 1.25 * datapoint.duration)
    {
      // There was a significant lapse in time since the last point,
      // which probably indicates that the client was turned off for
      // awhile, so draw a "drop" in the keyrate graph.
      POINT point1 = paintstr->PointToClientCoords((long)(paintstr->lasttime + 0.25 * datapoint.duration), 0);
      POINT point2 = paintstr->PointToClientCoords((long)(datapoint.timestamp - 0.25 * datapoint.duration), 0);
      LineTo(paintstr->dc, point1.x, point1.y);
      LineTo(paintstr->dc, point2.x, point2.y);
      LineTo(paintstr->dc, point.x, point.y);
    }
    else
    {
      // otherwise just connect the line from the last one.
      LineTo(paintstr->dc, point.x, point.y);
    }
    paintstr->lasttime = datapoint.timestamp;
    paintstr->lastrate = datapoint.rate;
  }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Window repaint handler, called in response to WM_PAINT handling.
int	MyGraphWindow::DoRedraw(HDC dc, RECT clientrect, HWND hwnd)
{
  RECT graphrect;
  TEXTMETRIC tmet;

  // compute size of the rectangles
  graphrect.left = clientrect.left + 55;
  graphrect.top = clientrect.top + 10;
  graphrect.right = clientrect.right - 20;
  graphrect.bottom = clientrect.bottom - 45;
  if (graphrect.right <= graphrect.left ||
    graphrect.bottom <= graphrect.top) return TRUE;

  if (loggerstate == loadinprogress)
  {
    SendMessage(hwnd, SB_SETTEXT, 0, (LPARAM)
        "Please wait, currently reloading log file.");
    return TRUE;
  }
  else if (loggerstate == lognotfound)
  {
    SendMessage(hwnd, SB_SETTEXT,
              0, (LPARAM) "Could not load any data for graphing.  This may "
        "indicate that there was a problem opening the log file.");
    return TRUE;
  }
  else if (loggerstate == nologloaded)
  {
    SendMessage(hwnd, SB_SETTEXT, 0, (LPARAM)
        "You must specify a log file to be used for graph visualization.");
    return TRUE;
  }
  else if (logdata.IsEmpty() || minrate == maxrate || mintime == maxtime)
  {
    SendMessage(hwnd, SB_SETTEXT, 0, (LPARAM)
      "Could not load any data for graphing.  This may "
        "indicate that there was no graphable data inside of the "
        "specified log file");
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
  GetTextMetrics(dc, &tmet);


  // set up the graphing window structure and scaling
  MyPaintStruct paintstr = {dc, true, 0, 0, minrate, maxrate,
      timelo, timehi, &graphrect};
  double yinterval = (maxrate - minrate) / (graphrect.bottom - graphrect.top) * (tmet.tmHeight * 4);
  time_t xinterval = (timehi - timelo) / (graphrect.right - graphrect.left) * (tmet.tmAveCharWidth * 10);


  // draw the area for the graph
  FillRect(dc, &graphrect, (HBRUSH) GetStockObject(WHITE_BRUSH));
  for (time_t t = timelo; t < timehi; t += 2 * xinterval)
  {
    RECT shade;
    POINT point = paintstr.PointToClientCoords(t, maxrate);
    shade.top = point.y;
    shade.left = point.x;
    point = paintstr.PointToClientCoords(t + xinterval, minrate);
    shade.bottom = point.y;
    shade.right = point.x;
    HBRUSH brush = CreateSolidBrush(RGB(0xEE, 0xEE, 0xEE));
    FillRect(dc, &shade, brush);
    DeleteObject(brush);
  }


  // draw horizontal dashed lines
  HPEN dashedline = CreatePen(PS_DOT, 1, RGB(0, 0, 0));
  HGDIOBJ oldpen = SelectObject(dc, dashedline);
  for (double r = minrate + yinterval; r < maxrate; r += yinterval)
  {
    POINT left = paintstr.PointToClientCoords(timelo, r);
    POINT right = paintstr.PointToClientCoords(timehi, r);
    MoveToEx(dc, left.x, left.y, NULL);
    LineTo(dc, right.x, right.y);
  }
  SelectObject(dc, oldpen);
  DeleteObject(dashedline);


  // start drawing the points
  HPEN graphline = CreatePen(PS_SOLID, 2, RGB(0x99, 0x33, 0x33));
  oldpen = SelectObject(dc, graphline);
  logdata.ForEach(IterDrawFuncRate, (void*)&paintstr);
  SelectObject(dc, oldpen);
  DeleteObject(graphline);
  

  // change to a small black pen for drawing the tick marks.
  HGDIOBJ oldbrush = SelectObject(dc, GetSysColorBrush(COLOR_WINDOWTEXT));
  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, RGB(0, 0, 0));
  SelectObject(dc, GetStockObject(BLACK_PEN));


  // draw the y-axis labels and ticks
  for (double y = minrate; y <= maxrate; y += yinterval)
  {
    // draw the tick mark
    POINT point = paintstr.PointToClientCoords(0, y);
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
  for (time_t x = timelo; x <= timehi; x += xinterval)
  {
    // draw the tick mark
    POINT point = paintstr.PointToClientCoords(x, 0);
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
  SelectObject(dc, oldfont);


  // display the axes labels
  HFONT unrotatedfont = CreateFont(tmet.tmHeight, 0, 0, 0,
      FW_DONTCARE, false, false, false, DEFAULT_CHARSET,
      OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
      DEFAULT_PITCH | FF_DONTCARE, "Arial");
  oldfont = SelectObject(dc, unrotatedfont);
  char *xlabel = "Work Unit completion date";
  SIZE xlabelsize;
  GetTextExtentPoint32(dc, xlabel, strlen(xlabel), &xlabelsize);
  TextOut(dc, graphrect.left + (graphrect.right - graphrect.left - xlabelsize.cx) / 2,
      clientrect.bottom - xlabelsize.cy, xlabel, strlen(xlabel));
  SelectObject(dc, oldfont);
  DeleteObject(unrotatedfont);


  // display the rotated y-axis label
  HFONT rotatedfont = CreateFont(tmet.tmHeight, 0, 900, 900,
      FW_DONTCARE, false, false, false, DEFAULT_CHARSET,
      OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
      DEFAULT_PITCH | FF_DONTCARE, "Arial");
  oldfont = SelectObject(dc, rotatedfont);
  char *ylabel = "Work Unit keyrate (kkeys/sec)";
  SIZE ylabelsize;
  GetTextExtentPoint32(dc, ylabel, strlen(ylabel), &ylabelsize);
  TextOut(dc, 0, graphrect.top + ylabelsize.cx +
    (graphrect.bottom - graphrect.top - ylabelsize.cx) / 2, ylabel, strlen(ylabel));
  SelectObject(dc, oldfont);
  DeleteObject(rotatedfont);


  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

