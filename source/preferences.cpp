/****************************************************************************
 * Snes9x Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2019
 *
 * preferences.cpp
 *
 * Preferences save/load to XML file
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ogcsys.h>
#include <mxml.h>

#include "snes9xgx.h"
#include "menu.h"
#include "fileop.h"
#include "filebrowser.h"
#include "input.h"
#include "button_mapping.h"

#include "snes9x/apu/apu.h"

struct SGCSettings GCSettings;

/****************************************************************************
 * Prepare Preferences Data
 *
 * This sets up the save buffer for saving.
 ***************************************************************************/
static mxml_node_t *xml = NULL;
static mxml_node_t *data = NULL;
static mxml_node_t *section = NULL;
static mxml_node_t *item = NULL;
static mxml_node_t *elem = NULL;

static char temp[200];

static const char * toStr(int i)
{
	sprintf(temp, "%d", i);
	return temp;
}

static const char * FtoStr(float i)
{
	sprintf(temp, "%.2f", i);
	return temp;
}

static void createXMLSection(const char * name, const char * description)
{
	section = mxmlNewElement(data, "section");
	mxmlElementSetAttr(section, "name", name);
	mxmlElementSetAttr(section, "description", description);
}

static void createXMLSetting(const char * name, const char * description, const char * value)
{
	item = mxmlNewElement(section, "setting");
	mxmlElementSetAttr(item, "name", name);
	mxmlElementSetAttr(item, "value", value);
	mxmlElementSetAttr(item, "description", description);
}

static void createXMLController(u32 controller[], const char * name, const char * description)
{
	item = mxmlNewElement(section, "controller");
	mxmlElementSetAttr(item, "name", name);
	mxmlElementSetAttr(item, "description", description);

	// create buttons
	for(int i=0; i < MAXJP; i++)
	{
		elem = mxmlNewElement(item, "button");
		mxmlElementSetAttr(elem, "number", toStr(i));
		mxmlElementSetAttr(elem, "assignment", toStr(controller[i]));
	}
}

static const char * XMLSaveCallback(mxml_node_t *node, int where)
{
	const char *name;

	name = node->value.element.name;

	if(where == MXML_WS_BEFORE_CLOSE)
	{
		if(!strcmp(name, "file") || !strcmp(name, "section"))
			return ("\n");
		else if(!strcmp(name, "controller"))
			return ("\n\t");
	}
	if (where == MXML_WS_BEFORE_OPEN)
	{
		if(!strcmp(name, "file"))
			return ("\n");
		else if(!strcmp(name, "section"))
			return ("\n\n");
		else if(!strcmp(name, "setting") || !strcmp(name, "controller"))
			return ("\n\t");
		else if(!strcmp(name, "button"))
			return ("\n\t\t");
	}
	return (NULL);
}

