#ifndef __VSLIDERRANGE_HPP
#define __VSLIDERRANGE_HPP

#include "vWindow.hpp"
#include "vPaintDC.hpp"

// WM_COMMAND code sent when the left or right edge of the control is moved
// nID = LOWORD(wParam)
// wNotifyCode = HIWORD(wParam)    // SRN_CHANGED
// leftside/rightside = lParam
#define SRN_LEFTCHANGED 0
#define SRN_RIGHTCHANGED 1

// Messages to set the left or right edge of the slider control
// wParam = 0
// lParam = new position
#define SRM_SETLEFT (WM_USER + 1)
#define SRM_SETRIGHT (WM_USER + 2)

// Messages to get the left or right edge of the slider control
// wParam = lParam = 0
#define SRM_GETLEFT (WM_USER + 3)
#define SRM_GETRIGHT (WM_USER + 4)

class VSliderRange : public VWindow
{
  int leftside, rightside;
  bool leftmoving, rightmoving;
  HCURSOR sizer, arrow;
  enum { sensitivity = 3 };

public:
	VSliderRange() : leftside(0), rightside(0), leftmoving(false), rightmoving(false)
	  {
      sizer = LoadCursor(NULL, IDC_SIZEWE);
      arrow = LoadCursor(NULL, IDC_ARROW);
	  };

	virtual ~VSliderRange()
		{;}

  virtual BOOL Create(LPRECT rect, HWND parent, int nID)
  {
    return VWindow::Create(NULL, "slider", WS_CHILD | WS_VISIBLE, rect, parent, nID);
  }

  int GetMaxVal()
  {
    RECT rect;
    GetClientRect(&rect);
    return (int)(rect.right);
  }

  inline void SetLeft(int value) { SendMessage(SRM_SETLEFT, 0, value); }
  inline void SetRight(int value) { SendMessage(SRM_SETRIGHT, 0, value); }
  inline int GetLeft() { return (int) SendMessage(SRM_GETLEFT, 0, 0); }
  inline int GetRight() { return (int) SendMessage(SRM_GETRIGHT, 0, 0); }

protected:
  virtual BOOL PostCreateWindow()
		{
		  RECT rect;
		  GetClientRect(&rect);
		  leftside = rect.left + 10;
		  rightside = rect.right - 10;
		  return VWindow::PostCreateWindow();
		}

  // Window procedure
	virtual LRESULT WindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
	  {
      switch(nMessage)
      {
        case SRM_SETLEFT:
        {
          leftside = (int)lParam;
          if (leftside < 0) leftside = 0;
          if (leftside > rightside) rightside = leftside;
          InvalidateRect(NULL, FALSE);
          return FALSE;
        }
        case SRM_SETRIGHT:
        {
    		  RECT rect;
    		  GetClientRect(&rect);
          rightside = (int) lParam;
          if (rightside > rect.right) rightside = rect.right;
          if (rightside < leftside) leftside = rightside;
          InvalidateRect(NULL, FALSE);
          return FALSE;
        }
        case SRM_GETLEFT:
          return leftside;
        case SRM_GETRIGHT:
          return rightside;
        case WM_LBUTTONDOWN:
        {
          int xPos = LOWORD(lParam);
          leftmoving = (xPos >= leftside - sensitivity &&
                        xPos <= leftside + sensitivity);
          rightmoving = (xPos >= rightside - sensitivity &&
                        xPos <= rightside + sensitivity);
          if (leftmoving && rightmoving)
          {
            if (leftside > 0) rightmoving = false;
            else leftmoving = false;
          }
          if (leftmoving || rightmoving) SetCapture();
          return FALSE;
        }
        case WM_LBUTTONUP:
        {
          ReleaseCapture();
          leftmoving = rightmoving = false;
          return FALSE;
        }
        case WM_SETCURSOR:
          return TRUE;
        case WM_MOUSEMOVE:
        {
          int xPos = (signed short)LOWORD(lParam);

          // match the slider to the mouse position
          if (leftmoving)
          {
            int newpos = xPos;
            if (newpos > rightside - sensitivity) newpos = rightside - sensitivity;
            if (newpos < 0) newpos = 0;
            if (leftside != newpos)
            {
              leftside = newpos;
              InvalidateRect(NULL, FALSE);
#ifdef WIN16
              ::PostMessage(GetParent(), WM_COMMAND,
                GetWindowWord(GWW_ID),
                MAKELONG(leftside,   // should be HWND of control but we pass the new value
                SRN_LEFTCHANGED) );
#else
              ::PostMessage(GetParent(), WM_COMMAND,
                MAKELONG(GetWindowLong(GWL_ID),
                SRN_LEFTCHANGED),
                leftside);     // should be HWND of control but we pass the new value
#endif
            }
          }
          else if (rightmoving)
          {
            RECT clientrect;
            GetClientRect(&clientrect);
            int newpos = xPos;
            if (newpos < leftside + sensitivity) newpos = leftside + sensitivity;
            if (newpos > clientrect.right) newpos = clientrect.right;
            if (rightside != newpos)
            {
              rightside = newpos;
              InvalidateRect(NULL, FALSE);
#ifdef WIN16
              ::PostMessage(GetParent(), WM_COMMAND,
                GetWindowWord(GWW_ID),
                MAKELONG(rightside,   // should be HWND of control but we pass the new value
                SRN_RIGHTCHANGED) );
#else
              ::PostMessage(GetParent(), WM_COMMAND,
                MAKELONG(GetWindowLong(GWL_ID),
                SRN_RIGHTCHANGED),
                rightside);     // should be HWND of control but we pass the new value
#endif
            }
          }

          // set the cursor shape to something special
          if ((xPos >= leftside - sensitivity && xPos <= leftside + sensitivity) ||
            (xPos >= rightside - sensitivity && xPos <= rightside + sensitivity))
            SetCursor(sizer);
          else
            SetCursor(arrow);

          // we're done
          return FALSE;
        }
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDBLCLK:
          // ignore all these suckers
          return FALSE;
        
        default: break;
      }
      return VWindow::WindowProc(hWnd, nMessage, wParam, lParam);
	  }

	// Handler for WM_PAINT. Return 0 if message was handled
	virtual int OnPaint()
		{
      RECT outer = {0,0,0,0};
		  VPaintDC dc(GetSafeWindow());
      static HPEN blackpen = (HPEN)GetStockObject(BLACK_PEN);
      static HBRUSH graybrush = (HBRUSH)GetStockObject(GRAY_BRUSH);
      static HBRUSH hollowbrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);

      // create a compatible memory dc
      GetClientRect(&outer);
      HDC memdc = CreateCompatibleDC(dc);
      HBITMAP membmp = CreateCompatibleBitmap(dc, outer.right, outer.bottom);
      SelectObject(memdc, membmp);

      // fill in the background
      SelectObject(memdc, graybrush);
      SelectObject(memdc, blackpen);
      Rectangle(memdc, outer.left, outer.top, outer.right, outer.bottom);

      // draw the inner region
      RECT inner = { leftside + 1, outer.top + 1, rightside, outer.bottom - 1 };
#ifdef WIN16
      // something here
#else
      DrawFrameControl(memdc, &inner, DFC_BUTTON, DFCS_BUTTONPUSH);
#endif

      // copy the object to the screen
      BitBlt(dc, 0, 0, outer.right, outer.bottom, memdc, 0, 0, SRCCOPY);

      // release everything
      DeleteObject(membmp);
      DeleteDC(memdc);
		  return 0;
		}
};

#endif // __VSLIDERRANGE_HPP

