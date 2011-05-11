/*
 *  Display_GP2X.h - C64 graphics display, emulator window handling,
 *                  gp2x specific stuff, adapted from Display_SDL.h
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
//#define EXTRA_DEBUG

#define TRACE(A) {std::cout<<std::dec<<__func__<<" "<<__FILE__<<" ("<<__LINE__<<"): "<<A<<std::endl; }

#ifdef EXTRA_DEBUG
#define EXTRACE(A) TRACE(A)
#else
#define EXTRACE(A) {}
#endif


#include "C64.h"
#include "SAM.h"
#include "Version.h"

#include <sys/mman.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <string>
#include <map>
#include <fstream>
#include <iostream>

int memdev;
unsigned short *gp2x_memregs;
unsigned long gp2x_dev[2], gp2x_physvram[2];
unsigned short *gp2x_logvram[2];

int stats_enabled=0;

int volume=0x5050;
int volume_key_pressed;
int mixer_fd;

uint8 *thumb_current=NULL;
uint8 *thumb_snap=NULL;
char thumb_snap_name[255]="";

int start_time;
int num_frames;
char speedometer_string[16];

#include "c64loader.h"

#if defined(__zipfile__)
#include <zip.h>
#endif

#define MENU_HEIGHT 25

uint8 sdl_joy_state;
uint8 last_sdl_joy_state;
int sdl_joy_state_counter;

bool sdl_prefs_open = false;
bool prefs_button_state = false;
bool last_prefs_button_state = false;
bool prefs_button_deselected;

bool vkeyb_open = false;
bool vkeyb_button_state = false;
bool last_vkeyb_button_state = false;
bool vkeyb_button_deselected;
int vkey_pressed=0;
int vkey_released=0;
int vkey=0;
int gotkeypress=0;
int rshoulder_released;

bool fire_deselected;


bool scaled=0;
bool tvout=0;
bool tvout_pal=0;
char brightness;
int contrast;

struct dir_item {
	char *name;
	int type; // 0=dir, 1=file, 2=zip archive
};

uint8 *sdl_palette;
uint16 palette16[256];

uint8 *screen_base;

// Keyboard
static bool num_locked = false;

// For LED error blinking
static C64Display *c64_disp;
static struct sigaction pulse_sa;
static itimerval pulse_tv;

// joysticks
int joy[2]={true, false};

// Colors for speedometer/drive LEDs
enum {
	black = 0,
	white = 1,
	yellow = 7,
	fill_gray = 1,
	shine_gray = 1,
	shadow_gray = 1,
	red = 2,
	green = 3,
};

/*
  C64 keyboard matrix:

    Bit 7   6   5   4   3   2   1   0
  0    CUD  F5  F3  F1  F7 CLR RET DEL
  1    SHL  E   S   Z   4   A   W   3
  2     X   T   F   C   6   D   R   5
  3     V   U   H   B   8   G   Y   7
  4     N   O   K   M   0   J   I   9
  5     ,   @   :   .   -   L   P   +
  6     /   ^   =  SHR HOM  ;   *   £
  7    R/S  Q   C= SPC  2  CTL  <-  1
*/

#define MATRIX(a,b) (((a) << 3) | (b))
 
/*
  GP2x joystick library by rlyeh, 2005.
  Thanks to Squidge for his previous work! :-)
*/
 
#define GP2X_UP            (0x01)
#define GP2X_LEFT          (0x04)
#define GP2X_DOWN          (0x10)
#define GP2X_RIGHT         (0x40)
#define GP2X_START         (1<<8)
#define GP2X_SELECT        (1<<9)
#define GP2X_L             (1<<10)
#define GP2X_R             (1<<11)
#define GP2X_A             (1<<12)
#define GP2X_B             (1<<13)
#define GP2X_X             (1<<14)
#define GP2X_Y             (1<<15)
#define GP2X_VOLM          (1<<22)
#define GP2X_VOLP          (1<<23)
#define GP2X_PUSH          (1<<27)
 
unsigned long gp2x_read_joystick(void) {
	unsigned long value=(gp2x_memregs[0x1198>>1] & 0x00FF);

	if(value==0xFD) value=0xFA;
	if(value==0xF7) value=0xEB;
	if(value==0xDF) value=0xAF;
	if(value==0x7F) value=0xBE;

	return ~((gp2x_memregs[0x1184>>1] & 0xFF00) | value | (gp2x_memregs[0x1186>>1] << 16));
}

#define SCALE_KEY    0x00000100
#define KEYB_KEY     0x00000200
#define PREF_KEY     0x00000400
#define VOL_UP_KEY   0x00000800
#define VOL_DOWN_KEY 0x00001000
#define JOY1_UP      0x00010000
#define JOY1_DOWN    0x00020000
#define JOY1_LEFT    0x00040000
#define JOY1_RIGHT   0x00080000
#define JOY1_FIRE    0x00100000 
#define JOY2_UP      0x00200000
#define JOY2_DOWN    0x00400000
#define JOY2_LEFT    0x00800000
#define JOY2_RIGHT   0x01000000
#define JOY2_FIRE    0x02000000
#define MENU_UP      0x04000000
#define MENU_DOWN    0x08000000
#define MENU_LEFT    0x10000000
#define MENU_RIGHT   0x20000000
#define MENU_SELECT  0x40000000
#define NO_KEY       0xFFFFFFFF




class GP2XButtonHandler
{
	public:
		
		struct ButtonSet
		{
			std::string name;
			struct ButtonMapping
			{
				unsigned int special;
				unsigned int key;
				ButtonMapping(unsigned int special=0,unsigned int key=NO_KEY):
					special(special),key(key)
				{
				}
			};
			typedef std::map<int,ButtonMapping> Keys;
			Keys keys;
		};
		
		
		enum GP2XButton
		{
			up,left,down,right,start,select,l,r,a,b,x,y,volleft,volright,stick,count
		};
		
		class KeyState
		{
			public:
				unsigned long key;
				bool state;
				bool pressed;
				bool released;
				bool newState;
				
				KeyState(unsigned long key):
					key(key),state(false),pressed(false),released(false),newState(false)
				{
				}
		};
		
		std::vector<ButtonSet> buttonSets;
		int currentButtonSet;
		
		std::vector<KeyState> keys;
		
		unsigned long specialState;
		unsigned long specialPressed;
		unsigned long specialReleased;
		
		unsigned long specialMap[count];
		unsigned long keyMap[count];

		unsigned long buttonBits[count];
		
		GP2XButtonHandler()
		{
			buttonBits[up]=GP2X_UP;
			buttonBits[left]=GP2X_LEFT;
			buttonBits[down]=GP2X_DOWN;
			buttonBits[right]=GP2X_RIGHT;
			buttonBits[start]=GP2X_START;
			buttonBits[select]=GP2X_SELECT;
			buttonBits[l]=GP2X_L;
			buttonBits[r]=GP2X_R;
			buttonBits[a]=GP2X_A;
			buttonBits[b]=GP2X_B;
			buttonBits[x]=GP2X_X;
			buttonBits[y]=GP2X_Y;
			buttonBits[volleft]=GP2X_VOLM;
			buttonBits[volright]=GP2X_VOLP;
			buttonBits[stick]=GP2X_PUSH;
			
			{
				ButtonSet bs;
				bs.name="Default";
				bs.keys[up]=ButtonSet::ButtonMapping(JOY2_UP|MENU_UP);
				bs.keys[left]=ButtonSet::ButtonMapping(JOY2_LEFT|MENU_LEFT);
				bs.keys[down]=ButtonSet::ButtonMapping(JOY2_DOWN|MENU_DOWN);
				bs.keys[right]=ButtonSet::ButtonMapping(JOY2_RIGHT|MENU_RIGHT);
				bs.keys[select]=ButtonSet::ButtonMapping(PREF_KEY);
				bs.keys[l]=ButtonSet::ButtonMapping(KEYB_KEY);
				bs.keys[r]=ButtonSet::ButtonMapping(SCALE_KEY);
				bs.keys[b]=ButtonSet::ButtonMapping(JOY2_FIRE|MENU_SELECT);
				bs.keys[x]=ButtonSet::ButtonMapping(0,MATRIX(7,4)); // space
				bs.keys[volleft]=ButtonSet::ButtonMapping(VOL_DOWN_KEY);
				bs.keys[volright]=ButtonSet::ButtonMapping(VOL_UP_KEY);
				bs.keys[y]=ButtonSet::ButtonMapping(JOY2_UP);
				bs.keys[start]=ButtonSet::ButtonMapping(KEYB_KEY);
				bs.keys[stick]=ButtonSet::ButtonMapping(JOY2_FIRE);
				buttonSets.push_back(bs);
			}
			parseFile("./frodogp2x.key");
			currentButtonSet=0;
			mapButtonSet(buttonSets[currentButtonSet]);
		}

		static std::string toLower(const std::string &in)
		{
			std::string ret(in.length(),0);
			for (unsigned i=0;i<in.length();i++)
				ret[i]=std::tolower(in[i]);
			return ret;
		}
		