static int
preparePrefsData ()
{
	xml = mxmlNewXML("1.0");
	mxmlSetWrapMargin(0); // disable line wrapping

	data = mxmlNewElement(xml, "file");
	mxmlElementSetAttr(data, "app", APPNAME);
	mxmlElementSetAttr(data, "version", APPVERSION);

	createXMLSection("File", "File Settings");

	createXMLSetting("AutoLoad", "Auto Load", toStr(GCSettings.AutoLoad));
	createXMLSetting("AutoSave", "Auto Save", toStr(GCSettings.AutoSave));
	createXMLSetting("LoadMethod", "Load Method", toStr(GCSettings.LoadMethod));
	createXMLSetting("SaveMethod", "Save Method", toStr(GCSettings.SaveMethod));
	createXMLSetting("LoadFolder", "Load Folder", GCSettings.LoadFolder);
	createXMLSetting("LastFileLoaded", "Last File Loaded", GCSettings.LastFileLoaded);
	createXMLSetting("SaveFolder", "Save Folder", GCSettings.SaveFolder);
	createXMLSetting("AppendAuto", "Append Auto to .SAV files", toStr(GCSettings.AppendAuto));
	createXMLSetting("CheatFolder", "Cheats Folder", GCSettings.CheatFolder);
	createXMLSetting("ScreenshotsFolder", "Screenshots Folder", GCSettings.ScreenshotsFolder);
	createXMLSetting("CoverFolder", "Covers Folder", GCSettings.CoverFolder);
	createXMLSetting("ArtworkFolder", "Artwork Folder", GCSettings.ArtworkFolder);
	
	createXMLSection("Network", "Network Settings");

	createXMLSetting("smbip", "Share Computer IP", GCSettings.smbip);
	createXMLSetting("smbshare", "Share Name", GCSettings.smbshare);
	createXMLSetting("smbuser", "Share Username", GCSettings.smbuser);
	createXMLSetting("smbpwd", "Share Password", GCSettings.smbpwd);

	createXMLSection("Video", "Video Settings");

	createXMLSetting("videomode", "Video Mode", toStr(GCSettings.videomode));
	createXMLSetting("zoomHor", "Horizontal Zoom Level", FtoStr(GCSettings.zoomHor));
	createXMLSetting("zoomVert", "Vertical Zoom Level", FtoStr(GCSettings.zoomVert));
	createXMLSetting("render", "Video Filtering", toStr(GCSettings.render));
	createXMLSetting("widescreen", "Aspect Ratio Correction", toStr(GCSettings.widescreen));
	createXMLSetting("crosshair", "Crosshair", toStr(GCSettings.crosshair));
	createXMLSetting("FilterMethod", "Filter Method", toStr(GCSettings.FilterMethod));
	createXMLSetting("ShowFrameRate", "Show Framerate", toStr(GCSettings.ShowFrameRate));
	createXMLSetting("ShowLocalTime", "Show Local Time", toStr(GCSettings.ShowLocalTime));
	createXMLSetting("xshift", "Horizontal Video Shift", toStr(GCSettings.xshift));
	createXMLSetting("yshift", "Vertical Video Shift", toStr(GCSettings.yshift));
	createXMLSetting("sfxOverclock", "SuperFX Overclock", toStr(GCSettings.sfxOverclock));
	createXMLSetting("TurboModeEnabled", "Turbo Mode Enabled", toStr(GCSettings.TurboModeEnabled));
	createXMLSetting("TurboModeButton", "Turbo Mode Button", toStr(GCSettings.TurboModeButton));

	createXMLSection("Audio", "Audio Settings");

	createXMLSetting("ReverseStereo", "Stereo Reverse", toStr(GCSettings.ReverseStereo));
	createXMLSetting("Interpolation", "Interpolation", toStr(GCSettings.Interpolation));

	createXMLSection("Menu", "Menu Settings");

#ifdef HW_RVL
	createXMLSetting("WiimoteOrientation", "Wiimote Orientation", toStr(GCSettings.WiimoteOrientation));
#endif
	createXMLSetting("ExitAction", "Exit Action", toStr(GCSettings.ExitAction));
	createXMLSetting("MusicVolume", "Music Volume", toStr(GCSettings.MusicVolume));
	createXMLSetting("SFXVolume", "Sound Effects Volume", toStr(GCSettings.SFXVolume));
	createXMLSetting("DisplayVM", "Display Virtual Memory", toStr(GCSettings.DisplayVM));
	createXMLSetting("language", "Language", toStr(GCSettings.language));
	createXMLSetting("PreviewImage", "Preview Image", toStr(GCSettings.PreviewImage));
	createXMLSetting("HideSRAMSaving", "Hide SRAM Saving", toStr(GCSettings.HideSRAMSaving));
	
	createXMLSection("Controller", "Controller Settings");

	createXMLSetting("Controller", "Controller", toStr(GCSettings.Controller));

	createXMLController(btnmap[CTRL_PAD][CTRLR_GCPAD], "btnmap_pad_gcpad", "SNES Pad - GameCube Controller");
#ifdef HW_RVL
	createXMLController(btnmap[CTRL_PAD][CTRLR_WIIMOTE], "btnmap_pad_wiimote", "SNES Pad - Wiimote");
	createXMLController(btnmap[CTRL_PAD][CTRLR_CLASSIC], "btnmap_pad_classic", "SNES Pad - Classic Controller");
	createXMLController(btnmap[CTRL_PAD][CTRLR_WUPC], "btnmap_pad_wupc", "SNES Pad - Wii U Pro Controller");
	createXMLController(btnmap[CTRL_PAD][CTRLR_WIIDRC], "btnmap_pad_wiidrc", "SNES Pad - Wii U Gamepad");
	createXMLController(btnmap[CTRL_PAD][CTRLR_NUNCHUK], "btnmap_pad_nunchuk", "SNES Pad - Nunchuk + Wiimote");
#endif
	createXMLController(btnmap[CTRL_SCOPE][CTRLR_GCPAD], "btnmap_scope_gcpad", "Super Scope - GameCube Controller");
#ifdef HW_RVL
	createXMLController(btnmap[CTRL_SCOPE][CTRLR_WIIMOTE], "btnmap_scope_wiimote", "Super Scope - Wiimote");
#endif
	createXMLController(btnmap[CTRL_MOUSE][CTRLR_GCPAD], "btnmap_mouse_gcpad", "Mouse - GameCube Controller");
#ifdef HW_RVL
	createXMLController(btnmap[CTRL_MOUSE][CTRLR_WIIMOTE], "btnmap_mouse_wiimote", "Mouse - Wiimote");
#endif
	createXMLController(btnmap[CTRL_JUST][CTRLR_GCPAD], "btnmap_just_gcpad", "Justifier - GameCube Controller");
#ifdef HW_RVL
	createXMLController(btnmap[CTRL_JUST][CTRLR_WIIMOTE], "btnmap_just_wiimote", "Justifier - Wiimote");
#endif
	int datasize = mxmlSaveString(xml, (char *)savebuffer, SAVEBUFFERSIZE, XMLSaveCallback);

	mxmlDelete(xml);

	return datasize;
}

