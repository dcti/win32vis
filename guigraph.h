// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#ifndef __GUIGRAPH_H__
#define __GUIGRAPH_H__

#include <time.h>
#include "guidlist.h"
#include "VSliderRange.hpp"
#include "vDialog.hpp"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Data Record unit entry, used to store each of the keyrate data samples
// that have been read in and parsed from the log file.

struct MyGraphEntry
{
  time_t timestamp;
  double rate;
  double duration;
  double keycount;

  friend bool operator==( const MyGraphEntry &a, const MyGraphEntry &b)
    { return (a.timestamp == b.timestamp) && (a.rate == b.rate) &&
        (a.duration == b.duration) && (a.keycount == b.keycount); }
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Class definition for the time range configuration dialog that is
// activated from the popup context menu within the graph viewer.

class MyGraphConfig : private VDialog
{
protected:
  // data storage.
  VSliderRange rangeslider;
  VComboBox combostart, comboend;
  time_t userstart, userend;

  // internal function to update displayable text to the specified date.
  void SetDlgItemGMT(int nID, time_t timestamp);

  // event handlers
  virtual int OnCommand(WORD wNotifyCode, WORD wID, HWND hWndControl);
  virtual int OnInitDialog(HWND hWndFocus, LPARAM lParam);
  virtual BOOL OnOK();

public:
  // class constructor.
  MyGraphConfig() : starttime(0), endtime(0), datastart(0), dataend(0),
    userstart(0), userend(0) {};

  // public method to invoke blocking dialog execution.
  virtual int DoModal(HWND parent);

  // Data storage for currently selected date ranges.
  // Modify these values before calling DoModal(), and read
  // from them after regaining control.
  time_t starttime, endtime;
  time_t datastart, dataend;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Class definition for the main graphing window itself.  This window
// repaints itself with the graph data and handles log parsing by itself.

class MyGraphWindow : private VWindow
{
protected:
  enum LoggerState { nologloaded, lognotfound, loadinprogress, logloaded };

  // storage variables.
  TDoubleListImp<MyGraphEntry> logdata;
  time_t mintime, maxtime;
  double minrate, maxrate;
  double totalkeys;
  time_t rangestart, rangeend;
  CRITICAL_SECTION repaintMutex;
  HANDLE hLogThread;

  // current graphing state.
  bool clientinitialized;
  LoggerState loggerstate;
  bool logrereadneeded;

  // log parsing
  static time_t ParseTimestamp(char *stamp);
  static double ParseDuration(char *stamp);
  void ReadLogData(void);
  static long LogParseThread(long lParam);

  // log painting
  static void IterDrawFuncRate(MyGraphEntry &datapoint, void *vptr);
  virtual int	OnPaint(void);
  virtual int DoRedraw(HDC dc, RECT clientrect);

  // event handlers
  virtual BOOL PostCreateWindow(void);
  virtual LRESULT	WindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);

public:
  // constructor and destructor.
  MyGraphWindow(void);
  ~MyGraphWindow(void);

  // native window creation operation.
  int Create(HWND parent) { return VWindow::Create(NULL, "Graph",
      WS_VISIBLE | WS_CHILD, NULL, parent, 0); }

  // return window visibility status.
  BOOL IsWindowVisible(void) { return ::IsWindowVisible(GetSafeWindow()); }

  // show or hide graph window.
  void ShowWindow(bool flag) { VWindow::ShowWindow(flag); }

  // reposition window dimensions and location.
  void MoveWindow(LPRECT rect) { VWindow::MoveWindow(rect); }

  // invalidate window contents and force a repaint.
  void Refresh(void);

public:
  // public interface methods.
  void LogRereadNeeded(void);

  void ClientInitDone(void) { clientinitialized = true; Refresh(); }

  void GetRange(time_t &start, time_t &end) { start = rangestart; end = rangeend; }
  
  void SetRange(time_t start, time_t end) { rangestart = start; rangeend = end; }
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


#endif