		static std::string strip(std::string in)
		{
			unsigned i;
			for (i=0;i<in.length();++i)
				if (in[i]!=' ' && in[i]!='\t')
					break;
			if (i!=0)
				in.erase(0,i);
			for (i=in.length();i>0;--i)
				if (in[i-1]!=' ' && in[i-1]!='\t')
					break;
			if (i!=in.length())
				in.erase(i);
			return in;
		}
		
		static std::vector<std::string> split(std::string in,char seperator)
		{
			std::vector<std::string> res;
			for (;;)
			{
				unsigned i;
				for (i=0;i<in.length();++i)
					if (in[i]==seperator)
						break;
				if (i==in.length())
					break;
				res.push_back(in.substr(0,i));
				in.erase(0,i+1);
			}
			res.push_back(in);
			return res;
		}
		
		void parseFile(std::string name)
		{
			typedef std::map<std::string,unsigned long>  Map;
			Map keyNames;
			keyNames["delete"]=0x00;
			keyNames["return"]=0x01;
			keyNames["cursor left"]=0x02;
			keyNames["f7"]=0x03;
			keyNames["f1"]=0x04;
			keyNames["f3"]=0x05;
			keyNames["f5"]=0x06;
			keyNames["cursor up"]=0x07;
			keyNames["3"]=0x08;
			keyNames["w"]=0x09;
			keyNames["a"]=0x0a;
			keyNames["4"]=0x0b;
			keyNames["z"]=0x0c;
			keyNames["s"]=0x0d;
			keyNames["e"]=0x0e;
			keyNames["left shift"]=0x0f;
			keyNames["5"]=0x10;
			keyNames["r"]=0x11;
			keyNames["d"]=0x12;
			keyNames["6"]=0x13;
			keyNames["c"]=0x14;
			keyNames["f"]=0x15;
			keyNames["t"]=0x16;
			keyNames["x"]=0x17;
			keyNames["7"]=0x18;
			keyNames["y"]=0x19;
			keyNames["g"]=0x1a;
			keyNames["8"]=0x1b;
			keyNames["b"]=0x1c;
			keyNames["h"]=0x1d;
			keyNames["u"]=0x1e;
			keyNames["v"]=0x1f;
			keyNames["9"]=0x20;
			keyNames["i"]=0x21;
			keyNames["j"]=0x22;
			keyNames["0"]=0x23;
			keyNames["m"]=0x24;
			keyNames["k"]=0x25;
			keyNames["o"]=0x26;
			keyNames["n"]=0x27;
			keyNames["plus"]=0x28;
			keyNames["p"]=0x29;
			keyNames["l"]=0x2a;
			keyNames["minus"]=0x2b;
			keyNames["period"]=0x2c;
			keyNames["colon"]=0x2d;
			keyNames["at"]=0x2e;
			keyNames["comma"]=0x2f;
			keyNames["pound"]=0x30;
			keyNames["asterisk"]=0x31;
			keyNames["semicolon"]=0x32;
			keyNames["clear"]=0x33;
			keyNames["right shift"]=0x34;
			keyNames["equal"]=0x35;
			keyNames["up arrow"]=0x36;
			keyNames["slash"]=0x37;
			keyNames["1"]=0x38;
			keyNames["left arrow"]=0x39;
			keyNames["control"]=0x3a;
			keyNames["2"]=0x3b;
			keyNames["space"]=0x3c;
			keyNames["commodore"]=0x3d;
			keyNames["q"]=0x3e;
			keyNames["run"]=0x3f;
			Map specialNames;
			specialNames["scale"]=SCALE_KEY;
			specialNames["keyboard"]=KEYB_KEY;
			specialNames["menu"]=PREF_KEY;
			specialNames["volume up"]=VOL_UP_KEY;
			specialNames["volume down"]=VOL_DOWN_KEY;
			specialNames["joy1 up"]=JOY1_UP;
			specialNames["joy1 down"]=JOY1_DOWN;
			specialNames["joy1 left"]=JOY1_LEFT;
			specialNames["joy1 right"]=JOY1_RIGHT;
			specialNames["joy1 fire"]=JOY1_FIRE;
			specialNames["joy2 up"]=JOY2_UP;
			specialNames["joy2 down"]=JOY2_DOWN;
			specialNames["joy2 left"]=JOY2_LEFT;
			specialNames["joy2 right"]=JOY2_RIGHT;
			specialNames["joy2 fire"]=JOY2_FIRE;
			specialNames["menu up"]=MENU_UP;
			specialNames["menu down"]=MENU_DOWN;
			specialNames["menu left"]=MENU_LEFT;
			specialNames["menu right"]=MENU_RIGHT;
			specialNames["menu select"]=MENU_SELECT;
			Map buttonNames;
			buttonNames["stick up"]=up;
			buttonNames["stick down"]=down;
			buttonNames["stick left"]=left;
			buttonNames["stick right"]=right;
			buttonNames["start"]=start;
			buttonNames["select"]=select;
			buttonNames["left shoulder"]=l;
			buttonNames["right shoulder"]=r;
			buttonNames["a"]=a;
			buttonNames["b"]=b;
			buttonNames["x"]=x;
			buttonNames["y"]=y;
			buttonNames["volume right"]=volright;
			buttonNames["volume left"]=volleft;
			buttonNames["stick push"]=stick;
			std::ifstream ifs(name.c_str());
			if (!ifs)
			{
				std::cout<<"Unable to open keymap file "<<name<<std::endl;
				std::cout<<"Trying to create keymap file "<<name<<std::endl;
				std::ofstream ofs(name.c_str());
				if (!ofs)
				{
					std::cout<<"Unable to create keymap file "<<name<<std::endl;
					return;
				}
				ofs<<"# Keymap file for FrodoGP2X"<<std::endl;
				ofs<<"# Syntax:"<<std::endl;
				ofs<<"# keymap=<keymap name>"<<std::endl;
				ofs<<"# <button>'='<key>|<function>':'<function>..."<<std::endl;
				ofs<<"#"<<std::endl;				
				ofs<<"# Where button is one of the following:"<<std::endl;
				ofs<<"#";
				int j=0;
				for (Map::iterator i=buttonNames.begin();i!=buttonNames.end();++i,++j)
				{
					ofs<<" \""<<i->first<<"\"";
					if (j==8)
					{
						ofs<<std::endl<<"#";
						j=0;
					}
				}
				ofs<<std::endl<<"#"<<std::endl;
				ofs<<"# Where key is one of the following:"<<std::endl;
				ofs<<"#";
				j=0;
				for (Map::iterator i=keyNames.begin();i!=keyNames.end();++i,++j)
				{
					ofs<<" \""<<i->first<<"\"";
					if (j==8)
					{
						ofs<<std::endl<<"#";
						j=0;
					}
				}
				ofs<<std::endl<<"#"<<std::endl;
				ofs<<"# Where function is one of the following:"<<std::endl;
				ofs<<"#";
				j=0;
				for (Map::iterator i=specialNames.begin();i!=specialNames.end();++i,++j)
				{
					ofs<<" \""<<i->first<<"\"";
					if (j==8)
					{
						ofs<<std::endl<<"#";
						j=0;
					}
				}
				ofs<<std::endl<<"#"<<std::endl;
				ifs.open(name.c_str());
				if (!ifs)
				{
					std::cerr<<"Unable to open keymap file "<<name<<std::endl<<"Only built in keymap available"<<std::endl;
					return;
				}
			}
			ButtonSet bs;
			bs.name="Custom";
			while (ifs)
			{
				std::string s;
				getline(ifs,s);
				std::vector<std::string> vec;
				s=strip(s);
				if (s.empty())
					continue;
				if (s[0]=='#')
					continue;
				vec=split(s,'=');
				if (vec.size()!=2)
				{
					std::cerr<<"Error reading line \""<<s<<"\" from keymap file "<<name<<std::endl;
					continue;
				}
				std::string buttonName=strip(toLower(vec[0]));
				unsigned long key=NO_KEY;
				unsigned long special=0;
				unsigned long button;
				if (buttonName=="keymap")
				{
					if (!bs.keys.empty())
					{
						buttonSets.push_back(bs);
						TRACE("Added keymap "<<bs.name);
					}
					bs=ButtonSet();
					bs.name=vec[1];
					EXTRACE("Started reading keymap "<<bs.name);
				}
				else if (buttonNames.find(buttonName)!=buttonNames.end())
				{
					button=buttonNames[buttonName];
					vec=split(toLower(vec[1]),':');
					for (unsigned i=0;i<vec.size();++i)
					{
						std::string keyName=strip(vec[i]);
						if (keyNames.find(keyName)!=keyNames.end())
							key=keyNames[keyName];
						else if (specialNames.find(keyName)!=specialNames.end())
							special|=specialNames[keyName];
						else
						{
							std::cerr<<"Not a valid key/function "<<keyName<<" in line \""<<s<<"\" from keymap file "<<name<<std::endl;
						}
					}
					bs.keys[button]=ButtonSet::ButtonMapping(special,key);
					EXTRACE(std::hex<<"Added special 0x"<<special<<" and key 0x"<<key<<" to button 0x"<<button);
				}
				else
				{
					std::cerr<<"Not a valid GP2X button "<<buttonName<<" in line \""<<s<<"\" from keymap file "<<name<<std::endl;
				}	
			}
			if (!bs.keys.empty())
			{
				buttonSets.push_back(bs);
				TRACE("Added final keymap "<<bs.name);
			}
		}
		
