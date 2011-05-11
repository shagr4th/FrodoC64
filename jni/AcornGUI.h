/*
 *  AcornGUI.h - Defines variables for the WIMP interface
 *
 *  (C) 1997 Andreas Dehmel
 *
 *  Frodo (C) 1994-1997,2002-2005 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



#ifndef _ACORN_GUI_H_
#define _ACORN_GUI_H_


// Determine which paths to load from
#ifdef FRODO_SC
# define DEFAULT_PREFS	"FrodoSC:Prefs"
# define DEFAULT_SYSCONF	"FrodoSC:SysConf"
#else
# ifdef FRODO_PC
#  define DEFAULT_PREFS		"FrodoPC:Prefs"
#  define DEFAULT_SYSCONF	"FrodoPC:SysConf"
# else
#  define DEFAULT_PREFS	"Frodo:Prefs"
#  define DEFAULT_SYSCONF	"Frodo:SysConf"
# endif
#endif


// Text written in pane icons:
#define PANE_TEXT_PAUSE		"Pause"
#define PANE_TEXT_RESUME	"Cont"
#define PANE_TEXT_ZOOM1		"1 x"
#define PANE_TEXT_ZOOM2		"2 x"


// OS units of extra space between EmuWindow and Pane
#define EmuPaneSpace		2
// OS units of the (volume) well's border
#define WellBorder		12
// Height of title bar in OS units
#define TitleBarHeight		44
// Maximum volume of the Sound system
#define MaximumVolume		127




// Message Block indices
#define MsgB_Size	0
#define MsgB_Sender	1
#define MsgB_MyRef	2
#define MsgB_YourRef	3
#define MsgB_Action	4


// Messages
#define Message_Quit		0x00000
#define Message_DataSave	0x00001
#define Message_DataSaveAck	0x00002
#define Message_DataLoad	0x00003
#define Message_DataLoadAck	0x00004
#define Message_DataOpen	0x00005
#define Message_RAMFetch	0x00006
#define Message_RAMTransmit	0x00007
#define Message_PreQuit		0x00008
#define Message_PaletteChange	0x00009
#define Message_MenuWarning	0x400c0
#define Message_ModeChange	0x400c1


// Redraw Window Block
#define RedrawB_Handle		0
#define RedrawB_VMinX		1
#define RedrawB_VMinY		2
#define RedrawB_VMaxX		3
#define RedrawB_VMaxY		4
#define RedrawB_ScrollX		5
#define RedrawB_ScrollY		6
#define RedrawB_CMinX		7
#define RedrawB_CMinY		8
#define RedrawB_CMaxX		9
#define RedrawB_CMaxY		10


// Window block (e.g. open, getstate.... For create: subtract -1 (no handle))
#define WindowB_Handle		0
#define WindowB_VMinX		1
#define WindowB_VMinY		2
#define WindowB_VMaxX		3
#define WindowB_VMaxY		4
#define WindowB_ScrollX		5
#define WindowB_ScrollY		6
#define WindowB_Stackpos	7
#define WindowB_WFlags		8
#define WindowB_Colours1	9
#define WindowB_Colours2	10
#define WindowB_WMinX		11
#define WindowB_WMinY		12
#define WindowB_WMaxX		13
#define WindowB_WMaxY		14
#define WindowB_TFlags		15
#define WindowB_WAFlags		16
#define WindowB_SpriteArea	17
#define WindowB_MinDims		18
#define WindowB_Data		19
#define WindowB_Icons		22


// Raw icon block
#define RawIB_MinX		0
#define RawIB_MinY		1
#define RawIB_MaxX		2
#define RawIB_MaxY		3
#define RawIB_Flags		4
#define RawIB_Data0		5
#define RawIB_Data1		6
#define RawIB_Data2		7


// Icon block (as in GetIconState)
#define IconB_Handle		0
#define IconB_Number		1
#define IconB_MinX		2
#define IconB_MinY		3
#define IconB_MaxX		4
#define IconB_MaxY		5
#define IconB_Flags		6
#define IconB_Data0		7
#define IconB_Data1		8
#define IconB_Data2		9


// Mouse click block (also: get pointer info):
#define MouseB_PosX		0
#define MouseB_PosY		1
#define MouseB_Buttons		2
#define MouseB_Window		3
#define MouseB_Icon		4


// Key pressed block
#define KeyPB_Window		0
#define KeyPB_Icon		1
#define KeyPB_PosX		2
#define KeyPB_PosY		3
#define KeyPB_CHeight		4
#define KeyPB_Index		5
#define KeyPB_Key		6


// Drag Block
#define DragB_Handle		0
#define DragB_Type		1
#define DragB_IMinX		2
#define DragB_IMinY		3
#define DragB_IMaxX		4
#define DragB_IMaxY		5
#define DragB_BBMinX		6
#define DragB_BBMinY		7
#define DragB_BBMaxX		8
#define DragB_BBMaxY		9
#define DragB_R12		10
#define DragB_DrawCode		11
#define DragB_RemoveCode	12
#define DragB_MoveCode		13


// Drag A Sprite Block
#define DASB_MinX		0
#define DASB_MinY		1
#define DASB_MaxX		2
#define DASB_MaxY		3





// Menu definitions
#define Menu_IBar		1
#define Menu_Emulator		2

#define Menu_Height		44
#define Menu_Flags		0x07003011

#define Menu_IBar_Items		5
#define Menu_IBar_Width		256
#define Menu_IBar_Info		0
#define Menu_IBar_Prefs		1
#define Menu_IBar_Config	2
#define Menu_IBar_Sound		3
#define Menu_IBar_Quit		4

#define Menu_EWind_Items	4
#define Menu_EWind_Width	200
#define Menu_EWind_Info		0
#define Menu_EWind_Sound	1
#define Menu_EWind_SaveRAM	2
#define Menu_EWind_Snapshot	3





// Icons used in window definitions:
#define Icon_Pane_LED0		1
#define Icon_Pane_LED1		3
#define Icon_Pane_LED2		5
#define Icon_Pane_LED3		7
#define Icon_Pane_Drive0	0
#define Icon_Pane_Drive1	2
#define Icon_Pane_Drive2	4
#define Icon_Pane_Drive3	6
#define Icon_Pane_Reset		8
#define Icon_Pane_Pause		9
#define Icon_Pane_Speed		10
#define Icon_Pane_Toggle	11

#define Icon_Prefs_Dr8DIR	6
#define Icon_Prefs_Dr8D64	7
#define Icon_Prefs_Dr8T64	8
#define Icon_Prefs_Dr8Path	9
#define Icon_Prefs_Dr9DIR	11
#define Icon_Prefs_Dr9D64	12
#define Icon_Prefs_Dr9T64	13
#define Icon_Prefs_Dr9Path	14
#define Icon_Prefs_Dr10DIR	16
#define Icon_Prefs_Dr10D64	17
#define Icon_Prefs_Dr10T64	18
#define Icon_Prefs_Dr10Path	19
#define Icon_Prefs_Dr11DIR	21
#define Icon_Prefs_Dr11D64	22
#define Icon_Prefs_Dr11T64	23
#define Icon_Prefs_Dr11Path	24
#define Icon_Prefs_Emul1541	25
#define Icon_Prefs_MapSlash	26
#define Icon_Prefs_SIDNone	29
#define Icon_Prefs_SIDDigi	30
#define Icon_Prefs_SIDCard	31
#define Icon_Prefs_SIDFilter	32
#define Icon_Prefs_REUNone	35
#define Icon_Prefs_REU128	36
#define Icon_Prefs_REU256	37
#define Icon_Prefs_REU512	38
#define Icon_Prefs_SkipFLeft	41
#define Icon_Prefs_SkipFRight	42
#define Icon_Prefs_SkipFText	43
#define Icon_Prefs_SprOn	47
#define Icon_Prefs_SprColl	48
#define Icon_Prefs_Joy1On	50
#define Icon_Prefs_Joy2On	51
#define Icon_Prefs_JoySwap	52
#define Icon_Prefs_LimSpeed	55
#define Icon_Prefs_FastReset	56
#define Icon_Prefs_CIAHack	57
#define Icon_Prefs_CycleNorm	64
#define Icon_Prefs_CycleBad	65
#define Icon_Prefs_CycleCIA	66
#define Icon_Prefs_CycleFloppy	67
#define Icon_Prefs_Cancel	68
#define Icon_Prefs_OK		69
#define Icon_Prefs_PrefPath	70
#define Icon_Prefs_Save		71
#define Icon_Prefs_PrefSprite	72
#define Icon_Prefs_XROMOn	75
#define Icon_Prefs_XROMPath	76

#define Icon_Conf_PollAfter	3
#define Icon_Conf_SpeedAfter	5
#define Icon_Conf_Joy1Up	15
#define Icon_Conf_Joy1Down	16
#define Icon_Conf_Joy1Left	17
#define Icon_Conf_Joy1Right	18
#define Icon_Conf_Joy1Fire	19
#define Icon_Conf_Joy2Up	27
#define Icon_Conf_Joy2Down	28
#define Icon_Conf_Joy2Left	29
#define Icon_Conf_Joy2Right	30
#define Icon_Conf_Joy2Fire	31
#define Icon_Conf_OK		32
#define Icon_Conf_Save		33
#define Icon_Conf_ConfPath	34
#define Icon_Conf_ConfSprite	35
#define Icon_Conf_SoundAfter	37

#define Icon_Info_Name		4
#define Icon_Info_Purpose	5
#define Icon_Info_Author	6
#define Icon_Info_AuthorPort	7
#define Icon_Info_Version	8

#define Icon_Sound_Volume	0
#define Icon_Sound_Notes	1

#define Icon_Save_Sprite	0
#define Icon_Save_Path		1
#define Icon_Save_OK		2




// Drag types
#define DRAG_PrefsSprite	1
#define DRAG_ConfSprite		2
#define DRAG_SaveSprite		3
#define DRAG_VolumeWell		16



// Save types
#define SAVE_RAM		1
#define SAVE_Snapshot		2




// variables

extern char LEDtoIcon[4];
extern char DriveToIcon[16];
extern char SIDtoIcon[3];
extern char REUtoIcon[4];





// Plotter structs and variables
typedef struct {
  int x, y, dimx, dimy;
} graph_env;

#define PLOTTER_ARGS	const graph_env *GraphEnv, const int *Clipwindow,\
			const uint8 *Bitmap, const unsigned int *TransTab

// Plotters provided in Plotters.s -- declare as C-functions !
extern "C"
{
extern void PlotZoom1(PLOTTER_ARGS);
extern void PlotZoom2(PLOTTER_ARGS);
}

#endif
