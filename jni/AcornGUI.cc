/*
 *  AcornGUI.cc - Frodo's Graphical User Interface for Acorn RISC OS machines
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

#include "sysdeps.h"

#include "C64.h"
#include "VIC.h"
#include "Display.h"
#include "Prefs.h"
#include "main.h"
#include "ROlib.h"
#include "AcornGUI.h"
#include "Version.h"



#define CUSTOM_SPRITES	"FrodoRsrc:Sprites"
#define TEMPLATES_FILE	"FrodoRsrc:Templates"
#define WIMP_SCRAP_FILE	"<Wimp$Scrap>"


#define FileType_Data		0xffd
#define FileType_Text		0xfff
#define FileType_C64File	0x064
#define FileType_D64File	0x164

#define IconSpriteSize		68	// OS units of an icon sprite (for dragging)
#define EstimatedPrefsSize	1024	// can't say how big it'll be before saving...
#define EstimatedConfSize	256
#define EstimatedRAMSize	0x10400
#define EstimatedSnapSize	0x11000




// For the scanning of the joystick keys in the SysConfig window
#define IntKey_ScanFrom		3
// This key gets translated to 0xff, i.e. none (joystick inactive)
#define IntKey_Void		44


#define Key_Return		13




// LED number to Icon number translation:
char LEDtoIcon[4] = {Icon_Pane_LED0, Icon_Pane_LED1, Icon_Pane_LED2, Icon_Pane_LED3};
// Drive number to Icon number (Pane) translation:
char PDrvToIcon[4] = {Icon_Pane_Drive0, Icon_Pane_Drive1, Icon_Pane_Drive2, Icon_Pane_Drive3};
// Drive number to Icon number (Prefs) translation:
#define IconsPerDrive	4
char DriveToIcon[16] = {
  Icon_Prefs_Dr8DIR,  Icon_Prefs_Dr8D64,  Icon_Prefs_Dr8T64,  Icon_Prefs_Dr8Path,
  Icon_Prefs_Dr9DIR,  Icon_Prefs_Dr9D64,  Icon_Prefs_Dr9T64,  Icon_Prefs_Dr9Path,
  Icon_Prefs_Dr10DIR, Icon_Prefs_Dr10D64, Icon_Prefs_Dr10T64, Icon_Prefs_Dr10Path,
  Icon_Prefs_Dr11DIR, Icon_Prefs_Dr11D64, Icon_Prefs_Dr11T64, Icon_Prefs_Dr11Path
};
// SID type to Icon number translation:
char SIDtoIcon[3] = {
  Icon_Prefs_SIDNone, Icon_Prefs_SIDDigi, Icon_Prefs_SIDCard
};
// REU type to Icon number translation:
char REUtoIcon[4] = {
  Icon_Prefs_REUNone, Icon_Prefs_REU128, Icon_Prefs_REU256, Icon_Prefs_REU512
};
char KJoyToIcon[2][5] = {
  {Icon_Conf_Joy1Up, Icon_Conf_Joy1Down, Icon_Conf_Joy1Left, Icon_Conf_Joy1Right, Icon_Conf_Joy1Fire},
  {Icon_Conf_Joy2Up, Icon_Conf_Joy2Down, Icon_Conf_Joy2Left, Icon_Conf_Joy2Right, Icon_Conf_Joy2Fire}
};





// Internal keynumbers lookup table
// Single characters are represented by single chars, others by pointers to strings
int IntKeyTable[128] = {
  (int)"Shft", (int)"Ctrl", (int)"Alt", (int)"ShftL",
  (int)"CtrlL", (int)"AltL", (int)"ShftR", (int)"CtrlR",
  (int)"AltR", (int)"Slct", (int)"Menu", (int)"Adjst",
  0, 0, 0, 0,
  'q', '3', '4', '5',
  (int)"F4", '8', (int)"F7", '-',
  '6', (int)"Left", (int)"num6", (int)"num7",
  (int)"F11", (int)"F12", (int)"F10", (int)"ScLck",
  (int)"Prnt", 'w', 'e', 't',
  '7', 'i', '9', '0',
  '-', (int)"Down", (int)"num8", (int)"num9",
  (int)"Brk", '`', '£', (int)"Del",
  '1', '2', 'd', 'r',
  '6', 'u', 'o', 'p',
  '[', (int)"Up", (int)"num+", (int)"num-",
  (int)"nmEnt", (int)"Isrt", (int)"Home", (int)"PgUp",
  (int)"CpLck", 'a', 'x', 'f',
  'y', 'j', 'k', '2',
  ';', (int)"Ret", (int)"num/", 0,
  (int)"num.", (int)"nmLck", (int)"PgDwn", '\'',
  0, 's', 'c', 'g',
  'h', 'n', 'l', ';',
  ']', (int)"Del", (int)"num#", (int)"num*",
  0, '=', (int)"Extra", 0,
  (int)"Tab", 'z', (int)"Space", 'v',
  'b', 'm', ',', '.',
  '/', (int)"Copy", (int)"num0", (int)"num1",
  (int)"num3", 0, 0, 0,
  (int)"Esc", (int)"F1", (int)"F2", (int)"F3",
  (int)"F5", (int)"F6", (int)"F8", (int)"F9",
  '\\', (int)"Right", (int)"num4", (int)"num5",
  (int)"num2", 0, 0, 0
};





// The icon bar icon
RO_IconDesc IBarIcon = {
  -1, 0, 0, 68, 68, 0x301a,
#ifdef FRODO_SC
  "!frodosc"
#else
# ifdef FRODO_PC
  "!frodopc"
# else
  "!frodo"
# endif
#endif
};





// The menus

char *MIBTextPrefs = "Preferences...";
char *MIBTextConf = "Configuration...";


struct MenuIconBar {
  RO_MenuHead head;
  RO_MenuItem item[Menu_IBar_Items];
} MenuIconBar = {
  {
#ifdef FRODO_SC
  "FrodoSC",
#else
# ifdef FRODO_PC
  "FrodoPC",
# else
  "Frodo",
# endif
#endif
  7, 2, 7, 0, Menu_IBar_Width, Menu_Height, 0},
  {
    {0, (RO_MenuHead*)-1, Menu_Flags, "Info"},
    {0, (RO_MenuHead*)-1, Menu_Flags + IFlg_Indir, ""},
    {0, (RO_MenuHead*)-1, Menu_Flags + IFlg_Indir, ""},
    {0, (RO_MenuHead*)-1, Menu_Flags, "Sound"},
    {MFlg_LastItem, (RO_MenuHead*)-1, Menu_Flags, "Quit"}
  }
};

struct MenuEmuWindow {
  RO_MenuHead head;
  RO_MenuItem item[Menu_EWind_Items];
} MenuEmuWindow = {
  {
#ifdef FRODO_SC
    "FrodoSC Job",
#else
# ifdef FRODO_PC
    "FrodoPC Job",
# else
    "Frodo Job",
# endif
#endif
    7, 2, 7, 0, Menu_EWind_Width, Menu_Height, 0},
  {
    {0, (RO_MenuHead*)-1, Menu_Flags, "Info"},
    {0, (RO_MenuHead*)-1, Menu_Flags, "Sound"},
    {MFlg_Warning, (RO_MenuHead*)-1, Menu_Flags, "Save RAM"},
    {MFlg_LastItem | MFlg_Warning, (RO_MenuHead*)-1, Menu_Flags, "Snapshot"}
  }
};




// For less writing in LoadATemplate...
#define wic(base,no,off) base[RO_WINDOW_WORDS + no*RO_ICON_WORDS + off]

// For less writing in WindowToThePrefs...
#define pread_opt(var,icn)	PrefsWindow->GetIconState(Icon_Prefs_##icn,AuxBlock);\
				prefs->##var = ((AuxBlock[IconB_Flags] & IFlg_Slct) == 0) ? false : true;



//
// WIMP member-functions
//

WIMP::WIMP(C64 *my_c64)
{
  int x,y,i;
  WIdata *wid;

  CMOS_DragType = ReadDragType();

  // Determine the dimensions of the icon bar sprite first.
  if (Wimp_SpriteInfo((char*)(&IBarIcon.dat),&x,&y,&i) == NULL)
  {
    IBarIcon.maxx = x << OS_ReadModeVariable(i,4);
    IBarIcon.maxy = y << OS_ReadModeVariable(i,5);
  }

  IBicon = new Icon(0,&IBarIcon); Mask = 0x00000830;
  LastMenu = LastClick = LastDrag = MenuType = DragType = 0;
  UseScrap = false; EmuPaused = false; UseNULL = 0;	// one NULL client - the emulator
  the_c64 = my_c64;
  EmuZoom = 1;

  // Read the customized sprite area (LEDs etc)
  if ((ReadCatalogueInfo(CUSTOM_SPRITES,AuxBlock) & 1) != 0)	// (image) file
  {
    int i;
    FILE *fp;

    i = AuxBlock[2]; SpriteArea = (int*)(new char[i+16]);	// allocate space for sprite
    fp = fopen(CUSTOM_SPRITES,"rb");
    i = fread(SpriteArea+1,1,i,fp); SpriteArea[0] = i+4;
    fclose(fp);
  }
  else
  {
    WimpError.errnum = 0; sprintf(WimpError.errmess,"Can't read sprites!!!");
    Wimp_ReportError(&WimpError,1,TASKNAME);
  }

  // Read Template and create windows
  if (Wimp_OpenTemplate(TEMPLATES_FILE) == NULL)
  {
    // Load Pane first
    LoadATemplate("EmuPane",&EmuPane);
    LoadATemplate("EmuWindow",&EmuWindow);
    LoadATemplate("ThePrefs",&PrefsWindow);
    LoadATemplate("ConfigWind",&ConfigWindow);
    LoadATemplate("InfoWindow",&InfoWindow);
    LoadATemplate("SoundWindow",&SoundWindow);
    LoadATemplate("SaveBox",&SaveBox);
  }
  else
  {
    Wimp_ReportError(&WimpError,1,TASKNAME);
    WimpError.errnum = 0; sprintf(WimpError.errmess,"Error in Templates!!!");
  }
  Wimp_CloseTemplate();

  // Add info window to icon bar menu
  MenuIconBar.item[Menu_IBar_Info].submenu  =
  MenuEmuWindow.item[Menu_EWind_Info].submenu = (RO_MenuHead*)InfoWindow->MyHandle();
  MenuIconBar.item[Menu_IBar_Sound].submenu =
  MenuEmuWindow.item[Menu_EWind_Sound].submenu = (RO_MenuHead*)SoundWindow->MyHandle();
  MenuEmuWindow.item[Menu_EWind_SaveRAM].submenu =
  MenuEmuWindow.item[Menu_EWind_Snapshot].submenu = (RO_MenuHead*)SaveBox->MyHandle();
  // I couldn't find ONE FUCKING WAY to do this in the initialisation directly and I'm
  // REALLY PISSED OFF!
  wid = &(MenuIconBar.item[Menu_IBar_Prefs].dat);
  wid->ind.tit = (int*)MIBTextPrefs; wid->ind.val = NULL; wid->ind.len = sizeof(MIBTextPrefs);
  wid = &(MenuIconBar.item[Menu_IBar_Config].dat);
  wid->ind.tit = (int*)MIBTextConf; wid->ind.val = NULL; wid->ind.len = sizeof(MIBTextConf);

  // Write default path into config window
  ConfigWindow->WriteIconText(Icon_Conf_ConfPath,DEFAULT_SYSCONF);

  // Set up the contents of the prefs window and init with the default prefs path
  ThePrefsToWindow();
  PrefsWindow->WriteIconText(Icon_Prefs_PrefPath,DEFAULT_PREFS);
  // Grey out SID card icon -- not supported!
  PrefsWindow->SetIconState(Icon_Prefs_SIDCard,IFlg_Grey,IFlg_Grey);

  // Open Emulator window + pane in the center of the screen and give it the input focus
  Wimp_GetCaretPosition(&LastCaret);
  OpenEmuWindow();
  Wimp_SetCaretPosition(EmuWindow->MyHandle(),-1,-100,100,-1,-1); // emu window gets input focus

  // Init export files
  sprintf(RAMFile+44,"C64_RAM"); sprintf(SnapFile+44,"FrodoSnap");
}


WIMP::~WIMP(void)
{
  delete IBicon;
  delete EmuWindow; delete EmuPane; delete PrefsWindow; delete ConfigWindow;
  delete InfoWindow; delete SoundWindow; delete SaveBox;
}


bool WIMP::LoadATemplate(char *Name, Window **Which)
{
  char *Buffer, *Indirect, *Desc;
  int IndSize, Pos;
  char TempName[12];

  strncpy((char*)TempName,Name,12);
  Buffer = NULL; Pos = 0;
  if (Wimp_LoadTemplate(&Buffer,&Indirect,0,(char*)-1,TempName,&Pos) != NULL) {return(false);}
  Buffer = new char[(int)Buffer + 4];
  IndSize = (int)Indirect; Indirect = new char[IndSize];
  Pos = 0; Desc = Buffer+4;
  Wimp_LoadTemplate(&Desc,&Indirect,Indirect+IndSize,(char*)-1,TempName,&Pos);
  if (Which == &EmuWindow)
  {
    int eigen, dx, dy;
    RO_Window *row = (RO_Window*)Buffer;
    RORes res;

    // Center emulator window on screen
    eigen = (res.eigx < res.eigy) ? res.eigx : res.eigy;
    dx = (DISPLAY_X << eigen); dy = (DISPLAY_Y << eigen);
    row->vminx = res.resx/2 - dx/2; row->vmaxx = row->vminx + dx;
    row->vminy = res.resy/2 - dy/2; row->vmaxy = row->vminy + dy;
    row->wminy = -dy; row->wmaxx = dx;
    Indirect = (char*)VERSION_STRING;
  }
  else
  {
    if (Which == &EmuPane)	// EmuPane needs customized sprites
    {
      register RO_Window *row = (RO_Window*)Buffer;
      register int *b = (int*)Buffer;

      // Insert sprite pointer in window and icon definitions
      row->SpriteAreaPtr = (int)SpriteArea;
      wic(b,Icon_Pane_LED0,RawIB_Data1) = wic(b,Icon_Pane_LED1,RawIB_Data1) =
      wic(b,Icon_Pane_LED2,RawIB_Data1) = wic(b,Icon_Pane_LED3,RawIB_Data1) = (int)SpriteArea;
      sprintf((char*)wic(b,Icon_Pane_Pause,RawIB_Data0),PANE_TEXT_PAUSE);
      sprintf((char*)wic(b,Icon_Pane_Toggle,RawIB_Data0),PANE_TEXT_ZOOM2);
    }
    else if (Which == &SoundWindow)	// ditto
    {
      register RO_Window *row = (RO_Window*)Buffer;
      register int *b = (int*)Buffer;
      register int orr;

      row->SpriteAreaPtr = (int)SpriteArea;
      // if sound emulation off grey out notes
      if (ThePrefs.SIDType == SIDTYPE_NONE) {orr = IFlg_Grey;} else {orr = 0;}
      wic(b,Icon_Sound_Notes,RawIB_Flags)=(wic(b,Icon_Sound_Notes,RawIB_Flags)&~IFlg_Grey)|orr;
    }
    else if (Which == &InfoWindow)	// ditto
    {
      register RO_Window *row = (RO_Window*)Buffer;

      row->SpriteAreaPtr = (int)SpriteArea;
    }
    Indirect = NULL;
  }
  *Which = new Window((int*)Buffer,Indirect);
  return(true);
}


// All changes in position of the Emulator window have to be treated seperately
// to keep the pane attached to it at all times!
void WIMP::OpenEmuWindow(void)
{
  RO_Window *wind;
  int work[WindowB_WFlags], i;

  wind = EmuWindow->Descriptor();
  EmuPane->GetWorkArea(&work[WindowB_VMinX]); i = work[WindowB_VMaxX] - work[WindowB_VMinX];
  if ((work[WindowB_VMinX] = wind->vminx - EmuPaneSpace - i) < 0)
  {
    // if main window still on screen then keep pane on screen as well
    if (wind->vminx >= 0) {work[WindowB_VMinX] = 0;}
    // otherwise align pane and main window on the left
    else {work[WindowB_VMinX] = wind->vminx;}
  }
  work[WindowB_VMaxX] = work[WindowB_VMinX] + i;
  work[WindowB_VMinY] = wind->vmaxy - (work[WindowB_VMaxY]-work[WindowB_VMinY]);
  work[WindowB_VMaxY] = wind->vmaxy;
  work[WindowB_Handle] = EmuPane->MyHandle();
  work[WindowB_ScrollX] = work[WindowB_ScrollY] = 0;
  work[WindowB_Stackpos] = -1;	// open on top of stack
  // open the pane first
  EmuPane->open(work);
  wind->stackpos = EmuPane->MyHandle();	// open window behind pane.
  EmuWindow->open();
}


void WIMP::OpenEmuWindow(int *Where)
{
  int work[WindowB_WFlags], i;

  EmuPane->GetWorkArea(&work[WindowB_VMinX]); i = work[WindowB_VMaxX] - work[WindowB_VMinX];
  if ((work[WindowB_VMinX] = Where[WindowB_VMinX] - EmuPaneSpace - i) < 0)
  {
    if (Where[WindowB_VMinX] >= 0) {work[WindowB_VMinX] = 0;}
    else {work[WindowB_VMinX] = Where[WindowB_VMinX];}
  }
  work[WindowB_VMaxX] = work[WindowB_VMinX] + i;
  work[WindowB_VMinY] = Where[WindowB_VMaxY] - (work[WindowB_VMaxY]-work[WindowB_VMinY]);
  work[WindowB_VMaxY] = Where[WindowB_VMaxY];
  work[WindowB_Handle] = EmuPane->MyHandle();
  work[WindowB_ScrollX] = work[WindowB_ScrollY] = 0;
  work[WindowB_Stackpos] = Where[WindowB_Stackpos];
  // open the pane first
  EmuPane->open(work);
  Where[WindowB_Stackpos] = EmuPane->MyHandle();	// open window behind pane
  EmuWindow->open(Where);
}


void WIMP::CloseEmuWindow(void)
{
  EmuWindow->close(); EmuPane->close();
}


void WIMP::UpdateEmuWindow(void)
{
  C64Display *disp = the_c64->TheDisplay;

  EmuWindow->update(disp->BitmapBase(),disp);
}


// Write the values given in ThePrefs into the Prefs Window
void WIMP::ThePrefsToWindow(void)
{
  int i, j, k;

  // Update the data for each of the drives
  for (i=0; i<4; i++)
  {
    switch(ThePrefs.DriveType[i])
    {
      case DRVTYPE_D64: j=1; break;
      case DRVTYPE_T64: j=2; break;
      default: j=0; break;	// otherwise type 0 = DIR
    }
    // unselect all but other icons
    for (k=0; k<3; k++)
    {
      if (k != j) {PrefsWindow->SetIconState(DriveToIcon[i*IconsPerDrive + k],0,IFlg_Slct);}
    }
    // and select the correct one.
    PrefsWindow->SetIconState(DriveToIcon[i*IconsPerDrive + j],IFlg_Slct,IFlg_Slct);
    // Now update the path name (buffer won't change, i.e. use original data)
    PrefsWindow->WriteIconText(DriveToIcon[i*IconsPerDrive + 3],ThePrefs.DrivePath[i]);
  }
  if (ThePrefs.Emul1541Proc) {i = IFlg_Slct;} else {i = 0;}
  PrefsWindow->SetIconState(Icon_Prefs_Emul1541,i,IFlg_Slct);
  SetLEDIcons(ThePrefs.Emul1541Proc);
  if (ThePrefs.MapSlash) {i = IFlg_Slct;} else {i = 0;}
  PrefsWindow->SetIconState(Icon_Prefs_MapSlash,i,IFlg_Slct);

  // SID prefs
  switch (ThePrefs.SIDType)
  {
    case SIDTYPE_DIGITAL: i = 1; break;
    case SIDTYPE_SIDCARD: i = 2; break;
    default: i = 0; break;	// includes NONE
  }
  for (j=0; j<3; j++)
  {
    if (j != i) {PrefsWindow->SetIconState(SIDtoIcon[j],0,IFlg_Slct);}
  }
  PrefsWindow->SetIconState(SIDtoIcon[i],IFlg_Slct,IFlg_Slct);
  PrefsWindow->SetIconState(Icon_Prefs_SIDFilter,(ThePrefs.SIDFilters)?IFlg_Slct:0,IFlg_Slct);

  // REU prefs
  switch (ThePrefs.REUSize)
  {
    case REU_128K: i=1; break;
    case REU_256K: i=2; break;
    case REU_512K: i=3; break;
    default: i=0; break;
  }
  for (j=0; j<4; j++)
  {
    if (j != i) {PrefsWindow->SetIconState(REUtoIcon[j],0,IFlg_Slct);}
  }
  PrefsWindow->SetIconState(REUtoIcon[i],IFlg_Slct,IFlg_Slct);

  // Skip Frames
  PrefsWindow->WriteIconNumber(Icon_Prefs_SkipFText,ThePrefs.SkipFrames);

  // Sprites
  PrefsWindow->SetIconState(Icon_Prefs_SprOn,(ThePrefs.SpritesOn)?IFlg_Slct:0,IFlg_Slct);
  PrefsWindow->SetIconState(Icon_Prefs_SprColl,(ThePrefs.SpriteCollisions)?IFlg_Slct:0,IFlg_Slct);
  // Joystick
  PrefsWindow->SetIconState(Icon_Prefs_Joy1On,(ThePrefs.Joystick1On)?IFlg_Slct:0,IFlg_Slct);
  PrefsWindow->SetIconState(Icon_Prefs_Joy2On,(ThePrefs.Joystick2On)?IFlg_Slct:0,IFlg_Slct);
  PrefsWindow->SetIconState(Icon_Prefs_JoySwap,(ThePrefs.JoystickSwap)?IFlg_Slct:0,IFlg_Slct);

  // Misc
  SetSpeedLimiter(ThePrefs.LimitSpeed);
  PrefsWindow->SetIconState(Icon_Prefs_FastReset,(ThePrefs.FastReset)?IFlg_Slct:0,IFlg_Slct);
  PrefsWindow->SetIconState(Icon_Prefs_CIAHack,(ThePrefs.CIAIRQHack)?IFlg_Slct:0,IFlg_Slct);

  // Cycles
  PrefsWindow->WriteIconNumber(Icon_Prefs_CycleNorm,ThePrefs.NormalCycles);
  PrefsWindow->WriteIconNumber(Icon_Prefs_CycleBad,ThePrefs.BadLineCycles);
  PrefsWindow->WriteIconNumber(Icon_Prefs_CycleCIA,ThePrefs.CIACycles);
  PrefsWindow->WriteIconNumber(Icon_Prefs_CycleFloppy,ThePrefs.FloppyCycles);

#ifdef SUPPORT_XROM
  // XROM
  PrefsWindow->SetIconState(Icon_Prefs_XROMOn,(ThePrefs.XPandROMOn)?IFlg_Slct:0,IFlg_Slct);
  PrefsWindow->WriteIconText(Icon_Prefs_XROMPath,ThePrefs.XPandROMFile);
#endif
}


// Update ThePrefs according to the values given in the Prefs Window
void WIMP::WindowToThePrefs(void)
{
  int i, j, k;
  Prefs *prefs = new Prefs(ThePrefs);

  for (i=0; i<4; i++)
  {
    // find out which of the drive type icons is selected
    j = -1;
    do
    {
      ++j; PrefsWindow->GetIconState(DriveToIcon[i*IconsPerDrive + j],AuxBlock);
    }
    while ((j < 3) && ((AuxBlock[IconB_Flags] & IFlg_Slct) == 0));
    switch (j)
    {
      case 1:  prefs->DriveType[i] = DRVTYPE_D64; break;
      case 2:  prefs->DriveType[i] = DRVTYPE_T64; break;
      default: prefs->DriveType[i] = DRVTYPE_DIR; break;
    }
    strcpy(prefs->DrivePath[i],PrefsWindow->ReadIconText(DriveToIcon[i*IconsPerDrive + 3]));
  }
  // Emulation of the 1541 processor is a special case as it also affects LED1-3
  pread_opt(Emul1541Proc,Emul1541);
  SetLEDIcons(prefs->Emul1541Proc);
  pread_opt(MapSlash,MapSlash);

  // SID
  j = -1;
  do
  {
    ++j; PrefsWindow->GetIconState(SIDtoIcon[j],AuxBlock);
  }
  while ((j < 3) && ((AuxBlock[IconB_Flags] & IFlg_Slct) == 0));
  switch (j)
  {
    case 1:  prefs->SIDType = SIDTYPE_DIGITAL; break;
    case 2:  prefs->SIDType = SIDTYPE_SIDCARD; break;
    default: prefs->SIDType = SIDTYPE_NONE; break;
  }
  pread_opt(SIDFilters,SIDFilter);
  SoundWindow->SetIconState(Icon_Sound_Notes,(prefs->SIDType==SIDTYPE_NONE)?IFlg_Grey:0,IFlg_Grey);

  // REU
  j = -1;
  do
  {
    ++j; PrefsWindow->GetIconState(REUtoIcon[j],AuxBlock);
  }
  while ((j < 4) && ((AuxBlock[IconB_Flags] & IFlg_Slct) == 0));
  switch (j)
  {
    case 1:  prefs->REUSize = REU_128K; break;
    case 2:  prefs->REUSize = REU_256K; break;
    case 3:  prefs->REUSize = REU_512K; break;
    default: prefs->REUSize = REU_NONE; break;
  }

  // Skip Frames
  prefs->SkipFrames = PrefsWindow->ReadIconNumber(Icon_Prefs_SkipFText);

  // Sprites
  pread_opt(SpritesOn,SprOn); pread_opt(SpriteCollisions,SprColl);

  // Joystick
  pread_opt(Joystick1On,Joy1On); pread_opt(Joystick2On,Joy2On); pread_opt(JoystickSwap,JoySwap);

  // Misc
  pread_opt(LimitSpeed,LimSpeed); pread_opt(FastReset,FastReset); pread_opt(CIAIRQHack,CIAHack);

  // Cycles
  prefs->NormalCycles	= PrefsWindow->ReadIconNumber(Icon_Prefs_CycleNorm);
  prefs->BadLineCycles	= PrefsWindow->ReadIconNumber(Icon_Prefs_CycleBad);
  prefs->CIACycles	= PrefsWindow->ReadIconNumber(Icon_Prefs_CycleCIA);
  prefs->FloppyCycles	= PrefsWindow->ReadIconNumber(Icon_Prefs_CycleFloppy);

#ifdef SUPPORT_XROM
  // XROM
  pread_opt(XPandROMOn,XROMOn);
  strcpy(prefs->XPandROMFile,PrefsWindow->ReadIconText(Icon_Prefs_XROMPath));
#endif

  // Finally make the changes known to the system:
  the_c64->NewPrefs(prefs);
  ThePrefs = *prefs;
  delete prefs;
}


// Update the SysConfig window according to the values used
void WIMP::SysConfToWindow(void)
{
  int i, j, k;
  Joy_Keys *jk;
  char OneChar[2], *b;

  OneChar[1] = 0;

  // Write timings
  the_c64->ReadTimings(&i,&j,&k);
  ConfigWindow->WriteIconNumber(Icon_Conf_PollAfter, i);
  ConfigWindow->WriteIconNumber(Icon_Conf_SpeedAfter, j);
  ConfigWindow->WriteIconNumber(Icon_Conf_SoundAfter, k);

  // Write joystick keys
  for (i=0; i<2; i++)
  {
    jk = &(the_c64->TheDisplay->JoystickKeys[i]); NewJoyKeys[i] = *jk;
    if ((j = jk->up) >= 128) {j = 127;}
    j = IntKeyTable[j]; if (j < 256) {OneChar[0] = j; b = OneChar;} else {b = (char*)j;}
    ConfigWindow->WriteIconText(KJoyToIcon[i][0], b);
    if ((j = jk->down) >= 128) {j = 127;}
    j = IntKeyTable[j]; if (j < 256) {OneChar[0] = j; b = OneChar;} else {b = (char*)j;}
    ConfigWindow->WriteIconText(KJoyToIcon[i][1], b);
    if ((j = jk->left) >= 128) {j = 127;}
    j = IntKeyTable[j]; if (j < 256) {OneChar[0] = j; b = OneChar;} else {b = (char*)j;}
    ConfigWindow->WriteIconText(KJoyToIcon[i][2], b);
    if ((j = jk->right) >= 128) {j = 127;}
    j = IntKeyTable[j]; if (j < 256) {OneChar[0] = j; b = OneChar;} else {b = (char*)j;}
    ConfigWindow->WriteIconText(KJoyToIcon[i][3], b);
    if ((j = jk->fire) >= 128) {j = 127;}
    j = IntKeyTable[j]; if (j < 256) {OneChar[0] = j; b = OneChar;} else {b = (char*)j;}
    ConfigWindow->WriteIconText(KJoyToIcon[i][4], b);
  }
}


// Update SysConfig according to the values in the window
void WIMP::WindowToSysConf(void)
{
  int i, j, k;
  Joy_Keys *jk;

  // Read timings
  i = ConfigWindow->ReadIconNumber(Icon_Conf_PollAfter);
  j = ConfigWindow->ReadIconNumber(Icon_Conf_SpeedAfter);
  k = ConfigWindow->ReadIconNumber(Icon_Conf_SoundAfter);
  the_c64->WriteTimings(i,j,k);

  // Read joystick keys
  for (i=0; i<2; i++)
  {
    jk = &(the_c64->TheDisplay->JoystickKeys[i]); *jk = NewJoyKeys[i];
  }
}


// Low-level keyboard scan in SysConfig Window
void WIMP::PollSysConfWindow(void)
{
  Wimp_GetPointerInfo(Block);
  if (Block[MouseB_Window] == ConfigWindow->MyHandle())
  {
    int i, j;

    for (i=0; i<2; i++)
    {
      for (j=0; j<5; j++)
      {
        if (Block[MouseB_Icon] == KJoyToIcon[i][j])
        {
          int key;

          // Gain caret (to window, but none of its icons!)
          Wimp_GetCaretPosition(&LastCaret);
          Wimp_SetCaretPosition(ConfigWindow->MyHandle(),-1,0,0,-1,-1);
          if ((key = ScanKeys(IntKey_ScanFrom)) != 0xff)
          {
            char OneChar[2], *b;

            if (key == IntKey_Void) {key = 0xff;}
            switch (j)
            {
              case 0: NewJoyKeys[i].up = key; break;
              case 1: NewJoyKeys[i].down = key; break;
              case 2: NewJoyKeys[i].left = key; break;
              case 3: NewJoyKeys[i].right = key; break;
              case 4: NewJoyKeys[i].fire = key; break;
            }
            if (key >= 128) {key = 127;}
            key = IntKeyTable[key];
            if (key < 256) {OneChar[0]=key; OneChar[1]=0; b=OneChar;} else {b=(char*)key;}
            ConfigWindow->WriteIconText(KJoyToIcon[i][j], b);
          }
        }
      }
    }
  }
}


// Start a drag operation on the icon <number> in the window <host>
void WIMP::DragIconSprite(Window *host, unsigned int number)
{
  host->GetIconState(number,AuxBlock);
  if ((AuxBlock[IconB_Flags] & IFlg_Sprite) != 0)	// it needs to have a sprite, of course
  {
    char spritename[16] = "\0";

    if ((AuxBlock[IconB_Flags] & IFlg_Indir) == 0)	// not indirected
    {
      strncpy(spritename,((char*)AuxBlock+IconB_Data0),15);
    }
    else
    {
      if ((AuxBlock[IconB_Flags] & IFlg_Text) == 0)
      {
        strncpy(spritename,(char*)AuxBlock[IconB_Data0],15);
      }
      else	// this necessitates parsing the validation string
      {
        register char *b, *d, *s, c;

        s = spritename;
        if ((b = (char*)AuxBlock[IconB_Data1]) != NULL)	// pointer to val str
        {
          do
          {
            c = *b++;
            if ((c == 's') || (c == 'S'))	// sprite command
            {
              c = *b++; while ((c != ';') && (c >= 32)) {*s++ = c; c = *b++;}
              c = 0; *s++ = c;	// we can stop now
            }
            else if (c >= 32)	// some other command ==> skip to next.
            {
              c = *b++;
              while ((c != ';') && (c >= 32)) {if (c == '\\') {b++;} c = *b++;}
            }
          }
          while (c >= 32);
        }
      }
    }
    // we should now have the spritename
    if (spritename[0] != '\0')
    {
      LastDrag = host->MyHandle(); LastIcon = number;
      if (CMOS_DragType == 0)	// Drag outline
      {
        ROScreen *screen = the_c64->TheDisplay->screen;

        AuxBlock[DragB_Handle] = LastDrag; AuxBlock[DragB_Type] = 5;
        Wimp_GetPointerInfo(AuxBlock + DragB_IMinX);
        // Drag fixed sized box of this size:
        AuxBlock[DragB_IMinX] -= IconSpriteSize/2;
        AuxBlock[DragB_IMaxX] = AuxBlock[DragB_IMinX] + IconSpriteSize;
        AuxBlock[DragB_IMinY] -= IconSpriteSize/2;
        AuxBlock[DragB_IMaxY] = AuxBlock[DragB_IMinY] + IconSpriteSize;
        // Parent box is whole screen
        AuxBlock[DragB_BBMinX] = AuxBlock[DragB_BBMinY] = 0;
        AuxBlock[DragB_BBMaxX] = screen->resx; AuxBlock[DragB_BBMaxY] = screen->resy;
        Wimp_DragBox(AuxBlock);
      }
      else	// DragASprite
      {
        Wimp_GetPointerInfo(AuxBlock);
        AuxBlock[DASB_MinX] -= IconSpriteSize/2;
        AuxBlock[DASB_MaxX] = AuxBlock[DASB_MinX] + IconSpriteSize;
        AuxBlock[DASB_MinY] -= IconSpriteSize/2;
        AuxBlock[DASB_MaxY] = AuxBlock[DASB_MinY] + IconSpriteSize;
        DragASprite_Start(0xc5,1,spritename,AuxBlock,NULL);
      }
    }
  }
}


// Blk is a block as in MouseClick or PointerInfo
int WIMP::CalculateVolume(int *Blk)
{
  int orgx, vol;

  SoundWindow->getstate(AuxBlock);
  orgx = AuxBlock[WindowB_VMinX] - AuxBlock[WindowB_ScrollX];
  SoundWindow->GetIconState(Icon_Sound_Volume, AuxBlock);
  vol = (MaximumVolume*((Blk[MouseB_PosX] - orgx) - AuxBlock[IconB_MinX] - WellBorder)) / (AuxBlock[IconB_MaxX] - AuxBlock[IconB_MinX] - 2*WellBorder);
  if (vol < 0) {vol = 0;} if (vol > MaximumVolume) {vol = MaximumVolume;}
  return(vol);
}


// Check whether a filename contains a full pathname and reports an error if not.
int WIMP::CheckFilename(char *name)
{
  bool OK = false;
  register char *b, *d, c;

  b = d = name; c = *b++;
  while (c > 32)
  {
    // valid path must contain '$' or ':'
    if ((c == '$') || (c == ':')) {OK = true; d = b;}
    if (c == '.') {d = b;}
    c = *b++;
  }
  if ((b - d) == 1) {OK = false;}	// filename mustn't end with '$', ':' or '.'
  if (OK) {return(0);}

  WimpError.errnum = 0; sprintf(WimpError.errmess,"Bad filename %s",name);
  Wimp_ReportError(&WimpError,1,TASKNAME);
  return(-1);
}


void WIMP::SnapshotSaved(bool OK)
{
  if (OK)
  {
    int *b = (int*)SnapFile;

    if (b[MsgB_Sender] != 0)
    {
      b[MsgB_YourRef] = b[MsgB_MyRef]; b[MsgB_Action] = Message_DataLoad;
      Wimp_SendMessage(18,b,b[MsgB_Sender],b[6]);
    }
    else {Wimp_CreateMenu((int*)-1,0,0);}
    SaveType = 0;
  }
}


void WIMP::IssueSnapshotRequest(void)
{
  if (EmuPaused)
  {
    EmuPane->WriteIconTextU(Icon_Pane_Pause, PANE_TEXT_PAUSE);
    EmuPaused = false;
  }
  the_c64->RequestSnapshot();
}


// Sets the Emu pane's LEDs according to the floppy emulation state
void WIMP::SetLEDIcons(bool FloppyEmulation)
{
  int i, eor;

  if (FloppyEmulation) {eor = IFlg_Grey;} else {eor = 0;}
  for (i=1; i<4; i++)
  {
    EmuPane->SetIconState(LEDtoIcon[i],eor,IFlg_Grey);
  }
}



// Doesn't open window, just resizes...
void WIMP::SetEmuWindowSize(void)
{
  register int i;
  register C64Display *disp = the_c64->TheDisplay;

  i = (disp->screen->eigx < disp->screen->eigy) ? disp->screen->eigx : disp->screen->eigy;
  if (EmuZoom == 2) {i++;}
  EmuWindow->extent(0,-(DISPLAY_Y << i),(DISPLAY_X << i),0);
}



// switch between zoom 1 and zoom 2
void WIMP::ToggleEmuWindowSize(void)
{
  int x,y;

  // Icon in pane shows _alternative_ zoom mode
  if (EmuZoom == 1)
  {
    EmuZoom = 2;
    EmuPane->WriteIconText(Icon_Pane_Toggle,"1 x");
  }
  else
  {
    EmuZoom = 1;
    EmuPane->WriteIconText(Icon_Pane_Toggle,"2 x");
  }
  SetEmuWindowSize();
  EmuWindow->GetWorkArea(AuxBlock);
  x = AuxBlock[2] - AuxBlock[0]; y = AuxBlock[3] - AuxBlock[1];
  EmuWindow->getstate(AuxBlock);
  AuxBlock[WindowB_VMaxX] = AuxBlock[WindowB_VMinX] + x;
  AuxBlock[WindowB_VMinY] = AuxBlock[WindowB_VMaxY] - y;
  // Open emu window alone to get the dimensions set by the WIMP
  Wimp_OpenWindow(AuxBlock);
  // Then open with the pane at the correct position
  EmuWindow->getstate(AuxBlock); OpenEmuWindow(AuxBlock); UpdateEmuWindow();
}



int WIMP::ReadEmuWindowSize(void)
{
  return EmuZoom;
}



// Set a new drive path for drive DrNum. MsgBlock is the DataLoad MessageBlock.
void WIMP::NewDriveImage(int DrNum, int *MsgBlock, bool SetNow)
{
  int type, j = -1, map = -1;

  // determine currently selected type
  do
  {
    ++j; PrefsWindow->GetIconState(DriveToIcon[DrNum*IconsPerDrive + j], AuxBlock);
  }
  while ((j < 3) && ((AuxBlock[6] & IFlg_Slct) == 0));
  if (j >= 3) {j = 0;}
  // Check the type and set the drive type accordingly
  type = ReadCatalogueInfo(((char*)Block)+44,AuxBlock);
  // New path is directory but DRVTYPE != DIR ==> set DIR
  if ((type == 2) && (j != 0)) {map = 0;}
  // New path is not directory/image but DRVTYPE == DIR ==> remap to D64
  if (((type & 2) == 0) && (j == 0)) {map = 1;}
  // Filetype indicated D64 image?
  if (((type = AuxBlock[0]) & 0xfff00000) == 0xfff00000)	// typed file
  {
    type = (type >> 8) & 0xfff;
    // D64 image can also be used in DIR mode (-> D64ImageFS). Only remap from T64!
    if ((type == FileType_D64File) && (j == 2)) {map = 1;}
  }
  // Do we need to remap?
  if (map >= 0)
  {
    PrefsWindow->SetIconState(DriveToIcon[DrNum*IconsPerDrive+j],0,IFlg_Slct);
    PrefsWindow->SetIconState(DriveToIcon[DrNum*IconsPerDrive+map],IFlg_Slct,IFlg_Slct);
    j = map;
  }
  // j is the number of the emulation mode (DIR (0), D64 (1), T64 (2))
  PrefsWindow->WriteIconText(DriveToIcon[DrNum*IconsPerDrive+3],((char*)Block)+44);
  // Send acknowledge message
  Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataLoadAck;
  Wimp_SendMessage(17,Block,Block[MsgB_Sender],Block[6]);

  // Set this drive path immediately?
  if (SetNow)
  {
    Prefs *prefs = new Prefs(ThePrefs);

    prefs->DriveType[DrNum] = j; strcpy(prefs->DrivePath[DrNum],((char*)Block)+44);
    the_c64->NewPrefs(prefs);
    ThePrefs = *prefs;
    delete prefs;
  }
}



void WIMP::SetSpeedLimiter(bool LimitSpeed)
{
  RO_Icon *roi;
  char *b, c;

  PrefsWindow->SetIconState(Icon_Prefs_LimSpeed,(LimitSpeed) ? IFlg_Slct : 0, IFlg_Slct);
  roi = EmuPane->GetIcon(Icon_Pane_Speed);
  if ((b = (char*)roi->dat.ind.val) != NULL)
  {
    do
    {
      c = *b++;
      if ((c == 'r') || (c == 'R'))	// boRder command?
      {
        if (LimitSpeed) {*b++ = '1';} else {*b++ = '2';}
        c = 0;	// stop now
      }
      else if (c >= 32)	// skip to next command
      {
        c = *b++;
        while ((c >= 32) && (c != ';')) {if (c == '\\') {b++;} c = *b++;}
      }
    }
    while (c >= 32);
    EmuPane->UpdateIcon(Icon_Pane_Speed);
  }
}





void WIMP::Poll(bool Paused)
{
  int event;
  bool LastPause;

  LastPause = EmuPaused; EmuPaused = Paused;

  // If emulator is paused disable null events
  if ((!EmuPaused) || (UseNULL > 0)) {Mask &= 0xfffffffe;} else {Mask |= 1;}

  do
  {
    // Loop until a null event is received, then continue the emulation.
    // (this looks best)
    do
    {
      event = Wimp_Poll(Mask,Block,NULL);
      switch (event)
      {
        case 1: Redraw(); break;
        case 2: OpenWindow(); break;
        case 3: CloseWindow(); break;
        case 6: MouseClick(); break;
        case 7: UserDrag(); break;
        case 8: if ((EmuPaused) && (Block[KeyPB_Window] == EmuWindow->MyHandle()))
                {
                  if (the_c64->TheDisplay->CheckForUnpause(!LastPause)) {EmuPaused = false;}
                }
                KeyPressed(); break;
        case 9: MenuSelection(); break;
        case 17:
        case 18: UserMessage(); break;
        case 19: UserMessageAck(); break;
        default: break;
      }
      // This is important
      if ((!EmuPaused) || (UseNULL > 0)) {Mask &= 0xfffffffe;} else {Mask |= 1;}
      LastPause = EmuPaused;
    }
    while (event != 0);
    if (UseNULL > 0) {PollSysConfWindow();}
    // This should probably become a new member-function one day...
    if (DragType == DRAG_VolumeWell)
    {
      Wimp_GetPointerInfo(Block);
      if (Block[MouseB_Icon] == Icon_Sound_Volume)	// should always be the case (BBox)!
      {
        the_c64->HostVolume = CalculateVolume(Block);
        SoundWindow->ForceIconRedraw(Icon_Sound_Volume);
      }
    }
  }
  while (EmuPaused);
}


void WIMP::Redraw(void)
{
  if (Block[RedrawB_Handle] == EmuWindow->MyHandle()) // emulator main window
  {
    C64Display *disp = the_c64->TheDisplay;

    EmuWindow->redraw(Block,disp->BitmapBase(),disp);
  }
  else if (Block[RedrawB_Handle] == SoundWindow->MyHandle())	// sound window
  {
    int more;
    int minx, miny, maxx, maxy, thresh;

    more = Wimp_RedrawWindow(Block);
    // compute screen coordinates of work (0,0)
    minx = Block[RedrawB_VMinX] - Block[RedrawB_ScrollX];
    maxy = Block[RedrawB_VMaxY] - Block[RedrawB_ScrollY];
    // get volume well bounding box
    SoundWindow->GetIconState(Icon_Sound_Volume, AuxBlock);
    maxx = minx + AuxBlock[IconB_MaxX] - WellBorder; minx += AuxBlock[IconB_MinX] + WellBorder;
    miny = maxy + AuxBlock[IconB_MinY] + WellBorder; maxy += AuxBlock[IconB_MaxY] - WellBorder;
    thresh = minx + (the_c64->HostVolume * (maxx - minx))/ MaximumVolume;
    while (more != 0)
    {
      // clip
      if ((minx <= Block[RedrawB_CMaxX]) && (maxx >= Block[RedrawB_CMinX]) &&
          (miny <= Block[RedrawB_CMaxY]) && (maxy >= Block[RedrawB_CMinY]))
      {
        if (Block[RedrawB_CMinX] < thresh)
        {
          ColourTrans_SetGCOL(0x00ff0000,0,0);	// green
          OS_Plot(0x04,minx,miny); OS_Plot(0x65,thresh,maxy);
        }
        if (Block[RedrawB_CMaxX] > thresh)
        {
          ColourTrans_SetGCOL(0xffffff00,0,0);	// white
          OS_Plot(0x04,thresh,miny); OS_Plot(0x65,maxx,maxy);
        }
      }
      more = Wimp_GetRectangle(Block);
    }
  }
  else // Dummy redraw loop
  {
    int more;

    more = Wimp_RedrawWindow(Block);
    while (more != 0)
    {
      more = Wimp_GetRectangle(Block);
    }
  }
}


void WIMP::OpenWindow(void)
{
  if (Block[WindowB_Handle] == EmuWindow->MyHandle()) {OpenEmuWindow(Block);}
  else if (Block[WindowB_Handle] != EmuPane->MyHandle())
  {
    Wimp_OpenWindow(Block);
  }
}


void WIMP::CloseWindow(void)
{
  if (Block[WindowB_Handle] == EmuWindow->MyHandle()) {CloseEmuWindow();}
  else if (Block[WindowB_Handle] != EmuPane->MyHandle())
  {
    if (Block[WindowB_Handle] == ConfigWindow->MyHandle()) {UseNULL--;}
    Wimp_CloseWindow(Block);
  }
}


void WIMP::MouseClick(void)
{
  if ((Block[MouseB_Window] == -2) && (Block[MouseB_Icon] == IBicon->IHandle))	// Icon Bar icon
  {
    if (Block[MouseB_Buttons] == 2)	// Menu
    {
      Wimp_CreateMenu((int*)&MenuIconBar,Block[MouseB_PosX]-MenuIconBar.head.width/2,96+Menu_Height*Menu_IBar_Items);
      LastMenu = Menu_IBar;
    }
    else	// Some other click
    {
      if (!EmuWindow->OpenStatus(Block)) {Block[WindowB_Stackpos] = -1; OpenEmuWindow(Block);}
    }
  }
  else if (Block[MouseB_Window] == EmuWindow->MyHandle())	// Emulator window
  {
    if (Block[MouseB_Buttons] >= 256)	// click
    {
      Wimp_GetCaretPosition(&LastCaret);
      Wimp_SetCaretPosition(Block[MouseB_Window],-1,-100,100,-1,-1);	// gain input focus
    }
    if (Block[MouseB_Buttons] == 2)	// menu
    {
      Wimp_CreateMenu((int*)&MenuEmuWindow,Block[MouseB_PosX]-MenuEmuWindow.head.width/2,Block[MouseB_PosY]);
      LastMenu = Menu_Emulator;
    }
  }
  else if (Block[MouseB_Window] == EmuPane->MyHandle())	// Emulator pane
  {
    if ((Block[MouseB_Buttons] == 1) || (Block[MouseB_Buttons] == 4))
    {
      switch (Block[MouseB_Icon])
      {
        case Icon_Pane_Pause:	// Pause icon
          if (EmuPaused)
          {
            EmuPane->WriteIconText(Icon_Pane_Pause,PANE_TEXT_PAUSE);
            the_c64->Resume(); EmuPaused = false;
          }
          else
          {
            EmuPane->WriteIconText(Icon_Pane_Pause,PANE_TEXT_RESUME);
            the_c64->Pause(); EmuPaused = true;
            // Update the window now!
            UpdateEmuWindow();
          }
          break;
        case Icon_Pane_Reset: the_c64->Reset(); break;
        case Icon_Pane_Toggle: ToggleEmuWindowSize(); break;
        case Icon_Pane_Speed:
          ThePrefs.LimitSpeed = !ThePrefs.LimitSpeed; SetSpeedLimiter(ThePrefs.LimitSpeed);
          break;
        default: break;
      }
    }
  }
  else if (Block[MouseB_Window] == PrefsWindow->MyHandle())		// Prefs window
  {
    if ((Block[MouseB_Buttons] == 1) || (Block[MouseB_Buttons] == 4))	// normal click
    {
      register int i;

      switch(Block[MouseB_Icon])
      {
        case Icon_Prefs_SkipFLeft:
             i = PrefsWindow->ReadIconNumber(Icon_Prefs_SkipFText);
             if (i > 0)
             {
               PrefsWindow->WriteIconNumber(Icon_Prefs_SkipFText,--i);
               ThePrefs.SkipFrames = i; // instant update
             }
             break;
        case Icon_Prefs_SkipFRight:
             i = PrefsWindow->ReadIconNumber(Icon_Prefs_SkipFText);
             PrefsWindow->WriteIconNumber(Icon_Prefs_SkipFText,++i);
             ThePrefs.SkipFrames = i;	// instant update
             break;
        case Icon_Prefs_Cancel: ThePrefsToWindow(); break;	// restore current settings
        case Icon_Prefs_OK: WindowToThePrefs();
             if (Block[MouseB_Buttons] == 4) {PrefsWindow->close();}
             break;		// use new prefs
        case Icon_Prefs_Save:
             WindowToThePrefs();
             if (CheckFilename(PrefsWindow->ReadIconText(Icon_Prefs_PrefPath)) == 0)
             {
               ThePrefs.Save(PrefsWindow->ReadIconText(Icon_Prefs_PrefPath));
             }
             break;
        default: break;
      }
    }
    else if ((Block[MouseB_Buttons] == 16) || (Block[MouseB_Buttons] == 64))
    // drag (only the sprite)
    {
      if (Block[MouseB_Icon] == Icon_Prefs_PrefSprite)
      {
        DragIconSprite(PrefsWindow, Icon_Prefs_PrefSprite);
        DragType = DRAG_PrefsSprite;
      }
    }
  }
  else if (Block[MouseB_Window] == ConfigWindow->MyHandle())	// sys config window
  {
    if ((Block[MouseB_Buttons] == 1) || (Block[MouseB_Buttons] == 4))
    {
      if (Block[MouseB_Icon] == Icon_Conf_OK)
      {
        WindowToSysConf(); UseNULL--;
        if (Block[MouseB_Buttons] == 4) {ConfigWindow->close();}
      }
      else if (Block[MouseB_Icon] == Icon_Conf_Save)
      {
        if (CheckFilename(ConfigWindow->ReadIconText(Icon_Conf_ConfPath)) == 0)
        {
          WindowToSysConf(); UseNULL--; ConfigWindow->close();
          the_c64->SaveSystemConfig(ConfigWindow->ReadIconText(Icon_Conf_ConfPath));
        }
      }
    }
    else if ((Block[MouseB_Buttons] == 16) || (Block[MouseB_Buttons] == 64))
    {
      if (Block[MouseB_Icon] == Icon_Conf_ConfSprite)
      {
        DragIconSprite(ConfigWindow, Icon_Conf_ConfSprite);
        DragType = DRAG_ConfSprite;
      }
    }
  }
  else if (Block[MouseB_Window] == SoundWindow->MyHandle())		// sound window
  {
    if (Block[MouseB_Icon] == Icon_Sound_Volume)
    {
      if ((Block[MouseB_Buttons] == 1) || (Block[MouseB_Buttons] == 4))		// click
      {
        the_c64->HostVolume = CalculateVolume(Block); Sound_Volume(the_c64->HostVolume);
        SoundWindow->ForceIconRedraw(Icon_Sound_Volume);
      }
      else if ((Block[MouseB_Buttons] == 16) || (Block[MouseB_Buttons] == 64))	// drag
      {
        int orgx, orgy;

        SoundWindow->getstate(AuxBlock);
        orgx = AuxBlock[WindowB_VMinX] - AuxBlock[WindowB_ScrollX];
        orgy = AuxBlock[WindowB_VMaxY] - AuxBlock[WindowB_ScrollY];
        SoundWindow->GetIconState(Icon_Sound_Volume, &AuxBlock[DragB_BBMinX - IconB_MinX]);
        AuxBlock[DragB_BBMinX] += orgx; AuxBlock[DragB_BBMinY] += orgy;
        AuxBlock[DragB_BBMaxX] += orgx; AuxBlock[DragB_BBMaxY] += orgy;
        AuxBlock[DragB_Type] = 7;
        Wimp_DragBox(AuxBlock);
        DragType = DRAG_VolumeWell; UseNULL++;
      }
    }
  }
  else if (Block[MouseB_Window] == SaveBox->MyHandle())
  {
    if ((Block[MouseB_Buttons] == 1) || (Block[MouseB_Buttons] == 4))
    {
      if (Block[MouseB_Icon] == Icon_Save_OK)
      {
        if (CheckFilename(SaveBox->ReadIconText(Icon_Save_Path)) == 0)
        {
          if (SaveType == SAVE_RAM)
          {
            strcpy(RAMFile+44,SaveBox->ReadIconText(Icon_Save_Path));
            the_c64->SaveRAM(RAMFile+44);
            Wimp_CreateMenu((int*)-1,0,0);
            SaveType = 0;
          }
          else if (SaveType == SAVE_Snapshot)
          {
            *(((int*)SnapFile) + MsgB_Sender) = 0;
            strcpy(SnapFile+44,SaveBox->ReadIconText(Icon_Save_Path));
            IssueSnapshotRequest();
          }
        }
      }
    }
    else if ((Block[MouseB_Buttons] == 16) || (Block[MouseB_Buttons] == 64))
    {
      if (Block[MouseB_Icon] == Icon_Save_Sprite)
      {
        DragIconSprite(SaveBox, Icon_Save_Sprite);
        DragType = DRAG_SaveSprite;
      }
    }
  }
}


// A drag operation has terminated
void WIMP::UserDrag(void)
{
  char *b = NULL;
  int filetype, size;

  if ((CMOS_DragType == 0) || (DragType == DRAG_VolumeWell))
  {
    Wimp_DragBox(NULL);
  }
  else
  {
    DragASprite_Stop();
  }

  if (DragType == DRAG_VolumeWell)
  {
    UseNULL--; DragType = 0; Sound_Volume(the_c64->HostVolume);	// just set the new volume.
  }

  // Drag of the path sprite
  if (DragType == DRAG_PrefsSprite)
  {
    b = PrefsWindow->ReadIconText(Icon_Prefs_PrefPath); filetype = FileType_Text;
    size = EstimatedPrefsSize;	// can't say how large it's gonna be
  }
  else if (DragType == DRAG_ConfSprite)
  {
    b = ConfigWindow->ReadIconText(Icon_Conf_ConfPath); filetype = FileType_Text;
    size = EstimatedConfSize;
  }
  else if (DragType == DRAG_SaveSprite)
  {
    b = SaveBox->ReadIconText(Icon_Save_Path); filetype = FileType_Data;
    if (SaveType == SAVE_RAM) {size = EstimatedRAMSize;}
    else if (SaveType == SAVE_Snapshot) {size = EstimatedSnapSize;}
    else {size = 0;}
  }

  // now b should point to the path and filetype should contain the type
  if (b != NULL)
  {
    Wimp_GetPointerInfo(Block);
    // Not on background and not on my own icon bar icon
    if ((Block[MouseB_Window] != -1) && ((Block[MouseB_Window] != -2) || (Block[MouseB_Icon] != IBicon->IHandle)))
    {
      int handle = Block[MouseB_Window];

      // None of my windows
      if ((handle != EmuWindow->MyHandle()) && (handle != EmuPane->MyHandle()) &&
          (handle != PrefsWindow->MyHandle()) && (handle != ConfigWindow->MyHandle()) &&
          (handle != InfoWindow->MyHandle()) && (handle != SaveBox->MyHandle()))
      {
        char *d, c;

        d = b; c = *b++;
        // get pointer to leafname in d
        while (c > 32) {if ((c == '.') || (c == ':')) {d = b;} c = *b++;}
        // Build message block
        Block[5] = Block[MouseB_Window]; Block[6] = Block[MouseB_Icon];
        Block[7] = Block[MouseB_PosX]; Block[8] = Block[MouseB_PosY];
        Block[9] = size; Block[10] = filetype;
        Block[MsgB_YourRef] = 0; Block[MsgB_Action] = Message_DataSave;
        b = ((char*)Block) + 44; c = *d++;
        while (c > 32) {*b++ = c; c = *d++;}
        *b++ = 0; Block[MsgB_Size] = (((int)(b - (char*)Block)) + 3) & 0xfffffffc;
        Wimp_SendMessage(18,Block,Block[5],Block[6]);
      }
    }
  }
  else {DragType = 0;}
  LastDrag = 0;
}


void WIMP::KeyPressed(void)
{
  register int key = Block[KeyPB_Key];

  if (Block[KeyPB_Window] == EmuWindow->MyHandle())
  {
    if ((key >= 0x180) && (key <= 0x1fd)) // special keys (incl. FKeys)
    {
      key -= 0x180;
      if ((((key & 0x4f) >= 0x05) && ((key & 0x4f) <= 0x09)) ||	// F5 -F9 [shift|ctrl]
          (((key & 0x4f) >= 0x4a) && ((key & 0x4f) <= 0x4c)))	// F10-F12 [shift|ctrl]
      {
        if ((key != 0x06) && (key != 0x07) && (key != 0x08))
        {
          Wimp_ProcessKey(Block[KeyPB_Key]);
        }
      }
    }
    return;
  }
  else if (Block[KeyPB_Window] == PrefsWindow->MyHandle())
  {
    if (key == Key_Return)
    {
      WindowToThePrefs();
      if (Block[KeyPB_Icon] == Icon_Prefs_PrefPath)	// return pressed in path string icon?
      {
        if (CheckFilename(PrefsWindow->ReadIconText(Icon_Prefs_PrefPath)) == 0)
        {
          ThePrefs.Save(PrefsWindow->ReadIconText(Icon_Prefs_PrefPath));
        }
      }
      return;
    }
  }
  else if (Block[KeyPB_Window] == ConfigWindow->MyHandle())
  {
    if ((key == Key_Return) && (Block[KeyPB_Icon] != -1))
    {
      WindowToSysConf(); UseNULL--; ConfigWindow->close();
      if (Block[KeyPB_Icon] == Icon_Conf_ConfPath)	// return pressed in path string icon?
      {
        if (CheckFilename(ConfigWindow->ReadIconText(Icon_Conf_ConfPath)) == 0)
        {
          the_c64->SaveSystemConfig(ConfigWindow->ReadIconText(Icon_Conf_ConfPath));
        }
      }
      return;
    }
  }
  else if (Block[KeyPB_Window] == SaveBox->MyHandle())
  {
    if (key == Key_Return)
    {
      if (Block[KeyPB_Icon] == Icon_Save_Path)
      {
        if (CheckFilename(SaveBox->ReadIconText(Icon_Save_Path)) == 0)
        {
          if (SaveType == SAVE_RAM)
          {
            strcpy(RAMFile+44,SaveBox->ReadIconText(Icon_Save_Path));
            the_c64->SaveRAM(RAMFile+44);
            Wimp_CreateMenu((int*)-1,0,0);
            SaveType = 0;
          }
          else if (SaveType == SAVE_Snapshot)
          {
            *(((int*)SnapFile) + MsgB_Sender) = 0;
            strcpy(SnapFile+44,SaveBox->ReadIconText(Icon_Save_Path));
            IssueSnapshotRequest();
          }
        }
      }
      return;
    }
  }
  Wimp_ProcessKey(Block[KeyPB_Key]);
}


void WIMP::MenuSelection(void)
{
  int Buttons;

  Wimp_GetPointerInfo(AuxBlock); Buttons = AuxBlock[MouseB_Buttons];

  switch (LastMenu)
  {
    case Menu_IBar:
        if (Block[0] == Menu_IBar_Quit) {EmuPaused = false; the_c64->Quit();}
        else if (Block[0] == Menu_IBar_Prefs)
        {
          // Is it already open? Then don't do anything
          if (!PrefsWindow->OpenStatus())
          {
            int y;

            // Open Prefs window with top left corner in top left corner of screen
            PrefsWindow->getstate(AuxBlock);
            y = the_c64->TheDisplay->screen->resy;
            AuxBlock[WindowB_VMaxX] -= AuxBlock[WindowB_VMinX]; AuxBlock[WindowB_VMinX] = 0;
            AuxBlock[WindowB_VMinY] = y - (AuxBlock[WindowB_VMaxY] - AuxBlock[WindowB_VMinY]);
            AuxBlock[WindowB_VMaxY] = y;
            // Open Prefs window on top
            AuxBlock[WindowB_Stackpos] = -1;
            PrefsWindow->open(AuxBlock);
          }
          else
          {
            PrefsWindow->getstate(AuxBlock);
            AuxBlock[WindowB_Stackpos] = -1; PrefsWindow->open(AuxBlock);
          }
        }
        else if (Block[0] == Menu_IBar_Config)
        {
          if (!ConfigWindow->OpenStatus())
          {
            int x, y;

            // Update window contents
            SysConfToWindow();
            // Open config window in top right corner of screen
            ConfigWindow->getstate(AuxBlock);
            x = the_c64->TheDisplay->screen->resx; y = the_c64->TheDisplay->screen->resy;
            AuxBlock[WindowB_VMinX] = x - (AuxBlock[WindowB_VMaxX] - AuxBlock[WindowB_VMinX]);
            AuxBlock[WindowB_VMaxX] = x;
            AuxBlock[WindowB_VMinY] = y - (AuxBlock[WindowB_VMaxY] - AuxBlock[WindowB_VMinY]);
            AuxBlock[WindowB_VMaxY] = y;
            AuxBlock[WindowB_Stackpos] = -1;
            ConfigWindow->open(AuxBlock);
            // We need NULL-events for the low-level keyboard scan.
            UseNULL++;
          }
          else
          {
            ConfigWindow->getstate(AuxBlock);
            AuxBlock[WindowB_Stackpos] = -1; ConfigWindow->open(AuxBlock);
          }
        }
        if (Buttons == 1) {Wimp_CreateMenu((int*)&MenuIconBar,0,0);}
        break;
    case Menu_Emulator:
        if (Buttons == 1) {Wimp_CreateMenu((int*)&MenuEmuWindow,0,0);}
        break;
    default: break;
  }
}


// Handle regular messages
void WIMP::UserMessage(void)
{
  C64Display *disp = the_c64->TheDisplay;
  int i;

  switch (Block[MsgB_Action]) // Message Action
  {
    case Message_Quit:
    	 EmuPaused = false; the_c64->Quit(); break;
    case Message_ModeChange:
    	 disp->ModeChange(); the_c64->TheVIC->ReInitColors(); SetEmuWindowSize();
    	 // The window could have changed position ==> reposition pane as well!
    	 // we have to force the window to the screen manually
    	 EmuWindow->getstate(AuxBlock);
    	 if ((AuxBlock[WindowB_WFlags] & (1<<16)) != 0)	// is it open anyway?
    	 {
    	   i = AuxBlock[WindowB_VMaxY] - AuxBlock[WindowB_VMinY];
    	   if (AuxBlock[WindowB_VMaxY] > disp->screen->resy - TitleBarHeight)
    	   {
    	     AuxBlock[WindowB_VMaxY] = disp->screen->resy - TitleBarHeight;
      	     if ((AuxBlock[WindowB_VMinY] = AuxBlock[WindowB_VMaxY] - i) < TitleBarHeight)
    	     {
    	       AuxBlock[WindowB_VMinY] = TitleBarHeight;
    	     }
    	   }
    	   i = AuxBlock[WindowB_VMaxX] - AuxBlock[WindowB_VMinX];
    	   if (AuxBlock[WindowB_VMaxX] > disp->screen->resx - TitleBarHeight)
    	   {
    	     AuxBlock[WindowB_VMaxX] = disp->screen->resx - TitleBarHeight;
    	     if ((AuxBlock[WindowB_VMinX] = AuxBlock[WindowB_VMaxX] - i) < 0)
    	     {
    	       AuxBlock[WindowB_VMinX] = 0;
    	     }
    	   }
    	   // Don't you just love it -- you can't open the window directly, you need
    	   // a delay... like for instance by sending yourself an OpenWindow message...
    	   Wimp_SendMessage(2,AuxBlock,TaskHandle,0);
    	 }
    	 break;
    case Message_PaletteChange:
         // Updating EmuWindow is pointless since the bitmap still contains data for another mode
         disp->ModeChange(); the_c64->TheVIC->ReInitColors();
         break;
    case Message_DataSave:
         i = -1;	// indicator whether file is accepted
         if ((Block[5] == EmuWindow->MyHandle()) && (Block[10] == FileType_C64File)) {i=0;}
         else if ((Block[5] == EmuWindow->MyHandle()) && (Block[10] == FileType_Data)) {i=0;}
         else if ((Block[5] == PrefsWindow->MyHandle()) && ((Block[10] == FileType_Text) || (Block[10] == FileType_C64File))) {i=0;}
         else if ((Block[5] == ConfigWindow->MyHandle()) && (Block[10] == FileType_Text)) {i=0;}
         if (i >= 0)
         {
           Block[9] = -1;	// file unsafe
           strcpy(((char*)Block)+44,WIMP_SCRAP_FILE);
           Block[MsgB_Size] = (48 + strlen(WIMP_SCRAP_FILE)) & 0xfffffffc;
           Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataSaveAck;
           Wimp_SendMessage(17,Block,Block[MsgB_Sender],Block[6]);
           UseScrap = true;
         }
         break;
    case Message_DataLoad:
         if (Block[5] == EmuWindow->MyHandle())	// Emulator window: load file?
         {
           if (Block[10] == FileType_C64File)	// Load only files with type &64 this way
           {
             FILE *fp;

             if ((fp = fopen(((char*)Block)+44,"rb")) != NULL)
             {
               uint8 lo, hi, *mem = the_c64->RAM;
               int length;

               lo = fgetc(fp); hi = fgetc(fp); length = lo + (hi<<8);
               length += fread(mem+length,1,0x10000-length,fp);
               // length is now end-address
               fclose(fp);
               mem[0xc3] = lo; mem[0xc4] = hi; // Load-address
               lo = length & 0xff; hi = (length >> 8) & 0xff;
               mem[0xae] = mem[0x2d] = mem[0x2f] = mem[0x31] = mem[0x33] = lo;
               mem[0xaf] = mem[0x2e] = mem[0x30] = mem[0x32] = mem[0x34] = hi;
               Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataLoadAck;
               Wimp_SendMessage(17,Block,Block[MsgB_Sender],Block[6]);
             }
           }
           else if (Block[10] == FileType_Data)
           {
             if (the_c64->LoadSnapshot(((char*)Block)+44))
             {
               Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataLoadAck;
               Wimp_SendMessage(17,Block,Block[MsgB_Sender],Block[6]);
             }
           }
         }
         else if (Block[5] == PrefsWindow->MyHandle())	// Prefs window?
         {
           if (Block[10] == FileType_Text)	// load a prefs file?
           {
             Prefs *prefs = new Prefs(ThePrefs);

             prefs->Load(((char*)Block)+44);
             the_c64->NewPrefs(prefs);
             ThePrefs = *prefs;
             delete prefs;
             PrefsWindow->WriteIconText(Icon_Prefs_PrefPath,((char*)Block)+44);
             ThePrefsToWindow();
             Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataLoadAck;
             Wimp_SendMessage(17,Block,Block[MsgB_Sender],Block[6]);
           }
	 	   else if ((Block[6] == Icon_Prefs_XROMPath) && (Block[10] == FileType_C64File))
		   {
		     PrefsWindow->WriteIconText(Icon_Prefs_XROMPath,((char*)Block)+44);
		     Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataLoadAck;
		     Wimp_SendMessage(17,Block,Block[MsgB_Sender],Block[6]);
		   }
           else		// interpret as drive path (if dragged on one of the drive path icons)
           {
             switch (Block[6])
             {
               case Icon_Prefs_Dr8Path:  i = 0; break;
               case Icon_Prefs_Dr9Path:  i = 1; break;
               case Icon_Prefs_Dr10Path: i = 2; break;
               case Icon_Prefs_Dr11Path: i = 3; break;
               default: i = -1; break;
             }
             if (i >= 0) {NewDriveImage(i,Block,false);}
           }
         }
         else if (Block[5] == ConfigWindow->MyHandle())	// load sys config file
         {
           if (Block[10] == FileType_Text)
           {
             the_c64->LoadSystemConfig(((char*)Block)+44);
             SysConfToWindow();
             ConfigWindow->WriteIconText(Icon_Conf_ConfPath,((char*)Block)+44);
             Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataLoadAck;
             Wimp_SendMessage(17,Block,Block[MsgB_Sender],Block[6]);
           }
         }
         else if (Block[5] == EmuPane->MyHandle())	// emulator pane
         {
           switch (Block[6])
           {
             case Icon_Pane_Drive0:
             case Icon_Pane_LED0: i = 0; break;
             case Icon_Pane_Drive1:
             case Icon_Pane_LED1: i = 1; break;
             case Icon_Pane_Drive2:
             case Icon_Pane_LED2: i = 2; break;
             case Icon_Pane_Drive3:
             case Icon_Pane_LED3: i = 3; break;
             default: i = -1; break;
           }
           if (i >= 0) {NewDriveImage(i,Block,true);}
         }
         // Clean up if necessary
         if (UseScrap) {DeleteFile(WIMP_SCRAP_FILE); UseScrap = false;}
         break;
    case Message_DataSaveAck:
         if (DragType == DRAG_PrefsSprite)
         {
           WindowToThePrefs();	// read window entries
           ThePrefs.Save(((char*)Block)+44);
           if (Block[9] != -1)	// we're talking to the filer ==> set new pathname
           {
             PrefsWindow->WriteIconText(Icon_Prefs_PrefPath,((char*)Block)+44);
           }
           Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataLoad;
           Wimp_SendMessage(18,Block,Block[MsgB_Sender],Block[6]);
         }
         else if (DragType == DRAG_ConfSprite)
         {
           WindowToSysConf();	// read window entries
           the_c64->SaveSystemConfig(((char*)Block)+44);
           if (Block[9] != -1)
           {
             ConfigWindow->WriteIconText(Icon_Conf_ConfPath,((char*)Block)+44);
           }
           Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataLoad;
           Wimp_SendMessage(18,Block,Block[MsgB_Sender],Block[6]);
         }
         else if (DragType == DRAG_SaveSprite)
         {
           if (SaveType == SAVE_RAM)
           {
             memcpy(RAMFile,(char*)Block,256); the_c64->SaveRAM(RAMFile+44);
             Block[MsgB_YourRef] = Block[MsgB_MyRef]; Block[MsgB_Action] = Message_DataLoad;
             Wimp_SendMessage(18,Block,Block[MsgB_Sender],Block[6]);
           }
           else if (SaveType == SAVE_Snapshot)
           {
             memcpy(SnapFile,(char*)Block,256);
             IssueSnapshotRequest();
           }
         }
         break;
    case Message_DataLoadAck:
         if (DragType == DRAG_ConfSprite)
         {
           UseNULL--; ConfigWindow->close();
         }
         if (DragType == DRAG_SaveSprite)
         {
           Wimp_CreateMenu((int*)-1,0,0);
         }
         DragType = SaveType = 0; break;
    case Message_MenuWarning:
         if (LastMenu == Menu_Emulator)
         {
           if (Block[8] == Menu_EWind_SaveRAM)
           {
             SaveType = SAVE_RAM; SaveBox->WriteIconText(Icon_Save_Path,RAMFile+44);
           }
           else if (Block[8] == Menu_EWind_Snapshot)
           {
             SaveType = SAVE_Snapshot; SaveBox->WriteIconText(Icon_Save_Path,SnapFile+44);
           }
           else {SaveType = 0;}
           Wimp_CreateSubMenu((int*)Block[5],Block[6],Block[7]);
         }
         break;
    default: break;
  }
}


// If a recorded message was not answered, i.e. something went wrong.
void WIMP::UserMessageAck(void)
{
  switch(Block[MsgB_Action])
  {
    case Message_DataSave:
         sprintf(WimpError.errmess,"Can't save data."); break;
    case Message_DataLoad:
         sprintf(WimpError.errmess,"Receiver couldn't load data."); break;
    default:
         sprintf(WimpError.errmess,"Some error occurred..."); break;
  }
  WimpError.errnum = 0; Wimp_ReportError(&WimpError,1,TASKNAME);
}