		void reset()
		{
			keys.clear();
			for (int i=0;i<count;++i)
			{
				keyMap[i]=NO_KEY;
				specialMap[i]=0;
			}
			specialState=0;
			specialPressed=0;
			specialReleased=0;
		}
		
		void mapButton(int button,unsigned long special,unsigned long key=NO_KEY)
		{
			specialMap[button]=special;
			if (key!=NO_KEY)
			{
				for (int i=0;i<keys.size();++i)
					if (keys[i].key==key)
					{
						keyMap[button]=i;
						return;
					}
				keyMap[button]=keys.size();
				keys.push_back(KeyState(key));
			}
		}
		
		std::string buttonSetName()
		{
			return buttonSets[currentButtonSet].name;
		}

		void nextButtonSet()
		{
			EXTRACE("");
			currentButtonSet++;
			if (currentButtonSet>=buttonSets.size())
				currentButtonSet=buttonSets.size()-1;
			mapButtonSet(buttonSets[currentButtonSet]);
			EXTRACE("");
		}	
		
		void prevButtonSet()
		{
			EXTRACE("");
			currentButtonSet--;
			if (currentButtonSet<0)
				currentButtonSet=0;
			mapButtonSet(buttonSets[currentButtonSet]);
			EXTRACE("");
		}	
		
		void mapButtonSet(const ButtonSet &bs)
		{
			EXTRACE(bs.name);
			reset();
			for (ButtonSet::Keys::const_iterator i=bs.keys.begin();i!=bs.keys.end();++i)
				mapButton(i->first,i->second.special,i->second.key);
			EXTRACE("");
		}
		
		void readGP2XButtons(unsigned long buttonMask)
		{
			EXTRACE("");
			unsigned long oldSpecialState=specialState;
			specialState=0;
			for (int i=keys.size();i>0;--i)
			{
				keys[i-1].pressed=false;
				keys[i-1].released=false;
				keys[i-1].newState=false;
			}
			EXTRACE("");
			
			for (int i=0;i<count;++i)
			{
				
				if ((buttonBits[i]&buttonMask)!=0)
				{
					specialState|=specialMap[i];
					if (keyMap[i]!=NO_KEY)
					{ 
						KeyState &state=keys[keyMap[i]];
						if (!state.state)
						{
							state.pressed=true;
							state.released=false;
						}
						state.state=true;
						state.newState=true;
					}
				}
				else if (keyMap[i]!=NO_KEY)
				{
					KeyState &state=keys[keyMap[i]];
					if (state.state && !state.newState)
					{
						state.state=false;
						state.released=true;
					}
				}		
			}
			unsigned long specialChanged=specialState^oldSpecialState;
			specialPressed=specialState&specialChanged;
			specialReleased=oldSpecialState&specialChanged;
			EXTRACE("");
		}
		
		
} gp2xButtonHandler;



void set_scaling(void) {
	if(scaled) {
		if(tvout) {
			gp2x_memregs[0x2906>>1]=614; /* scale horizontal */
			if(tvout_pal) gp2x_memregs[0x2908>>1]=360; /* scale vertical PAL */
			else gp2x_memregs[0x2908>>1]=460; /* scale vertical NTSC */
		} else {
			gp2x_memregs[0x2906>>1]=1224; // scale horizontal
			gp2x_memregs[0x2908>>1]=DISPLAY_X+50; // vertical
		}
	} else {
		gp2x_memregs[0x2906>>1]=1024; // scale horizontal
		gp2x_memregs[0x2908>>1]=DISPLAY_X; // vertical
	}
}

void sync_buffers(void) {
	unsigned long *fb0=(unsigned long *)gp2x_logvram[0];
	unsigned long *fb1=(unsigned long *)gp2x_logvram[1];
	for(int i=0; i<((DISPLAY_X/4)*DISPLAY_Y); i++) fb1[i]=fb0[i];
}

void double_buffer(void) {
	unsigned short *fb=gp2x_logvram[0];
	gp2x_logvram[0]=gp2x_logvram[1];
	gp2x_logvram[1]=fb;
	screen_base=(uint8 *)gp2x_logvram[1];

	unsigned long address=gp2x_physvram[1];
	gp2x_physvram[1]=gp2x_physvram[0];
	gp2x_physvram[0]=address;

	if(!scaled) address+=(DISPLAY_X*16)+32;

	gp2x_memregs[0x290E>>1]=(unsigned short)(address & 0xffff);
	gp2x_memregs[0x2910>>1]=(unsigned short)(address >> 16);
	gp2x_memregs[0x2912>>1]=(unsigned short)(address & 0xffff);
	gp2x_memregs[0x2914>>1]=(unsigned short)(address >> 16);
} 

void set_luma(char b, int c) {
	gp2x_memregs[0x2934]=((c&0x07)<<8)|b;
	brightness=gp2x_memregs[0x2934]&0x0ff;
	contrast=gp2x_memregs[0x2934]>>8;
}

/*
 *  Open window
 */

int init_graphics(void)
{
	// init memdev
	memdev = open("/dev/mem", O_RDWR);
	gp2x_memregs=(unsigned short *)mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, memdev, 0xc0000000);

	// set bit depth 8
	gp2x_memregs[0x28da>>1]=0x02ab;
	
	// set hwidth
	gp2x_memregs[0x290c>>1]=DISPLAY_X;

	// get brightness/contrast
	brightness=gp2x_memregs[0x2934]&0x0ff;
	contrast=gp2x_memregs[0x2934]>>8;

	return 1;
}


/*
 *  Display constructor
 */

C64Display::C64Display(C64 *the_c64) : TheC64(the_c64)
{
	quit_requested = false;

	start_time=(int)time(NULL);

	gp2x_dev[0] = open("/dev/fb0", O_RDWR);
	gp2x_dev[1] = open("/dev/fb1", O_RDWR);

	struct fb_fix_screeninfo fixed_info;

	ioctl (gp2x_dev[0], FBIOGET_FSCREENINFO, &fixed_info);
	gp2x_logvram[0]=(unsigned short *)mmap(0, 320*240*2, PROT_WRITE, MAP_SHARED, gp2x_dev[0], 0);
	gp2x_physvram[0]=fixed_info.smem_start;

	ioctl (gp2x_dev[1], FBIOGET_FSCREENINFO, &fixed_info);
	gp2x_logvram[1]=(unsigned short *)mmap(0, 320*240*2, PROT_WRITE, MAP_SHARED, gp2x_dev[1], 0);
	gp2x_physvram[1]=fixed_info.smem_start;

	double_buffer();

	// tv out
	if(gp2x_memregs[0x2800>>1]&0x100) {
		tvout=1;
		tvout_pal=1;
		scaled=1;
		set_scaling();
	}

	// volume mixer
	mixer_fd=open("/dev/mixer", O_RDONLY);
	ioctl(mixer_fd, SOUND_MIXER_WRITE_PCM, &volume);

	// LEDs off
	for (int i=0; i<4; i++)
		led_state[i] = old_led_state[i] = LED_OFF;

	// Start timer for LED error blinking
	c64_disp = this;
	pulse_sa.sa_handler = (void (*)(int))pulse_handler;
	pulse_sa.sa_flags = SA_RESTART;
	sigemptyset(&pulse_sa.sa_mask);
	sigaction(SIGALRM, &pulse_sa, NULL);
	pulse_tv.it_interval.tv_sec = 0;
	pulse_tv.it_interval.tv_usec = 400000;
	pulse_tv.it_value.tv_sec = 0;
	pulse_tv.it_value.tv_usec = 400000;
	setitimer(ITIMER_REAL, &pulse_tv, NULL);
}


/*
 *  Display destructor
 */

C64Display::~C64Display()
{
}

/*
 *  Prefs may have changed
 */

void C64Display::NewPrefs(Prefs *prefs)
{
}