/****************************************************************************
 * loadXMLSetting
 *
 * Load XML elements into variables for an individual variable
 ***************************************************************************/

static void loadXMLSetting(char * var, const char * name, int maxsize)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			snprintf(var, maxsize, "%s", tmp);
	}
}
static void loadXMLSetting(int * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			*var = atoi(tmp);
	}
}
static void loadXMLSetting(float * var, const char * name)
{
	item = mxmlFindElement(xml, xml, "setting", "name", name, MXML_DESCEND);
	if(item)
	{
		const char * tmp = mxmlElementGetAttr(item, "value");
		if(tmp)
			*var = atof(tmp);
	}
}

/****************************************************************************
 * loadXMLController
 *
 * Load XML elements into variables for a controller mapping
 ***************************************************************************/

static void loadXMLController(u32 controller[], const char * name)
{
	item = mxmlFindElement(xml, xml, "controller", "name", name, MXML_DESCEND);

	if(item)
	{
		// populate buttons
		for(int i=0; i < MAXJP; i++)
		{
			elem = mxmlFindElement(item, xml, "button", "number", toStr(i), MXML_DESCEND);
			if(elem)
			{
				const char * tmp = mxmlElementGetAttr(elem, "assignment");
				if(tmp)
					controller[i] = atoi(tmp);
			}
		}
	}
}

/****************************************************************************
 * decodePrefsData
 *
 * Decodes preferences - parses XML and loads preferences into the variables
 ***************************************************************************/

