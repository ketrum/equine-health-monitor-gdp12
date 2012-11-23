/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2012  SEGGER Microcontroller GmbH & Co. KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V5.16 - Graphical user interface for embedded applications **
All  Intellectual Property rights  in the Software belongs to  SEGGER.
emWin is protected by  international copyright laws.  Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with the following terms:

The software has been licensed to Energy Micro AS whose registered office
is situated at  Sandakerveien 118, N-0484 Oslo, NORWAY solely
for  the  purposes  of  creating  libraries  for Energy Micros ARM Cortex-M3, M4F
processor-based  devices,  sublicensed  and distributed  under the terms and
conditions  of  the   End  User  License Agreement supplied by Energy Micro AS.
Full source code is available at: www.segger.com

We appreciate your understanding and fairness.
----------------------------------------------------------------------
File        : LCDConf.c
Purpose     : Display controller configuration (single layer)
---------------------------END-OF-HEADER------------------------------
*/

#include "GUI.h"

#include "GUIDRV_Lin.h"
#include "dvk.h"
#include "em_cmu.h"
#include "em_ebi.h"
#include "em_rtc.h"
#include "trace.h"
#include "touch.h"
#include "lcdconf.h"
#include "sdd2119.h"

/*********************************************************************
*
*       Layer configuration (to be modified)
*
**********************************************************************
*/
//
// Physical display size
//
#define XSIZE_PHYS 320
#define YSIZE_PHYS 240
#define FRAME_BUFFER_SIZE (XSIZE_PHYS * YSIZE_PHYS * 2)

extern const GUI_DEVICE_API GUIDRV_SSD2119;
static bool runOnce = true;
static GUI_DEVICE *guiDevice;

//
// Color conversion
//
#define COLOR_CONVERSION GUICC_565

//
// Display driver
//
#define DISPLAY_DRIVER &GUIDRV_SSD2119

//
// Buffers / VScreens
//
#define NUM_BUFFERS  1 // Number of multiple buffers to be used
#define NUM_VSCREENS 1 // Number of virtual screens to be used

/*********************************************************************
*
*       Configuration checking
*
**********************************************************************
*/
#ifndef   VRAM_ADDR
  #define VRAM_ADDR (void *)EBI_BankAddress(EBI_BANK2)
#endif
#ifndef   XSIZE_PHYS
  #error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
  #error Physical Y size of display is not defined!
#endif
#ifndef   COLOR_CONVERSION
  #error Color conversion not defined!
#endif
#ifndef   DISPLAY_DRIVER
  #error No display driver defined!
#endif

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/


/***************************************************************************//**
 * @brief
 *   Upcall function for touch panel activity
 *
 * @details
 *   This function is call from TOUCH driver every time pen state is changed
 *   (up/down) or is moved. There is translation to emWin structure done.
 * @note
 *   This function is called from ADC interrupt, so it should return as soon
 *   as possible.
 ******************************************************************************/
/*  */
/*     */
void LCD_TOUCH_Upcall(TOUCH_Pos_TypeDef *pos)
{ static GUI_PID_STATE gui_pos;

  if(pos->x < LCD_GetXSize()) gui_pos.x = pos->x;
    else gui_pos.x = LCD_GetXSize()-1;
  if(pos->y < LCD_GetYSize()) gui_pos.y = pos->y;
    else gui_pos.y = LCD_GetYSize()-1;
  gui_pos.Pressed = pos->pen;
  GUI_TOUCH_StoreStateEx(&gui_pos);
}

/***************************************************************************//**
 * @brief
 *   Initialize board and LCD controller
 *
 * @details
 *   This function is called during initialization and when exited from AEM mode
 ******************************************************************************/
static void _InitController(void) {
      /* Initialize EBI banks (Board Controller, external PSRAM, ..) */

  DVK_init(DVK_Init_EBI);
    /* If first word of user data page is non-zero, enable eA Profiler trace */
  TRACE_ProfilerSetup();

  /* Configure for 32MHz HFXO operation of core clock */
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

  /* on Giant & Leopard we have to reduce core clock when
   * talking with slow TFT controller */
  CMU_ClockDivSet(cmuClock_CORE, cmuClkDiv_2);

  CMU_ClockEnable( cmuClock_GPIO, true);
  CMU_ClockEnable( cmuClock_ADC0, true);
  LCD_InitializeLCD();
}