void C64Display::Draw_VKeyb(C64 *TheC64)
{
	if(!vkeyb_button_state) vkeyb_button_deselected=1;
	if(vkeyb_button_state && vkeyb_button_deselected) {
		vkeyb_open=false;
		vkeyb_button_deselected=0;
		return;
	}

	int i;
	static int cursor_x=15;
	static int cursor_y=20;
	static bool shifted, ctrld=false;

#define VKEYB_HEIGHT 8
#define VKEYB_WIDTH 29
	static int vkeyb_x=120;
	static int vkeyb_y=160;

		//" =---=====================*= ",
		//"                             ",
		//"    x 1234567890+-xc del  F1 ",
		//"  ctrl QWERTYUIOP@*x rstr F3 ",
		//" r/st x ASDFGHJKL:;= rtrn F5 ",
		//" c= sh   ZXCVBNM,./ sh .  F7 ",
		//"          space       ...    ",
		//"                             "
	char vkeyb[VKEYB_HEIGHT][VKEYB_WIDTH+1]={
		" =---=====================*= ",
		"                             ",
		"    \x1f 1234567890+-\x1ch del  F1 ",
		"  ctrl QWERTYUIOP@*\x1e rstr F3 ",
		" r/s sh ASDFGHJKL:;= rtrn F5 ",
		" c=  sh  ZXCVBNM,./ sh .  F7 ",
		"          space       ...    ",
		"                             "
	};

	for(i=0; i<VKEYB_HEIGHT; i++) {
        	draw_ascii_string(screen_base, vkeyb_x, vkeyb_y+(i*8), vkeyb[i], 1, 9);
	}

	// shift key
	if(shifted) {
        	draw_ascii_string(screen_base, vkeyb_x+(8*5), vkeyb_y+(8*4), "sh", yellow, 9);
        	draw_ascii_string(screen_base, vkeyb_x+(8*5), vkeyb_y+(8*5), "sh", yellow, 9);
        	draw_ascii_string(screen_base, vkeyb_x+(8*20), vkeyb_y+(8*5), "sh", yellow, 9);
	}
	// ctrl key
	if(ctrld) {
        	draw_ascii_string(screen_base, vkeyb_x+(8*2), vkeyb_y+(8*3), "ctrl", yellow, 9);
	}

	// power led :)
        draw_ascii_string(screen_base, vkeyb_x+(8*26), vkeyb_y, "*", red, 9);

	// cursor
	draw_ascii_string(screen_base, 8*cursor_x, 8*cursor_y, "+", 7, 7);

	if(sdl_joy_state_counter>2) {
		sdl_joy_state_counter=0;
		if(!(sdl_joy_state&0x02)) { //down
			if(!(sdl_joy_state&0x10)&&cursor_y==vkeyb_y/8) {
				if(vkeyb_y+(VKEYB_HEIGHT*8)<DISPLAY_Y) {
					vkeyb_y+=8;
					cursor_y++;
				}
			} else {
				if(cursor_y<(DISPLAY_Y/8)-1) cursor_y++;
			}
		} else if(!(sdl_joy_state&0x01)) { // up
			if(!(sdl_joy_state&0x10)&&cursor_y==vkeyb_y/8) {
				if(vkeyb_y>0) {
					vkeyb_y-=8;
					cursor_y--;
				}
			} else {
				if(cursor_y>0) cursor_y--;
			}
		} 
		if(!(sdl_joy_state&0x04)) { // left
			if(!(sdl_joy_state&0x10)&&cursor_y==vkeyb_y/8) {
				if(vkeyb_x>0) {
					vkeyb_x-=8;
					cursor_x--;
				}
			} else {
				if(cursor_x>0) cursor_x--;
			}
		} else if(!(sdl_joy_state&0x08)) { // right
			if(!(sdl_joy_state&0x10)&&cursor_y==vkeyb_y/8) {
				if(vkeyb_x+(VKEYB_WIDTH*8)<DISPLAY_X) {
					vkeyb_x+=8;
					cursor_x++;
				}
			} else {
				if(cursor_x<(DISPLAY_X/8)-1) cursor_x++;
			}
		}
	}

	int keytable[] = {
		//x, y, key value
		4, 2, MATRIX(7,1), //<-
		6, 2, MATRIX(7,0), //1
		7, 2, MATRIX(7,3), //2
		8, 2, MATRIX(1,0), //3
		9, 2, MATRIX(1,3), //4
		10, 2, MATRIX(2,0), //5
		11, 2, MATRIX(2,3), //6
		12, 2, MATRIX(3,0), //7
		13, 2, MATRIX(3,3), //8
		14, 2, MATRIX(4,0), //9
		15, 2, MATRIX(4,3), //0
		16, 2, MATRIX(5,0), //+
		17, 2, MATRIX(5,3), //-
		18, 2, MATRIX(6,0), //£
		19, 2, MATRIX(6,3), //c/h

		7, 3, MATRIX(7,6), //q
		8, 3, MATRIX(1,1), //w
		9, 3, MATRIX(1,6), //e
		10, 3, MATRIX(2,1), //r
		11, 3, MATRIX(2,6), //t
		12, 3, MATRIX(3,1), //y
		13, 3, MATRIX(3,6), //u
		14, 3, MATRIX(4,1), //i
		15, 3, MATRIX(4,6), //o
		16, 3, MATRIX(5,1), //p
		17, 3, MATRIX(5,6), //@
		18, 3, MATRIX(6,1), //*
		19, 3, MATRIX(6,6), //up arrow

		8, 4, MATRIX(1,2), //a
		9, 4, MATRIX(1,5), //s
		10, 4, MATRIX(2,2), //d
		11, 4, MATRIX(2,5), //f
		12, 4, MATRIX(3,2), //g
		13, 4, MATRIX(3,5), //h
		14, 4, MATRIX(4,2), //j
		15, 4, MATRIX(4,5), //k
		16, 4, MATRIX(5,2), //l
		17, 4, MATRIX(5,5), //:
		18, 4, MATRIX(6,2), //;
		19, 4, MATRIX(6,5), //=

		1, 5, MATRIX(7,5), //c=
		2, 5, MATRIX(7,5), //c=
		9, 5, MATRIX(1,4), //z
		10, 5, MATRIX(2,7), //x
		11, 5, MATRIX(2,4), //c
		12, 5, MATRIX(3,7), //v
		13, 5, MATRIX(3,4), //b
		14, 5, MATRIX(4,7), //n
		15, 5, MATRIX(4,4), //m
		16, 5, MATRIX(5,7), //,
		17, 5, MATRIX(5,4), //.
		18, 5, MATRIX(6,7), ///

		2, 6, MATRIX(7,5), //c=

		1, 4, MATRIX(7,7), //run/stop
		2, 4, MATRIX(7,7), //
		3, 4, MATRIX(7,7), //

		10,6, MATRIX(7,4), // space
		11,6, MATRIX(7,4), // space
		12,6, MATRIX(7,4), // space
		13,6, MATRIX(7,4), // space
		14,6, MATRIX(7,4), // space

		21, 4, MATRIX(0,1), //return
		22, 4, MATRIX(0,1), //return
		23, 4, MATRIX(0,1), //return
		24, 4, MATRIX(0,1), //return

		23, 5, (MATRIX(0,7))|128, //crsr up
		23, 6, MATRIX(0,7), //crsr down
		22, 6, (MATRIX(0,2))|128, //crsr left
		24, 6, MATRIX(0,2), //crsr right

		21, 2, MATRIX(0,0), //delete
		22, 2, MATRIX(0,0), //delete
		23, 2, MATRIX(0,0), //delete

		26, 2, MATRIX(0,4), //f1
		27, 2, MATRIX(0,4), //f1
		26, 3, MATRIX(0,5), //f3
		27, 3, MATRIX(0,5), //f3
		26, 4, MATRIX(0,6), //f5
		27, 4, MATRIX(0,6), //f5
		26, 5, MATRIX(0,3), //f7
		27, 5, MATRIX(0,3), //f7

		0
	};

	if(sdl_joy_state&0x10) fire_deselected=true;
	if(sdl_joy_state&0x10 && gotkeypress) {
		if(//vkey!=MATRIX(7,7) && 
			vkey!=MATRIX(7,2)) {
			gotkeypress=0;
			vkey_pressed=0;
			vkey_released=1;
		}
	} else if(fire_deselected && !(sdl_joy_state&0x10)) { // fire
		fire_deselected=false;

		if(cursor_x==(vkeyb_x/8)+5&&cursor_y==(vkeyb_y/8)+4) shifted=!shifted;
		else if(cursor_x==(vkeyb_x/8)+5&&cursor_y==(vkeyb_y/8)+5) shifted=!shifted;
		else if(cursor_x==(vkeyb_x/8)+6&&cursor_y==(vkeyb_y/8)+4) shifted=!shifted;
		else if(cursor_x==(vkeyb_x/8)+6&&cursor_y==(vkeyb_y/8)+5) shifted=!shifted;
		else if(cursor_x==(vkeyb_x/8)+20&&cursor_y==(vkeyb_y/8)+5) shifted=!shifted;
		else if(cursor_x==(vkeyb_x/8)+21&&cursor_y==(vkeyb_y/8)+5) shifted=!shifted;

		// ctrl key
		else if(cursor_y==(vkeyb_y/8)+3
			&&cursor_x>=(vkeyb_x/8)+2
			&&cursor_x<=(vkeyb_x/8)+5) {
			if(ctrld==0) {
				ctrld=1;
				vkey=MATRIX(7,2);
				vkey_pressed=1;
				gotkeypress=1;
			} else {
				ctrld=0;
				vkey=MATRIX(7,2);
				vkey_pressed=0;
				gotkeypress=0;
				vkey_released=1;
			}
		}
		// restore key
		  else if(cursor_y==(vkeyb_y/8)+3
				&&cursor_x>=(vkeyb_x/8)+21
				&&cursor_x<=(vkeyb_x/8)+24) {
			TheC64->NMI();
		}

		for(i=0; keytable[i]; i+=3) {
			if((cursor_x-(vkeyb_x/8)==keytable[i]) && (cursor_y-(vkeyb_y/8)==keytable[i+1])) {
				vkey=keytable[i+2];
				if(shifted) vkey|=128;
				vkey_pressed=1;
				gotkeypress=1;
				break;
			}
		}
	}

}

