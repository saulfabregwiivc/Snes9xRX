/****************************************************************************
 * Snes9x Nintendo Wii/Gamecube Port
 *
 * softdev July 2006
 * crunchy2 May-June 2007
 * Michniewski 2008
 * Tantric 2008-2019
 *
 * input.cpp
 *
 * Wii/Gamecube controller management
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ogcsys.h>
#include <unistd.h>
#ifdef HW_RVL
#include <wiiuse/wpad.h>
#endif
#include <ogc/lwp_watchdog.h>

#include "snes9x/port.h"
#include "snes9xgx.h"
#include "button_mapping.h"
#include "menu.h"
#include "video.h"
#include "input.h"
#include "gui/gui.h"

#include "snes9x/snes9x.h"
#include "snes9x/memmap.h"
#include "snes9x/controls.h"

#ifdef HW_RVL
#include "utils/retrode.h"
#include "utils/xbox360.h"
#include "utils/hornet.h"
#include "utils/mayflash.h"
extern "C"{
#include "utils/sicksaxis.h"
}
/* sicksaxis lib (by xerpi) */
	static ss_instance_t sicksaxis;
#define SICKSAXIS_DEADZONE 115
#endif

#define ANALOG_SENSITIVITY 30

int playerMapping[4] = {0,1,2,3};
GuiTrigger userInput[4];

// hold superscope/mouse/justifier cursor positions
static int cursor_x[5] = {0,0,0,0,0};
static int cursor_y[5] = {0,0,0,0,0};

/****************************************************************************
 * Controller Functions
 *
 * The following map the Wii controls to the Snes9x controller system
 ***************************************************************************/
#define ASSIGN_BUTTON_TRUE( keycode, snescmd ) \
	  S9xMapButton( keycode, cmd = S9xGetCommandT(snescmd), true)

#define ASSIGN_BUTTON_FALSE( keycode, snescmd ) \
	  S9xMapButton( keycode, cmd = S9xGetCommandT(snescmd), false)

static int scopeTurbo = 0; // tracks whether superscope turbo is on or off
u32 btnmap[4][6][12]; // button mapping

