// Copyright distributed.net 1997-1999 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.

#ifndef __VSLIDERRANGE_HPP
#define __VSLIDERRANGE_HPP


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

// Message to get the range of the slider control
#define SRM_GETMAXRANGE (WM_USER + 5)


// Name of the window class used for our control.
#define SLIDERCONTROLCLASSNAME 		TEXT("SlideControl")


// Function prototypes.
extern BOOL RegisterSliderRangeClass(void);



#endif // __VSLIDERRANGE_HPP