void load_prg(C64 *TheC64, uint8 *prg, int prg_size) {
	uint8 start_hi, start_lo;
	uint16 start;
	int i;

	start_lo=*prg++;
	start_hi=*prg++;
	start=(start_hi<<8)+start_lo;

	for(i=0; i<(prg_size-2); i++) {
		TheC64->RAM[start+i]=prg[i];
	}
}

char kbd_feedbuf[255];
int kbd_feedbuf_pos;

void kbd_buf_feed(char *s) {
	strcpy(kbd_feedbuf, s);
	kbd_feedbuf_pos=0;
}

void kbd_buf_update(C64 *TheC64) {
	if((kbd_feedbuf[kbd_feedbuf_pos]!=0)
			&& TheC64->RAM[198]==0) {
		TheC64->RAM[631]=kbd_feedbuf[kbd_feedbuf_pos];
		TheC64->RAM[198]=1;

		kbd_feedbuf_pos++;
	}
}

void sort_dir(struct dir_item *list, int num_items, int sepdir) {
	int i;
	struct dir_item temp;

	for(i=0; i<(num_items-1); i++) {
		if(strcmp(list[i].name, list[i+1].name)>0) {
			temp=list[i];
			list[i]=list[i+1];
			list[i+1]=temp;
			i=0;
		}
	}
	if(sepdir) {
		for(i=0; i<(num_items-1); i++) {
			if((list[i].type!=0)&&(list[i+1].type==0)) {
				temp=list[i];
				list[i]=list[i+1];
				list[i+1]=temp;
				i=0;
			}
		}
	}
}

char *C64Display::FileReq(char *dir)
{
	static char *cwd=NULL;
	static int cursor_pos=1;
	static int first_visible;
	uint8 bg;
	static int num_items=0;
	DIR *dirstream;
	struct dirent *direntry;
	struct dir_item dir_items[1024];
	char *path;
	struct stat item;
	char *blank_line="                            ";
	static int row;
	int pathlength;
	char tmp_string[32];
	char *selected;
#define MENU_Y 25
#define MENU_LS MENU_Y+8

	sync_buffers();

	if(dir!=NULL) cwd=dir;
	if(cwd==NULL) cwd=AppDirPath;

	if(num_items==0) {
#ifdef __zipfile__
		if(!strcmp(cwd+(strlen(cwd)-4), ".zip")
			|| !strcmp(cwd+(strlen(cwd)-4), ".ZIP")) {
			LOGE( "reading zip archive %s\n", cwd);
			int zip_error;
			struct zip *zip_archive=zip_open(cwd, 0, &zip_error);
			int num_files=zip_get_num_files(zip_archive);
			const char *zip_file_name;
			dir_items[0].name="..";
			dir_items[0].type=0;
			num_items++;
			for(int i=0; i<num_files && i<1024; i++) {
				zip_file_name=zip_get_name(zip_archive, i, 0);
				dir_items[num_items].name=(char *)malloc(strlen(zip_file_name)+1);
				strcpy(dir_items[num_items].name, zip_file_name);
				num_items++;
			}
			zip_close(zip_archive);
			// entry types assume all zip content is files
			for(int i=1; i<num_items; i++) dir_items[i].type=1;
		} else {
#endif
			dirstream=opendir(cwd);
			if(dirstream==NULL) {
				printf("error opening directory %s\n", cwd);
				return (char *)-1;
			}
			// read directory entries
			while(direntry=readdir(dirstream)) {
				dir_items[num_items].name=(char *)malloc(strlen(direntry->d_name)+1);
				strcpy(dir_items[num_items].name, direntry->d_name);
				num_items++;
				if(num_items>1024) break;
			}
			closedir(dirstream);
			// get entry types
			for(int i=0; i<num_items; i++) {
				path=(char *)malloc(strlen(cwd)+strlen(dir_items[i].name)+2);
				sprintf(path, "%s/%s", cwd, dir_items[i].name);
				if(!stat(path, &item)) {
					if(S_ISDIR(item.st_mode)) {
						dir_items[i].type=0;
#ifdef __zipfile__
					} else if(!strcmp(path+(strlen(path)-4), ".zip")
						|| !strcmp(path+(strlen(path)-4), ".ZIP")) {
						dir_items[i].type=2;
#endif
					} else {
						dir_items[i].type=1;
					}
				} else {
					dir_items[i].type=0;
				}
				free(path);
			}
#ifdef __zipfile__
		}
#endif
		sort_dir(dir_items, num_items, 1);
		cursor_pos=0;
		first_visible=0;
	}

	// display current directory
        draw_ascii_string(screen_base, 80, MENU_Y, blank_line, black, fill_gray);
        draw_ascii_string(screen_base, 80, MENU_Y, cwd, black, fill_gray);

	// display directory contents
	row=0;
	while(row<num_items && row<MENU_HEIGHT) {
		if(row==(cursor_pos-first_visible)) {
			bg=yellow;
			selected=dir_items[row+first_visible].name;
		} else bg=fill_gray;
        	draw_ascii_string(screen_base, 80, MENU_LS+(8*row), blank_line, black, bg);
		if(dir_items[row+first_visible].type==0) {
        		draw_ascii_string(screen_base, 80, MENU_LS+(8*row), "<DIR> ", black, bg);
#ifdef __zipfile__
		} else if(dir_items[row+first_visible].type==2) {
        		draw_ascii_string(screen_base, 80, MENU_LS+(8*row), "<ZIP> ", black, bg);
#endif
		}
		snprintf(tmp_string, 16, "%s", dir_items[row+first_visible].name);
        	draw_ascii_string(screen_base, 80+(8*6), MENU_LS+(8*row), tmp_string, black, bg);
		row++;
	}
	while(row<MENU_HEIGHT) {
        	draw_ascii_string(screen_base, 80, MENU_LS+(8*row), blank_line, black, fill_gray);
		row++;
	}

	FILE *thumb_snap_file;
	char thumb_snap_filename[255];
	if(strcmp(selected, thumb_snap_name)) {
		strncpy(thumb_snap_name, selected, 255);
		snprintf(thumb_snap_filename, 255, "%s/c64/snapshots/thumbs/%s.thm", AppDirPath, selected);
		if(thumb_snap_file=fopen(thumb_snap_filename, "r")) {
			if(!thumb_snap) thumb_snap=(uint8 *)malloc((DISPLAY_X/4)*(DISPLAY_Y/4));
			fread(thumb_snap, (DISPLAY_X/4)*(DISPLAY_Y/4), 1, thumb_snap_file);
			fclose(thumb_snap_file);
		} else {
			if(thumb_snap) free(thumb_snap);
			thumb_snap=NULL;
		}
	}
	if(thumb_snap) {
		for(int y=0; y<DISPLAY_Y/4; y++) {
			for(int x=0; x<DISPLAY_X/4; x++) {
				screen_base[((y+160)*DISPLAY_X)+x+200]=thumb_snap[(y*(DISPLAY_X/4))+x];
			}
		}
	}
 
 	if(sdl_joy_state&0x10) fire_deselected=1;

	if(sdl_joy_state_counter>1) {
		sdl_joy_state_counter=0;
		if(!(sdl_joy_state&0x02)) { //down
			if(cursor_pos<(num_items-1)) cursor_pos++;
			if((cursor_pos-first_visible)>=MENU_HEIGHT) first_visible++;
		} else if(!(sdl_joy_state&0x01)) { // up
			if(cursor_pos>0) cursor_pos--;
			if(cursor_pos<first_visible) first_visible--;
		} else if(!(sdl_joy_state&0x04)) { //left
			if(cursor_pos>=10) cursor_pos-=10;
			else cursor_pos=0;
			if(cursor_pos<first_visible) first_visible=cursor_pos;
		} else if(!(sdl_joy_state&0x08)) { //right
			if(cursor_pos<(num_items-11)) cursor_pos+=10;
			else cursor_pos=num_items-1;
			if((cursor_pos-first_visible)>=MENU_HEIGHT) 
				first_visible=cursor_pos-(MENU_HEIGHT-1);
		} else if((!(sdl_joy_state&0x10))&&fire_deselected) { // button 1
			fire_deselected=0;
			int i;
			path=(char *)malloc(strlen(cwd)
				+strlen(dir_items[cursor_pos].name)
				+2);
			sprintf(path, "%s/%s", cwd, dir_items[cursor_pos].name);
			//FIXME
			//for(i=0; i<num_items; i++) free(dir_items[i].name);
			num_items=0;
			if(dir_items[cursor_pos].type==0) {
				// directory selected
				pathlength=strlen(path);
				if(path[pathlength-1]=='.'
					// check for . selected
						&& path[pathlength-2]=='/') {
					path[pathlength-2]='\0';
					return path;
				} else if(path[pathlength-1]=='.'
					// check for .. selected
						&& path[pathlength-2]=='.'
						&& path[pathlength-3]=='/'
						&&pathlength>8) {
					for(i=4; path[pathlength-i]!='/'; i++);
					path[pathlength-i]='\0';
					if(strlen(path)<=5) {
						path[5]='/';
						path[6]='\0';
					}
					cwd=path;
				} else {
					cwd=path;
				}
#ifdef __zipfile__
			} else if(dir_items[cursor_pos].type==2) {
				cwd=path;
#endif
			} else {
				// file selected
				return path;
			}
		} else if(prefs_button_state) { // prefs button
			prefs_button_deselected=0;
			return (char *)-1;
		}
	}

	return NULL;
}