void ResetControls(int consoleCtrl, int wiiCtrl)
{
	int i;
	/*** Gamecube controller Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_GCPAD))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_A;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_B;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_X;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_Y;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_TRIGGER_L;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_TRIGGER_R;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_START;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_TRIGGER_Z;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_GCPAD][i++] = PAD_BUTTON_RIGHT;
	}

	/*** Wiimote Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_WIIMOTE))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_B;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_2;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_1;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_A;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = 0x0000;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = 0x0000;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_PLUS;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_MINUS;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_RIGHT;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_DOWN;
	}

	/*** Wii Classic Controller Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_CLASSIC))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_A;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_X;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_Y;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_FULL_L;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_FULL_R;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_PLUS;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_MINUS;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_RIGHT;
	}

	/*** Wii U Pro Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_WUPC))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_A;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_X;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_Y;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_FULL_L;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_FULL_R;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_PLUS;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_MINUS;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_RIGHT;
	}

	/*** Wii U Gamepad Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_WIIDRC))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_A;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_B;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_X;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_Y;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_L;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_R;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_PLUS;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_MINUS;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_RIGHT;
	}

	/*** Nunchuk + Wiimote Padmap ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_PAD && wiiCtrl == CTRLR_NUNCHUK))
	{
		i=0;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_A;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_B;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_NUNCHUK_BUTTON_C;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_NUNCHUK_BUTTON_Z;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_2;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_1;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_PLUS;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_MINUS;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_UP;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_DOWN;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_LEFT;
		btnmap[CTRL_PAD][CTRLR_NUNCHUK][i++] = WPAD_BUTTON_RIGHT;
	}

	/*** Superscope : GC controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_SCOPE && wiiCtrl == CTRLR_GCPAD))
	{
		i=0;
		btnmap[CTRL_SCOPE][CTRLR_GCPAD][i++] = PAD_BUTTON_A;
		btnmap[CTRL_SCOPE][CTRLR_GCPAD][i++] = PAD_BUTTON_B;
		btnmap[CTRL_SCOPE][CTRLR_GCPAD][i++] = PAD_TRIGGER_Z;
		btnmap[CTRL_SCOPE][CTRLR_GCPAD][i++] = PAD_BUTTON_Y;
		btnmap[CTRL_SCOPE][CTRLR_GCPAD][i++] = PAD_BUTTON_X;
		btnmap[CTRL_SCOPE][CTRLR_GCPAD][i++] = PAD_BUTTON_START;
	}

	/*** Superscope : Wiimote button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_SCOPE && wiiCtrl == CTRLR_WIIMOTE))
	{
		i=0;
		btnmap[CTRL_SCOPE][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_B;
		btnmap[CTRL_SCOPE][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_A;
		btnmap[CTRL_SCOPE][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_MINUS;
		btnmap[CTRL_SCOPE][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_UP;
		btnmap[CTRL_SCOPE][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_DOWN;
		btnmap[CTRL_SCOPE][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_PLUS;
	}

	/*** Superscope : Wii Classic Controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_SCOPE && wiiCtrl == CTRLR_CLASSIC))
	{
		i=0;
		btnmap[CTRL_SCOPE][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_SCOPE][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_A;
		btnmap[CTRL_SCOPE][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_MINUS;
		btnmap[CTRL_SCOPE][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_Y;
		btnmap[CTRL_SCOPE][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_X;
		btnmap[CTRL_SCOPE][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_PLUS;
	}

	/*** Superscope : Wii U Pro Controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_SCOPE && wiiCtrl == CTRLR_WUPC))
	{
		i=0;
		btnmap[CTRL_SCOPE][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_SCOPE][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_A;
		btnmap[CTRL_SCOPE][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_MINUS;
		btnmap[CTRL_SCOPE][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_Y;
		btnmap[CTRL_SCOPE][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_X;
		btnmap[CTRL_SCOPE][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_PLUS;
	}

	/*** Superscope : Wii U Gamepad button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_SCOPE && wiiCtrl == CTRLR_WIIDRC))
	{
		i=0;
		btnmap[CTRL_SCOPE][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_B;
		btnmap[CTRL_SCOPE][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_A;
		btnmap[CTRL_SCOPE][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_MINUS;
		btnmap[CTRL_SCOPE][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_Y;
		btnmap[CTRL_SCOPE][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_X;
		btnmap[CTRL_SCOPE][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_PLUS;
	}

	/*** Mouse : GC controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_MOUSE && wiiCtrl == CTRLR_GCPAD))
	{
		i=0;
		btnmap[CTRL_MOUSE][CTRLR_GCPAD][i++] = PAD_BUTTON_A;
		btnmap[CTRL_MOUSE][CTRLR_GCPAD][i++] = PAD_BUTTON_B;
	}

	/*** Mouse : Wiimote button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_MOUSE && wiiCtrl == CTRLR_WIIMOTE))
	{
		i=0;
		btnmap[CTRL_MOUSE][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_A;
		btnmap[CTRL_MOUSE][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_B;
	}

	/*** Mouse : Wii Classic Controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_MOUSE && wiiCtrl == CTRLR_CLASSIC))
	{
		i=0;
		btnmap[CTRL_MOUSE][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_MOUSE][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_A;
	}

	/*** Mouse : Wii U Pro Controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_MOUSE && wiiCtrl == CTRLR_WUPC))
	{
		i=0;
		btnmap[CTRL_MOUSE][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_MOUSE][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_A;
	}

	/*** Mouse : Wii U Gamepad button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_MOUSE && wiiCtrl == CTRLR_WIIDRC))
	{
		i=0;
		btnmap[CTRL_MOUSE][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_B;
		btnmap[CTRL_MOUSE][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_A;
	}

	/*** Justifier : GC controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_JUST && wiiCtrl == CTRLR_GCPAD))
	{
		i=0;
		btnmap[CTRL_JUST][CTRLR_GCPAD][i++] = PAD_BUTTON_A;
		btnmap[CTRL_JUST][CTRLR_GCPAD][i++] = PAD_BUTTON_B;
		btnmap[CTRL_JUST][CTRLR_GCPAD][i++] = PAD_BUTTON_START;
	}

	/*** Justifier : Wiimote button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_JUST && wiiCtrl == CTRLR_WIIMOTE))
	{
		i=0;
		btnmap[CTRL_JUST][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_B;
		btnmap[CTRL_JUST][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_A;
		btnmap[CTRL_JUST][CTRLR_WIIMOTE][i++] = WPAD_BUTTON_PLUS;
	}

	/*** Justifier : Wii Classic Controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_JUST && wiiCtrl == CTRLR_CLASSIC))
	{
		i=0;
		btnmap[CTRL_JUST][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_JUST][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_A;
		btnmap[CTRL_JUST][CTRLR_CLASSIC][i++] = WPAD_CLASSIC_BUTTON_PLUS;
	}

	/*** Justifier : Wii U Pro Controller button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_JUST && wiiCtrl == CTRLR_WUPC))
	{
		i=0;
		btnmap[CTRL_JUST][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_B;
		btnmap[CTRL_JUST][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_A;
		btnmap[CTRL_JUST][CTRLR_WUPC][i++] = WPAD_CLASSIC_BUTTON_PLUS;
	}

	/*** Justifier : Wii U Gamepad button mapping ***/
	if(consoleCtrl == -1 || (consoleCtrl == CTRL_JUST && wiiCtrl == CTRLR_WIIDRC))
	{
		i=0;
		btnmap[CTRL_JUST][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_B;
		btnmap[CTRL_JUST][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_A;
		btnmap[CTRL_JUST][CTRLR_WIIDRC][i++] = WIIDRC_BUTTON_PLUS;
	}
}

/****************************************************************************
 * UpdatePads
 *
 * Scans pad and wpad
 ***************************************************************************/

void
UpdatePads()
{
	#ifdef HW_RVL
	WiiDRC_ScanPads();
	Retrode_ScanPads();
	XBOX360_ScanPads();
	Hornet_ScanPads();
	Mayflash_ScanPads();
	WPAD_ScanPads();
	#endif

	#ifdef HW_RVL
	/* SickSaxis lib 1.0 (by xerpi) */
	u16 buttonsHeld = WPAD_ButtonsHeld(0);
	if(sicksaxis.connected)
	{
		if(buttonsHeld & WPAD_BUTTON_1  && buttonsHeld & WPAD_BUTTON_2)
		{
			ss_close(&sicksaxis);
		}
	}
	else
	{
		if(buttonsHeld & WPAD_BUTTON_MINUS  && buttonsHeld & WPAD_BUTTON_PLUS)
		{
			if(ss_open(&sicksaxis) > 0)
			{
				ss_set_led(&sicksaxis, 1);
				ss_start_reading(&sicksaxis);
			}
		}
	}
	#endif

	PAD_ScanPads();

	for(int i=3; i >= 0; i--)
	{
		userInput[i].pad.btns_d = PAD_ButtonsDown(i);
		userInput[i].pad.btns_u = PAD_ButtonsUp(i);
		userInput[i].pad.btns_h = PAD_ButtonsHeld(i);
		userInput[i].pad.stickX = PAD_StickX(i);
		userInput[i].pad.stickY = PAD_StickY(i);
		userInput[i].pad.substickX = PAD_SubStickX(i);
		userInput[i].pad.substickY = PAD_SubStickY(i);
		userInput[i].pad.triggerL = PAD_TriggerL(i);
		userInput[i].pad.triggerR = PAD_TriggerR(i);
	}

#ifdef HW_RVL
	if(WiiDRC_Inited() && WiiDRC_Connected())
	{
		userInput[0].wiidrcdata.btns_d = WiiDRC_ButtonsDown();
		userInput[0].wiidrcdata.btns_u = WiiDRC_ButtonsUp();
		userInput[0].wiidrcdata.btns_h = WiiDRC_ButtonsHeld();
		userInput[0].wiidrcdata.stickX = WiiDRC_lStickX();
		userInput[0].wiidrcdata.stickY = WiiDRC_lStickY();
		userInput[0].wiidrcdata.substickX = WiiDRC_rStickX();
		userInput[0].wiidrcdata.substickY = WiiDRC_rStickY();
	}
#endif
}

/****************************************************************************
 * SetupPads
 *
 * Sets up userInput triggers for use
 ***************************************************************************/
static bool soundSync = false;

void
SetupPads()
{
	soundSync = Settings.SoundSync;
	PAD_Init();

	#ifdef HW_RVL
	/* SickSaxis lib 1.0 (by xerpi) */
	ss_init();
	if(ss_open(&sicksaxis) > 0) ss_start_reading(&sicksaxis);
	#endif

	#ifdef HW_RVL
	// read Wiimote accelerometer and IR data
	WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL, screenwidth, screenheight);
	#endif

	for(int i=0; i < 4; i++)
	{
		userInput[i].chan = i;
		#ifdef HW_RVL
		userInput[i].wpad = WPAD_Data(i);
		#endif
	}
}

/****************************************************************************
 * UpdateCursorPosition
 *
 * Updates X/Y coordinates for Superscope/mouse/justifier position
 ***************************************************************************/
static void UpdateCursorPosition (int chan, int &pos_x, int &pos_y)
{
	#define SCOPEPADCAL 20

	// gc left joystick

	if (userInput[chan].pad.stickX > ANALOG_SENSITIVITY)
	{
		pos_x += (userInput[chan].pad.stickX*1.0)/SCOPEPADCAL;
		if (pos_x > 256) pos_x = 256;
	}
	else if (userInput[chan].pad.stickX < -ANALOG_SENSITIVITY)
	{
		pos_x -= (userInput[chan].pad.stickX*-1.0)/SCOPEPADCAL;
		if (pos_x < 0) pos_x = 0;
	}

	if (userInput[chan].pad.stickY < -ANALOG_SENSITIVITY)
	{
		pos_y += (userInput[chan].pad.stickY*-1.0)/SCOPEPADCAL;
		if (pos_y > 224) pos_y = 224;
	}
	else if (userInput[chan].pad.stickY > ANALOG_SENSITIVITY)
	{
		pos_y -= (userInput[chan].pad.stickY*1.0)/SCOPEPADCAL;
		if (pos_y < 0) pos_y = 0;
	}

#ifdef HW_RVL
	if (userInput[chan].wpad->ir.valid)
	{
		pos_x = (userInput[chan].wpad->ir.x * 256) / 640;
		pos_y = (userInput[chan].wpad->ir.y * 224) / 480;
	}
	else
	{
		s8 wm_ax = userInput[chan].WPAD_StickX(0);
		s8 wm_ay = userInput[chan].WPAD_StickY(0);

		if (wm_ax > ANALOG_SENSITIVITY)
		{
			pos_x += (wm_ax*1.0)/SCOPEPADCAL;
			if (pos_x > 256) pos_x = 256;
		}
		else if (wm_ax < -ANALOG_SENSITIVITY)
		{
			pos_x -= (wm_ax*-1.0)/SCOPEPADCAL;
			if (pos_x < 0) pos_x = 0;
		}

		if (wm_ay < -ANALOG_SENSITIVITY)
		{
			pos_y += (wm_ay*-1.0)/SCOPEPADCAL;
			if (pos_y > 224) pos_y = 224;
		}
		else if (wm_ay > ANALOG_SENSITIVITY)
		{
			pos_y -= (wm_ay*1.0)/SCOPEPADCAL;
			if (pos_y < 0) pos_y = 0;
		}
	}
#endif

}

/****************************************************************************
 * decodepad
 *
 * Reads the changes (buttons pressed, etc) from a controller and reports
 * these changes to Snes9x
 ***************************************************************************/
static void decodepad (int chan, int emuChan)
{
	int i, offset;

	s8 pad_x = userInput[chan].pad.stickX;
	s8 pad_y = userInput[chan].pad.stickY;
	u32 jp = userInput[chan].pad.btns_h;

#ifdef HW_RVL
	s8 wm_ax = userInput[chan].WPAD_StickX(0);
	s8 wm_ay = userInput[chan].WPAD_StickY(0);
	u32 wp = userInput[chan].wpad->btns_h;
	bool isWUPC = userInput[chan].wpad->exp.classic.type == 2;

	u32 exp_type;
	if ( WPAD_Probe(chan, &exp_type) != 0 )
		exp_type = WPAD_EXP_NONE;

	s16 wiidrc_ax = userInput[chan].wiidrcdata.stickX;
	s16 wiidrc_ay = userInput[chan].wiidrcdata.stickY;
	u32 wiidrcp = userInput[chan].wiidrcdata.btns_h;

	jp |= Retrode_ButtonsHeld(chan);
	jp |= XBOX360_ButtonsHeld(chan);
	jp |= Hornet_ButtonsHeld(chan);
	jp |= Mayflash_ButtonsHeld(chan);
#endif

#ifdef HW_RVL
	/* Sicksaxis lib 1.0 (by xerpi) */
	if(sicksaxis.connected)
	{
		int8_t aX = sicksaxis.gamepad.leftAnalog.x - 128;
		int8_t aY = sicksaxis.gamepad.leftAnalog.y - 128;
		
		uint8_t up    = sicksaxis.gamepad.buttons.up    ||  (aY < -SICKSAXIS_DEADZONE);
		uint8_t down  = sicksaxis.gamepad.buttons.down  ||  (aY > SICKSAXIS_DEADZONE);
		uint8_t right = sicksaxis.gamepad.buttons.right ||  (aX > SICKSAXIS_DEADZONE);
		uint8_t left  = sicksaxis.gamepad.buttons.left  ||  (aX < -SICKSAXIS_DEADZONE);

		jp |= up    ? PAD_BUTTON_UP    : 0;
		jp |= down  ? PAD_BUTTON_DOWN  : 0;
		jp |= right ? PAD_BUTTON_RIGHT : 0;
		jp |= left  ? PAD_BUTTON_LEFT  : 0;

		jp |= sicksaxis.gamepad.buttons.circle   ? PAD_BUTTON_A : 0;
		jp |= sicksaxis.gamepad.buttons.cross    ? PAD_BUTTON_B : 0;
		jp |= sicksaxis.gamepad.buttons.triangle ? PAD_BUTTON_X : 0;
		jp |= sicksaxis.gamepad.buttons.square   ? PAD_BUTTON_Y : 0;

		jp |= sicksaxis.gamepad.buttons.L1 ? PAD_TRIGGER_L : 0;
		jp |= sicksaxis.gamepad.buttons.R1 ? PAD_TRIGGER_R : 0;
		
		jp |= sicksaxis.gamepad.buttons.select ? PAD_TRIGGER_Z : 0;
		jp |= sicksaxis.gamepad.buttons.start ? PAD_BUTTON_START : 0;

		jp |= sicksaxis.gamepad.buttons.L2 ? PAD_TRIGGER_L : 0;
		jp |= sicksaxis.gamepad.buttons.R2 ? PAD_TRIGGER_R : 0;
		
	}
#endif

	/***
	Gamecube Joystick input
	***/
	if (pad_y > ANALOG_SENSITIVITY)
		jp |= PAD_BUTTON_UP;
	else if (pad_y < -ANALOG_SENSITIVITY)
		jp |= PAD_BUTTON_DOWN;
	if (pad_x < -ANALOG_SENSITIVITY)
		jp |= PAD_BUTTON_LEFT;
	else if (pad_x > ANALOG_SENSITIVITY)
		jp |= PAD_BUTTON_RIGHT;

	// Count as pressed if down far enough (~50% down)
	if (userInput[chan].pad.triggerL > 0x80)
		jp |= PAD_TRIGGER_L;
	if (userInput[chan].pad.triggerR > 0x80)
		jp |= PAD_TRIGGER_R;

#ifdef HW_RVL
	/***
	Wii Joystick (classic, nunchuk) input
	***/
	if (wm_ay > ANALOG_SENSITIVITY)
		wp |= (exp_type == WPAD_EXP_CLASSIC) ? WPAD_CLASSIC_BUTTON_UP : WPAD_BUTTON_UP;
	else if (wm_ay < -ANALOG_SENSITIVITY)
		wp |= (exp_type == WPAD_EXP_CLASSIC) ? WPAD_CLASSIC_BUTTON_DOWN : WPAD_BUTTON_DOWN;
	if (wm_ax < -ANALOG_SENSITIVITY)
		wp |= (exp_type == WPAD_EXP_CLASSIC) ? WPAD_CLASSIC_BUTTON_LEFT : WPAD_BUTTON_LEFT;
	else if (wm_ax > ANALOG_SENSITIVITY)
		wp |= (exp_type == WPAD_EXP_CLASSIC) ? WPAD_CLASSIC_BUTTON_RIGHT : WPAD_BUTTON_RIGHT;

	/* Wii U Gamepad */
	if (wiidrc_ay > ANALOG_SENSITIVITY)
		wiidrcp |= WIIDRC_BUTTON_UP;
	else if (wiidrc_ay < -ANALOG_SENSITIVITY)
		wiidrcp |= WIIDRC_BUTTON_DOWN;
	if (wiidrc_ax < -ANALOG_SENSITIVITY)
		wiidrcp |= WIIDRC_BUTTON_LEFT;
	else if (wiidrc_ax > ANALOG_SENSITIVITY)
		wiidrcp |= WIIDRC_BUTTON_RIGHT;
#endif

	/*** Fix offset to pad ***/
	offset = ((emuChan + 1) << 4);

	/*** Report pressed buttons (gamepads) ***/
	for (i = 0; i < MAXJP; i++)
    {
		if ( (jp & btnmap[CTRL_PAD][CTRLR_GCPAD][i]) // gamecube controller
#ifdef HW_RVL
		|| ( (exp_type == WPAD_EXP_NONE) && (wp & btnmap[CTRL_PAD][CTRLR_WIIMOTE][i]) )	// wiimote
		|| ( (exp_type == WPAD_EXP_CLASSIC && !isWUPC) && (wp & btnmap[CTRL_PAD][CTRLR_CLASSIC][i]) ) // classic controller
		|| ( (exp_type == WPAD_EXP_CLASSIC && isWUPC) && (wp & btnmap[CTRL_PAD][CTRLR_WUPC][i]) ) // wii u pro controller
		|| ( (exp_type == WPAD_EXP_NUNCHUK) && (wp & btnmap[CTRL_PAD][CTRLR_NUNCHUK][i]) ) // nunchuk + wiimote
		|| ( (wiidrcp & btnmap[CTRL_PAD][CTRLR_WIIDRC][i]) ) // Wii U Gamepad
#endif
		)
			S9xReportButton (offset + i, true);
		else
			S9xReportButton (offset + i, false);
    }

	/*** Superscope ***/
	if (Settings.SuperScopeMaster && emuChan == 0) // report only once
	{
		// buttons
		offset = 0x50;
		for (i = 0; i < 6; i++)
		{
			if (jp & btnmap[CTRL_SCOPE][CTRLR_GCPAD][i]
#ifdef HW_RVL
			|| wp & btnmap[CTRL_SCOPE][CTRLR_WIIMOTE][i]
			|| wp & btnmap[CTRL_SCOPE][CTRLR_CLASSIC][i]
			|| wp & btnmap[CTRL_SCOPE][CTRLR_WUPC][i]
			|| wiidrcp & btnmap[CTRL_SCOPE][CTRLR_WIIDRC][i]
#endif
			)
			{
				if(i == 3 || i == 4) // turbo
				{
					if((i == 3 && scopeTurbo == 1) || // turbo ON already, don't change
						(i == 4 && scopeTurbo == 0)) // turbo OFF already, don't change
					{
						S9xReportButton(offset + i, false);
					}
					else // turbo changed to ON or OFF
					{
						scopeTurbo = 4-i;
						S9xReportButton(offset + i, true);
					}
				}
				else
					S9xReportButton(offset + i, true);
			}
			else
				S9xReportButton(offset + i, false);
		}
		// pointer
		offset = 0x80;
		UpdateCursorPosition(emuChan, cursor_x[0], cursor_y[0]);
		S9xReportPointer(offset, (u16) cursor_x[0], (u16) cursor_y[0]);
	}
	/*** Mouse ***/
	else if (Settings.MouseMaster && emuChan == 0)
	{
		// buttons
		offset = 0x60 + (2 * emuChan);
		for (i = 0; i < 2; i++)
		{
			if (jp & btnmap[CTRL_MOUSE][CTRLR_GCPAD][i]
#ifdef HW_RVL
			|| wp & btnmap[CTRL_MOUSE][CTRLR_WIIMOTE][i]
			|| wp & btnmap[CTRL_MOUSE][CTRLR_CLASSIC][i]
			|| wp & btnmap[CTRL_MOUSE][CTRLR_WUPC][i]
			|| wiidrcp & btnmap[CTRL_MOUSE][CTRLR_WIIDRC][i]
#endif
			)
				S9xReportButton(offset + i, true);
			else
				S9xReportButton(offset + i, false);
		}
		// pointer
		offset = 0x81;
		UpdateCursorPosition(emuChan, cursor_x[1 + emuChan], cursor_y[1 + emuChan]);
		S9xReportPointer(offset + emuChan, (u16) cursor_x[1 + emuChan],
				(u16) cursor_y[1 + emuChan]);
	}
	/*** Justifier ***/
	else if (Settings.JustifierMaster && emuChan < 2)
	{
		// buttons
		offset = 0x70 + (3 * emuChan);
		for (i = 0; i < 3; i++)
		{
			if (jp & btnmap[CTRL_JUST][CTRLR_GCPAD][i]
#ifdef HW_RVL
			|| wp & btnmap[CTRL_JUST][CTRLR_WIIMOTE][i]
			|| wp & btnmap[CTRL_JUST][CTRLR_CLASSIC][i]
			|| wp & btnmap[CTRL_JUST][CTRLR_WUPC][i]
			|| wiidrcp & btnmap[CTRL_JUST][CTRLR_WIIDRC][i]
#endif
			)
				S9xReportButton(offset + i, true);
			else
				S9xReportButton(offset + i, false);
		}
		// pointer
		offset = 0x83;
		UpdateCursorPosition(emuChan, cursor_x[3 + emuChan], cursor_y[3 + emuChan]);
		S9xReportPointer(offset + emuChan, (u16) cursor_x[3 + emuChan],
				(u16) cursor_y[3 + emuChan]);
	}

#ifdef HW_RVL
	// screenshot (temp)
	if (wp & CLASSIC_CTRL_BUTTON_ZR)
		S9xReportButton(0x90, true);
	else
		S9xReportButton(0x90, false);
#endif
}

bool MenuRequested()
{
	for(int i=0; i<4; i++)
	{
		if (
			(userInput[i].pad.substickX < -70) ||
			(userInput[i].pad.btns_h & PAD_BUTTON_START &&
			userInput[i].pad.btns_h & PAD_BUTTON_A &&
			userInput[i].pad.btns_h & PAD_BUTTON_B &&
			userInput[i].pad.btns_h & PAD_TRIGGER_Z
			)
			#ifdef HW_RVL
			|| (userInput[i].wpad->btns_h & WPAD_BUTTON_HOME) ||
			(userInput[i].wpad->btns_h & WPAD_CLASSIC_BUTTON_HOME) ||
			(userInput[i].wiidrcdata.btns_h & WIIDRC_BUTTON_HOME) ||
			(userInput[i].wpad->btns_h & WPAD_CLASSIC_BUTTON_PLUS &&
			userInput[i].wpad->btns_h & WPAD_CLASSIC_BUTTON_A &&
			userInput[i].wpad->btns_h & WPAD_CLASSIC_BUTTON_B &&
			userInput[i].wpad->btns_h & WPAD_CLASSIC_BUTTON_MINUS)
			#endif
		)
		{
			return true;
		}
	}
	return false;
}

bool IsTurboModeInputPressed()
{
	switch(GCSettings.TurboModeButton)
	{
		case TURBO_BUTTON_RSTICK:
			return (
				userInput[0].pad.substickX > 70 ||
				userInput[0].WPAD_StickX(1) > 70 ||
				userInput[0].wiidrcdata.substickX > 45);
		case TURBO_BUTTON_A:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_A ||
				userInput[0].wpad->btns_h & WPAD_BUTTON_A ||
				userInput[0].pad.btns_h & PAD_BUTTON_A ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_A);
		case TURBO_BUTTON_B:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_B ||
				userInput[0].wpad->btns_h & WPAD_BUTTON_B ||
				userInput[0].pad.btns_h & PAD_BUTTON_B ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_B);
		case TURBO_BUTTON_X:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_X ||
				userInput[0].pad.btns_h & PAD_BUTTON_X ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_X);
		case TURBO_BUTTON_Y:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_Y ||
				userInput[0].pad.btns_h & PAD_BUTTON_Y ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_Y);
		case TURBO_BUTTON_L:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_FULL_L ||
				userInput[0].pad.btns_h & PAD_TRIGGER_L ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_L);
		case TURBO_BUTTON_R:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_FULL_R ||
				userInput[0].pad.btns_h & PAD_TRIGGER_R ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_R);
		case TURBO_BUTTON_ZL:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_ZL ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_ZL);
		case TURBO_BUTTON_ZR:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_ZR ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_ZR);
		case TURBO_BUTTON_Z:
			return (
				userInput[0].pad.btns_h & PAD_TRIGGER_Z ||
				(userInput[0].wpad->exp.type == WPAD_EXP_NUNCHUK &&
				userInput[0].wpad->btns_h & WPAD_NUNCHUK_BUTTON_Z));
		case TURBO_BUTTON_C:
			return (
				userInput[0].wpad->exp.type == WPAD_EXP_NUNCHUK &&
				userInput[0].wpad->btns_h & WPAD_NUNCHUK_BUTTON_C);
		case TURBO_BUTTON_1:
			return (
				userInput[0].wpad->btns_h & WPAD_BUTTON_1);
		case TURBO_BUTTON_2:
			return (
				userInput[0].wpad->btns_h & WPAD_BUTTON_2);
		case TURBO_BUTTON_PLUS:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_PLUS ||
				userInput[0].wpad->btns_h & WPAD_BUTTON_PLUS ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_PLUS);
		case TURBO_BUTTON_MINUS:
			return (
				userInput[0].wpad->btns_h & WPAD_CLASSIC_BUTTON_MINUS ||
				userInput[0].wpad->btns_h & WPAD_BUTTON_MINUS ||
				userInput[0].wiidrcdata.btns_h & WIIDRC_BUTTON_MINUS);
		default:
			return false;
	}
}