/*********************************************************************
*
*       _SetVRAMAddr
*
* Purpose:
*   Should set the frame buffer base address
*/
static void _SetVRAMAddr(void * pVRAM) {
  (void)pVRAM;                          /* Unused parameter */
   // VRAM_pointer = (uint32_t)pVRAM - EBI_BankAddress(EBI_BANK2);
    //EBI_TFTFrameBaseSet(VRAM_pointer);
}

/*********************************************************************
*
*       _SetOrg
*
* Purpose:
*   Should set the origin of the display typically by modifying the
*   frame buffer base address register
*/
static void _SetOrg(int xPos, int yPos) {
  (void)xPos;                              /* Unused parameter */
  (void)yPos;                              /* Unused parameter */
    //orgx = xPos;
    //orgy = yPos;
    //EBI_TFTFrameBaseSet(VRAM_pointer+2*( (XSIZE_PHYS*yPos) + xPos ) );
}

/*********************************************************************
*
*       _SetLUTEntry
*
* Purpose:
*   Should set the desired LUT entry
*/
static void _SetLUTEntry(LCD_COLOR Color, U8 Pos) {
  (void)Color;                             /* Unused parameter */
  (void)Pos;                               /* Unused parameter */
  /* TBD by customer */
}
/*********************************************************************
*
*       Public code
*
**********************************************************************
*/



/* Initialize LCD in memory mapped mode and touch panel */
void LCD_InitializeLCD(void)
{ TOUCH_Config_TypeDef touch_init = TOUCH_INIT_DEFAULT;
  int      i;

  /* If we are in BC_UIF_AEM_EFM state, we can redraw graphics */
  if (DVK_readRegister(&BC_REGISTER->UIF_AEM) == BC_UIF_AEM_EFM)
  {
    /* If we're not BC_ARB_CTRL_EBI state, we need to reconfigure display controller */
    if ((DVK_readRegister(&BC_REGISTER->ARB_CTRL) != BC_ARB_CTRL_EBI) || runOnce)
    {
      /* Configure for EBI mode and reset display */
      DVK_displayControl(DVK_Display_EBI);
      DVK_displayControl(DVK_Display_ResetAssert);
      DVK_displayControl(DVK_Display_PowerDisable);
      /* Short delay */
      for (i = 0; i < 10000; i++) ;
      /* Configure display for Direct Drive + SPI mode */
      DVK_displayControl(DVK_Display_Mode8080);
      DVK_displayControl(DVK_Display_PowerEnable);
      DVK_displayControl(DVK_Display_ResetRelease);

      runOnce = false;
    }
  }

  touch_init.oversampling = adcOvsRateSel32;
  TOUCH_Init(&touch_init);
  TOUCH_RegisterUpcall(LCD_TOUCH_Upcall);
}

/***************************************************************************//**
 * @brief
 *   Initialize LCD driver
 *
 * @details
 *   This function is called during initialization and when exited from AEM mode
 ******************************************************************************/
void LCD_InitializeDriver(void)
{ int (* pInit) (GUI_DEVICE *);
  pInit = (int (*)(GUI_DEVICE *)) guiDevice->pDeviceAPI->pfGetDevFunc(&guiDevice, LCD_DEVFUNC_INIT);
  pInit(guiDevice);
}

void yourInitFunction(GUI_DEVICE * pDevice)
{ /* here you can write your own initialization routine */
  (void)pDevice;                              /* Unused parameter */
}

/*********************************************************************
*
*       LCD_X_Config
*
* Purpose:
*   Called during the initialization process in order to set up the
*   display driver configuration.
*
*/
void LCD_X_Config(void) {

  GUI_MULTIBUF_Config(NUM_BUFFERS);
  //
  // Set display driver and color conversion for 1st layer
  //
  guiDevice = GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER, COLOR_CONVERSION, 0, 0);
  //
  // Display driver configuration, required for Lin-driver
  //
  if (LCD_GetSwapXY()) {
    LCD_SetSizeEx (0, YSIZE_PHYS, XSIZE_PHYS);
    LCD_SetVSizeEx(0, YSIZE_PHYS, XSIZE_PHYS);
  } else {
    LCD_SetSizeEx (0, XSIZE_PHYS, YSIZE_PHYS);
    LCD_SetVSizeEx(0, XSIZE_PHYS, YSIZE_PHYS);
  }
  LCD_SetVRAMAddrEx(0, (void *)VRAM_ADDR);