static bool
decodePrefsData ()
{
	bool result = false;

	xml = mxmlLoadString(NULL, (char *)savebuffer, MXML_TEXT_CALLBACK);

	if(xml)
	{
		// check settings version
		item = mxmlFindElement(xml, xml, "file", "version", NULL, MXML_DESCEND);
		if(item) // a version entry exists
		{
			const char * version = mxmlElementGetAttr(item, "version");

			if(version && strlen(version) == 5)
			{
				// this code assumes version in format X.X.X
				// XX.X.X, X.XX.X, or X.X.XX will NOT work
				int verMajor = version[0] - '0';
				int verMinor = version[2] - '0';
				int verPoint = version[4] - '0';

				// check that the versioning is valid
				if(!(verMajor >= 0 && verMajor <= 9 &&
					verMinor >= 0 && verMinor <= 9 &&
					verPoint >= 0 && verPoint <= 9))
					result = false;
				else if(verMajor < 4) // less than version 4.0.0 for Snes9x RX
					result = false; // reset settings
				else if(verMajor == 4 && verMinor == 7 && verPoint < 0)	// anything less than 4.7.0 for Snes9x RX
					result = false; // reset settings
				else
					result = true;
			}
		}

		if(result)
		{
			// File Settings

			loadXMLSetting(&GCSettings.AutoLoad, "AutoLoad");
			loadXMLSetting(&GCSettings.AutoSave, "AutoSave");
			loadXMLSetting(&GCSettings.LoadMethod, "LoadMethod");
			loadXMLSetting(&GCSettings.SaveMethod, "SaveMethod");
			loadXMLSetting(GCSettings.LoadFolder, "LoadFolder", sizeof(GCSettings.LoadFolder));
			loadXMLSetting(GCSettings.LastFileLoaded, "LastFileLoaded", sizeof(GCSettings.LastFileLoaded));
			loadXMLSetting(GCSettings.SaveFolder, "SaveFolder", sizeof(GCSettings.SaveFolder));
			loadXMLSetting(&GCSettings.AppendAuto, "AppendAuto");
			loadXMLSetting(GCSettings.CheatFolder, "CheatFolder", sizeof(GCSettings.CheatFolder));
			loadXMLSetting(GCSettings.ScreenshotsFolder, "ScreenshotsFolder", sizeof(GCSettings.ScreenshotsFolder));
			loadXMLSetting(GCSettings.CoverFolder, "CoverFolder", sizeof(GCSettings.CoverFolder));
			loadXMLSetting(GCSettings.ArtworkFolder, "ArtworkFolder", sizeof(GCSettings.ArtworkFolder));
			
			// Network Settings

			loadXMLSetting(GCSettings.smbip, "smbip", sizeof(GCSettings.smbip));
			loadXMLSetting(GCSettings.smbshare, "smbshare", sizeof(GCSettings.smbshare));
			loadXMLSetting(GCSettings.smbuser, "smbuser", sizeof(GCSettings.smbuser));
			loadXMLSetting(GCSettings.smbpwd, "smbpwd", sizeof(GCSettings.smbpwd));

			// Video Settings

			loadXMLSetting(&GCSettings.videomode, "videomode");
			loadXMLSetting(&GCSettings.zoomHor, "zoomHor");
			loadXMLSetting(&GCSettings.zoomVert, "zoomVert");
			loadXMLSetting(&GCSettings.render, "render");
			loadXMLSetting(&GCSettings.widescreen, "widescreen");
			loadXMLSetting(&GCSettings.crosshair, "crosshair");
			loadXMLSetting(&GCSettings.FilterMethod, "FilterMethod");
			loadXMLSetting(&GCSettings.ShowFrameRate, "ShowFrameRate");
			loadXMLSetting(&GCSettings.ShowLocalTime, "ShowLocalTime");
			loadXMLSetting(&GCSettings.xshift, "xshift");
			loadXMLSetting(&GCSettings.yshift, "yshift");
			loadXMLSetting(&GCSettings.TurboModeEnabled, "TurboModeEnabled");
			loadXMLSetting(&GCSettings.TurboModeButton, "TurboModeButton");

			// Audio Settings

			loadXMLSetting(&GCSettings.ReverseStereo, "ReverseStereo");
			loadXMLSetting(&GCSettings.Interpolation, "Interpolation");

			// Emulation Settings

			loadXMLSetting(&GCSettings.sfxOverclock, "sfxOverclock");

			// Menu Settings

			loadXMLSetting(&GCSettings.WiimoteOrientation, "WiimoteOrientation");
			loadXMLSetting(&GCSettings.ExitAction, "ExitAction");
			loadXMLSetting(&GCSettings.MusicVolume, "MusicVolume");
			loadXMLSetting(&GCSettings.SFXVolume, "SFXVolume");
			loadXMLSetting(&GCSettings.DisplayVM, "DisplayVM");
			loadXMLSetting(&GCSettings.language, "language");
			loadXMLSetting(&GCSettings.PreviewImage, "PreviewImage");
			loadXMLSetting(&GCSettings.HideSRAMSaving, "HideSRAMSaving");

			// Controller Settings

			loadXMLSetting(&GCSettings.Controller, "Controller");

			loadXMLController(btnmap[CTRL_PAD][CTRLR_GCPAD], "btnmap_pad_gcpad");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_WIIMOTE], "btnmap_pad_wiimote");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_CLASSIC], "btnmap_pad_classic");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_WUPC], "btnmap_pad_wupc");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_WIIDRC], "btnmap_pad_wiidrc");
			loadXMLController(btnmap[CTRL_PAD][CTRLR_NUNCHUK], "btnmap_pad_nunchuk");
			loadXMLController(btnmap[CTRL_SCOPE][CTRLR_GCPAD], "btnmap_scope_gcpad");
			loadXMLController(btnmap[CTRL_SCOPE][CTRLR_WIIMOTE], "btnmap_scope_wiimote");
			loadXMLController(btnmap[CTRL_MOUSE][CTRLR_GCPAD], "btnmap_mouse_gcpad");
			loadXMLController(btnmap[CTRL_MOUSE][CTRLR_WIIMOTE], "btnmap_mouse_wiimote");
			loadXMLController(btnmap[CTRL_JUST][CTRLR_GCPAD], "btnmap_just_gcpad");
			loadXMLController(btnmap[CTRL_JUST][CTRLR_WIIMOTE], "btnmap_just_wiimote");

		}
		mxmlDelete(xml);
	}
	return result;
}