/****************************************************************************
 * ReportButtons
 *
 * Called on each rendered frame
 * Our way of putting controller input into Snes9x
 ***************************************************************************/
void ReportButtons ()
{
	int i;

	UpdatePads();

	if (GCSettings.TurboModeEnabled == 1)
	{
		Settings.TurboMode = IsTurboModeInputPressed();
	}

	if(Settings.TurboMode) {
		Settings.SoundSync = false;
	}
	else {
		Settings.SoundSync = soundSync;
	}

	/* Check for menu:
	 * Gamecube c-stick left
	 * OR "A+B+Start+Z" on the Gamecube controller ports (eg. Homebrew/Adapted NES-SNES controllers)
	 * OR "Home" on the Wiimote or Wii Classic Controller
	 * OR "Select+Start+A+B" on Wiimote controller extensions (eg. NES/SNES/3rd party controllers)
	 */
	if(MenuRequested())
		ScreenshotRequested = 1; // go to the menu

	int numControllers = (Settings.MultiPlayer5Master == true ? 4 : 2);

	for (i = 0; i < 4; i++) {
		if(playerMapping[i] < numControllers) {
			decodepad (i, playerMapping[i]);
		}
	}
}

void SetControllers()
{
	if (Settings.MultiPlayer5Master == true)
	{
		S9xSetController (0, CTL_JOYPAD, 0, 0, 0, 0);
		S9xSetController (1, CTL_MP5, 1, 2, 3, -1);
	}
	else if (Settings.SuperScopeMaster == true)
	{
		S9xSetController (0, CTL_JOYPAD, 0, 0, 0, 0);
		S9xSetController (1, CTL_SUPERSCOPE, 0, 0, 0, 0);
	}
	else if (Settings.MouseMaster == true)
	{
		S9xSetController (0, CTL_MOUSE, 0, 0, 0, 0);
		S9xSetController (1, CTL_JOYPAD, 1, 0, 0, 0);
	}
	else if (Settings.JustifierMaster == true)
	{
		S9xSetController (0, CTL_JOYPAD, 0, 0, 0, 0);
		S9xSetController(1, CTL_JUSTIFIER, 1, 0, 0, 0);
	}
	else
	{
		// Plugin 2 Joypads by default
		S9xSetController (0, CTL_JOYPAD, 0, 0, 0, 0);
		S9xSetController (1, CTL_JOYPAD, 1, 0, 0, 0);
	}
}

