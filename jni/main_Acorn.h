/*
 *  main_Acorn.h - Main program, RISC OS specific stuff
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

#include "ROlib.h"
#include "AcornGUI.h"


// Shared, system-specific data
// The application
Frodo *the_app;

// My task handle
unsigned int TaskHandle;

int WimpMessages[] = {
  Message_DataSave, Message_DataSaveAck, Message_DataLoad, Message_DataLoadAck,
  Message_DataOpen, Message_PaletteChange, Message_ModeChange, Message_MenuWarning, 0
};




// RORes member-functions - very simple reading of screen res and eigen
RORes::RORes(void)
{
  resx = OS_ReadModeVariable(-1,11); resy = OS_ReadModeVariable(-1,12);
  eigx = OS_ReadModeVariable(-1,4) ; eigy = OS_ReadModeVariable(-1,5);
  resx = (resx+1)<<eigx; resy = (resy+1)<<eigy;
}

RORes::~RORes(void) {;}





// ROScreen member-functions - handle currently selected screenmode
ROScreen::ROScreen(void)
{
  if (ReadMode() != 0)
  {
    ModeError.errnum = 0x0; strcpy((char*)&ModeError.errmess,"Can't read mode data!");
    Wimp_ReportError(&ModeError,1,TASKNAME);
  }
}


ROScreen::~ROScreen(void) {;}


// read data for current screenmode, returns 0 if OK, > 0 if error
int ROScreen::ReadMode(void)
{
  register int h=0;

  if ((resx = OS_ReadModeVariable(-1,11)) < 0) h++;
  if ((resy = OS_ReadModeVariable(-1,12)) < 0) h++;
  if ((eigx = OS_ReadModeVariable(-1,4))  < 0) h++;
  if ((eigy = OS_ReadModeVariable(-1,5))  < 0) h++;
  resx = (resx+1)<<eigx; resy = (resy+1)<<eigy;
  if ((ldbpp = OS_ReadModeVariable(-1,9)) < 0) h++;
  if ((ladd  = OS_ReadModeVariable(-1,6)) < 0) h++;
  scrbase = (char*)OS_ReadDynamicArea(2);
  if (eigx > eigy)
  {
    ModeError.errnum = 0x0;
    sprintf((char*)ModeError.errmess,"Can't handle screen modes with eigen_x > eigen_y!");
    Wimp_ReportError(&ModeError,1,TASKNAME);
  }
  return(h);
}




// Window member-functions - handle all things concerned with windows
// WDesc is a pointer to a complete window descriptor (incl. Handle at pos 0!)
Window::Window(const int *WDesc, const char *Title)
{
  register int h;

  wind = (RO_Window *)WDesc;
  if (Title != NULL)
  {
    if ((wind->tflags & IFlg_Indir) != 0)
    {
      strcpy((char*)(wind->dat.ind.tit),Title);		// indirected
    }
    else {strncpy((char *)&wind->dat,Title,12);}
  }
  if ((wind->Handle = Wimp_CreateWindow((int*)&wind->vminx)) == 0)
  {
    _kernel_oserror WindError;

    WindError.errnum = 0x0; strcpy((char*)WindError.errmess,"Can't create window!");
    Wimp_ReportError(&WindError,1,TASKNAME);
  }
  // Isopen is unreliable. Works only if the window is never opened/closed by other
  // means than calling the member-functions. For 100% accuracy use OpenStatus().
  isopen = false;
}


Window::~Window(void)
{
  Wimp_DeleteWindow((int*)wind);
}


int  Window::MyHandle(void)
{
  return(wind->Handle);
}


void  Window::GetWorkArea(int *Dest)
{
  Dest[0] = wind->wminx; Dest[1] = wind->wminy;
  Dest[2] = wind->wmaxx; Dest[3] = wind->wmaxy;
}


void Window::open(void) {isopen = true; Wimp_OpenWindow((int*)wind);}


void Window::open(int *Block) {isopen = true; Wimp_OpenWindow(Block);}


void Window::close(void) {isopen = false; Wimp_CloseWindow((int*)wind);}


void Window::forceredraw(int minx, int miny, int maxx, int maxy)
{
  Wimp_ForceRedraw(wind->Handle,minx,miny,maxx,maxy);
}


void Window::redraw(int *Block, uint8 *Bitmap, C64Display *Disp)
{
  int more;

  more = Wimp_RedrawWindow(Block);
  while (more != 0)
  {
    RedrawAWindow(Block,Bitmap,Disp);
    more = Wimp_GetRectangle(Block);
  }
}


// Block contains the coordinates to update, the handle is entered by this
// memberfunction
void Window::update(int *Block, uint8 *Bitmap, C64Display *Disp)
{
  int more;

  Block[0] = wind->Handle;
  more = Wimp_UpdateWindow(Block);
  while (more != 0)
  {
    RedrawAWindow(Block,Bitmap,Disp);
    more = Wimp_GetRectangle(Block);
  }
}


// This updated the entire window
void Window::update(uint8 *Bitmap, C64Display *Disp)
{
  int more;
  int Block[11];

  Block[0] = wind->Handle;
  GetWorkArea(Block+1);
  more = Wimp_UpdateWindow(Block);
  while (more != 0)
  {
    RedrawAWindow(Block,Bitmap,Disp);
    more = Wimp_GetRectangle(Block);
  }
}


void Window::extent(int minx, int miny, int maxx, int maxy)
{
  int extent[4];

  extent[0] = minx; extent[1] = miny; extent[2] = maxx; extent[3] = maxy;
  Wimp_SetExtent(wind->Handle,(int*)extent);
  // update internal window info as well
  wind->wminx = minx; wind->wminy = miny;
  wind->wmaxx = maxx; wind->wmaxy = maxy;
}


void Window::getstate(void) {Wimp_GetWindowState((int*)wind);}


void Window::getstate(int *dest)
{
  dest[0] = wind->Handle;
  Wimp_GetWindowState(dest);
}


// The actual redrawing: if the bitmap pointer is not NULL the bitmap is
// painted into the window.
void Window::RedrawAWindow(int *Block, uint8 *Bitmap, C64Display *Disp)
{
  if (Bitmap != NULL)
  {
    // Plot the bitmap into the window
    graph_env ge;
    unsigned int *ct;

    // Coordinates are TOP left of rectangle (not bottom, like in RO)
    ge.x = Block[RedrawB_VMinX] - Block[RedrawB_ScrollX];
    ge.y = Block[RedrawB_VMaxY] - Block[RedrawB_ScrollY];
    ge.dimx = DISPLAY_X; ge.dimy = DISPLAY_Y;
    ct = Disp->GetColourTable();

    if (Disp->TheC64->TheWIMP->ReadEmuWindowSize() == 1)
    {
      PlotZoom1(&ge,&Block[RedrawB_CMinX],Bitmap,ct);
    }
    else
    {
      PlotZoom2(&ge,&Block[RedrawB_CMinX],Bitmap,ct);
    }
  }
}


// Returns a pointer to a window's icon (or NULL of invalid number)
RO_Icon *Window::GetIcon(unsigned int number)
{
  if (number > wind->icon_no) {return(NULL);}
  return((RO_Icon*)(((int*)wind) + RO_WINDOW_WORDS + RO_ICON_WORDS*number));
}


void Window::SetIconState(unsigned int number, unsigned int eor, unsigned int clear)
{
  int Block[4];

  Block[0] = wind->Handle; Block[1] = number; Block[2] = eor; Block[3] = clear;
  Wimp_SetIconState((int*)Block);
}


void Window::GetIconState(unsigned int number, int *Block)
{
  Block[0] = wind->Handle; Block[1] = number;
  Wimp_GetIconState(Block);
}


// Returns true if this window has the input focus
bool Window::HaveInput(void)
{
  RO_Caret Store;

  Wimp_GetCaretPosition(&Store);
  return(Store.WHandle == wind->Handle);
}


// Writes text into an indirected icon
void Window::WriteIconText(unsigned int number, const char *text)
{
  RO_Icon *ic;

  if ((ic = GetIcon(number)) != NULL)
  {
    // This only makes sense for indirected icons!
    if ((ic->iflags & IFlg_Indir) != 0)
    {
      strncpy((char*)ic->dat.ind.tit,text,ic->dat.ind.len);
      ForceIconRedraw(number);
    }
  }
}


// The same but update the window (i.e. immediate result)
void Window::WriteIconTextU(unsigned int number, const char *text)
{
  RO_Icon *ic;

  if ((ic = GetIcon(number)) != NULL)
  {
    if ((ic->iflags & IFlg_Indir) != 0)
    {
      strncpy((char*)ic->dat.ind.tit,text,ic->dat.ind.len);
      UpdateIcon(number);
    }
  }
}


// Writes the value as a decimal string into the indirected icon
void Window::WriteIconNumber(unsigned int number, int value)
{
  RO_Icon *ic;

  if ((ic = GetIcon(number)) != NULL)
  {
    if ((ic->iflags & IFlg_Indir) != 0)
    {
      ConvertInteger4(value,(char*)ic->dat.ind.tit,ic->dat.ind.len);
      ForceIconRedraw(number);
    }
  }
}


// The same but update the window
void Window::WriteIconNumberU(unsigned int number, int value)
{
  RO_Icon *ic;

  if ((ic = GetIcon(number)) != NULL)
  {
    if ((ic->iflags & IFlg_Indir) != 0)
    {
      ConvertInteger4(value,(char*)ic->dat.ind.tit,ic->dat.ind.len);
      UpdateIcon(number);
    }
  }
}


// Returns a pointer to the text in the icon
char *Window::ReadIconText(unsigned int number)
{
  RO_Icon *ic;

  if ((ic = GetIcon(number)) != NULL)
  {
    // only indirected icons!
    if ((ic->iflags & IFlg_Indir) != 0) {return((char*)ic->dat.ind.tit);}
  }
  return(NULL);
}


// Reads the number in an indirected icon.
int Window::ReadIconNumber(unsigned int number)
{
  RO_Icon *ic;

  if ((ic = GetIcon(number)) != NULL)
  {
    if ((ic->iflags & IFlg_Indir) != 0)
    {
      return(atoi((char*)ic->dat.ind.tit));
    }
  }
  return(-1);	// rather arbitrary, but we only have positive numbers here...
}


void Window::WriteTitle(const char *title)
{
  // only indirected window titles, must contain text
  if ((wind->tflags & (IFlg_Indir | IFlg_Text)) == (IFlg_Indir | IFlg_Text))
  {
    strcpy((char*)wind->dat.ind.tit,title);
    UpdateTitle();
  }
}


char *Window::ReadTitle(void)
{
  if ((wind->tflags & (IFlg_Indir | IFlg_Text)) == (IFlg_Indir | IFlg_Text))
  {
    return((char*)wind->dat.ind.tit);
  }
  else {return(NULL);}
}


// Forces a redraw of the window title
void Window::UpdateTitle(void)
{
  getstate();
  // Force global redraw (in screen-coordinates)
  Wimp_ForceRedraw(-1, wind->vminx, wind->vmaxy, wind->vmaxx, wind->vmaxy + TitleBarHeight);
}


RO_Window *Window::Descriptor(void) {return(wind);}


// Force a redraw on an icon (visible after the next Poll)
void Window::ForceIconRedraw(unsigned int number)
{
  if (number <= wind->icon_no)
  {
    register RO_Icon *ic;

    ic = GetIcon(number);
    forceredraw(ic->minx, ic->miny, ic->maxx, ic->maxy);
  }
}


// Update an icon (visible immediately -- works only on purely WIMP-drawn icons!)
void Window::UpdateIcon(unsigned int number)
{
  if (number <= wind->icon_no)
  {
    register RO_Icon *ic;
    int Block[11];	// redraw block
    int more;

    ic = GetIcon(number);
    Block[RedrawB_Handle] = wind->Handle;
    Block[RedrawB_VMinX] = ic->minx; Block[RedrawB_VMinY] = ic->miny;
    Block[RedrawB_VMaxX] = ic->maxx; Block[RedrawB_VMaxY] = ic->maxy;
    more = Wimp_UpdateWindow(Block);	// standard redraw loop
    while (more != 0)
    {
      more = Wimp_GetRectangle(Block);
    }
  }
}


// returns the current open-state of the window (true if open)
bool Window::OpenStatus(void)
{
  getstate();
  return(((wind->wflags & (1<<16)) == 0) ? false : true);
}


// Same as above, but reads the current state to Block
bool Window::OpenStatus(int *Block)
{
  getstate(Block);
  return(((Block[8] & (1<<16)) == 0) ? false : true);
}







// Icon member-functions - handle all things concerned with icons
Icon::Icon(int Priority, const RO_IconDesc *IDesc)
{
  memcpy((char*)&icon,(char*)IDesc,sizeof(RO_IconDesc));
  IHandle = Wimp_CreateIcon(Priority,IDesc);
}


Icon::~Icon(void)
{
  int blk[2];

  blk[0] = icon.WindowHandle; blk[1] = IHandle;
  Wimp_DeleteIcon((int*)blk);
}


void Icon::setstate(unsigned int eor, unsigned int clear)
{
  int blk[4];

  blk[0] = icon.WindowHandle; blk[1] = IHandle; blk[2] = eor; blk[3] = clear;
  Wimp_SetIconState((int*)blk);
}


void Icon::getstate(void)
{
  int blk[10];

  blk[0] = icon.WindowHandle; blk[1] = IHandle;
  Wimp_GetIconState((int*)blk);
  memcpy((char*)&icon,(int*)&blk[2],sizeof(RO_Icon));
}




// Frodo member functions
Frodo::Frodo(void) {TheC64 = NULL;}


Frodo::~Frodo(void)
{
  if (TheC64 != NULL) {delete TheC64;}
}


void Frodo::ReadyToRun(void)
{
  ThePrefs.Load(DEFAULT_PREFS);
  TheC64 = new C64;
  load_rom_files();
  TheC64->Run();
  delete TheC64; TheC64 = NULL;
}





extern void (*__new_handler)(void);

// Out of memory
void OutOfMemory(void)
{
  _kernel_oserror err;


  err.errnum = 0; sprintf(err.errmess,"Out of memory error! Aborting.");
  Wimp_ReportError(&err,1,TASKNAME);
  delete the_app;
  Wimp_CloseDown(TaskHandle,TASK_WORD);
  exit(1);
}




// Frodo main
int main(int argc, char *argv[])
{
#ifdef __GNUG__
  // Switch off filename conversions in UnixLib:
  //__uname_control = __UNAME_NO_PROCESS;
#endif

#ifdef FRODO_SC
  TaskHandle = Wimp_Initialise(310,TASK_WORD,"FrodoSC",(int*)WimpMessages);
#else
# ifdef FRODO_PC
  TaskHandle = Wimp_Initialise(310,TASK_WORD,"FrodoPC",(int*)WimpMessages);
# else
  TaskHandle = Wimp_Initialise(310,TASK_WORD,"Frodo",(int*)WimpMessages);
# endif
#endif

  // Install handler for failed new
  __new_handler = OutOfMemory;

  the_app = new Frodo();
  the_app->ReadyToRun();
  // Clean up directory scrap file
  DeleteFile(RO_TEMPFILE);
  delete the_app;
  Wimp_CloseDown(TaskHandle,TASK_WORD);
  return(0);
}
