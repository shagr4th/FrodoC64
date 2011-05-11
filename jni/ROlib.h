/*
 *  ROlib.h - Defines Classes, variables and OS interface calls for Acorn
 *            RISC OS computers
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

#ifndef RO_CUSTOM_LIB
#define RO_CUSTOM_LIB

#include <kernel.h>


#define TASKNAME   "Frodo"
#define TASK_WORD  0x4b534154

/* Scrap-file for 1541fs directory function */
#define RO_TEMPFILE	"<Wimp$ScrapDir>.FrodoDIR"



/* Icon Flags */
#define IFlg_Text	1
#define IFlg_Sprite	2
#define IFlg_Border	4
#define IFlg_HCenter	8
#define IFlg_VCenter	16
#define IFlg_Filled	32
#define IFlg_AntiA	64
#define IFlg_AutoRdrw	128
#define IFlg_Indir	256
#define IFlg_RAdjust	512
#define IFlg_Slct	(1<<21)
#define IFlg_Grey	(1<<22)

/* Menu Flags */
#define MFlg_Tick	1
#define MFlg_Dotted	2
#define MFlg_Writable	4
#define MFlg_Warning	8
#define MFlg_LastItem	128

/* Joystick stuff */
#define JoyDir_Thresh   32
#define JoyButton1	(1<<16)
#define JoyButton2	(1<<17)



/* Size of WIMP data types */
#define RO_WINDOW_WORDS	23
#define RO_WINDOW_BYTES	92
#define RO_ICON_WORDS	8
#define RO_ICON_BYTES	32
#define RO_MHEAD_WORDS	7
#define RO_MHEAD_BYTES	28
#define RO_MITEM_WORDS	6
#define RO_MITEM_BYTES	24




/* Structures for directory scanning (mainly 1541fs) */
/* May hold entire pathname, so DON'T REDUCE!!! */
#define FILENAME_MAX_CHARS	256	/* must be >= NAMEBUF_LENGTH (256) */

typedef struct {
  unsigned int load, exec, length, attrib, otype;
  char name[FILENAME_MAX_CHARS];
} dir_full_info;

typedef struct {
  int readno, offset, buffsize;
  char *match;
} dir_env;



/* WIMP structures: */
typedef struct {
  int *tit, *val;
  int len;
} WIdatI;

/* data type for window / icon data */
typedef union {
  char strg[12];
  WIdatI ind;
} WIdata;


/* Window descriptor - 23 words = 92 bytes */
typedef struct {
  int Handle;
  int vminx,vminy,vmaxx,vmaxy;
  int scrollx,scrolly;
  int stackpos;
  unsigned int wflags;
  char col_tfg, col_tbg, col_wfg, col_wbg; /* title/work_area fore/background colour */
  char col_sbo, col_sbi, col_tsl, reserved;/* scroll bar inner/outer, title if input focus */
  int wminx, wminy, wmaxx, wmaxy;
  unsigned int tflags, waflags;
  int SpriteAreaPtr;
  short min_width, min_height;
  WIdata dat;
  int icon_no;
} RO_Window;


/* Icon descriptor */
typedef struct {
  int minx, miny, maxx, maxy;
  int iflags;
  WIdata dat;
} RO_Icon;


typedef struct {
  int WindowHandle;
  int minx, miny, maxx, maxy;
  int iflags;
  WIdata dat;
} RO_IconDesc;


typedef struct {
  char title[12];
  char col_tfg, col_tbg, col_wfg, col_wbg;
  int width, height, vgap;
} RO_MenuHead;


typedef struct {
  unsigned int mflags;
  RO_MenuHead *submenu;
  unsigned int iflags;
  WIdata dat;
} RO_MenuItem;


/* Caret descriptor */
typedef struct {
  int WHandle;
  int IHandle;
  int offx, offy;
  int height, index;
} RO_Caret;



/* Joystick key descriptor */
typedef struct {
  unsigned char up, down, left, right, fire;
} Joy_Keys;




/* Declare classes that are needed in the following new classes */
class C64Display;
class C64;



