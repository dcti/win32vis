// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#ifndef __GUIWIN_H__
#define __GUIWIN_H__

#if defined(__TURBOC__)
  // Borland C++ warnings
  #pragma warn -inl               // function cannot be inlined
  #if (__TURBOC__ <= 0x400)
    typedef int bool;
    #define false (0)
    #define true (!0)
  #endif
#elif defined(_MSC_VER)
  // MS Visual C++ warnings
  #pragma warning(disable:4068)   // unknown pragma
  #if (_MSC_VER < 1100)
    typedef int bool;
    #define false (0)
    #define true (!0)
  #endif
#endif

#include "version.h"


// Windows headers.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

// C Library functions
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// gui encapsulation headers
#include "resource.h"
#include "guidlist.h"
#include "sliderrange.h"


// function prototypes.
extern const char *LogGetCurrentLogFilename(void);
extern void LogSetCurrentLogFilename(const char *filename, bool removeQuotes);
extern LRESULT CALLBACK Main_WindowProc(HWND,UINT,WPARAM,LPARAM);
extern void Main_CmOpenLogfile(HWND hwnd);
extern void Main_CmAbout(HWND hwnd);
extern void Main_UpdateTitlebar(HWND hwnd);

// data prototypes.
class MyGraphWindow;
extern MyGraphWindow graphwin;


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

// Class definition for the main graphing window itself.  This window
// repaints itself with the graph data and handles log parsing by itself.

class MyGraphWindow
{
public:
  enum LoggerState { nologloaded, lognotfound, loadinprogress, logloaded, loginvalid };

protected:
  // storage variables.
  TDoubleListImp<MyGraphEntry> logdata;
  time_t mintime, maxtime;
  double minrate, maxrate;
  double totalkeys;
  time_t rangestart, rangeend;
  HANDLE hLogThread;


  // current graphing state.
  LoggerState loggerstate;
  bool bStateChanged;

  // log parsing
  static time_t ParseTimestamp(char *stamp);
  static double ParseDuration(char *stamp);
  void ReadLogData(void);
  static long LogParseThread(long lParam);

  // log painting
  static void IterDrawFuncRate(MyGraphEntry &datapoint, void *vptr);

public:
  // constructor and destructor.
  MyGraphWindow(void);
  ~MyGraphWindow(void);

  // window repainting.
  int	DoRedraw(HDC dc, RECT clientrect);

  // public interface methods.
  void LogRereadNeeded(HWND hwnd);

  // public interface methods.
  void GetDataRange(time_t &start, time_t &end)
    { start = mintime; end = maxtime; }

  // public interface methods.
  void GetRange(time_t &start, time_t &end)
    { start = rangestart; end = rangeend; }
  
  // public interface methods.
  void SetRange(time_t start, time_t end)
    { rangestart = start; rangeend = end; }

  // public interface methods.
  bool IsDataAvailable(void)
    { return loggerstate == logloaded && !logdata.IsEmpty() &&
          minrate != maxrate && mintime != maxtime; }

  // public interface methods.
  const char *GetStatusString(void);

  // public interface methods.
  LoggerState GetStatusValue(void) { return loggerstate; }

  // public interface methods.
  bool HasStatusChanged(void) { return bStateChanged; }

};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Class definition for the time range configuration dialog that is
// activated from the popup context menu within the graph viewer.

class MyGraphConfig
{
protected:
  // data storage.
  time_t userstart, userend;

  // event handlers
  static BOOL CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
  int OnCommand(HWND hwndDlg, WORD wNotifyCode, WORD wID, HWND hWndControl);
  int OnInitDialog(HWND hwndDlg, HWND hWndFocus, LPARAM lParam);
  BOOL OnOK(HWND hwndDlg);

public:
  // class constructor.
  MyGraphConfig() : starttime(0), endtime(0), datastart(0), dataend(0),
    userstart(0), userend(0) {};

  // public method to invoke blocking dialog execution.
  int DoModal(HWND parent);

  // Data storage for currently selected date ranges.
  // Modify these values before calling DoModal(), and read
  // from them after regaining control.
  time_t starttime, endtime;
  time_t datastart, dataend;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif  // __GUIWIN_H__