/****************************************************************************
 * FixInvalidSettings
 *
 * Attempts to correct at least some invalid settings - the ones that
 * might cause crashes
 ***************************************************************************/
void FixInvalidSettings()
{
	if(GCSettings.LoadMethod > 6)
		GCSettings.LoadMethod = DEVICE_AUTO;
	if(GCSettings.SaveMethod > 6)
		GCSettings.SaveMethod = DEVICE_AUTO;	
	if(!(GCSettings.zoomHor > 0.5 && GCSettings.zoomHor < 1.5))
		GCSettings.zoomHor = 1.0;
	if(!(GCSettings.zoomVert > 0.5 && GCSettings.zoomVert < 1.5))
		GCSettings.zoomVert = 1.0;
	if(!(GCSettings.xshift > -50 && GCSettings.xshift < 50))
		GCSettings.xshift = 0;
	if(!(GCSettings.yshift > -50 && GCSettings.yshift < 50))
		GCSettings.yshift = 0;
	if(!(GCSettings.MusicVolume >= 0 && GCSettings.MusicVolume <= 100))
		GCSettings.MusicVolume = 20;
	if(!(GCSettings.SFXVolume >= 0 && GCSettings.SFXVolume <= 100))
		GCSettings.SFXVolume = 40;
	if(GCSettings.language < 0 || GCSettings.language >= LANG_LENGTH)
		GCSettings.language = LANG_ENGLISH;
	if(GCSettings.Controller > CTRL_PAD4 || GCSettings.Controller < CTRL_MOUSE)
		GCSettings.Controller = CTRL_PAD2;
	if(!(GCSettings.render >= 0 && GCSettings.render < 5))
		GCSettings.render = 4;
	if(!(GCSettings.videomode >= 0 && GCSettings.videomode < 5))
		GCSettings.videomode = 0;
}

