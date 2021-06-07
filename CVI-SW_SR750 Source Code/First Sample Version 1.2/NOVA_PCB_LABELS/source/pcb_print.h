/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2021. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1       /* callback function: quit */
#define  PANEL_CB_MAIN_2                  2       /* control type: command, callback function: test */
#define  PANEL_CB_MAIN_3                  3       /* control type: command, callback function: reset_count */
#define  PANEL_CB_MAIN                    4       /* control type: command, callback function: start */
#define  PANEL_STRING_2                   5       /* control type: string, callback function: (none) */
#define  PANEL_NUM_S                      6       /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_Y                      7       /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_MONTH                  8       /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_DAY                    9       /* control type: numeric, callback function: (none) */
#define  PANEL_NUM_X                      10      /* control type: numeric, callback function: (none) */
#define  PANEL_NUMERIC_3                  11      /* control type: numeric, callback function: (none) */
#define  PANEL_NUMERIC_4                  12      /* control type: numeric, callback function: (none) */
#define  PANEL_NUMERIC_2                  13      /* control type: numeric, callback function: (none) */
#define  PANEL_NUMERIC                    14      /* control type: numeric, callback function: (none) */
#define  PANEL_TEXTMSG                    15      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_2                  16      /* control type: textMsg, callback function: (none) */
#define  PANEL_DECORATION_2               17      /* control type: deco, callback function: (none) */
#define  PANEL_DECORATION_3               18      /* control type: deco, callback function: (none) */
#define  PANEL_DECORATION                 19      /* control type: deco, callback function: (none) */
#define  PANEL_TEXTMSG_5                  20      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_3                  21      /* control type: textMsg, callback function: (none) */
#define  PANEL_TEXTMSG_4                  22      /* control type: textMsg, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK quit(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK reset_count(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK start(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK test(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