void C64Display::Draw_Prefs(C64 *TheC64) 
{
	static int cursor_pos=0;
	uint8 bg, fg;
	char tmp_string[1024];
	char tmp_string2[1024];
	static bool getfilename=false;
	char *imagefile;
	//static int xfiletype;
	static Prefs *prefs=NULL;
	int update_prefs=0;
	int i,j;
	char zip_error_str[255];

	sync_buffers();

	if(prefs==NULL) prefs=new Prefs(ThePrefs);
	
        enum {
                D64, LOADSTAR, LOADER, SAVE_SNAP,
                BLANK1,
		RESET64,
                SOUND, FRAMESKIP, LIMITSPEED,
                JOYSTICK, BUTTONMAP, EMU1541CPU,
		SCALED,
                BLANK2,
                EXITFRODO,
                NUM_OPTIONS,
		BRIGHTNESS, CONTRAST,
                BLANK3, BLANK4,
		LOAD_SNAP,
		PRG
        };

				//char *option_txt[NUM_OPTIONS+5];
				char *option_txt[255];
				option_txt[SOUND]=              "Sound                       ";
				option_txt[BRIGHTNESS]=         "Brightness                  ";
				option_txt[CONTRAST]=           "Contrast                    ";
				option_txt[FRAMESKIP]=          "Frameskip                   ";
				option_txt[LIMITSPEED]=         "Limit speed                 ";
				option_txt[JOYSTICK]=           "Joystick in port            ";
				option_txt[EMU1541CPU]=         "1541 CPU emulation          ";
				option_txt[D64]=                "Attach image/load prg/sna...";
				option_txt[LOADSTAR]=           "Load first program          ";
				option_txt[LOADER]=             "Load disk browser           ";
				option_txt[PRG]=                "Load .prg file...           ";
				option_txt[SAVE_SNAP]=          "Save snapshot               ";
				option_txt[LOAD_SNAP]=          "Load snapshot...            ";
				option_txt[RESET64]=            "Reset C=64                  ";
				option_txt[BUTTONMAP]=          "Controls                    ";
				option_txt[SCALED]=             "Display                     ";
				option_txt[EXITFRODO]=          "Exit Frodo                  ";
				option_txt[BLANK1]=             "                            ";
				option_txt[BLANK2]=             "                            ";
				option_txt[BLANK3]=             "                            ";
				option_txt[BLANK4]=             "                            ";
				option_txt[NUM_OPTIONS]=        "                            ";
				option_txt[NUM_OPTIONS+1]=      "                            ";
				option_txt[NUM_OPTIONS+2]=      "                            ";
				option_txt[NUM_OPTIONS+3]=      "                            ";
				option_txt[NUM_OPTIONS+4]=      NULL;

	if(getfilename) {
		imagefile=FileReq(NULL);
		if(imagefile==NULL) return;
		else if(imagefile==(char *)-1) getfilename=0;
		else {
#ifdef __zipfile__
			LOGE( "opening zip file\n");
			if(strstr(imagefile, ".zip/") || strstr(imagefile, ".ZIP/")) {
				strcpy(tmp_string, imagefile);
				for(i=strlen(tmp_string); tmp_string[i]!='/'; i--);
				tmp_string[i]='\0'; // tmp_string points now to zip archive
				char *zip_member=tmp_string+i+1;
				int zip_error;
				struct zip *zip_archive=zip_open(tmp_string, 0, &zip_error);
				if(zip_archive) {
					struct zip_file *zip_content=zip_fopen(zip_archive, zip_member, 0);
					if(zip_content) {
						char *zipfile_buffer=(char *)malloc(174848);
						int filesize=zip_fread(zip_content, zipfile_buffer, 174848);

						snprintf(tmp_string2, 255, "%s/c64/tmp/frodo.%s", 
							AppDirPath, (zip_member+(strlen(zip_member)-3)));
						if(!strcmp(prefs->DrivePath[0], tmp_string2)) {
							snprintf(tmp_string2, 255, "%s/c64/tmp/frodo2.%s", 
								AppDirPath, (zip_member+(strlen(zip_member)-3)));
						}
						zip_fclose(zip_content);
						LOGE( "creating temporary file %s\n", tmp_string2);
						FILE *tmp_image=fopen(tmp_string2, "w");
						fwrite(zipfile_buffer, filesize, 1, tmp_image);
						fclose(tmp_image);
					} else {
						zip_error_to_str(zip_error_str, 255, errno, zip_error);
						LOGE( "error in zip_fopen: %s\n", zip_error_str);
					}
					zip_close(zip_archive);
				} else {
					zip_error_to_str(zip_error_str, 255, errno, zip_error);
					LOGE( "error in zip_open: %s\n", zip_error_str);
				}
				imagefile=tmp_string2;
			}
#endif
			if(!strcmp(imagefile+(strlen(imagefile)-4), ".sna")
				|| !strcmp(imagefile+(strlen(imagefile)-4), ".SNA")) {
				TheC64->LoadSnapshot(imagefile);
				TheC64->Resume();
				sdl_prefs_open=false;
			} else if(!strcmp(imagefile+(strlen(imagefile)-4), ".prg")
				|| !strcmp(imagefile+(strlen(imagefile)-4), ".PRG")) {
				char loadprg[255];
				for(i=strlen(imagefile); i>0; i--) {
					if(imagefile[i]=='/') break;
				}
				strcpy(loadprg, "\rLOAD\"");
				for(j=0; imagefile[i+1+j]; j++) {
					unsigned char c;
					c=imagefile[i+1+j];
					if ((c >= 'A') && (c <= 'Z') || (c >= 'a') && (c <= 'z')) c ^= 0x20;
					tmp_string[j]=c;
				}
				tmp_string[j]='\0';
				strcat(loadprg, tmp_string);
				strcat(loadprg, "\",8,1\rRUN\r");
				imagefile[i]='\0';
				strcpy(prefs->DrivePath[0], imagefile);
				kbd_buf_feed(loadprg);
				update_prefs=1;
				TheC64->Resume();
				sdl_prefs_open=false;
			} else {
				strcpy(prefs->DrivePath[0], imagefile);
				update_prefs=1;
				cursor_pos=LOADSTAR;
			}
			getfilename=0;
			fire_deselected=0;
		}
	}

	if(sdl_joy_state_counter>1) {
		if(!(sdl_joy_state&0x02)) { //down
			cursor_pos++;
			if(cursor_pos>=NUM_OPTIONS) cursor_pos=NUM_OPTIONS-1;
		} else if(!(sdl_joy_state&0x01)) { // up
			cursor_pos--;
			if(cursor_pos<0) cursor_pos=0;
		} else if(!(sdl_joy_state&0x04)) { // left
			if(cursor_pos==LIMITSPEED) {
				prefs->LimitSpeed=false;
				update_prefs=1;
			} else if(cursor_pos==FRAMESKIP) {
				if(prefs->SkipFrames>1) prefs->SkipFrames--;
				update_prefs=1;
			} else if(cursor_pos==RESET64) {
				prefs->FastReset=false;
				update_prefs=1;
			} else if(cursor_pos==SOUND) {
				prefs->SIDType=SIDTYPE_NONE;
				update_prefs=1;
			} else if(cursor_pos==JOYSTICK) {
				prefs->JoystickSwap=false;
				update_prefs=1;
			} else if(cursor_pos==EMU1541CPU) {
				prefs->Emul1541Proc=false;
				update_prefs=1;
			} else if(cursor_pos==BUTTONMAP) {
				gp2xButtonHandler.prevButtonSet();
			} else if(cursor_pos==SCALED) {
				scaled=0;
				set_scaling();
			} else if(cursor_pos==BRIGHTNESS) {
				set_luma(brightness-1, contrast);
			} else if(cursor_pos==CONTRAST) {
				set_luma(brightness, contrast-1);
			}
		} else if(!(sdl_joy_state&0x08)) { // right
			if(cursor_pos==LIMITSPEED) {
				prefs->LimitSpeed=true;
				update_prefs=1;
			} else if(cursor_pos==FRAMESKIP) {
				if(prefs->SkipFrames<10) prefs->SkipFrames++;
				update_prefs=1;
			} else if(cursor_pos==RESET64) {
				prefs->FastReset=true;
				update_prefs=1;
			} else if(cursor_pos==SOUND) {
				prefs->SIDType=SIDTYPE_DIGITAL;
				update_prefs=1;
			} else if(cursor_pos==JOYSTICK) {
				prefs->JoystickSwap=true;
				update_prefs=1;
			} else if(cursor_pos==EMU1541CPU) {
				prefs->Emul1541Proc=true;
				update_prefs=1;
			} else if(cursor_pos==BUTTONMAP) {
				gp2xButtonHandler.nextButtonSet();
			} else if(cursor_pos==SCALED) {
				scaled=1;
				set_scaling();
			} else if(cursor_pos==BRIGHTNESS) {
				set_luma(brightness+1, contrast);
			} else if(cursor_pos==CONTRAST) {
				set_luma(brightness, contrast+1);
			}
		}
		sdl_joy_state_counter=0;
	}

	if(sdl_joy_state&0x10) fire_deselected=true;
	if(!(sdl_joy_state&0x10) && fire_deselected) { // fire
		fire_deselected=0;
		if(cursor_pos==D64) {
			//xfiletype=0;
			getfilename=true;
		} else if(cursor_pos==LOADSTAR) {
			kbd_buf_feed("\rLOAD\":*\",8,1\rRUN\r");
			TheC64->Resume();
			sdl_prefs_open=false;
		} else if(cursor_pos==LOADER) {
			load_prg(TheC64, c64loader, sizeof(c64loader));
			kbd_buf_feed("\rLOAD\"X\",8,1\rRUN\r");
			TheC64->Resume();
			sdl_prefs_open=false;
		} else if(cursor_pos==PRG) {
			//xfiletype=1;
			getfilename=true;
		} else if(cursor_pos==RESET64) {
			TheC64->Reset();
			TheC64->Resume();
			sdl_prefs_open=false;
		} else if(cursor_pos==LOAD_SNAP) {
			TheC64->LoadSnapshot("test.sna");
			TheC64->Resume();
			sdl_prefs_open=false;
		} else if(cursor_pos==SAVE_SNAP) {
#ifdef __gp2x__
			chdir(AppDirPath);
			chdir("c64/snapshots/");
#else
			chdir("/tmp/c64/snapshots/");
#endif
			int freename=0;
			for(int snapnum=0; !freename; snapnum++) {
				DIR *snaps_dir=opendir(".");
				struct dirent *direntry;
				freename=1;
				while(direntry=readdir(snaps_dir)) {
					sprintf(tmp_string, "%04d.sna", snapnum);
					if(!strcmp(tmp_string, direntry->d_name)) freename=0;
				}
				closedir(snaps_dir);
			}
			TheC64->SaveSnapshot(tmp_string);


			// save thumbnail
			if(thumb_current) {
				sprintf(tmp_string2, "thumbs/%s.thm", tmp_string);
				FILE *thumb_file=fopen(tmp_string2, "w");
				fwrite(thumb_current, (DISPLAY_X/4)*(DISPLAY_Y/4), 1, thumb_file);
				fclose(thumb_file);
			}

			TheC64->Resume();
			sdl_prefs_open=false;
		} else if(cursor_pos==EXITFRODO) {
			TheC64->Resume();
			quit_requested=true;
		}
	}

	if(!prefs_button_state) prefs_button_deselected=1;
	if(prefs_button_state && prefs_button_deselected) {
		TheC64->Resume();
		sdl_prefs_open=false;
		prefs_button_deselected=0;
	}

	for(i=0; option_txt[i]; i++) {
		bg=fill_gray;
		if(i==cursor_pos) bg=yellow;
		draw_ascii_string(screen_base, 80, 25+(i*8), option_txt[i], black, bg);
	}
	while(i<=MENU_HEIGHT) {
		draw_ascii_string(screen_base, 80, 25+(i*8), option_txt[BLANK1], black, bg);
		i++;
	}

	/*sprintf(tmp_string, "%d", brightness);
        draw_ascii_string(screen_base, 80+(8*23), 25+(8*BRIGHTNESS), tmp_string, black, fill_gray);
	sprintf(tmp_string, "%d", contrast);
        draw_ascii_string(screen_base, 80+(8*23), 25+(8*CONTRAST), tmp_string, black, fill_gray);*/

        // reset speed
        if(prefs->FastReset) {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*RESET64), "Fast", black, fill_gray);
	} else {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*RESET64), "Slow", black, fill_gray);
	}
        // limit speed
        if(prefs->LimitSpeed) {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*LIMITSPEED), "On", black, fill_gray);
	} else {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*LIMITSPEED), "Off", black, fill_gray);
	}
        // joystick port
        if(prefs->JoystickSwap==false) {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*JOYSTICK), "1", black, fill_gray);
	} else {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*JOYSTICK), "2", black, fill_gray);
	}
	// sound
        if(prefs->SIDType==SIDTYPE_NONE) {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*SOUND), "Off", black, fill_gray);
	} else {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*SOUND), "On", black, fill_gray);
	}
	// sound
        if(prefs->Emul1541Proc) {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*EMU1541CPU), "On", black, fill_gray);
	} else {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*EMU1541CPU), "Off", black, fill_gray);
	}
	// frameskip
	snprintf(tmp_string, 255, "%d", (prefs->SkipFrames)-1);
        draw_ascii_string(screen_base, 80+(8*24), 25+(8*FRAMESKIP), tmp_string, black, fill_gray);

	draw_ascii_string(screen_base, 80+(8*10), 25+(8*BUTTONMAP), gp2xButtonHandler.buttonSetName().substr(0,17).c_str(), black, fill_gray);
   
	if(scaled) {
        	draw_ascii_string(screen_base, 80+(8*20), 25+(8*SCALED), "Scaled", black, fill_gray);
	} else {
        	draw_ascii_string(screen_base, 80+(8*23), 25+(8*SCALED), "1:1", black, fill_gray);
	}

	// attached media
	snprintf(tmp_string, 22, "%s", prefs->DrivePath[0]);
        draw_ascii_string(screen_base, 80, 25+(8*BLANK3), "drive 8 media: ", black, fill_gray);
        draw_ascii_string(screen_base, 80, 25+(8*BLANK4), tmp_string, black, fill_gray);

	if(update_prefs) {
		TheC64->NewPrefs(prefs);
		ThePrefs=*prefs;
		delete prefs;
		prefs=NULL;
		update_prefs=0;
	}

}


