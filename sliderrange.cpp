// Copyright distributed.net 1997-2004 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "sliderrange.h"


struct SliderRangeStruc
{
  int leftside, rightside;
  bool leftmoving, rightmoving;
  HCURSOR sizer, arrow;
};



#define SRN_MAX_CLOSENESS 5
#define SRN_SENSITIVITY_OUTER 3
#define SRN_SENSITIVITY_INNER 8



// Handler for WM_PAINT. Return 0 if message was handled
static void SliderRangeOnPaint(HWND hwnd, SliderRangeStruc *srs)
{
  HDC dc;
  PAINTSTRUCT ps;
  RECT outer;

  if ((dc = BeginPaint(hwnd, &ps)) != NULL)
  {
    static HPEN blackpen = (HPEN)GetStockObject(BLACK_PEN);
    static HBRUSH graybrush = (HBRUSH)GetStockObject(GRAY_BRUSH);
    static HBRUSH hollowbrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);

    // create a compatible memory dc
    GetClientRect(hwnd, &outer);
    HDC memdc = CreateCompatibleDC(dc);
    HBITMAP membmp = CreateCompatibleBitmap(dc, outer.right, outer.bottom);
    SelectObject(memdc, membmp);

    // fill in the background
    SelectObject(memdc, graybrush);
    SelectObject(memdc, blackpen);
    Rectangle(memdc, outer.left, outer.top, outer.right, outer.bottom);

    // draw the inner region
    RECT inner = { srs->leftside + 1, outer.top + 1,
          srs->rightside, outer.bottom - 1 };
    DrawFrameControl(memdc, &inner, DFC_BUTTON, DFCS_BUTTONPUSH);

    if (srs->rightside - srs->leftside > 16)
    {
      // draw ridges on the left side.
      RECT ridge1 = { srs->leftside + 3, outer.top + 4,
            srs->leftside + 8, outer.bottom - 5 };
      DrawEdge(memdc, &ridge1, EDGE_BUMP, BF_LEFT | BF_RIGHT);

      // draw ridges on the right side.
      RECT ridge2 = { srs->rightside - 9, outer.top + 4,
            srs->rightside - 4, outer.bottom - 5 };
      DrawEdge(memdc, &ridge2, EDGE_BUMP, BF_LEFT | BF_RIGHT);
    }

    // copy the object to the screen
    BitBlt(dc, 0, 0, outer.right, outer.bottom, memdc, 0, 0, SRCCOPY);

    // release everything
    DeleteObject(membmp);
    DeleteDC(memdc);
    EndPaint(hwnd, &ps);
  }
}


