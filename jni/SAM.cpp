/*
 *  SAM.h - Simple Assembler and Monitor With Integrated System Explorer
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

#include "SAM.h"
#include "C64.h"
#include "CPUC64.h"
#include "CPU1541.h"
#include "VIC.h"
#include "SID.h"
#include "CIA.h"


// Pointers to chips
static MOS6510 *TheCPU;
static MOS6502_1541 *TheCPU1541;
static MOS6569 *TheVIC;
static MOS6581 *TheSID;
static MOS6526_1 *TheCIA1;
static MOS6526_2 *TheCIA2;

// 6510/6502 registers
static MOS6510State R64;
static MOS6502State R1541;

static bool access_1541;	// false: accessing C64, true: accessing 1541

// Access to 6510/6502 address space
static inline uint8 SAMReadByte(uint16 adr)
{
	if (access_1541)
		return TheCPU1541->ExtReadByte(adr);
	else
		return TheCPU->ExtReadByte(adr);
}

static inline void SAMWriteByte(uint16 adr, uint8 byte)
{
	if (access_1541)
		TheCPU1541->ExtWriteByte(adr, byte);
	else
		TheCPU->ExtWriteByte(adr, byte);
}


// Streams for input, output and error messages
static FILE *fin, *fout, *ferr;

// Input line
#define INPUT_LENGTH 80
static char input[INPUT_LENGTH];
static char *in_ptr;

static uint16 address, end_address;


// Input tokens
enum Token {
	T_NULL,		// Invalid token
	T_END,		// End of line
	T_NUMBER,	// Hexadecimal number
	T_STRING,	// String enclosed in ""
	T_LPAREN,	// '('
	T_RPAREN,	// ')'
	T_ADD,		// '+'
	T_SUB,		// '-'
	T_MUL,		// '*'
	T_DIV,		// '/'
	T_COMMA,	// ','
	T_IMMED,	// '#'
	T_X,		// 'x'
	T_Y,		// 'y'
	T_PC,		// 'pc'
	T_SP,		// 'sp'

	T_A,		// 'a'	(get_reg_token() only)
	T_DR,		// 'dr'	(get_reg_token() only)
	T_PR		// 'pr'	(get_reg_token() only)
};

static enum Token the_token;			// Last token read
static uint16 the_number;				// Contains the number if the_token==T_NUMBER
static char the_string[INPUT_LENGTH];	// Contains the string if the_token==T_STRING


// Addressing modes
enum {
	A_IMPL,
	A_ACCU,		// A
	A_IMM,		// #zz
	A_REL,		// Branches
	A_ZERO,		// zz
	A_ZEROX,	// zz,x
	A_ZEROY,	// zz,y
	A_ABS,		// zzzz
	A_ABSX,		// zzzz,x
	A_ABSY,		// zzzz,y
	A_IND,		// (zzzz)
	A_INDX,		// (zz,x)
	A_INDY		// (zz),y
};

// Mnemonics
enum {
	M_ADC, M_AND, M_ASL, M_BCC, M_BCS, M_BEQ, M_BIT, M_BMI, M_BNE, M_BPL,
	M_BRK, M_BVC, M_BVS, M_CLC, M_CLD, M_CLI, M_CLV, M_CMP, M_CPX, M_CPY,
	M_DEC, M_DEX, M_DEY, M_EOR, M_INC, M_INX, M_INY, M_JMP, M_JSR, M_LDA,
	M_LDX, M_LDY, M_LSR, M_NOP, M_ORA, M_PHA, M_PHP, M_PLA, M_PLP, M_ROL,
	M_ROR, M_RTI, M_RTS, M_SBC, M_SEC, M_SED, M_SEI, M_STA, M_STX, M_STY,
	M_TAX, M_TAY, M_TSX, M_TXA, M_TXS, M_TYA,

	M_ILLEGAL,  // Undocumented opcodes start here

	M_IANC, M_IANE, M_IARR, M_IASR, M_IDCP, M_IISB, M_IJAM, M_INOP, M_ILAS,
	M_ILAX, M_ILXA, M_IRLA, M_IRRA, M_ISAX, M_ISBC, M_ISBX, M_ISHA, M_ISHS,
	M_ISHX, M_ISHY, M_ISLO, M_ISRE,

	M_MAXIMUM  // Highest element
};

// Mnemonic for each opcode
static const char mnemonic[256] = {
	M_BRK , M_ORA , M_IJAM, M_ISLO, M_INOP, M_ORA, M_ASL , M_ISLO,	// 00
	M_PHP , M_ORA , M_ASL , M_IANC, M_INOP, M_ORA, M_ASL , M_ISLO,
	M_BPL , M_ORA , M_IJAM, M_ISLO, M_INOP, M_ORA, M_ASL , M_ISLO,	// 10
	M_CLC , M_ORA , M_INOP, M_ISLO, M_INOP, M_ORA, M_ASL , M_ISLO,
	M_JSR , M_AND , M_IJAM, M_IRLA, M_BIT , M_AND, M_ROL , M_IRLA,	// 20
	M_PLP , M_AND , M_ROL , M_IANC, M_BIT , M_AND, M_ROL , M_IRLA,
	M_BMI , M_AND , M_IJAM, M_IRLA, M_INOP, M_AND, M_ROL , M_IRLA,	// 30
	M_SEC , M_AND , M_INOP, M_IRLA, M_INOP, M_AND, M_ROL , M_IRLA,
	M_RTI , M_EOR , M_IJAM, M_ISRE, M_INOP, M_EOR, M_LSR , M_ISRE,	// 40
	M_PHA , M_EOR , M_LSR , M_IASR, M_JMP , M_EOR, M_LSR , M_ISRE,
	M_BVC , M_EOR , M_IJAM, M_ISRE, M_INOP, M_EOR, M_LSR , M_ISRE,	// 50
	M_CLI , M_EOR , M_INOP, M_ISRE, M_INOP, M_EOR, M_LSR , M_ISRE,
	M_RTS , M_ADC , M_IJAM, M_IRRA, M_INOP, M_ADC, M_ROR , M_IRRA,	// 60
	M_PLA , M_ADC , M_ROR , M_IARR, M_JMP , M_ADC, M_ROR , M_IRRA,
	M_BVS , M_ADC , M_IJAM, M_IRRA, M_INOP, M_ADC, M_ROR , M_IRRA,	// 70
	M_SEI , M_ADC , M_INOP, M_IRRA, M_INOP, M_ADC, M_ROR , M_IRRA,
	M_INOP, M_STA , M_INOP, M_ISAX, M_STY , M_STA, M_STX , M_ISAX,	// 80
	M_DEY , M_INOP, M_TXA , M_IANE, M_STY , M_STA, M_STX , M_ISAX,
	M_BCC , M_STA , M_IJAM, M_ISHA, M_STY , M_STA, M_STX , M_ISAX,	// 90
	M_TYA , M_STA , M_TXS , M_ISHS, M_ISHY, M_STA, M_ISHX, M_ISHA,
	M_LDY , M_LDA , M_LDX , M_ILAX, M_LDY , M_LDA, M_LDX , M_ILAX,	// a0
	M_TAY , M_LDA , M_TAX , M_ILXA, M_LDY , M_LDA, M_LDX , M_ILAX,
	M_BCS , M_LDA , M_IJAM, M_ILAX, M_LDY , M_LDA, M_LDX , M_ILAX,	// b0
	M_CLV , M_LDA , M_TSX , M_ILAS, M_LDY , M_LDA, M_LDX , M_ILAX,
	M_CPY , M_CMP , M_INOP, M_IDCP, M_CPY , M_CMP, M_DEC , M_IDCP,	// c0
	M_INY , M_CMP , M_DEX , M_ISBX, M_CPY , M_CMP, M_DEC , M_IDCP,
	M_BNE , M_CMP , M_IJAM, M_IDCP, M_INOP, M_CMP, M_DEC , M_IDCP,	// d0
	M_CLD , M_CMP , M_INOP, M_IDCP, M_INOP, M_CMP, M_DEC , M_IDCP,
	M_CPX , M_SBC , M_INOP, M_IISB, M_CPX , M_SBC, M_INC , M_IISB,	// e0
	M_INX , M_SBC , M_NOP , M_ISBC, M_CPX , M_SBC, M_INC , M_IISB,
	M_BEQ , M_SBC , M_IJAM, M_IISB, M_INOP, M_SBC, M_INC , M_IISB,	// f0
	M_SED , M_SBC , M_INOP, M_IISB, M_INOP, M_SBC, M_INC , M_IISB
};

// Addressing mode for each opcode
static const char adr_mode[256] = {
	A_IMPL, A_INDX, A_IMPL, A_INDX, A_ZERO , A_ZERO , A_ZERO , A_ZERO,	// 00
	A_IMPL, A_IMM , A_ACCU, A_IMM , A_ABS  , A_ABS  , A_ABS  , A_ABS,
	A_REL , A_INDY, A_IMPL, A_INDY, A_ZEROX, A_ZEROX, A_ZEROX, A_ZEROX,	// 10
	A_IMPL, A_ABSY, A_IMPL, A_ABSY, A_ABSX , A_ABSX , A_ABSX , A_ABSX,
	A_ABS , A_INDX, A_IMPL, A_INDX, A_ZERO , A_ZERO , A_ZERO , A_ZERO,	// 20
	A_IMPL, A_IMM , A_ACCU, A_IMM , A_ABS  , A_ABS  , A_ABS  , A_ABS,
	A_REL , A_INDY, A_IMPL, A_INDY, A_ZEROX, A_ZEROX, A_ZEROX, A_ZEROX,	// 30
	A_IMPL, A_ABSY, A_IMPL, A_ABSY, A_ABSX , A_ABSX , A_ABSX , A_ABSX,
	A_IMPL, A_INDX, A_IMPL, A_INDX, A_ZERO , A_ZERO , A_ZERO , A_ZERO,	// 40
	A_IMPL, A_IMM , A_ACCU, A_IMM , A_ABS  , A_ABS  , A_ABS  , A_ABS,
	A_REL , A_INDY, A_IMPL, A_INDY, A_ZEROX, A_ZEROX, A_ZEROX, A_ZEROX,	// 50
	A_IMPL, A_ABSY, A_IMPL, A_ABSY, A_ABSX , A_ABSX , A_ABSX , A_ABSX,
	A_IMPL, A_INDX, A_IMPL, A_INDX, A_ZERO , A_ZERO , A_ZERO , A_ZERO,	// 60
	A_IMPL, A_IMM , A_ACCU, A_IMM , A_IND  , A_ABS  , A_ABS  , A_ABS,
	A_REL , A_INDY, A_IMPL, A_INDY, A_ZEROX, A_ZEROX, A_ZEROX, A_ZEROX,	// 70
	A_IMPL, A_ABSY, A_IMPL, A_ABSY, A_ABSX , A_ABSX , A_ABSX , A_ABSX,
	A_IMM , A_INDX, A_IMM , A_INDX, A_ZERO , A_ZERO , A_ZERO , A_ZERO,	// 80
	A_IMPL, A_IMM , A_IMPL, A_IMM , A_ABS  , A_ABS  , A_ABS  , A_ABS,
	A_REL , A_INDY, A_IMPL, A_INDY, A_ZEROX, A_ZEROX, A_ZEROY, A_ZEROY,	// 90
	A_IMPL, A_ABSY, A_IMPL, A_ABSY, A_ABSX , A_ABSX , A_ABSY , A_ABSY,
	A_IMM , A_INDX, A_IMM , A_INDX, A_ZERO , A_ZERO , A_ZERO , A_ZERO,	// a0
	A_IMPL, A_IMM , A_IMPL, A_IMM , A_ABS  , A_ABS  , A_ABS  , A_ABS,
	A_REL , A_INDY, A_IMPL, A_INDY, A_ZEROX, A_ZEROX, A_ZEROY, A_ZEROY,	// b0
	A_IMPL, A_ABSY, A_IMPL, A_ABSY, A_ABSX , A_ABSX , A_ABSY , A_ABSY,
	A_IMM , A_INDX, A_IMM , A_INDX, A_ZERO , A_ZERO , A_ZERO , A_ZERO,	// c0
	A_IMPL, A_IMM , A_IMPL, A_IMM , A_ABS  , A_ABS  , A_ABS  , A_ABS,
	A_REL , A_INDY, A_IMPL, A_INDY, A_ZEROX, A_ZEROX, A_ZEROX, A_ZEROX,	// d0
	A_IMPL, A_ABSY, A_IMPL, A_ABSY, A_ABSX , A_ABSX , A_ABSX , A_ABSX,
	A_IMM , A_INDX, A_IMM , A_INDX, A_ZERO , A_ZERO , A_ZERO , A_ZERO,	// e0
	A_IMPL, A_IMM , A_IMPL, A_IMM , A_ABS  , A_ABS  , A_ABS  , A_ABS,
	A_REL , A_INDY, A_IMPL, A_INDY, A_ZEROX, A_ZEROX, A_ZEROX, A_ZEROX,	// f0
	A_IMPL, A_ABSY, A_IMPL, A_ABSY, A_ABSX , A_ABSX , A_ABSX , A_ABSX
};

// Chars for each mnemonic
static const char mnem_1[] = "aaabbbbbbbbbbcccccccdddeiiijjllllnopppprrrrssssssstttttt?aaaadijnlllrrsssssssss";
static const char mnem_2[] = "dnscceimnprvvllllmppeeeonnnmsdddsorhhlloottbeeetttaasxxy?nnrscsaoaaxlrabbhhhhlr";
static const char mnem_3[] = "cdlcsqtielkcscdivpxycxyrcxypraxyrpaapaplrisccdiaxyxyxasa?cerrpbmpsxaaaxcxasxyoe";

// Instruction length for each addressing mode
static const char adr_length[] = {1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2};


// Prototypes
static void error(char *s);
static void handle_abort(...);
static void init_abort(void);
static void exit_abort(void);
static bool aborted(void);

static void read_line(void);			// Scanner
static char get_char(void);
static void put_back(char c);
static enum Token get_token(void);
static enum Token get_reg_token(void);
static uint16 get_number(void);
static enum Token get_string(char *str);

static bool expression(uint16 *number);	// Parser
static bool term(uint16 *number);
static bool factor(uint16 *number);
static bool address_args(void);
static bool range_args(int def_range);
static bool instr_args(uint16 *number, char *mode);

static void help(void);				// Routines for commands
static void registers(void);
static void display_registers(void);
static void memory_dump(void);
static void ascii_dump(void);
static char conv_from_64(char c);
static void screen_dump(void);
static char conv_from_scode(char c);
static void binary_dump(void);
static void sprite_dump(void);
static void byte_to_bin(uint8 byte, char *str);
static void disassemble(void);
static int disass_line(uint16 adr, uint8 op, uint8 lo, uint8 hi);
static void assemble(void);
static char find_mnemonic(char op1, char op2, char op3);
static bool find_opcode(char mnem, char mode, uint8 *opcode);
static void mem_config(void);
static void fill(void);
static void compare(void);
static void transfer(void);
static void modify(void);
static void print_expr(void);
static void redir_output(void);
static void int_vectors(void);
static void view_state(void);
static void view_cia_state(void);
static void dump_cia_ints(uint8 i);
static void view_sid_state(void);
static void dump_sid_waveform(uint8 wave);
static void view_vic_state(void);
static void dump_spr_flags(uint8 f);
static void dump_vic_ints(uint8 i);
static void view_1541_state(void);
static void dump_via_ints(uint8 i);
static void load_data(void);
static void save_data(void);


/*
 *  Open and handle SAM
 */