/* Very simple class to read the resolution and eigen values */
class RORes
{
  public:
  RORes(void);
  ~RORes(void);

  int resx,resy,eigx,eigy;
};


/* Handle current screenmode */
class ROScreen
{
  private:
  _kernel_oserror ModeError;

  public:
  ROScreen(void);
  ~ROScreen(void);
  int ReadMode(void);

  int resx, resy, ldbpp, eigx, eigy, ladd;
  char *scrbase;
};


class Icon
{
  private:
  RO_IconDesc icon;

  public:
  Icon(int IconHandle, const RO_IconDesc *IDesc);
  ~Icon(void);
  void setstate(unsigned int eor, unsigned int clear);
  void getstate(void);

  int IHandle;
};


class Window
{
  private:
  RO_Window *wind;
  bool isopen;

  public:
  Window(const int *WDesc, const char *Title);
  ~Window(void);
  int  MyHandle(void);
  void GetWorkArea(int *Dest);
  void RedrawAWindow(int *Block, uint8 *Bitmap, C64Display *Disp);
  RO_Window *Descriptor(void);
  RO_Icon *GetIcon(unsigned int number);
  void SetIconState(unsigned int number, unsigned int eor, unsigned int clear);
  void GetIconState(unsigned int number, int *Block);
  void WriteIconText(unsigned int number, const char *text);
  void WriteIconTextU(unsigned int number, const char *text);	// update instead of force redrw
  void WriteIconNumber(unsigned int number, int value);
  void WriteIconNumberU(unsigned int number, int value);
  char *ReadIconText(unsigned int number);
  int  ReadIconNumber(unsigned int number);
  void ForceIconRedraw(unsigned int number);
  void UpdateIcon(unsigned int number);
  void WriteTitle(const char *title);
  char *ReadTitle(void);
  void UpdateTitle(void);
  bool HaveInput(void);
  bool OpenStatus(void);
  bool OpenStatus(int *Block);
  void open(void);
  void open(int *Block);
  void close(void);
  void forceredraw(int minx, int miny, int maxx, int maxy);
  void update(uint8 *Bitmap, C64Display *Disp);
  void update(int *Block, uint8 *Bitmap, C64Display *Disp);
  void redraw(int *Block, uint8 *Bitmap, C64Display *Disp);
  void extent(int minx, int miny, int maxx, int maxy);
  void getstate(void);
  void getstate(int *dest); // read window definition to external block
};


class WIMP
{
  private:
  _kernel_oserror WimpError;
  int Block[64], AuxBlock[64];		// two WIMP blocks for convenience
  int Mask;
  int DragType, CMOS_DragType;
  bool UseScrap;	// Scrap file used (data transfer protocol!)
  bool EmuPaused;
  int UseNULL;		// Number of clients that need NULL events
  int RAMsize, RAMtransfered, RAMpartner;	// for RAM transfer
  int LastMenu, LastClick, LastDrag, MenuType, LastIcon, SaveType;
  int *SpriteArea;
  RO_Caret LastCaret;
  int EmuZoom;
  Joy_Keys NewJoyKeys[2];
  C64 *the_c64;

  public:
  WIMP(C64 *my_c64);
  ~WIMP(void);

  // On startup
  bool LoadATemplate(char *Name, Window **Which);

  // To make window and pane work as a team
  void OpenEmuWindow(void);
  void OpenEmuWindow(int *Block);	// open at pos
  void CloseEmuWindow(void);
  void UpdateEmuWindow(void);		// update window and pane
  void ThePrefsToWindow(void);		// write ThePrefs into the window
  void WindowToThePrefs(void);		// update ThePrefs from window
  void SysConfToWindow(void);
  void WindowToSysConf(void);
  void PollSysConfWindow(void);		// low-level koyboard scan
  void DragIconSprite(Window *host, unsigned int number);
  int  CalculateVolume(int *Block);
  int  CheckFilename(char *name);
  void SnapshotSaved(bool OK);
  void IssueSnapshotRequest(void);
  void SetLEDIcons(bool FloppyEmulation);
  void SetEmuWindowSize(void);
  void ToggleEmuWindowSize(void);
  int  ReadEmuWindowSize(void);
  void NewDriveImage(int DrNum, int *MsgBlock, bool SetNow);
  void SetSpeedLimiter(bool LimitSpeed);