// Main window callback to handle all window events for a SliderRange control.
static LRESULT CALLBACK SliderRangeWindowProc(
  HWND hwnd,      // handle to window
  UINT uMsg,      // message identifier
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
)
{
  switch(uMsg)
  {
    case WM_CREATE:
    {
      LRESULT result;
      SliderRangeStruc *srs;
      RECT clientrect;

      // Allow normal window construction to occur.
      if ((result = DefWindowProc(hwnd, uMsg, wParam, lParam)) == -1)
        return -1;

      // Allocate memory for the private control structure.
      if ((srs = (SliderRangeStruc *) HeapAlloc(GetProcessHeap(),
          HEAP_ZERO_MEMORY, sizeof(SliderRangeStruc))) == NULL)
        return -1;

      // Save the pointer to our memory in the user-data area.
      SetWindowLong(hwnd, GWL_USERDATA, (LONG) srs);

      // Finish initializing the private control structure.
      srs->sizer = LoadCursor(NULL, IDC_SIZEWE);
      srs->arrow = LoadCursor(NULL, IDC_ARROW);
      srs->leftmoving = srs->rightmoving = false;

      // Store initial window dimensions.
      GetClientRect(hwnd, &clientrect);
      srs->leftside = clientrect.left + 10;
      srs->rightside = clientrect.right - 10;

      return result;
    }
    case WM_DESTROY:
    {
      SliderRangeStruc *srs;
      if ((srs = (SliderRangeStruc *) GetWindowLong(hwnd, GWL_USERDATA)) != NULL)
      {
        // Cleanup and free the allocated private control structure.
        DestroyCursor(srs->sizer);
        DestroyCursor(srs->arrow);
        HeapFree(GetProcessHeap(), 0, srs);
      }
      break;      // allow normal code to also run.
    }
    case WM_PAINT:
    {
      SliderRangeStruc *srs;
      if ((srs = (SliderRangeStruc *) GetWindowLong(hwnd, GWL_USERDATA)) != NULL)
      {
        SliderRangeOnPaint(hwnd, srs);
      }
      return FALSE;
    }
    case SRM_SETLEFT:
    {
      SliderRangeStruc *srs;
      if ((srs = (SliderRangeStruc *) GetWindowLong(hwnd, GWL_USERDATA)) != NULL)
      {
        srs->leftside = (int)lParam;
        if (srs->leftside < 0) srs->leftside = 0;
        if (srs->leftside > srs->rightside)
          srs->rightside = srs->leftside;
        InvalidateRect(hwnd, NULL, FALSE);
      }
      return FALSE;
    }
    case SRM_SETRIGHT:
    {
      SliderRangeStruc *srs;
      if ((srs = (SliderRangeStruc *) GetWindowLong(hwnd, GWL_USERDATA)) != NULL)
      {
        RECT rect;
        GetClientRect(hwnd, &rect);
        srs->rightside = (int) lParam;
        if (srs->rightside > rect.right) srs->rightside = rect.right;
        if (srs->rightside < srs->leftside) srs->leftside = srs->rightside;
        InvalidateRect(hwnd, NULL, FALSE);
      }
      return FALSE;
    }
    case SRM_GETLEFT:
    {
      SliderRangeStruc *srs;
      if ((srs = (SliderRangeStruc *) GetWindowLong(hwnd, GWL_USERDATA)) != NULL)
      {
        return (LRESULT) srs->leftside;
      }
      return (LRESULT) -1;
    }
    case SRM_GETRIGHT:
    {
      SliderRangeStruc *srs;
      if ((srs = (SliderRangeStruc *) GetWindowLong(hwnd, GWL_USERDATA)) != NULL)
      {
        return (LRESULT) srs->rightside;
      }
      return (LRESULT) -1;
    }
    case SRM_GETMAXRANGE:
    {
      RECT clientrect;
      GetClientRect(hwnd, &clientrect);
      return (LRESULT) clientrect.right;
    }
    case WM_LBUTTONDOWN:
    {
      SliderRangeStruc *srs;
      if ((srs = (SliderRangeStruc *) GetWindowLong(hwnd, GWL_USERDATA)) != NULL)
      {
        int xPos = LOWORD(lParam);
        srs->leftmoving = (xPos >= srs->leftside - SRN_SENSITIVITY_OUTER &&
                    xPos <= srs->leftside + SRN_SENSITIVITY_INNER);
        srs->rightmoving = (xPos >= srs->rightside - SRN_SENSITIVITY_INNER &&
                    xPos <= srs->rightside + SRN_SENSITIVITY_OUTER);
        if (srs->leftmoving && srs->rightmoving)
        {
          if (xPos >= srs->rightside) srs->leftmoving = false;
          else if (xPos <= srs->leftside) srs->rightmoving = false;
          else if (srs->leftside > 0) srs->rightmoving = false;
          else srs->leftmoving = false;
        }
        if (srs->leftmoving || srs->rightmoving) SetCapture(hwnd);
      }
      return FALSE;
    }
    case WM_LBUTTONUP:
    {
      SliderRangeStruc *srs;
      if ((srs = (SliderRangeStruc *) GetWindowLong(hwnd, GWL_USERDATA)) != NULL)
      {
        srs->leftmoving = srs->rightmoving = false;
        ReleaseCapture();
      }
      return FALSE;
    }
    case WM_SETCURSOR:
        return TRUE;
    case WM_MOUSEMOVE:
    {
      SliderRangeStruc *srs;
      if ((srs = (SliderRangeStruc *) GetWindowLong(hwnd, GWL_USERDATA)) != NULL)
      {
        int xPos = (signed short)LOWORD(lParam);

        // match the slider to the mouse position
        if (srs->leftmoving)
        {
          int newpos = xPos;
          if (newpos > srs->rightside - SRN_MAX_CLOSENESS)
            newpos = srs->rightside - SRN_MAX_CLOSENESS;
          if (newpos < 0) newpos = 0;
          if (srs->leftside != newpos)
          {
            srs->leftside = newpos;
            InvalidateRect(hwnd, NULL, FALSE);
            PostMessage(GetParent(hwnd), WM_COMMAND,
                MAKELONG(GetWindowLong(hwnd, GWL_ID),
                SRN_LEFTCHANGED), srs->leftside);
          }
        }
        else if (srs->rightmoving)
        {
          RECT clientrect;
          GetClientRect(hwnd, &clientrect);
          int newpos = xPos;
          if (newpos < srs->leftside + SRN_MAX_CLOSENESS)
            newpos = srs->leftside + SRN_MAX_CLOSENESS;
          if (newpos > clientrect.right) newpos = clientrect.right;
          if (srs->rightside != newpos)
          {
            srs->rightside = newpos;
            InvalidateRect(hwnd, NULL, FALSE);
            PostMessage(GetParent(hwnd), WM_COMMAND,
                MAKELONG(GetWindowLong(hwnd, GWL_ID),
                SRN_RIGHTCHANGED), srs->rightside);
          }
        }

        // set the cursor shape to something special
        if ((xPos >= srs->leftside - SRN_SENSITIVITY_OUTER &&
            xPos <= srs->leftside + SRN_SENSITIVITY_INNER) ||
            (xPos >= srs->rightside - SRN_SENSITIVITY_INNER &&
            xPos <= srs->rightside + SRN_SENSITIVITY_OUTER))
          SetCursor(srs->sizer);
        else
          SetCursor(srs->arrow);
      }
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
  };
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}



// Returns FALSE on error.
BOOL RegisterSliderRangeClass(void)
{
  WNDCLASS wc;

  RtlZeroMemory(&wc, sizeof(wc));
  wc.lpfnWndProc = (WNDPROC) SliderRangeWindowProc;
  wc.cbWndExtra = sizeof(LPVOID);
  wc.hCursor = LoadIcon(NULL, IDC_ARROW);
  wc.lpszClassName = SLIDERCONTROLCLASSNAME;
  return (RegisterClass(&wc) != NULL);
}