void SAM(C64 *the_c64)
{
	bool done = false;
	char c;

	TheCPU = the_c64->TheCPU;
	TheCPU1541 = the_c64->TheCPU1541;
	TheVIC = the_c64->TheVIC;
	TheSID = the_c64->TheSID;
	TheCIA1 = the_c64->TheCIA1;
	TheCIA2 = the_c64->TheCIA2;

	// Get CPU registers and current memory configuration
	TheCPU->GetState(&R64);
	TheCPU->ExtConfig = (~R64.ddr | R64.pr) & 7;
	TheCPU1541->GetState(&R1541);

#ifdef __riscos__
	Wimp_CommandWindow((int)"SAM");
#endif

#ifdef AMIGA
	if (!(fin = fout = ferr = fopen("CON:0/0/640/480/SAM", "w+")))
		return;
#else
	fin = stdin;
	fout = stdout;
	ferr = stdout;
#endif

	access_1541 = false;
	address = R64.pc;

	fprintf(ferr, "\n *** SAM - Simple Assembler and Monitor ***\n ***         Press 'h' for help         ***\n\n");
	init_abort();
	display_registers();

	while (!done) {
		if (access_1541)
			fprintf(ferr, "1541> ");
		else
			fprintf(ferr, "C64> ");
		fflush(ferr);
		read_line();
		while ((c = get_char()) == ' ') ;

		switch (c) {
			case 'a':		// Assemble
				get_token();
				assemble();
				break;

			case 'b':		// Binary dump
				get_token();
				binary_dump();
				break;

			case 'c':		// Compare
				get_token();
				compare();
				break;

			case 'd':		// Disassemble
				get_token();
				disassemble();
				break;

			case 'e':       // Interrupt vectors
				int_vectors();
				break;

			case 'f':		// Fill
				get_token();
				fill();
				break;

			case 'h':		// Help
				help();
				break;

			case 'i':		// ASCII dump
				get_token();
				ascii_dump();
				break;

			case 'k':		// Memory configuration
				get_token();
				mem_config();
				break;

			case 'l':		// Load data
				get_token();
				load_data();
				break;

			case 'm':		// Memory dump
				get_token();
				memory_dump();
				break;

			case 'n':		// Screen code dump
				get_token();
				screen_dump();
				break;

			case 'o':		// Redirect output
				get_token();
				redir_output();
				break;

			case 'p':		// Sprite dump
				get_token();
				sprite_dump();
				break;

			case 'r':		// Registers
				get_reg_token();
				registers();
				break;

			case 's':		// Save data
				get_token();
				save_data();
				break;

			case 't':		// Transfer
				get_token();
				transfer();
				break;

			case 'v':		// View machine state
				view_state();
				break;

			case 'x':		// Exit
				done = true;
				break;

			case ':':		// Change memory
				get_token();
				modify();
				break;

			case '1':		// Switch to 1541 mode
				access_1541 = true;
				break;

			case '6':		// Switch to C64 mode
				access_1541 = false;
				break;

			case '?':		// Compute expression
				get_token();
				print_expr();
				break;

			case '\n':		// Blank line
				break;

			default:		// Unknown command
				error("Unknown command");
				break;
		}
	}

	exit_abort();

#ifdef AMIGA
	fclose(fin);
#endif
	if (fout != ferr)
		fclose(fout);

#ifdef __riscos__
	Wimp_CommandWindow(-1);
#endif

	// Set CPU registers
	TheCPU->SetState(&R64);
	TheCPU1541->SetState(&R1541);
}