  // Standard WIMP functions
  void Poll(bool Paused);
  void Redraw(void);
  void OpenWindow(void);
  void CloseWindow(void);
  void MouseClick(void);
  void UserDrag(void);
  void KeyPressed(void);
  void MenuSelection(void);
  void UserMessage(void);
  void UserMessageAck(void);

  // WIMP's Windows and Icons
  Icon *IBicon;
  Window *EmuWindow, *EmuPane, *PrefsWindow, *ConfigWindow, *InfoWindow, *SoundWindow, *SaveBox;
  char SnapFile[256], RAMFile[256];
};



/* system-specific Variables */

extern RO_IconDesc IBarIcon;
extern unsigned int TaskHandle;
extern int WimpMessages[];


/* Functions available via ROlib (not RISC OS Lib, mind you)
   All SWIs are called in the X-Form! */

extern "C"
{

/* WIMP FUNCTIONS */

/* returns handle or 0 if error */
extern int Wimp_Initialise(int Version, int tw, const char *TaskName, const int *Messages);

extern _kernel_oserror *Wimp_CloseDown(int Handle, int tw);

/* returns handle or 0 if error */
extern int Wimp_CreateWindow(const int *Window);

extern int Wimp_CreateIcon(int Priority, const RO_IconDesc *Icon);

extern _kernel_oserror *Wimp_DeleteWindow(const int *Window);

extern _kernel_oserror *Wimp_DeleteIcon(int *Block);

extern _kernel_oserror *Wimp_OpenWindow(const int *Window);

extern _kernel_oserror *Wimp_CloseWindow(const int *Window);

extern int Wimp_Poll(unsigned int Mask, int *Block, int *PollWord);

/* returns 0 if no more to do or error */
extern int Wimp_RedrawWindow(int *Block);

extern int Wimp_UpdateWindow(int *Block);

extern int Wimp_GetRectangle(int *Block);

extern _kernel_oserror *Wimp_GetWindowState(int *Block);

extern _kernel_oserror *Wimp_GetWindowInfo(int *Block);

extern _kernel_oserror *Wimp_SetIconState(int *Block);

extern _kernel_oserror *Wimp_GetIconState(int *Block);

extern _kernel_oserror *Wimp_GetPointerInfo(int *Block);

extern _kernel_oserror *Wimp_DragBox(int *Block);

extern _kernel_oserror *Wimp_ForceRedraw(int Handle, int minx, int miny, int maxx, int maxy);

extern _kernel_oserror *Wimp_SetCaretPosition(int WHandle, int IHandle, int xoff, int yoff, int height, int index);

extern _kernel_oserror *Wimp_GetCaretPosition(RO_Caret *Caret);

extern _kernel_oserror *Wimp_CreateMenu(const int *Menu, int cx, int cy);

extern _kernel_oserror *Wimp_SetExtent(int Handle, int *Block);

extern _kernel_oserror *Wimp_OpenTemplate(char *Name);

extern _kernel_oserror *Wimp_CloseTemplate(void);

extern _kernel_oserror *Wimp_LoadTemplate(char **Template, char **Indirect, char *IndirLimit, char *Fonts, char *Name, int *Position);

extern _kernel_oserror *Wimp_ProcessKey(int Key);

extern int Wimp_StartTask(char *command);

extern _kernel_oserror *Wimp_ReportError(const _kernel_oserror *Error, unsigned int Flags, const char *AppName);

extern _kernel_oserror *Wimp_GetWindowOutline(int *Block);

extern int Wimp_PollIdle(unsigned int Mask, int *Block, int MinTime, int *PollWord);

extern _kernel_oserror *Wimp_PlotIcon(int *Block);

extern _kernel_oserror *Wimp_SendMessage(int Event, int *Block, int THandle, int IHandle);

extern _kernel_oserror *Wimp_CreateSubMenu(int *MenuBlock, int cx, int xy);

extern _kernel_oserror *Wimp_SpriteOp(int, int, int, int, int, int, int, int);

extern _kernel_oserror *Wimp_BaseOfSprites(int *ROM, int *RAM);

extern _kernel_oserror *Wimp_CommandWindow(int Action);

extern _kernel_oserror *Wimp_TransferBlock(int SHandle, char *SBuff, int DHandle, char *DBuff, int BuffSize);

extern _kernel_oserror *Wimp_SpriteInfo(char *name, int *width, int *height, int *mode);

extern _kernel_oserror *DragASprite_Start(unsigned int Flags, int SpriteArea, char *SpriteName, int *Box, int *BBox);

extern _kernel_oserror *DragASprite_Stop(void);

extern _kernel_oserror *ColourTrans_SelectTable(int SMode, int SPal, int DMode, int DPal, char **Buffer, unsigned int Flags, int *TransWork, int *TransFunc);

extern _kernel_oserror *ColourTrans_SetFontColours(int Handle, int BPal, int FPal, int Offset);

extern _kernel_oserror *ColourTrans_SetColour(int GCOL, unsigned int Flags, int Action);

extern _kernel_oserror *ColourTrans_SetGCOL(int Palette, unsigned int Flags, int Action);



/* OS FUNCTIONS */

extern int OS_ReadModeVariable(int mode, int var);

extern int OS_ReadDynamicArea(int area);

extern int ScanKeys(int keys);

extern int ReadKeyboardStatus(void);

extern int ReadDragType(void);

extern int SetMousePointer(int NewShape);


/* Generic sprite op call */
extern _kernel_oserror *OS_SpriteOp(int, int, int, int, int, int, int, int);

extern _kernel_oserror *OS_Plot(int Command, int x, int y);

extern _kernel_oserror *MouseBoundingBox(char Box[8]);

extern int OS_ReadMonotonicTime(void);

extern _kernel_oserror *OS_ReadC(char *Code);

/* returns length of characters read; if length negative ==> terminated by escape */
extern int OS_ReadLine(char *Buffer, int BuffSize, int minasc, int maxasc, int Echo);

/* true ==> escape */
extern bool OS_ReadEscapeState(void);

/* File related calls */

/* Returns object type */
extern int ReadCatalogueInfo(char *Name, int Result[4]);

/* Read the next name in the directory */
extern _kernel_oserror *ReadDirName(const char *dirname, char *buffer, dir_env *env);

/* Read the next entry (name, length, type,...) in the directory */
extern _kernel_oserror *ReadDirNameInfo(const char *dirname,dir_full_info *buffer,dir_env *env);

extern _kernel_oserror *DeleteFile(char *name);

extern _kernel_oserror *OS_FlushBuffer(int BuffNum);

/* These functions are more secure than using sprintf because they allow buffersize */
/* Return value is a pointer to the terminating null */
extern char *ConvertInteger1(int value, char *buffer, int buffsize);
extern char *ConvertInteger2(int value, char *buffer, int buffsize);
extern char *ConvertInteger3(int value, char *buffer, int buffsize);
extern char *ConvertInteger4(int value, char *buffer, int buffsize);


/* Misc */

extern unsigned int ModeColourNumber(unsigned int pal_entry);

/* Returns -1 if error in joystick module, -2 if SWI unknown, joystate otherwise */
extern int Joystick_Read(int joyno);


/* Sound stuff */

#define DRState_Active		1
#define DRState_NeedData	2
#define DRState_Overflow	4

/* Sound calls */

extern int Sound_Volume(int volume);

/* Digital Renderer SWI calls */
extern _kernel_oserror *DigitalRenderer_Activate(int Channels, int Length, int SamPeriod);

extern _kernel_oserror *DigitalRenderer_Deactivate(void);

extern _kernel_oserror *DigitalRenderer_Pause(void);

extern _kernel_oserror *DigitalRenderer_Resume(void);

extern _kernel_oserror *DigitalRenderer_GetTables(uint8 **LinToLog, uint8 **LogScale);

extern int DigitalRenderer_ReadState(void);

extern _kernel_oserror *DigitalRenderer_NewSample(uint8 *Sample);

}

#endif
