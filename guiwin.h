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

#include "cputypes.h"
#include "version.h"

// vwcl headers
#define _USTR_H
#define _U(str) str
#include "vStandard.h"
#include "v3DRect.hpp"
#if (CLIENT_OS == OS_WIN32)
  #include "vStatusBarCtrl.hpp"       // must be included before vMainWindow
#endif
#include "vMainWindow.hpp"
#include "vEdit.hpp"
#include "vCheckBox.hpp"
#include "vComboBox.hpp"
#include "vPaintDC.hpp"


// gui encapsulation headers
#include "resource.h"
#include "guigraph.h"


// retrieve the logfile that should be displayed.
const char *LogGetCurrentLogFilename(void);


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class MyClientWindow : public VMainWindow
{
protected:
  MyGraphWindow graphwin;

  // event handlers
  virtual BOOL PreCreateWindow(LPCREATESTRUCT cs);
  virtual BOOL PostCreateWindow(void);
  virtual void RecalcLayout(LPRECT lpRect);
  virtual int OnCommand(WORD wNotifyCode, WORD wID, HWND hWndControl);
	virtual LRESULT	WindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);
  void OnInitMenu(HMENU hMenu);
  void CmAbout(void);
  void CmOpenLogfile(void);

public:
  MyClientWindow(void) : VMainWindow() {};
  
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif  // GUI32_H