/*
 *  Print error message
 */

static void error(char *s)
{
	fprintf(ferr, "*** %s\n", s);
}


/*
 *  CTRL-C pressed?
 */

static bool WasAborted;

#ifdef HAVE_SIGACTION
struct sigaction my_sa;
#endif

static void handle_abort(...)
{
	WasAborted = true;
#if !defined(HAVE_SIGACTION) && defined(HAVE_SIGNAL)
#ifdef __riscos__
	signal(SIGINT, (Handler*) handle_abort);
#else
	signal(SIGINT, (sighandler_t) handle_abort);
#endif
#endif
}

static void init_abort(void)
{
	WasAborted = false;
#if defined(HAVE_SIGACTION)
	my_sa.sa_handler = (void (*)(int))handle_abort;
	my_sa.sa_flags = 0;
	sigemptyset(&my_sa.sa_mask);
	sigaction(SIGINT, &my_sa, NULL);
#elif defined(HAVE_SIGNAL)
#ifdef __riscos__
	signal(SIGINT, (Handler*) handle_abort);
#else
	signal(SIGINT, (sighandler_t) handle_abort);
#endif
#endif
}

static void exit_abort(void)
{
#if defined(HAVE_SIGACTION)
	my_sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &my_sa, NULL);
#elif defined(HAVE_SIGNAL)
	signal(SIGINT, SIG_DFL);
#endif
}

static bool aborted(void)
{
	bool ret = WasAborted;

	WasAborted = false;
	return ret;
}


/*
 *  Read a line from the keyboard
 */

static void read_line(void)
{
#ifdef __riscos__
	OS_ReadLine(in_ptr = input, INPUT_LENGTH, 0, 255, 0);
#else
	fgets(in_ptr = input, INPUT_LENGTH, fin);
#endif
}


/*
 *  Read a character from the input line
 */

static char get_char(void)
{
	return *in_ptr++;
}


/*
 *  Stuff back a character into the input line
 */

static void put_back(char c)
{
	*(--in_ptr) = c;
}


/*
 *  Scanner: Get a token from the input line
 */

static enum Token get_token(void)
{
	char c;

	// Skip spaces
	while ((c = get_char()) == ' ') ;

	switch (c) {
		case '\n':
			return the_token = T_END;
		case '(':
			return the_token = T_LPAREN;
		case ')':
			return the_token = T_RPAREN;
		case '+':
			return the_token = T_ADD;
		case '-':
			return the_token = T_SUB;
		case '*':
			return the_token = T_MUL;
		case '/':
			return the_token = T_DIV;
		case ',':
			return the_token = T_COMMA;
		case '#':
			return the_token = T_IMMED;
		case 'x':
			return the_token = T_X;
		case 'y':
			return the_token = T_Y;
		case 'p':
			if (get_char() == 'c')
				return the_token = T_PC;
			else {
				error("Unrecognized token");
				return the_token = T_NULL;
			}
		case 's':
			if (get_char() == 'p')
				return the_token = T_SP;
			else {
				error("Unrecognized token");
				return the_token = T_NULL;
			}
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			put_back(c);
			the_number = get_number();
			return the_token = T_NUMBER;
		case '"':
			return the_token = get_string(the_string);
		default:
			error("Unrecognized token");
			return the_token = T_NULL;
	}
}

static enum Token get_reg_token(void)
{
	char c;

	// Skip spaces
	while ((c = get_char()) == ' ') ;

	switch (c) {
		case '\n':
			return the_token = T_END;
		case 'a':
			return the_token = T_A;
		case 'd':
			if (get_char() == 'r')
				return the_token = T_DR;
			else {
				error("Unrecognized token");
				return the_token = T_NULL;
			}
		case 'p':
			if ((c = get_char()) == 'c')
				return the_token = T_PC;
			else if (c == 'r')
				return the_token = T_PR;
			else {
				error("Unrecognized token");
				return the_token = T_NULL;
			}
		case 's':
			if (get_char() == 'p')
				return the_token = T_SP;
			else {
				error("Unrecognized token");
				return the_token = T_NULL;
			}
		case 'x':
			return the_token = T_X;
		case 'y':
			return the_token = T_Y;
		default:
			error("Unrecognized token");
			return the_token = T_NULL;
	}
}

static uint16 get_number(void)
{
	char c;
	uint16 i = 0;

	while (((c = get_char()) >= '0') && (c <= '9') || (c >= 'a') && (c <= 'f'))
		if (c < 'a')
			i = (i << 4) + (c - '0');
		else
			i = (i << 4) + (c - 'a' + 10);

	put_back(c);
	return i;
}

static enum Token get_string(char *str)
{
	char c;

	while ((c = get_char()) != '\n') {
		if (c == '"') {
			*str = 0;
			return T_STRING;
		}
		*str++ = c;
	}

	error("Unterminated string");
	return T_NULL;
}


/*
 *  expression = term {(ADD | SUB) term}
 *  true: OK, false: Error
 */

static bool expression(uint16 *number)
{
	uint16 accu, trm;

	if (!term(&accu))
		return false;

	for (;;)
		switch (the_token) {
			case T_ADD:
				get_token();
				if (!term(&trm))
					return false;
				accu += trm;
				break;

			case T_SUB:
				get_token();
				if (!term(&trm))
					return false;
				accu -= trm;
				break;

			default:
				*number = accu;
				return true;
		}
}


/*
 *  term = factor {(MUL | DIV) factor}
 *  true: OK, false: Error
 */