/****************************************************************************
 * Set the default mapping
 ***************************************************************************/
void SetDefaultButtonMap ()
{
	int maxcode = 0x10;
	s9xcommand_t cmd;

	/*** Joypad 1 ***/
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 A");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 B");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 X");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 Y");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 L");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 R");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 Start");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 Select");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 Up");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 Down");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 Left");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad1 Right");

	maxcode = 0x20;
	/*** Joypad 2 ***/
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 A");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 B");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 X");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 Y");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 L");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 R");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 Start");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 Select");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 Up");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 Down");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 Left");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad2 Right");

	maxcode = 0x30;
	/*** Joypad 3 ***/
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 A");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 B");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 X");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 Y");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 L");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 R");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 Start");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 Select");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 Up");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 Down");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 Left");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad3 Right");

	maxcode = 0x40;
	/*** Joypad 4 ***/
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 A");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 B");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 X");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 Y");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 L");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 R");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 Start");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 Select");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 Up");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 Down");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 Left");
	ASSIGN_BUTTON_FALSE (maxcode++, "Joypad4 Right");

	maxcode = 0x50;
	/*** Superscope ***/
	ASSIGN_BUTTON_FALSE (maxcode++, "Superscope Fire");
	ASSIGN_BUTTON_FALSE (maxcode++, "Superscope AimOffscreen");
	ASSIGN_BUTTON_FALSE (maxcode++, "Superscope Cursor");
	ASSIGN_BUTTON_FALSE (maxcode++, "Superscope ToggleTurbo");
	ASSIGN_BUTTON_FALSE (maxcode++, "Superscope ToggleTurbo");
	ASSIGN_BUTTON_FALSE (maxcode++, "Superscope Pause");

	maxcode = 0x60;
	/*** Mouse ***/
	ASSIGN_BUTTON_FALSE (maxcode++, "Mouse1 L");
	ASSIGN_BUTTON_FALSE (maxcode++, "Mouse1 R");
	ASSIGN_BUTTON_FALSE (maxcode++, "Mouse2 L");
	ASSIGN_BUTTON_FALSE (maxcode++, "Mouse2 R");

	maxcode = 0x70;
	/*** Justifier ***/
	ASSIGN_BUTTON_FALSE (maxcode++, "Justifier1 Trigger");
	ASSIGN_BUTTON_FALSE (maxcode++, "Justifier1 AimOffscreen");
	ASSIGN_BUTTON_FALSE (maxcode++, "Justifier1 Start");
	ASSIGN_BUTTON_FALSE (maxcode++, "Justifier2 Trigger");
	ASSIGN_BUTTON_FALSE (maxcode++, "Justifier2 AimOffscreen");
	ASSIGN_BUTTON_FALSE (maxcode++, "Justifier2 Start");

	maxcode = 0x80;
	S9xMapPointer(maxcode++, S9xGetCommandT("Pointer Superscope"), false);
	S9xMapPointer(maxcode++, S9xGetCommandT("Pointer Mouse1"), false);
	S9xMapPointer(maxcode++, S9xGetCommandT("Pointer Mouse2"), false);
	S9xMapPointer(maxcode++, S9xGetCommandT("Pointer Justifier1"), false);
	S9xMapPointer(maxcode++, S9xGetCommandT("Pointer Justifier2"), false);

	maxcode = 0x90;
	//ASSIGN_BUTTON_FALSE (maxcode++, "Screenshot");

	SetControllers();
}

#ifdef HW_RVL
char* GetUSBControllerInfo()
{
    static char info[100];
    snprintf(info, 100, "Retrode: %s, XBOX360: %s, Hornet: %s, Mayflash: %s", Retrode_Status(), XBOX360_Status(), Hornet_Status(), Mayflash_Status());
    return info;
}
#endif