/*
 *  Redraw bitmap
 */

void C64Display::Update(void)
{
	char tmp_string[255];

	kbd_buf_update(TheC64);
	if(sdl_prefs_open) Draw_Prefs(TheC64);
	else if(vkeyb_open) Draw_VKeyb(TheC64);

	int time_now, elapsed_time;
	static int num_frames, fps, time_then;;

	num_frames++;
	time_now=(int)time(NULL);
	if(time_now!=time_then) {
		fps=num_frames;
		num_frames=0;
	}
	time_then=time_now;

	if(stats_enabled) {
		snprintf(tmp_string, 255, "%s, %dfps", speedometer_string, fps);
        	draw_ascii_string(screen_base, 32, 16, tmp_string, black, fill_gray);
	}

	if(volume_key_pressed) {
		//snprintf(tmp_string, 255, "volume %x", volume);
		//snprintf(tmp_string, 255, "volume %d", volume&0xff);
        	//draw_ascii_string(screen_base, 128, 16, tmp_string, black, fill_gray);
        	draw_ascii_string(screen_base, 72, 248, "volume ", black, fill_gray);
		int vol_len=(volume&0xff)/8;
		int i;
		for(i=0; i<=vol_len; i++) {
        		draw_ascii_string(screen_base, 128+(i*8), 248, "-", black, fill_gray);
		}
		for(; i<=10; i++) {
        		draw_ascii_string(screen_base, 128+(i*8), 248, " ", black, fill_gray);
		}
        	//draw_ascii_string(screen_base, 220, 248, " ", black, fill_gray);
		volume_key_pressed--;
	}

	// Update display
	double_buffer();
}


/*
 *  Draw string into surface using the C64 ROM font
 */

void C64Display::draw_ascii_string(uint8 *s, int x, int y, const char *str, uint8 front_color, uint8 back_color)
{
	uint8 *pb = s + (DISPLAY_X*y) + x;

	char c;
	while ((c = *str++) != 0) {
		if(c>=97&&c<=122) c+=160;
	        //else if(x>=65&&x<=90) x-=64;

		uint8 *q = TheC64->Char + c*8 + 0x800;
		uint8 *p = pb;
		for (int y=0; y<8; y++) {
			uint8 v = *q++;
			p[0] = (v & 0x80) ? front_color : back_color;
			p[1] = (v & 0x40) ? front_color : back_color;
			p[2] = (v & 0x20) ? front_color : back_color;
			p[3] = (v & 0x10) ? front_color : back_color;
			p[4] = (v & 0x08) ? front_color : back_color;
			p[5] = (v & 0x04) ? front_color : back_color;
			p[6] = (v & 0x02) ? front_color : back_color;
			p[7] = (v & 0x01) ? front_color : back_color;
			p += DISPLAY_X;
		}
		pb += 8;
	}
}