static bool term(uint16 *number)
{
	uint16 accu, fact;

	if (!factor(&accu))
		return false;

	for (;;)
		switch (the_token) {
			case T_MUL:
				get_token();
				if (!factor(&fact))
					return false;
				accu *= fact;
				break;

			case T_DIV:
				get_token();
				if (!factor(&fact))
					return false;
				if (fact == 0) {
					error("Division by 0");
					return false;
				}
				accu /= fact;
				break;

			default:
				*number = accu;
				return true;
		}
}


/*
 *  factor = NUMBER | PC | SP | LPAREN expression RPAREN
 *  true: OK, false: Error
 */

static bool factor(uint16 *number)
{
	switch (the_token) {
		case T_NUMBER:
			*number = the_number;
			get_token();
			return true;

		case T_PC:
			get_token();
			*number = access_1541 ? R1541.pc : R64.pc;
			return true;

		case T_SP:
			get_token();
			*number = access_1541 ? R1541.sp : R64.sp;
			return true;

		case T_LPAREN:
			get_token();
			if (expression(number))
				if (the_token == T_RPAREN) {
					get_token();
					return true;
				} else {
					error("Missing ')'");
					return false;
				}
			else {
				error("Error in expression");
				return false;
			}

		case T_END:
			error("Required argument missing");
			return false;

		default:
			error("'pc', 'sp', '(' or number expected");
			return false;
	}
}


/*
 *  address_args = [expression] END
 *
 *  Read start to "address"
 *
 *  true: OK, false: Error
 */

static bool address_args(void)
{
	if (the_token == T_END)
		return true;
	else {
		if (!expression(&address))
			return false;
		return the_token == T_END;
	}
}


/*
 *  range_args = [expression] [[COMMA] expression] END
 *
 *  Read start address to "address", end address to "end_address"
 *
 *  true: OK, false: Error
 */

static bool range_args(int def_range)
{
	end_address = address + def_range;

	if (the_token == T_END)
		return true;
	else {
		if (!expression(&address))
			return false;
		end_address = address + def_range;
		if (the_token == T_END)
			return true;
		else {
			if (the_token == T_COMMA) get_token();
			if (!expression(&end_address))
				return false;
			return the_token == T_END;
		}
	}
}


/*
 *  instr_args = END
 *             | IMMED NUMBER END
 *             | NUMBER [COMMA (X | Y)] END
 *             | LPAREN NUMBER (RPAREN [COMMA Y] | COMMA X RPAREN) END
 *
 *  Read arguments of a 6510 instruction, determine address and addressing mode
 *
 *  true: OK, false: Error
 */

static bool instr_args(uint16 *number, char *mode)
{
	switch (the_token) {

		case T_END:
			*mode = A_IMPL;
			return true;

		case T_IMMED:
			get_token();
			if (the_token == T_NUMBER) {
				*number = the_number;
				*mode = A_IMM;
				get_token();
				return the_token == T_END;
			} else {
				error("Number expected");
				return false;
			}

		case T_NUMBER:
			*number = the_number;
			get_token();
			switch (the_token) {

				case T_END:
					if (*number < 0x100)
						*mode = A_ZERO;
					else
						*mode = A_ABS;
					return true;

				case T_COMMA:
					get_token();
					switch (the_token) {

						case T_X:
							get_token();
							if (*number < 0x100)
								*mode = A_ZEROX;
							else
								*mode = A_ABSX;
							return the_token == T_END;

						case T_Y:
							get_token();
							if (*number < 0x100)
								*mode = A_ZEROY;
							else
								*mode = A_ABSY;
							return the_token == T_END;

						default:
							error("Illegal index register");
							return false;
					}

				default:
					return false;
			}

		case T_LPAREN:
			get_token();
			if (the_token == T_NUMBER) {
				*number = the_number;
				get_token();
				switch (the_token) {

					case T_RPAREN:
						get_token();
						switch (the_token) {

							case T_END:
								*mode = A_IND;
								return true;

							case T_COMMA:
								get_token();
								if (the_token == T_Y) {
									*mode = A_INDY;
									get_token();
									return the_token == T_END;
								} else {
									error("Only 'y' index register allowed");
									return false;
								}

							default:
								error("Illegal characters after ')'");
								return false;
						}

					case T_COMMA:
						get_token();
						if (the_token == T_X) {
							get_token();
							if (the_token == T_RPAREN) {
								*mode = A_INDX;
								get_token();
								return the_token == T_END;
							} else {
								error("')' expected");
								return false;
							}
						} else {
							error("Only 'x' index register allowed");
							return false;
						}

					default:
						error("')' or ',' expected");
						return false;
				}
			} else {
				error("Number expected");
				return false;
			}

		default:
			error("'(', '#' or number expected");
			return false;
	}
}


/*
 *  Display help
 *  h
 */

static void help(void)
{
	fprintf(fout, "a [start]           Assemble\n"
				"b [start] [end]     Binary dump\n"
				"c start end dest    Compare memory\n"
				"d [start] [end]     Disassemble\n"
				"e                   Show interrupt vectors\n"
				"f start end byte    Fill memory\n"
				"i [start] [end]     ASCII/PETSCII dump\n"
				"k [config]          Show/set C64 memory configuration\n"
				"l start \"file\"      Load data\n"
				"m [start] [end]     Memory dump\n"
				"n [start] [end]     Screen code dump\n"
				"o [\"file\"]          Redirect output\n"
				"p [start] [end]     Sprite dump\n"
				"r [reg value]       Show/set CPU registers\n"
				"s start end \"file\"  Save data\n"
				"t start end dest    Transfer memory\n"
				"vc1                 View CIA 1 state\n"
				"vc2                 View CIA 2 state\n"
				"vf                  View 1541 state\n"
				"vs                  View SID state\n"
				"vv                  View VIC state\n"
				"x                   Return to Frodo\n"
				": addr {byte}       Modify memory\n"
				"1541                Switch to 1541\n"
				"64                  Switch to C64\n"
				"? expression        Calculate expression\n");
}


/*
 *  Display/change 6510 registers
 *  r [reg value]
 */

static void registers(void)
{
	enum Token the_reg;
	uint16 value;

	if (the_token != T_END)
		switch (the_reg = the_token) {
			case T_A:
			case T_X:
			case T_Y:
			case T_PC:
			case T_SP:
			case T_DR:
			case T_PR:
				get_token();
				if (!expression(&value))
					return;

				switch (the_reg) {
					case T_A:
						if (access_1541)
							R1541.a = value;
						else
							R64.a = value;
						break;
					case T_X:
						if (access_1541)
							R1541.x = value;
						else
							R64.x = value;
						break;
					case T_Y:
						if (access_1541)
							R1541.y = value;
						else
							R64.y = value;
						break;
					case T_PC:
						if (access_1541)
							R1541.pc = value;
						else
							R64.pc = value;
						break;
					case T_SP:
						if (access_1541)
							R1541.sp = (value & 0xff) | 0x0100;
						else
							R64.sp = (value & 0xff) | 0x0100;
						break;
					case T_DR:
						if (!access_1541)
							R64.ddr = value;
						break;
					case T_PR:
						if (!access_1541)
							R64.pr = value;
						break;
					default:
						break;
				}
				break;

			default:
				return;
		}

	display_registers();
}

static void display_registers(void)
{
	if (access_1541) {
		fprintf(fout, " PC  A  X  Y   SP  NVDIZC  Instruction\n");
		fprintf(fout, "%04lx %02lx %02lx %02lx %04lx %c%c%c%c%c%c ",
			R1541.pc, R1541.a, R1541.x, R1541.y, R1541.sp,
			R1541.p & 0x80 ? '1' : '0', R1541.p & 0x40 ? '1' : '0', R1541.p & 0x08 ? '1' : '0',
			R1541.p & 0x04 ? '1' : '0', R1541.p & 0x02 ? '1' : '0', R1541.p & 0x01 ? '1' : '0');
		disass_line(R1541.pc, SAMReadByte(R1541.pc), SAMReadByte(R1541.pc+1), SAMReadByte(R1541.pc+2));
	} else {
		fprintf(fout, " PC  A  X  Y   SP  DR PR NVDIZC  Instruction\n");
		fprintf(fout, "%04lx %02lx %02lx %02lx %04lx %02lx %02lx %c%c%c%c%c%c ",
			R64.pc, R64.a, R64.x, R64.y, R64.sp, R64.ddr, R64.pr,
			R64.p & 0x80 ? '1' : '0', R64.p & 0x40 ? '1' : '0', R64.p & 0x08 ? '1' : '0',
			R64.p & 0x04 ? '1' : '0', R64.p & 0x02 ? '1' : '0', R64.p & 0x01 ? '1' : '0');
		disass_line(R64.pc, SAMReadByte(R64.pc), SAMReadByte(R64.pc+1), SAMReadByte(R64.pc+2));
	}
}