/****************************************************************************
 * DefaultSettings
 *
 * Sets all the defaults!
 ***************************************************************************/
void
DefaultSettings ()
{
	memset (&GCSettings, 0, sizeof (GCSettings));

	ResetControls(); // controller button mappings

	GCSettings.LoadMethod = DEVICE_AUTO; // Auto, SD, DVD, USB, Network (SMB)
	GCSettings.SaveMethod = DEVICE_AUTO; // Auto, SD, USB, Network (SMB)
	sprintf (GCSettings.LoadFolder, "%s/roms", APPFOLDER); // Path to game files
	sprintf (GCSettings.SaveFolder, "%s/saves", APPFOLDER); // Path to save files
	sprintf (GCSettings.CheatFolder, "%s/cheats", APPFOLDER); // Path to cheat files
	sprintf (GCSettings.ScreenshotsFolder, "%s/screenshots", APPFOLDER); // Path to screenshot files
	sprintf (GCSettings.CoverFolder, "%s/covers", APPFOLDER); // Path to cover files
	sprintf (GCSettings.ArtworkFolder, "%s/artwork", APPFOLDER); // Path to artwork files
	GCSettings.AutoLoad = 1;
	GCSettings.AutoSave = 1;

	GCSettings.Controller = CTRL_PAD2;

	GCSettings.videomode = 0; // automatic video mode detection
	GCSettings.render = 2; // Unfiltered
	GCSettings.FilterMethod = FILTER_NONE;	// no hq2x

	GCSettings.widescreen = 0;

#ifdef HW_RVL // correct the default aspect ratio if the Wii has 16:9 set up
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
		GCSettings.widescreen = 1;
#endif

	GCSettings.zoomHor = 1.0; // horizontal zoom level
	GCSettings.zoomVert = 1.0; // vertical zoom level
	GCSettings.xshift = 0; // horizontal video shift
	GCSettings.yshift = 0; // vertical video shift
	GCSettings.crosshair = 1;

	GCSettings.WiimoteOrientation = 0;
	GCSettings.ExitAction = 0; // Auto
	GCSettings.AutoloadGame = 0;
	GCSettings.MusicVolume = 20;
	GCSettings.SFXVolume = 40;
	GCSettings.DisplayVM = 0; // Disabled
	GCSettings.PreviewImage = 0;
	GCSettings.HideSRAMSaving = 0;
	
#ifdef HW_RVL
	GCSettings.language = CONF_GetLanguage();

	if(GCSettings.language == LANG_JAPANESE || 
		GCSettings.language == LANG_SIMP_CHINESE || 
		GCSettings.language == LANG_TRAD_CHINESE || 
		GCSettings.language == LANG_KOREAN)
		GCSettings.language = LANG_ENGLISH;
#else
	GCSettings.language = LANG_ENGLISH;
#endif

	/****************** SNES9x Settings ***********************/

	// Default ALL to false
	memset (&Settings, 0, sizeof (Settings));

	// General

	Settings.MouseMaster = true;
	Settings.SuperScopeMaster = true;
	Settings.JustifierMaster = true;
	Settings.MultiPlayer5Master = true;
	Settings.DontSaveOopsSnapshot = true;
	Settings.ApplyCheats = true;

	Settings.HDMATimingHack = 100;
	Settings.BlockInvalidVRAMAccessMaster = true;

	// Sound defaults. On Wii this is 32Khz/16bit/Stereo
	Settings.SoundSync = true;
	Settings.SixteenBitSound = true;
	Settings.Stereo = true;
	Settings.SoundPlaybackRate = 48000;
	Settings.SoundInputRate = 31920;
	Settings.DynamicRateControl = true;
	Settings.DynamicRateLimit = 1;
	GCSettings.ReverseStereo = 1; // Enabled to fix inverted L/R audio channels

	// Interpolation Method
	GCSettings.Interpolation = 0;
	Settings.InterpolationMethod = DSP_INTERPOLATION_GAUSSIAN;

	// Graphics
	Settings.Transparency = true;
	Settings.SupportHiRes = true;
	Settings.SkipFrames = AUTO_FRAMERATE;
	Settings.TurboSkipFrames = 19;
	Settings.AutoDisplayMessages = false;
	Settings.InitialInfoStringTimeout = 200; // # frames to display messages for
	GCSettings.ShowFrameRate = 0;
	GCSettings.ShowLocalTime = 0;

	// Frame timings in 50hz and 60hz cpu mode
	Settings.FrameTimePAL = 20000;
	Settings.FrameTimeNTSC = 16667;

	GCSettings.sfxOverclock = 0;
	/* Initialize SuperFX CPU to normal speed by default */
	Settings.SuperFXSpeedPerLine = 0.417 * 10.5e6;

	GCSettings.TurboModeEnabled = 1; // Enabled by default
	GCSettings.TurboModeButton = 0; // Default is Right Analog Stick (0)
}