/*
 *  LED error blink
 */

void C64Display::pulse_handler(...)
{
	for (int i=0; i<4; i++)
		switch (c64_disp->led_state[i]) {
			case LED_ERROR_ON:
				c64_disp->led_state[i] = LED_ERROR_OFF;
				break;
			case LED_ERROR_OFF:
				c64_disp->led_state[i] = LED_ERROR_ON;
				break;
		}
}


/*
 *  Draw speedometer
 */

void C64Display::Speedometer(int speed)
{
	static int delay = 0;

	if (delay >= 20) {
		delay = 0;
		sprintf(speedometer_string, "%d%%", speed);
	} else
		delay++;
}


/*
 *  Return pointer to bitmap data
 */

uint8 *C64Display::BitmapBase(void)
{
	return screen_base;
}


/*
 *  Return number of bytes per row
 */

int C64Display::BitmapXMod(void)
{
	return DISPLAY_X;
}

static int keystate[256];

void KeyPress(int key, uint8 *key_matrix, uint8 *rev_matrix) {
	int c64_byte, c64_bit, shifted;
	if(!keystate[key]) {
		keystate[key]=1;
		c64_byte=key>>3;
		c64_bit=key&7;
		shifted=key&128;
		c64_byte&=7;
		if(shifted) {
			key_matrix[6] &= 0xef;
			rev_matrix[4] &= 0xbf;
		}
		key_matrix[c64_byte]&=~(1<<c64_bit);
		rev_matrix[c64_bit]&=~(1<<c64_byte);
	}
}

void KeyRelease(int key, uint8 *key_matrix, uint8 *rev_matrix) {
	int c64_byte, c64_bit, shifted;
	if(keystate[key]) {
		keystate[key]=0;
		c64_byte=key>>3;
		c64_bit=key&7;
		shifted=key&128;
		c64_byte&=7;
		if(shifted) {
			key_matrix[6] |= 0x10;
			rev_matrix[4] |= 0x40;
		}
		key_matrix[c64_byte]|=(1<<c64_bit);
		rev_matrix[c64_bit]|=(1<<c64_byte);
	}
}

void C64Display::PollKeyboard(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick)
{	
	if (!vkeyb_open && !sdl_prefs_open)
		for (int i=gp2xButtonHandler.keys.size();i>0;--i)
		{
			if (gp2xButtonHandler.keys[i-1].pressed)
				KeyPress(gp2xButtonHandler.keys[i-1].key,key_matrix,rev_matrix);
			else if (gp2xButtonHandler.keys[i-1].released)
				KeyRelease(gp2xButtonHandler.keys[i-1].key,key_matrix,rev_matrix);
		}

	// check virtual keyboard
	if(vkeyb_open) {
		if(vkey_pressed) {
			KeyPress(vkey, key_matrix, rev_matrix);
			vkey_pressed=0;
		}
		if(vkey_released) {
			KeyRelease(vkey, key_matrix, rev_matrix);
			vkey_released=0;
		}
	}
}


/*
 *  Check if NumLock is down (for switching the joystick keyboard emulation)
 */

bool C64Display::NumLock(void)
{
	return num_locked;
}


/*
 *  Open/close joystick drivers given old and new state of
 *  joystick preferences
 */

void C64::open_close_joystick(int port, int oldjoy, int newjoy)
{
}

void C64::open_close_joysticks(int oldjoy1, int oldjoy2, int newjoy1, int newjoy2)
{
	open_close_joystick(0, oldjoy1, newjoy1);
	open_close_joystick(1, oldjoy2, newjoy2);
}


/*
 *  Poll joystick port, return CIA mask
 */

uint8 C64::poll_joystick(int port)
{
	uint8 j = 0xff;

	if(port==0) {
		// space
		gp2xButtonHandler.readGP2XButtons(gp2x_read_joystick());

		// prefs screen
		prefs_button_state=gp2xButtonHandler.specialState&PREF_KEY;
		if(!sdl_prefs_open) {
			if(!prefs_button_state) prefs_button_deselected=1;
			if(prefs_button_state&&prefs_button_deselected) {
				TheC64->Pause();
				sdl_prefs_open=true;
				prefs_button_deselected=false;

				// grab thumb of current screen
				if(!thumb_current) thumb_current=(uint8 *)malloc((DISPLAY_X/4)*(DISPLAY_Y/4));
				for(int y=0; y<DISPLAY_Y/4; y++) {
					for(int x=0; x<DISPLAY_X/4; x++) {
						thumb_current[(y*(DISPLAY_X/4))+x]=screen_base[(y*DISPLAY_X*4)+(x*4)];
					}
				}
			}
		}

		// virtual keyboard
		vkeyb_button_state=gp2xButtonHandler.specialState&KEYB_KEY;
		
		if(!vkeyb_open) {
			if(!vkeyb_button_state) vkeyb_button_deselected=1;
			if(vkeyb_button_state&&vkeyb_button_deselected) {
				vkeyb_open=true;
				vkeyb_button_deselected=false;
			}
		}

		// toggle scaling
		bool scale_key_state;
		scale_key_state=gp2xButtonHandler.specialState&SCALE_KEY;
			
		if(!scale_key_state) rshoulder_released=1;
		if(scale_key_state && rshoulder_released) {
			scaled=!scaled;
			set_scaling();
			rshoulder_released=0;
		}
		// volume
		if(gp2xButtonHandler.specialState&VOL_DOWN_KEY) {
			volume_key_pressed=50;
			//ioctl(mixer_fd, SOUND_MIXER_READ_VOLUME, &volume);
			//ioctl(mixer_fd, SOUND_MIXER_READ_PCM, &volume);
			int volume_l=volume&0xff;
			if(volume_l>0) {
				volume_l--;
				volume=(volume_l<<8)|volume_l;
				//ioctl(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &volume);
				ioctl(mixer_fd, SOUND_MIXER_WRITE_PCM, &volume);
			}
		} else if(gp2xButtonHandler.specialState&VOL_UP_KEY) {
			volume_key_pressed=50;
			//ioctl(mixer_fd, SOUND_MIXER_READ_VOLUME, &volume);
			//ioctl(mixer_fd, SOUND_MIXER_READ_PCM, &volume);
			int volume_l=volume&0xff;
			if(volume_l<0x50) {
				volume_l++;
				volume=(volume_l<<8)|volume_l;
				//ioctl(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &volume);
				ioctl(mixer_fd, SOUND_MIXER_WRITE_PCM, &volume);
			}
		}
	}

	if (joy[port]) {
			if (!vkeyb_open && !sdl_prefs_open)
			{
				if(gp2xButtonHandler.specialState&JOY2_UP) j&=0xfe;
				if(gp2xButtonHandler.specialState&JOY2_DOWN) j&=0xfd;
				if(gp2xButtonHandler.specialState&JOY2_LEFT) j&=0xfb;
				if(gp2xButtonHandler.specialState&JOY2_RIGHT) j&=0xf7;
				if(gp2xButtonHandler.specialState&JOY2_FIRE) j&=0xef; // fire
			}
			else
			{
				if(gp2xButtonHandler.specialState&MENU_UP) j&=0xfe;
				if(gp2xButtonHandler.specialState&MENU_DOWN) j&=0xfd;
				if(gp2xButtonHandler.specialState&MENU_LEFT) j&=0xfb;
				if(gp2xButtonHandler.specialState&MENU_RIGHT) j&=0xf7;
				if(gp2xButtonHandler.specialState&MENU_SELECT) j&=0xef; // fire
			}
	}
	else
	{
		if(gp2xButtonHandler.specialState&JOY1_UP) j&=0xfe;
		if(gp2xButtonHandler.specialState&JOY1_DOWN) j&=0xfd;
		if(gp2xButtonHandler.specialState&JOY1_LEFT) j&=0xfb;
		if(gp2xButtonHandler.specialState&JOY1_RIGHT) j&=0xf7;
		if(gp2xButtonHandler.specialState&JOY1_FIRE) j&=0xef; // fire
	}

	if(port==0 && (sdl_prefs_open||vkeyb_open)) {
		sdl_joy_state=j;
		if(sdl_joy_state!=last_sdl_joy_state) {
			sdl_joy_state_counter=4;
			last_sdl_joy_state=sdl_joy_state;
		} else {
			sdl_joy_state_counter++;
		}
		return 0xff;
	} else {
		return j;
	}
}


/*
 *  Allocate C64 colors
 */

void C64Display::InitColors(uint8 *colors)
{
	sdl_palette=colors;

	for (int i=0; i<256; i++)
		colors[i] = i & 0x0f;

	gp2x_memregs[0x2958>>1]=0;
	for(int i=0; i<512; i++) {
		gp2x_memregs[0x295a>>1]=((palette_green[i&0x0f])<<8)|palette_blue[i&0x0f];
		gp2x_memregs[0x295a>>1]=palette_red[i&0x0f];
	}
}


/*
 *  Show a requester (error message)
 */

long int ShowRequester(char *a,char *b,char *)
{
	printf("%s: %s\n", a, b);
	return 1;
}