/*
 *  Memory dump
 *  m [start] [end]
 */

#define MEMDUMP_BPL 16  // Bytes per line

static void memory_dump(void)
{
	bool done = false;
	short i;
	uint8 mem[MEMDUMP_BPL + 2];
	uint8 byte;

	mem[MEMDUMP_BPL] = 0;

	if (!range_args(16 * MEMDUMP_BPL - 1))  // 16 lines unless end address specified
		return;

	do {
		fprintf(fout, "%04lx:", address);
		for (i=0; i<MEMDUMP_BPL; i++, address++) {
			if (address == end_address) done = true;

			fprintf(fout, " %02lx", byte = SAMReadByte(address));
			if ((byte >= ' ') && (byte <= '~'))
				mem[i] = conv_from_64(byte);
			else
				mem[i] = '.';
		}
		fprintf(fout, "  '%s'\n", mem);
	} while (!done && !aborted());
}


/*
 *  ASCII dump
 *  i [start] [end]
 */

#define ASCIIDUMP_BPL 64  // Bytes per line

static void ascii_dump(void)
{
	bool done = false;
	short i;
	uint8 mem[ASCIIDUMP_BPL + 2];
	uint8 byte;

	mem[ASCIIDUMP_BPL] = 0;

	if (!range_args(16 * ASCIIDUMP_BPL - 1))  // 16 lines unless end address specified
		return;

	do {
		fprintf(fout, "%04lx:", address);
		for (i=0; i<ASCIIDUMP_BPL; i++, address++) {
			if (address == end_address) done = true;

			byte = SAMReadByte(address);
			if ((byte >= ' ') && (byte <= '~'))
				mem[i] = conv_from_64(byte);
			else
				mem[i] = '.';
		}
		fprintf(fout, " '%s'\n", mem);
	} while (!done && !aborted());
}


/*
 *  Convert PETSCII->ASCII
 */

static char conv_from_64(char c)
{
	if ((c >= 'A') && (c <= 'Z') || (c >= 'a') && (c <= 'z'))
		return c ^ 0x20;
	else
		return c;
}


/*
 *  Screen code dump
 *  n [start] [end]
 */

#define SCRDUMP_BPL 64  // Bytes per line

static void screen_dump(void)
{
	bool done = false;
	short i;
	uint8 mem[SCRDUMP_BPL + 2];
	uint8 byte;

	mem[SCRDUMP_BPL] = 0;

	if (!range_args(16 * SCRDUMP_BPL - 1))  // 16 Zeilen unless end address specified
		return;

	do {
		fprintf(fout, "%04lx:", address);
		for (i=0; i<SCRDUMP_BPL; i++, address++) {
			if (address == end_address) done = true;

			byte = SAMReadByte(address);
			if (byte < 90)
				mem[i] = conv_from_scode(byte);
			else
				mem[i] = '.';
		}
		fprintf(fout, " '%s'\n", mem);
	} while (!done && !aborted());
}


/*
 *  Convert screen code->ASCII
 */

static char conv_from_scode(char c)
{
	c &= 0x7f;

	if (c <= 31)
		return c + 64;
	else
		if (c >= 64)
			return c + 32;
		else
			return c;
}


/*
 *  Binary dump
 *  b [start] [end]
 */

static void binary_dump(void)
{
	bool done = false;
	char bin[10];

	bin[8] = 0;

	if (!range_args(7))  // 8 lines unless end address specified
		return;

	do {
		if (address == end_address) done = true;

		byte_to_bin(SAMReadByte(address), bin);
		fprintf(fout, "%04lx: %s\n", address++, bin);
	} while (!done && !aborted());
}


/*
 *  Sprite data dump
 *  p [start] [end]
 */

static void sprite_dump(void)
{
	bool done = false;
	short i;
	char bin[10];

	bin[8] = 0;

	if (!range_args(21 * 3 - 1))  // 21 lines unless end address specified
		return;

	do {
		fprintf(fout, "%04lx: ", address);
		for (i=0; i<3; i++, address++) {
			if (address == end_address) done = true;

			byte_to_bin(SAMReadByte(address), bin);
			fprintf(fout, "%s", bin);
		}
		fputc('\n', fout);
	} while (!done && !aborted());
}


/*
 *  Convert byte to binary representation
 */

static void byte_to_bin(uint8 byte, char *str)
{
	short i;

	for (i=0; i<8; i++, byte<<=1)
		if (byte & 0x80)
			str[i] = '#';
		else
			str[i] = '.';
}


/*
 *  Disassemble
 *  d [start] [end]
 */

static void disassemble(void)
{
	bool done = false;
	short i;
	uint8 op[3];
	uint16 adr;

	if (!range_args(31))  // 32 bytes unless end address specified
		return;

	do {
		fprintf(fout, "%04lx:", adr = address);
		for (i=0; i<3; i++, adr++) {
			if (adr == end_address) done = true;
			op[i] = SAMReadByte(adr);
		}
		address += disass_line(address, op[0], op[1], op[2]);
	} while (!done && !aborted());
}


/*
 *  Disassemble one instruction, return length
 */

static int disass_line(uint16 adr, uint8 op, uint8 lo, uint8 hi)
{
	char mode = adr_mode[op], mnem = mnemonic[op];

	// Display instruction bytes in hex
	switch (adr_length[mode]) {
		case 1:
			fprintf(fout, " %02lx       ", op);
			break;

		case 2:
			fprintf(fout, " %02lx %02lx    ", op, lo);
			break;

		case 3:
			fprintf(fout, " %02lx %02lx %02lx ", op, lo, hi);
			break;
	}

	// Tag undocumented opcodes with an asterisk
	if (mnem > M_ILLEGAL)
		fputc('*', fout);
	else
		fputc(' ', fout);

	// Print mnemonic
	fprintf(fout, "%c%c%c ", mnem_1[mnem], mnem_2[mnem], mnem_3[mnem]);

	// Pring argument
	switch (mode) {
		case A_IMPL:
			break;

		case A_ACCU:
			fprintf(fout, "a");
			break;

		case A_IMM:
			fprintf(fout, "#%02lx", lo);
			break;

		case A_REL:
			fprintf(fout, "%04lx", ((adr + 2) + (int8)lo) & 0xffff);
			break;

		case A_ZERO:
			fprintf(fout, "%02lx", lo);
			break;

		case A_ZEROX:
			fprintf(fout, "%02lx,x", lo);
			break;

		case A_ZEROY:
			fprintf(fout, "%02lx,y", lo);
			break;

		case A_ABS:
			fprintf(fout, "%04lx", (hi << 8) | lo);
			break;

		case A_ABSX:
			fprintf(fout, "%04lx,x", (hi << 8) | lo);
			break;

		case A_ABSY:
			fprintf(fout, "%04lx,y", (hi << 8) | lo);
			break;

		case A_IND:
			fprintf(fout, "(%04lx)", (hi << 8) | lo);
			break;

		case A_INDX:
			fprintf(fout, "(%02lx,x)", lo);
			break;

		case A_INDY:
			fprintf(fout, "(%02lx),y", lo);
			break;
	}

	fputc('\n', fout);
	return adr_length[mode];
}


/*
 *  Assemble
 *  a [start]
 */