/****************************************************************************
 * Save Preferences
 ***************************************************************************/
static char prefpath[MAXPATHLEN] = { 0 };

bool
SavePrefs (bool silent)
{
	char filepath[MAXPATHLEN];
	int datasize;
	int offset = 0;
	int device = 0;
	
	if(prefpath[0] != 0)
	{
		sprintf(filepath, "%s/%s", prefpath, PREF_FILE_NAME);
		FindDevice(filepath, &device);
	}
	else if(appPath[0] != 0)
	{
		sprintf(filepath, "%s/%s", appPath, PREF_FILE_NAME);
		strcpy(prefpath, appPath);
		FindDevice(filepath, &device);
	}
	else
	{
		device = autoSaveMethod(silent);
		
		if(device == 0)
			return false;
		
		sprintf(filepath, "%s%s", pathPrefix[device], APPFOLDER);
		DIR *dir = opendir(filepath);
		if (!dir)
		{
			if(mkdir(filepath, 0777) != 0)
				return false;
			sprintf(filepath, "%s%s/roms", pathPrefix[device], APPFOLDER);
			if(mkdir(filepath, 0777) != 0)
				return false;
			sprintf(filepath, "%s%s/saves", pathPrefix[device], APPFOLDER);
			if(mkdir(filepath, 0777) != 0)
				return false;
		}
		else
		{
			closedir(dir);
		}
		sprintf(filepath, "%s%s/%s", pathPrefix[device], APPFOLDER, PREF_FILE_NAME);
		sprintf(prefpath, "%s%s", pathPrefix[device], APPFOLDER);
	}
	
	if(device == 0)
		return false;

	if (!silent)
		ShowAction ("Saving preferences...");

	FixInvalidSettings();

	AllocSaveBuffer ();
	datasize = preparePrefsData ();

	offset = SaveFile(filepath, datasize, silent);

	FreeSaveBuffer ();

	CancelAction();

	if (offset > 0)
	{
		if (!silent)
			InfoPrompt("Preferences saved");
		return true;
	}
	return false;
}

/****************************************************************************
 * Load Preferences from specified filepath
 ***************************************************************************/
bool
LoadPrefsFromMethod (char * path)
{
	bool retval = false;
	int offset = 0;
	char filepath[MAXPATHLEN];
	sprintf(filepath, "%s/%s", path, PREF_FILE_NAME);

	AllocSaveBuffer ();

	offset = LoadFile(filepath, SILENT);

	if (offset > 0)
		retval = decodePrefsData ();

	FreeSaveBuffer ();
	
	if(retval)
	{
		strcpy(prefpath, path);

		if(appPath[0] == 0)
			strcpy(appPath, prefpath);
	}

	return retval;
}