#ifdef LCD_DEVFUNC_CONTRADDR
  if(guiDevice)
  { void (* pSetControllerAddress) (GUI_DEVICE *, uint32_t reg, uint32_t data);
    /* now let's take function for setting controller address */
    pSetControllerAddress = (void (*) (GUI_DEVICE *, uint32_t, uint32_t))guiDevice->pDeviceAPI->pfGetDevFunc(&guiDevice, LCD_DEVFUNC_CONTRADDR);
    /* and configure addresses - for writing register and data */
    pSetControllerAddress(guiDevice, BC_SSD2119_BASE, BC_SSD2119_BASE + 2);
  }
#endif

/* example how to disable default controller initialization function */
#ifdef removeit_LCD_DEVFUNC_INITIALIZE
  if(guiDevice)
  { void (* pSetFunc) (GUI_DEVICE *, int, void (*)(void));
    /* now let's take function for changing functions in driver */
    pSetFunc = (void (*) (GUI_DEVICE *, int, void (*)(void)))guiDevice->pDeviceAPI->pfGetDevFunc(&guiDevice, LCD_DEVFUNC_SETFUNC);
    /* and replace initialization function. It is possible to remove */
    /* initialization function by giving null instead of new function pointer */
    pSetFunc(guiDevice, LCD_DEVFUNC_INITIALIZE, (void (*)(void))yourInitFunction);
  }
#endif

  //
  // Set user palette data (only required if no fixed palette is used)
  //
  #if defined(PALETTE)
    LCD_SetLUTEx(0, PALETTE);
  #endif
}

/*********************************************************************
*
*       LCD_X_DisplayDriver
*
* Purpose:
*   This function is called by the display driver for several purposes.
*   To support the according task the routine needs to be adapted to
*   the display controller. Please note that the commands marked with
*   'optional' are not cogently required and should only be adapted if
*   the display controller supports these features.
*
* Parameter:
*   LayerIndex - Index of layer to be configured
*   Cmd        - Please refer to the details in the switch statement below
*   pData      - Pointer to a LCD_X_DATA structure
*
* Return Value:
*   < -1 - Error
*     -1 - Command not handled
*      0 - Ok
*/
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData) {
  int r;

  (void)LayerIndex;                              /* Unused parameter */

  switch (Cmd) {
  //
  // Required
  //
  case LCD_X_INITCONTROLLER: {
    //
    // Called during the initialization process in order to set up the
    // display controller and put it into operation. If the display
    // controller is not initialized by any external routine this needs
    // to be adapted by the customer...
    //
    _InitController();
    return 0;
  }
  case LCD_X_SETVRAMADDR: {
    //
    // Required for setting the address of the video RAM for drivers
    // with memory mapped video RAM which is passed in the 'pVRAM' element of p
    //
    LCD_X_SETVRAMADDR_INFO * p;
    p = (LCD_X_SETVRAMADDR_INFO *)pData;
    _SetVRAMAddr(p->pVRAM);
    return 0;
  }
  case LCD_X_SETORG: {
    //
    // Required for setting the display origin which is passed in the 'xPos' and 'yPos' element of p
    //
    LCD_X_SETORG_INFO * p;
    p = (LCD_X_SETORG_INFO *)pData;
    _SetOrg(p->xPos, p->yPos);
    return 0;
  }
  case LCD_X_SETLUTENTRY: {
    //
    // Required for setting a lookup table entry which is passed in the 'Pos' and 'Color' element of p
    //
    LCD_X_SETLUTENTRY_INFO * p;
    p = (LCD_X_SETLUTENTRY_INFO *)pData;
    _SetLUTEntry(p->Color, p->Pos);
    return 0;
  }
  case LCD_X_ON: {
    //
    // Required if the display controller should support switching on and off
    //
    return 0;
  }
  case LCD_X_OFF: {
    //
    // Required if the display controller should support switching on and off
    //
    // ...
    return 0;
  }
  /* This command is received every time the GUI is done drawing a frame */
  case LCD_X_SHOWBUFFER:
  {
    /* Get the data object */
//    LCD_X_SHOWBUFFER_INFO * p;
//    p = (LCD_X_SHOWBUFFER_INFO *)pData;
  }

  default:
    r = -1;
  }
  return r;
}

/*************************** End of file ****************************/