static void assemble(void)
{
	bool done = false;
	char c1, c2, c3;
	char mnem, mode;
	uint8 opcode;
	uint16 arg;
	int16 rel;

	// Read parameters
	if (!address_args())
		return;

	do {
		fprintf(fout, "%04lx> ", address);
		fflush(ferr);
		read_line();

		c1 = get_char();
		c2 = get_char();
		c3 = get_char();

		if (c1 != '\n') {

			if ((mnem = find_mnemonic(c1, c2, c3)) != M_ILLEGAL) {

				get_token();
				if (instr_args(&arg, &mode)) {

					// Convert A_IMPL -> A_ACCU if necessary
					if ((mode == A_IMPL) && find_opcode(mnem, A_ACCU, &opcode))
						mode = A_ACCU;

					// Handle relative addressing seperately
					if (((mode == A_ABS) || (mode == A_ZERO)) && find_opcode(mnem, A_REL, &opcode)) {
						mode = A_REL;
						rel = arg - (address + 2) & 0xffff;
						if ((rel < -128) || (rel > 127)) {
							error("Branch too long");
							continue;
						} else
							arg = rel & 0xff;
					}

					if (find_opcode(mnem, mode, &opcode)) {

						// Print disassembled line
						fprintf(fout, "\v%04lx:", address);
						disass_line(address, opcode, arg & 0xff, arg >> 8);

						switch (adr_length[mode]) {
							case 1:
								SAMWriteByte(address++, opcode);
								break;

							case 2:
								SAMWriteByte(address++, opcode);
								SAMWriteByte(address++, arg);
								break;

							case 3:
								SAMWriteByte(address++, opcode);
								SAMWriteByte(address++, arg & 0xff);
								SAMWriteByte(address++, arg >> 8);
								break;

							default:
								error("Internal error");
								break;
						}

					} else
						error("Addressing mode not supported by instruction");

				} else
					error("Unrecognized addressing mode");

			} else
				error("Unknown instruction");

		} else			// Input is terminated with a blank line
			done = true;
	} while (!done);
}


/*
 *  Find mnemonic code to three letters
 *  M_ILLEGAL: No matching mnemonic found
 */

static char find_mnemonic(char op1, char op2, char op3)
{
	int i;

	for (i=0; i<M_MAXIMUM; i++)
		if ((mnem_1[i] == op1) && (mnem_2[i] == op2) && (mnem_3[i] == op3))
			return i;

	return M_ILLEGAL;
}


/*
 *  Determine opcode of an instruction given mnemonic and addressing mode
 *  true: OK, false: Mnemonic can't have specified addressing mode
 */

static bool find_opcode(char mnem, char mode, uint8 *opcode)
{
	int i;

	for (i=0; i<256; i++)
		if ((mnemonic[i] == mnem) && (adr_mode[i] == mode)) {
			*opcode = i;
			return true;
		}

	return false;
}


/*
 *  Show/set memory configuration
 *  k [config]
 */

static void mem_config(void)
{
	uint16 con;

	if (the_token != T_END)
		if (!expression(&con))
			return;
		else
			TheCPU->ExtConfig = con;
	else
		con = TheCPU->ExtConfig;

	fprintf(fout, "Configuration: %ld\n", con & 7);
	fprintf(fout, "A000-BFFF: %s\n", (con & 3) == 3 ? "Basic" : "RAM");
	fprintf(fout, "D000-DFFF: %s\n", (con & 3) ? ((con & 4) ? "I/O" : "Char") : "RAM");
	fprintf(fout, "E000-FFFF: %s\n", (con & 2) ? "Kernal" : "RAM");
}


/*
 *  Fill
 *  f start end byte
 */

static void fill(void)
{
	bool done = false;
	uint16 adr, end_adr, value;

	if (!expression(&adr))
		return;
	if (!expression(&end_adr))
		return;
	if (!expression(&value))
		return;

	do {
		if (adr == end_adr) done = true;

		SAMWriteByte(adr++, value);
	} while (!done);
}


/*
 *  Compare
 *  c start end dest
 */

static void compare(void)
{
	bool done = false;
	uint16 adr, end_adr, dest;
	int num = 0;

	if (!expression(&adr))
		return;
	if (!expression(&end_adr))
		return;
	if (!expression(&dest))
		return;

	do {
		if (adr == end_adr) done = true;

		if (SAMReadByte(adr) != SAMReadByte(dest)) {
			fprintf(fout, "%04lx ", adr);
			num++;
			if (!(num & 7))
				fputc('\n', fout);
		}
		adr++; dest++;
	} while (!done && !aborted());

	if (num & 7)
		fputc('\n', fout);
	fprintf(fout, "%ld byte(s) different\n", num);
}


/*
 *  Transfer memory
 *  t start end dest
 */

static void transfer(void)
{
	bool done = false;
	uint16 adr, end_adr, dest;

	if (!expression(&adr))
		return;
	if (!expression(&end_adr))
		return;
	if (!expression(&dest))
		return;

	if (dest < adr)
		do {
			if (adr == end_adr) done = true;
			SAMWriteByte(dest++, SAMReadByte(adr++));
		} while (!done);
	else {
		dest += end_adr - adr;
		do {
			if (adr == end_adr) done = true;
			SAMWriteByte(dest--, SAMReadByte(end_adr--));
		} while (!done);
	}
}


/*
 *  Change memory
 *  : addr {byte}
 */

static void modify(void)
{
	uint16 adr, val;

	if (!expression(&adr))
		return;

	while (the_token != T_END)
		if (expression(&val))
			SAMWriteByte(adr++, val);
		else
			return;
}


/*
 *  Compute and display expression
 *  ? expression
 */

static void print_expr(void)
{
	uint16 val;

	if (!expression(&val))
		return;

	fprintf(fout, "Hex: %04lx\nDec: %lu\n", val, val);
}


/*
 *  Redirect output
 *  o [file]
 */

static void redir_output(void)
{
	// Close old file
	if (fout != ferr) {
		fclose(fout);
		fout = ferr;
		return;
	}

	// No argument given?
	if (the_token == T_END)
		return;

	// Otherwise open file
	if (the_token == T_STRING) {
		if (!(fout = fopen(the_string, "w")))
			error("Unable to open file");
	} else
		error("'\"' around file name expected");
}


/*
 *  Display interrupt vectors
 */

static void int_vectors(void)
{
	fprintf(fout, "        IRQ  BRK  NMI\n");
	fprintf(fout, "%d  : %04lx %04lx %04lx\n",
		access_1541 ? 6502 : 6510,
		SAMReadByte(0xffff) << 8 | SAMReadByte(0xfffe),
		SAMReadByte(0xffff) << 8 | SAMReadByte(0xfffe),
		SAMReadByte(0xfffb) << 8 | SAMReadByte(0xfffa));

	if (!access_1541 && TheCPU->ExtConfig & 2)
		fprintf(fout, "Kernal: %04lx %04lx %04lx\n",
			SAMReadByte(0x0315) << 8 | SAMReadByte(0x0314),
			SAMReadByte(0x0317) << 8 | SAMReadByte(0x0316),
			SAMReadByte(0x0319) << 8 | SAMReadByte(0x0318));
}


/*
 *  Display state of custom chips
 */

static void view_state(void)
{
	switch (get_char()) {
		case 'c':		// CIA
			view_cia_state();
			break;

		case 's':		// SID
			view_sid_state();
			break;

		case 'v':		// VIC
			view_vic_state();
			break;

		case 'f':		// Floppy
			view_1541_state();
			break;

		default:
			error("Unknown command");
			break;
	}
}