/****************************************************************************
 * Load Preferences
 * Checks sources consecutively until we find a preference file
 ***************************************************************************/
static bool prefLoaded = false;

bool LoadPrefs()
{
	if(prefLoaded) // already attempted loading
		return true;

	bool prefFound = false;
	char filepath[5][MAXPATHLEN];
	int numDevices;
	
#ifdef HW_RVL
	numDevices = 5;
	sprintf(filepath[0], "%s", appPath);
	sprintf(filepath[1], "sd:/apps/%s", APPFOLDER);
	sprintf(filepath[2], "usb:/apps/%s", APPFOLDER);
	sprintf(filepath[3], "sd:/%s", APPFOLDER);
	sprintf(filepath[4], "usb:/%s", APPFOLDER);
#else
	numDevices = 2;
	sprintf(filepath[0], "carda:/%s", APPFOLDER);
	sprintf(filepath[1], "cardb:/%s", APPFOLDER);
#endif

	for(int i=0; i<numDevices; i++)
	{
		prefFound = LoadPrefsFromMethod(filepath[i]);
		
		if(prefFound)
			break;
	}

	prefLoaded = true; // attempted to load preferences

	if(prefFound) {
		FixInvalidSettings();
	}
	
	// rename snes9x to snes9xgx
	if(GCSettings.LoadMethod == DEVICE_SD)
	{
		if(ChangeInterface(DEVICE_SD, NOTSILENT) && opendir("sd:/snes9x"))
			rename("sd:/snes9x", "sd:/snes9xgx");
	}
	else if(GCSettings.LoadMethod == DEVICE_USB)
	{
		if(ChangeInterface(DEVICE_USB, NOTSILENT) && opendir("usb:/snes9x"))
			rename("usb:/snes9x", "usb:/snes9xgx");
	}
	else if(GCSettings.LoadMethod == DEVICE_SMB)
	{
		if(ChangeInterface(DEVICE_SMB, NOTSILENT) && opendir("smb:/snes9x"))
			rename("smb:/snes9x", "smb:/snes9xgx");
	}

	// update folder locations
	if(strcmp(GCSettings.LoadFolder, "snes9x/roms") == 0)
		sprintf(GCSettings.LoadFolder, "snes9xgx/roms");
	
	if(strcmp(GCSettings.SaveFolder, "snes9x/saves") == 0)
		sprintf(GCSettings.SaveFolder, "snes9xgx/saves");
	
	if(strcmp(GCSettings.CheatFolder, "snes9x/cheats") == 0)
		sprintf(GCSettings.CheatFolder, "snes9xgx/cheats");
		
	if(strcmp(GCSettings.ScreenshotsFolder, "snes9x/screenshots") == 0)
		sprintf(GCSettings.ScreenshotsFolder, "snes9xgx/screenshots");

	if(strcmp(GCSettings.CoverFolder, "snes9x/covers") == 0)
		sprintf(GCSettings.CoverFolder, "snes9xgx/covers");
	
	if(strcmp(GCSettings.ArtworkFolder, "snes9x/artwork") == 0)
		sprintf(GCSettings.ArtworkFolder, "snes9xgx/artwork");
	
	// attempt to create directories if they don't exist
	if(GCSettings.LoadMethod == DEVICE_SD || GCSettings.LoadMethod == DEVICE_USB) {
		char dirPath[MAXPATHLEN];
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.LoadFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.SaveFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.CheatFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.ScreenshotsFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.CoverFolder);
		CreateDirectory(dirPath);
		sprintf(dirPath, "%s%s", pathPrefix[GCSettings.LoadMethod], GCSettings.ArtworkFolder);
		CreateDirectory(dirPath);
	}

	ChangeLanguage();

#ifdef HW_RVL
	bg_music = (u8 * )bg_music_ogg;
	bg_music_size = bg_music_ogg_size;
	LoadBgMusic();
#endif

	return prefFound;
}