static void view_cia_state(void)
{
	MOS6526State cs;

	switch (get_char()) {
		case '1':
			TheCIA1->GetState(&cs);
			break;
		case '2':
			TheCIA2->GetState(&cs);
			break;
		default:
			error("Unknown command");
			return;
	}

	fprintf(fout, "Timer A  : %s\n", cs.cra & 1 ? "On" : "Off");
	fprintf(fout, " Counter : %04lx  Latch: %04lx\n", (cs.ta_hi << 8) | cs.ta_lo, cs.latcha);
	fprintf(fout, " Run mode: %s\n", cs.cra & 8 ? "One-shot" : "Continuous");
	fprintf(fout, " Input   : %s\n", cs.cra & 0x20 ? "CNT" : "Phi2");
	fprintf(fout, " Output  : ");
	if (cs.cra & 2)
		if (cs.cra & 4)
			fprintf(fout, "PB6 Toggle\n\n");
		else
			fprintf(fout, "PB6 Pulse\n\n");
	else
		fprintf(fout, "None\n\n");

	fprintf(fout, "Timer B  : %s\n", cs.crb & 1 ? "On" : "Off");
	fprintf(fout, " Counter : %04lx  Latch: %04lx\n", (cs.tb_hi << 8) | cs.tb_lo, cs.latchb);
	fprintf(fout, " Run mode: %s\n", cs.crb & 8 ? "One-shot" : "Continuous");
	fprintf(fout, " Input   : ");
	if (cs.crb & 0x40)
		if (cs.crb & 0x20)
			fprintf(fout, "Timer A underflow (CNT high)\n");
		else
			fprintf(fout, "Timer A underflow\n");
	else
		if (cs.crb & 0x20)
			fprintf(fout, "CNT\n");
		else
			fprintf(fout, "Phi2\n");
	fprintf(fout, " Output  : ");
	if (cs.crb & 2)
		if (cs.crb & 4)
			fprintf(fout, "PB7 Toggle\n\n");
		else
			fprintf(fout, "PB7 Pulse\n\n");
	else
		fprintf(fout, "None\n\n");

	fprintf(fout, "TOD         : %lx%lx:%lx%lx:%lx%lx.%lx %s\n",
		(cs.tod_hr >> 4) & 1, cs.tod_hr & 0x0f,
		(cs.tod_min >> 4) & 7, cs.tod_min & 0x0f,
		(cs.tod_sec >> 4) & 7, cs.tod_sec & 0x0f,
		cs.tod_10ths & 0x0f, cs.tod_hr & 0x80 ? "PM" : "AM");
	fprintf(fout, "Alarm       : %lx%lx:%lx%lx:%lx%lx.%lx %s\n",
		(cs.alm_hr >> 4) & 1, cs.alm_hr & 0x0f,
		(cs.alm_min >> 4) & 7, cs.alm_min & 0x0f,
		(cs.alm_sec >> 4) & 7, cs.alm_sec & 0x0f,
		cs.alm_10ths & 0x0f, cs.alm_hr & 0x80 ? "PM" : "AM");
	fprintf(fout, "TOD input   : %s\n", cs.cra & 0x80 ? "50Hz" : "60Hz");
	fprintf(fout, "Write to    : %s registers\n\n", cs.crb & 0x80 ? "Alarm" : "TOD");

	fprintf(fout, "Serial data : %02lx\n", cs.sdr);
	fprintf(fout, "Serial mode : %s\n\n", cs.cra & 0x40 ? "Output" : "Input");

	fprintf(fout, "Pending int.: ");
	dump_cia_ints(cs.int_data);
	fprintf(fout, "Enabled int.: ");
	dump_cia_ints(cs.int_mask);
}

static void dump_cia_ints(uint8 i)
{
	if (i & 0x1f) {
		if (i & 1) fprintf(fout, "TA ");
		if (i & 2) fprintf(fout, "TB ");
		if (i & 4) fprintf(fout, "Alarm ");
		if (i & 8) fprintf(fout, "Serial ");
		if (i & 0x10) fprintf(fout, "Flag");
	} else
		fprintf(fout, "None");
	fputc('\n', fout);
}

static void view_sid_state(void)
{
	MOS6581State ss;

	TheSID->GetState(&ss);

	fprintf(fout, "Voice 1\n");
	fprintf(fout, " Frequency  : %04lx\n", (ss.freq_hi_1 << 8) | ss.freq_lo_1);
	fprintf(fout, " Pulse Width: %04lx\n", ((ss.pw_hi_1 & 0x0f) << 8) | ss.pw_lo_1);
	fprintf(fout, " Env. (ADSR): %lx %lx %lx %lx\n", ss.AD_1 >> 4, ss.AD_1 & 0x0f, ss.SR_1 >> 4, ss.SR_1 & 0x0f);
	fprintf(fout, " Waveform   : ");
	dump_sid_waveform(ss.ctrl_1);
	fprintf(fout, " Gate       : %s  Ring mod.: %s\n", ss.ctrl_1 & 0x01 ? "On " : "Off", ss.ctrl_1 & 0x04 ? "On" : "Off");
	fprintf(fout, " Test bit   : %s  Synchron.: %s\n", ss.ctrl_1 & 0x08 ? "On " : "Off", ss.ctrl_1 & 0x02 ? "On" : "Off");
	fprintf(fout, " Filter     : %s\n", ss.res_filt & 0x01 ? "On" : "Off");

	fprintf(fout, "\nVoice 2\n");
	fprintf(fout, " Frequency  : %04lx\n", (ss.freq_hi_2 << 8) | ss.freq_lo_2);
	fprintf(fout, " Pulse Width: %04lx\n", ((ss.pw_hi_2 & 0x0f) << 8) | ss.pw_lo_2);
	fprintf(fout, " Env. (ADSR): %lx %lx %lx %lx\n", ss.AD_2 >> 4, ss.AD_2 & 0x0f, ss.SR_2 >> 4, ss.SR_2 & 0x0f);
	fprintf(fout, " Waveform   : ");
	dump_sid_waveform(ss.ctrl_2);
	fprintf(fout, " Gate       : %s  Ring mod.: %s\n", ss.ctrl_2 & 0x01 ? "On " : "Off", ss.ctrl_2 & 0x04 ? "On" : "Off");
	fprintf(fout, " Test bit   : %s  Synchron.: %s\n", ss.ctrl_2 & 0x08 ? "On " : "Off", ss.ctrl_2 & 0x02 ? "On" : "Off");
	fprintf(fout, " Filter     : %s\n", ss.res_filt & 0x02 ? "On" : "Off");

	fprintf(fout, "\nVoice 3\n");
	fprintf(fout, " Frequency  : %04lx\n", (ss.freq_hi_3 << 8) | ss.freq_lo_3);
	fprintf(fout, " Pulse Width: %04lx\n", ((ss.pw_hi_3 & 0x0f) << 8) | ss.pw_lo_3);
	fprintf(fout, " Env. (ADSR): %lx %lx %lx %lx\n", ss.AD_3 >> 4, ss.AD_3 & 0x0f, ss.SR_3 >> 4, ss.SR_3 & 0x0f);
	fprintf(fout, " Waveform   : ");
	dump_sid_waveform(ss.ctrl_3);
	fprintf(fout, " Gate       : %s  Ring mod.: %s\n", ss.ctrl_3 & 0x01 ? "On " : "Off", ss.ctrl_3 & 0x04 ? "On" : "Off");
	fprintf(fout, " Test bit   : %s  Synchron.: %s\n", ss.ctrl_3 & 0x08 ? "On " : "Off", ss.ctrl_3 & 0x02 ? "On" : "Off");
	fprintf(fout, " Filter     : %s  Mute     : %s\n", ss.res_filt & 0x04 ? "On" : "Off", ss.mode_vol & 0x80 ? "Yes" : "No");

	fprintf(fout, "\nFilters/Volume\n");
	fprintf(fout, " Frequency: %04lx\n", (ss.fc_hi << 3) | (ss.fc_lo & 0x07));
	fprintf(fout, " Resonance: %lx\n", ss.res_filt >> 4);
	fprintf(fout, " Mode     : ");
	if (ss.mode_vol & 0x70) {
		if (ss.mode_vol & 0x10) fprintf(fout, "Low-pass ");
		if (ss.mode_vol & 0x20) fprintf(fout, "Band-pass ");
		if (ss.mode_vol & 0x40) fprintf(fout, "High-pass");
	} else
		fprintf(fout, "None");
	fprintf(fout, "\n Volume   : %lx\n", ss.mode_vol & 0x0f);
}

static void dump_sid_waveform(uint8 wave)
{
	if (wave & 0xf0) {
		if (wave & 0x10) fprintf(fout, "Triangle ");
		if (wave & 0x20) fprintf(fout, "Sawtooth ");
		if (wave & 0x40) fprintf(fout, "Rectangle ");
		if (wave & 0x80) fprintf(fout, "Noise");
	} else
		fprintf(fout, "None");
	fputc('\n', fout);
}

static void view_vic_state(void)
{
	MOS6569State vs;
	short i;

	TheVIC->GetState(&vs);

	fprintf(fout, "Raster line       : %04lx\n", vs.raster | ((vs.ctrl1 & 0x80) << 1));
	fprintf(fout, "IRQ raster line   : %04lx\n\n", vs.irq_raster);

	fprintf(fout, "X scroll          : %ld\n", vs.ctrl2 & 7);
	fprintf(fout, "Y scroll          : %ld\n", vs.ctrl1 & 7);
	fprintf(fout, "Horizontal border : %ld columns\n", vs.ctrl2 & 8 ? 40 : 38);
	fprintf(fout, "Vertical border   : %ld rows\n\n", vs.ctrl1 & 8 ? 25 : 24);

	fprintf(fout, "Display mode      : ");
	switch (((vs.ctrl1 >> 4) & 6) | ((vs.ctrl2 >> 4) & 1)) {
		case 0:
			fprintf(fout, "Standard text\n");
			break;
		case 1:
			fprintf(fout, "Multicolor text\n");
			break;
		case 2:
			fprintf(fout, "Standard bitmap\n");
			break;
		case 3:
			fprintf(fout, "Multicolor bitmap\n");
			break;
		case 4:
			fprintf(fout, "ECM text\n");
			break;
		case 5:
			fprintf(fout, "Invalid text (ECM+MCM)\n");
			break;
		case 6:
			fprintf(fout, "Invalid bitmap (ECM+BMM)\n");
			break;
		case 7:
			fprintf(fout, "Invalid bitmap (ECM+BMM+ECM)\n");
			break;
	}
	fprintf(fout, "Sequencer state   : %s\n", vs.display_state ? "Display" : "Idle");
	fprintf(fout, "Bad line state    : %s\n", vs.bad_line ? "Yes" : "No");
	fprintf(fout, "Bad lines enabled : %s\n", vs.bad_line_enable ? "Yes" : "No");
	fprintf(fout, "Video counter     : %04lx\n", vs.vc);
	fprintf(fout, "Video counter base: %04lx\n", vs.vc_base);
	fprintf(fout, "Row counter       : %ld\n\n", vs.rc);

	fprintf(fout, "VIC bank          : %04lx-%04lx\n", vs.bank_base, vs.bank_base + 0x3fff);
	fprintf(fout, "Video matrix base : %04lx\n", vs.matrix_base);
	fprintf(fout, "Character base    : %04lx\n", vs.char_base);
	fprintf(fout, "Bitmap base       : %04lx\n\n", vs.bitmap_base);

	fprintf(fout, "         Spr.0  Spr.1  Spr.2  Spr.3  Spr.4  Spr.5  Spr.6  Spr.7\n");
	fprintf(fout, "Enabled: "); dump_spr_flags(vs.me);
	fprintf(fout, "Data   : %04lx   %04lx   %04lx   %04lx   %04lx   %04lx   %04lx   %04lx\n",
		vs.sprite_base[0], vs.sprite_base[1], vs.sprite_base[2], vs.sprite_base[3],
		vs.sprite_base[4], vs.sprite_base[5], vs.sprite_base[6], vs.sprite_base[7]);
	fprintf(fout, "MC     : %02lx     %02lx     %02lx     %02lx     %02lx     %02lx     %02lx     %02lx\n",
		vs.mc[0], vs.mc[1], vs.mc[2], vs.mc[3], vs.mc[4], vs.mc[5], vs.mc[6], vs.mc[7]);

	fprintf(fout, "Mode   : ");
	for (i=0; i<8; i++)
		if (vs.mmc & (1<<i))
			fprintf(fout, "Multi  ");
		else
			fprintf(fout, "Std.   ");

	fprintf(fout, "\nX-Exp. : "); dump_spr_flags(vs.mxe);
	fprintf(fout, "Y-Exp. : "); dump_spr_flags(vs.mye);

	fprintf(fout, "Prio.  : ");
	for (i=0; i<8; i++)
		if (vs.mdp & (1<<i))
			fprintf(fout, "Back   ");
		else
			fprintf(fout, "Fore   ");

	fprintf(fout, "\nSS Coll: "); dump_spr_flags(vs.mm);
	fprintf(fout, "SD Coll: "); dump_spr_flags(vs.md);

	fprintf(fout, "\nPending interrupts: ");
	dump_vic_ints(vs.irq_flag);
	fprintf(fout, "Enabled interrupts: ");
	dump_vic_ints(vs.irq_mask);
}

static void dump_spr_flags(uint8 f)
{
	short i;

	for (i=0; i<8; i++)
		if (f & (1<<i))
			fprintf(fout, "Yes    ");
		else
			fprintf(fout, "No     ");

	fputc('\n', fout);
}

static void dump_vic_ints(uint8 i)
{
	if (i & 0x1f) {
		if (i & 1) fprintf(fout, "Raster ");
		if (i & 2) fprintf(fout, "Spr-Data ");
		if (i & 4) fprintf(fout, "Spr-Spr ");
		if (i & 8) fprintf(fout, "Lightpen");
	} else
		fprintf(fout, "None");
	fputc('\n', fout);
}

static void view_1541_state(void)
{
	fprintf(fout, "VIA 1:\n");
	fprintf(fout, " Timer 1 Counter: %04x  Latch: %04x\n", R1541.via1_t1c, R1541.via1_t1l);
	fprintf(fout, " Timer 2 Counter: %04x  Latch: %04x\n", R1541.via1_t2c, R1541.via1_t2l);
	fprintf(fout, " ACR: %02x\n", R1541.via1_acr);
	fprintf(fout, " PCR: %02x\n", R1541.via1_pcr);
	fprintf(fout, " Pending interrupts: ");
	dump_via_ints(R1541.via1_ifr);
	fprintf(fout, " Enabled interrupts: ");
	dump_via_ints(R1541.via1_ier);

	fprintf(fout, "\nVIA 2:\n");
	fprintf(fout, " Timer 1 Counter: %04x  Latch: %04x\n", R1541.via2_t1c, R1541.via2_t1l);
	fprintf(fout, " Timer 2 Counter: %04x  Latch: %04x\n", R1541.via2_t2c, R1541.via2_t2l);
	fprintf(fout, " ACR: %02x\n", R1541.via2_acr);
	fprintf(fout, " PCR: %02x\n", R1541.via2_pcr);
	fprintf(fout, " Pending interrupts: ");
	dump_via_ints(R1541.via2_ifr);
	fprintf(fout, " Enabled interrupts: ");
	dump_via_ints(R1541.via2_ier);
}

static void dump_via_ints(uint8 i)
{
	if (i & 0x7f) {
		if (i & 0x40) fprintf(fout, "T1 ");
		if (i & 0x20) fprintf(fout, "T2 ");
		if (i & 2) fprintf(fout, "CA1 ");
		if (i & 1) fprintf(fout, "CA2 ");
		if (i & 0x10) fprintf(fout, "CB1 ");
		if (i & 8) fprintf(fout, "CB2 ");
		if (i & 4) fprintf(fout, "Serial ");
	} else
		fprintf(fout, "None");
	fputc('\n', fout);
}


/*
 *  Load data
 *  l start "file"
 */

static void load_data(void)
{
	uint16 adr;
	FILE *file;
	int fc;

	if (!expression(&adr))
		return;
	if (the_token == T_END) {
		error("Missing file name");
		return;
	}
	if (the_token != T_STRING) {
		error("'\"' around file name expected");
		return;
	}

	if (!(file = fopen(the_string, "rb")))
		error("Unable to open file");
	else {
		while ((fc = fgetc(file)) != EOF)
			SAMWriteByte(adr++, fc);
		fclose(file);
	}
}


/*
 *  Save data
 *  s start end "file"
 */

static void save_data(void)
{
	bool done = false;
	uint16 adr, end_adr;
	FILE *file;

	if (!expression(&adr))
		return;
	if (!expression(&end_adr))
		return;
	if (the_token == T_END) {
		error("Missing file name");
		return;
	}
	if (the_token != T_STRING) {
		error("'\"' around file name expected");
		return;
	}

	if (!(file = fopen(the_string, "wb")))
		error("Unable to create file");
	else {
		do {
			if (adr == end_adr) done = true;

			fputc(SAMReadByte(adr++), file);
		} while (!done);
		fclose(file);
	}
}
