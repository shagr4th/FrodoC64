;*
;* ROlib.h
;*
;* Assembler interface to RISC OS
;* (C) 1997 Andreas Dehmel
;*
;* Frodo (C) 1994-1997,2002-2005 Christian Bauer
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


a1	rn	0
a2	rn	1
a3	rn	2
a4	rn	3
v1	rn	4
v2	rn	5
v3	rn	6
v4	rn	7
v5	rn	8
v6	rn	9
sl	rn	10
fp	rn	11
ip	rn	12
sp	rn	13
lr	rn	14
pc	rn	15


	idfn	(C) 1997 by Andreas Dehmel

; ************ WIMP STUFF ****************

	AREA	CODE, READONLY
	align	4
	export	|Wimp_Initialise|
	=	"Wimp_Initialise"
	align	4

|Wimp_Initialise|:
	swi	0x600c0		;XWimp_Initialise
	movvs	a1,#0		;return 0 if error
	movvc	a1,a2
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_CloseDown|
	=	"Wimp_CloseDown"
	align	4

|Wimp_CloseDown|
	swi	0x600dd		;XWimp_CloseDown
	movvc	a1,#0		;return pointer to error block
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_CreateWindow|
	=	"Wimp_CreateWindow"
	align	4

|Wimp_CreateWindow|:
	mov	a2,a1
	swi	0x600c1
	movvs	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_CreateIcon|
	=	"Wimp_CreateIcon"
	align	4

|Wimp_CreateIcon|:
	swi	0x600c2
	movvs	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_DeleteWindow|
	=	"Wimp_DeleteWindow"
	align	4

|Wimp_DeleteWindow|:
	mov	a2,a1
	swi	0x600c3
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_DeleteIcon|
	=	"Wimp_DeleteIcon"
	align	4

|Wimp_DeleteIcon|:
	mov	a2,a1
	swi	0x600c4
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_OpenWindow|
	=	"Wimp_OpenWindow"
	align	4

|Wimp_OpenWindow|:
	mov	a2,a1
	swi	0x600c5
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_CloseWindow|
	=	"Wimp_CloseWindow"
	align	4

|Wimp_CloseWindow|:
	mov	a2,a1
	swi	0x600c6
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_Poll|
	=	"Wimp_Poll"
	align	4

|Wimp_Poll|:
	mov	a4,a3
	swi	0x600c7
	mvnvs	a1,#0		;return -1 if error
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_RedrawWindow|
	=	"Wimp_RedrawWindow"
	align	4

|Wimp_RedrawWindow|:
	mov	a2,a1
	swi	0x600c8
	movvs	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_UpdateWindow|
	=	"Wimp_UpdateWindow"
	align	4

|Wimp_UpdateWindow|
	mov	a2,a1
	swi	0x600c9
	movvs	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_GetRectangle|
	=	"Wimp_GetRectangle"
	align	4

|Wimp_GetRectangle|:
	mov	a2,a1
	swi	0x600ca
	movvs	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_GetWindowState|
	=	"Wimp_GetWindowState"
	align	4

|Wimp_GetWindowState|:
	mov	a2,a1
	swi	0x600cb
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_GetWindowInfo|
	=	"Wimp_GetWindowInfo"
	align	4

|Wimp_GetWindowInfo|:
	mov	a2,a1
	swi	0x600cc
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_SetIconState|
	=	"Wimp_SetIconState"
	align	4

|Wimp_SetIconState|:
	mov	a2,a1
	swi	0x600cd
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_GetIconState|
	=	"Wimp_GetIconState"
	align	4

|Wimp_GetIconState|:
	mov	a2,a1
	swi	0x600ce
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_GetPointerInfo|
	=	"Wimp_GetPointerInfo"
	align	4

|Wimp_GetPointerInfo|:
	mov	a2,a1
	swi	0x600cf
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_DragBox|
	=	"Wimp_DragBox"
	align	4

|Wimp_DragBox|:
	mov	a2,a1
	swi	0x600d0
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_ForceRedraw|
	=	"Wimp_ForceRedraw"
	align	4

|Wimp_ForceRedraw|:
	stmdb	sp!,{v1}
	ldr	v1,[sp,#4]
	swi	0x600d1
	movvc	a1,#0
	ldmia	sp!,{v1}
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_SetCaretPosition|
	=	"Wimp_SetCaretPosition"
	align	4

|Wimp_SetCaretPosition|:
	stmdb	sp!,{v1,v2}
	add	v1,sp,#8
	ldmia	v1,{v1,v2}
	swi	0x600d2
	movvc	a1,#0
	ldmia	sp!,{v1,v2}
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_GetCaretPosition|
	=	"Wimp_GetCaretPosition"
	align	4

|Wimp_GetCaretPosition|:
	mov	a2,a1
	swi	0x600d3
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_CreateMenu|
	=	"Wimp_CreateMenu"
	align	4

|Wimp_CreateMenu|:
	mov	a4,a3
	mov	a3,a2
	mov	a2,a1
	swi	0x600d4
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_SetExtent|
	=	"Wimp_SetExtent"
	align	4

|Wimp_SetExtent|:
	swi	0x600d7
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_OpenTemplate|
	=	"Wimp_OpenTemplate"
	align	4

|Wimp_OpenTemplate|:
	mov	a2,a1
	swi	0x600d9
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_CloseTemplate|
	=	"Wimp_CloseTemplate"
	align	4

|Wimp_CloseTemplate|:
	swi	0x600da
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_LoadTemplate|
	=	"Wimp_LoadTemplate"
	align	4

|Wimp_LoadTemplate|:
	stmdb	sp!,{a1,a2,v1-v4,lr}	;7 registers
	mov	v1,a4			;Fonts
	mov	a4,a3			;IndirectLimit
	ldr	a3,[a2]			;*Indirect
	ldr	a2,[a1]			;*Template
	add	v2,sp,#28
	ldmia	v2,{v2,v4}
	ldr	v3,[v4]			;Position
	swi	0x600db
	addvs	sp,sp,#8
	bvs	|WLTexit|
	str	v3,[v4]			;store Position
	ldmia	sp!,{v1,v2}
	str	a2,[v1]
	str	a3,[v2]
	mov	a1,#0
|WLTexit|:
	ldmia	sp!,{v1-v4,pc}^


	AREA	CODE, READONLY
	align	4
	export	|Wimp_ProcessKey|
	=	"Wimp_ProcessKey"
	align	4

|Wimp_ProcessKey|:
	swi	0x600dc
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_StartTask|
	=	"Wimp_StartTask"
	align	4

|Wimp_StartTask|:
	swi	0x600de
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_ReportError|
	=	"Wimp_ReportError"
	align	4

|Wimp_ReportError|:
	swi	0x600df
	movvs	a1,#0
	movvc	a1,a2
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_GetWindowOutline|
	=	"Wimp_GetWindowOutline"
	align	4

|Wimp_GetWindowOutline|:
	mov	a2,a1
	swi	0x600e0
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_PollIdle|
	=	"Wimp_PollIdle"
	align	4

|Wimp_PollIdle|:
	swi	0x600e1
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_PlotIcon|
	=	"Wimp_PlotIcon"
	align	4

|Wimp_PlotIcon|:
	mov	a2,a1
	swi	0x600e2
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_SendMessage|
	=	"Wimp_SendMessage"
	align	4

|Wimp_SendMessage|:
	swi	0x600e7
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_CreateSubMenu|
	=	"Wimp_CreateSubMenu"
	align	4

|Wimp_CreateSubMenu|:
	mov	a4,a3
	mov	a3,a2
	mov	a2,a1
	swi	0x600e8
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_SpriteOp|
	=	"Wimp_SpriteOp"
	align	4

|Wimp_SpriteOp|:
	stmdb	sp!,{v1-v4,lr}
	add	v1,sp,#20
	ldmia	v1,{v1-v4}
	swi	0x600e9
	movvc	a1,#0
	ldmia	sp!,{v1-v4,pc}^


	AREA	CODE, READONLY
	align	4
	export	|Wimp_BaseOfSprites|
	=	"Wimp_BaseOfSprites"
	align	4

|Wimp_BaseOfSprites|:
	mov	a3,a1
	mov	a4,a2
	swi	0x600ea
	strvc	a1,[a3]
	strvc	a2,[a4]
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_CommandWindow|
	=	"Wimp_CommandWindow"
	align	4

|Wimp_CommandWindow|:
	swi	0x600ef
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_TransferBlock|
	=	"Wimp_TransferBlock"
	align	4

|Wimp_TransferBlock|:
	str	v1,[sp,#-4]!
	ldr	v1,[sp,#4]
	swi	0x600f1
	ldr	v1,[sp],#4
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|Wimp_SpriteInfo|
	=	"Wimp_SpriteInfo"
	align	4

|Wimp_SpriteInfo|:
	stmdb	sp!,{v1-v6,lr}
	mov	v4,a2
	mov	v5,a3
	mov	v6,a4
	mov	a3,a1
	mov	a1,#40
	movvc	a1,#0
	swi	0x600e9		;Wimp_SpriteOp
	str	a4,[v4]
	str	v1,[v5]
	str	v3,[v6]
	ldmia	sp!,{v1-v6,pc}^


	AREA	CODE, READONLY
	align	4
	export	|DragASprite_Start|
	=	"DragASprite_Start"
	align	4

|DragASprite_Start|:
	swi	0x62400
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|DragASprite_Stop|
	=	"DragASprite_Stop"
	align	4

|DragASprite_Stop|:
	swi	0x62401
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ColourTrans_SelectTable|
	=	"ColourTrans_SelectTable"
	align	4

|ColourTrans_SelectTable|:
	stmdb	sp!,{v1-v4,lr}
	add	v1,sp,#20
	ldmia	v1,{v1-v4}
	ldr	v1,[v1]
	swi	0x60740
	addvs	sp,sp,#4
	bvs	|CTSTexit|
	mov	a1,v1
	ldr	v1,[sp],#4
	str	a1,[v1]
	mov	a1,#0
|CTSTexit|:
	ldmia	sp!,{v2-v4,pc}^


	AREA	CODE, READONLY
	align	4
	export	|ColourTrans_SetFontColours|
	=	"ColourTrans_SetFontColours"
	align	4

|ColourTrans_SetFontColours|:
	swi	0x6074f
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ColourTrans_SetColour|
	=	"ColourTrans_SetColour"
	align	4

|ColourTrans_SetColour|:
	str	v1,[sp,#-4]!
	mov	v1,a3
	mov	a4,a2
	swi	0x6075e
	movvc	a1,#0
	ldr	v1,[sp],#4
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ColourTrans_SetGCOL|
	=	"ColourTrans_SetGCOL"
	align	4

|ColourTrans_SetGCOL|:
	str	v1,[sp,#-4]!
	mov	v1,a3
	mov	a4,a2
	swi	0x60743
	movvc	a1,#0
	ldr	v1,[sp],#4
	movs	pc,lr





; ************* OS STUFF ***************

	AREA	CODE, READONLY
	align	4
	export	|OS_ReadModeVariable|
	=	"OS_ReadModeVariable"
	align	4

|OS_ReadModeVariable|:
	swi	0x20035
	mvncs	a1,#0
	movcc	a1,a3
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|OS_ReadDynamicArea|
	=	"OS_ReadDynamicArea"
	align	4

|OS_ReadDynamicArea|:
	swi	0x2005c
	movvs	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ModeColourNumber|
	=	"ModeColourNumber"
	align	4

|ModeColourNumber|:
	swi	0x60744		;XColourTrans_ReturnColourNumber
	mvnvs	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ScanKeys|
	=	"ScanKeys"
	align	4

|ScanKeys|:
	mov	a2,a1
	mov	a1,#121
	swi	0x20006		;XOS_Byte 121
	mov	a1,a2
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ReadKeyboardStatus|
	=	"ReadKeyboardStatus"
	align	4

|ReadKeyboardStatus|:
	mov	a1,#202
	mov	a2,#0
	mov	a3,#255
	swi	0x20006
	mov	a1,a2
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ReadDragType|
	=	"ReadDragType"
	align	4

|ReadDragType|:
	mov	a1,#161
	mov	a2,#28
	swi	0x20006
	and	a1,a3,#2
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|SetMousePointer|
	=	"SetMousePointer"
	align	4

|SetMousePointer|:
	mov	a2,a1
	mov	a1,#106
	swi	0x20006
	mov	a1,a2
	movs	pc,lr


	AREA	CODE, READONLY	;sprite ops (I have 52, 53 and 56 in mind)
	align	4
	export	|OS_SpriteOp|
	=	"OS_SpriteOp"
	align	4

|OS_SpriteOp|:
	stmdb	sp!,{v1-v4,lr}
	add	v1,sp,#20
	ldmia	v1,{v1-v4}
	swi	0x2002e
	movvc	a1,#0
	ldmia	sp!,{v1-v4,pc}^


	AREA	CODE, READONLY
	align	4
	export	|OS_Plot|
	=	"OS_Plot"
	align	4

|OS_Plot|:
	swi	0x20045
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|MouseBoundingBox|
	=	"MouseBoundingBox"
	align	4

|MouseBoundingBox|:
	mov	a2,a1
	mov	a1,#21
	swi	0x20007		;OS_Word21,1
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|OS_ReadMonotonicTime|
	=	"OS_ReadMonotonicTime"
	align	4

|OS_ReadMonotonicTime|:
	swi	0x20042
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|OS_ReadC|
	=	"OS_ReadC"
	align	4

|OS_ReadC|:
	mov	a2,a1
	swi	0x20004
	strccb	a1,[a2]
	movcc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|OS_ReadLine|
	=	"OS_ReadLine"
	align	4

|OS_ReadLine|:
	stmdb	sp!,{a1,v1,v2}
	stmdb	sp!,{a1-a3}
	mov	a1,#200
	mov	a2,#1
	mvn	a3,#1
	swi	0x20006
	mov	v2,a2
	ldmia	sp!,{a1-a3}
	ldr	v1,[sp,#8]
	swi	0x2000e
	mov	v1,a2
	mov	a1,#200
	mov	a2,v2
	mvn	a3,#0
	swi	0x20006
	mov	a2,v1
	ldmia	sp!,{a4,v1,v2}
	bic	a4,a4,#0xc0000000
	mov	a1,#10
	strb	a1,[a4,a2]	;make compatible with fgets (terminated by 10, not 13)
	mov	a1,a2
	rsbcs	a1,a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|OS_ReadEscapeState|
	=	"OS_ReadEscapeState"
	align	4

|OS_ReadEscapeState|:
	swi	0x2002c
	movcc	a1,#0
	movcs	a1,#1
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ReadCatalogueInfo|
	=	"ReadCatalogueInfo"
	align	4

|ReadCatalogueInfo|:
	stmdb	sp!,{v1-v3,lr}
	mov	v3,a2
	mov	a2,a1
	mov	a1,#17
	swi	0x20008		;OS_File 17
	stmia	v3,{a3-v2}
	ldmia	sp!,{v1-v3,pc}^


	AREA	CODE, READONLY
	align	4
	export	|ReadDirName|
	=	"ReadDirName"
	align	4

|ReadDirName|:
	stmdb	sp!,{v1-v4,lr}
	mov	v4,a3		;preserve dir_env *
	ldmia	a3,{a4-v3}
	mov	a3,a2
	mov	a2,a1
	mov	a1,#9
	swi	0x2000c		;OS_GBPB 9
	stmia	v4,{a4,v1}
	movvc	a1,#0
	ldmia	sp!,{v1-v4,pc}^


	AREA	CODE, READONLY
	align	4
	export	|ReadDirNameInfo|
	=	"ReadDirNameInfo"
	align	4

|ReadDirNameInfo|:
	stmdb	sp!,{v1-v4,lr}
	mov	v4,a3
	ldmia	a3,{a4-v3}
	mov	a3,a2
	mov	a2,a1
	mov	a1,#10
	swi	0x2000c		;OS_GBPB 10
	stmia	v4,{a4,v1}
	movvc	a1,#0
	ldmia	sp!,{v1-v4,pc}^


	AREA	CODE, READONLY
	align	4
	export	|DeleteFile|
	=	"DeleteFile"
	align	4

|DeleteFile|:
	stmdb	sp!,{v1,v2}
	mov	a2,a1
	mov	a1,#6
	swi	0x20008		;OS_File 6
	movvc	a1,#0
	ldmia	sp!,{v1,v2}
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ConvertInteger1|
	=	"ConvertInteger1"
	align	4

|ConvertInteger1|:
	swi	0x200d9
	mov	a1,a2
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ConvertInteger2|
	=	"ConvertInteger2"
	align	4

|ConvertInteger2|:
	swi	0x200da
	mov	a1,a2
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ConvertInteger3|
	=	"ConvertInteger3"
	align	4

|ConvertInteger3|:
	swi	0x200db
	mov	a1,a2
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|ConvertInteger4|
	=	"ConvertInteger4"
	align	4

|ConvertInteger4|:
	swi	0x200dc
	mov	a1,a2
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|OS_FlushBuffer|
	=	"OS_FlushBuffer"
	align	4

|OS_FlushBuffer|:
	mov	a2,a1
	mov	a1,#21
	swi	0x20006
	movvc	a1,#0
	movs	pc,lr






; ************ MISC STUFF ***************

	AREA	CODE, READONLY
	align	4
	export	|Joystick_Read|
	=	"Joystick_Read"
	align	4

|Joystick_Read|:
	swi	0x63f40		;XJoystick_Read (Acorn)
	bvc	|JRexit|
	ldr	a2,[a1]		;get error-number
	eor	a2,a2,#0xe6
	eors	a2,a2,#0x100
	mvneq	a1,#1		;-2 ==> unknown SWI
	mvnne	a1,#0		;-1 ==> error generated from joystick module
|JRexit|:
	movs	pc,lr





; ************** SOUND SWIS *************

	AREA	CODE, READONLY
	align	4
	export	|Sound_Volume|
	=	"Sound_Volume"
	align	4

|Sound_Volume|:
	swi	0x60180
	mvnvs	a1,#0
	movs	pc,lr


; *********** DIGITAL RENDERER **********

DigiRendChunk	equ	0x6F700		;X flag set


	AREA	CODE, READONLY
	align	4
	export	|DigitalRenderer_Activate|
	=	"DigitalRenderer_Activate"
	align	4

|DigitalRenderer_Activate|:
	swi	DigiRendChunk
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|DigitalRenderer_Deactivate|
	=	"DigitalRenderer_Deactivate"
	align	4

|DigitalRenderer_Deactivate|:
	swi	DigiRendChunk+1
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|DigitalRenderer_Pause|
	=	"DigitalRenderer_Pause"
	align	4

|DigitalRenderer_Pause|:
	swi	DigiRendChunk+2
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|DigitalRenderer_Resume|
	=	"DigitalRenderer_Resume"
	align	4

|DigitalRenderer_Resume|:
	swi	DigiRendChunk+3
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|DigitalRenderer_GetTables|
	=	"DigitalRenderer_GetTables"
	align	4

|DigitalRenderer_GetTables|:
	mov	a3,a1
	mov	a4,a2
	swi	DigiRendChunk+4
	strvc	a1,[a3]
	strvc	a2,[a4]
	movvc	a1,#0
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|DigitalRenderer_ReadState|
	=	"DigitalRenderer_ReadState"
	align	4

|DigitalRenderer_ReadState|:
	swi	DigiRendChunk+5
	mvnvs	a1,#0			;returns -1 on error
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|DigitalRenderer_NewSample|
	=	"DigitalRenderer_NewSample"
	align	4

|DigitalRenderer_NewSample|:
	swi	DigiRendChunk+6
	movvc	a1,#0
	movs	pc,lr


; *********** PLOTTER **************

BPlotChunk	equ	0x70100		;X bit set

	AREA	CODE, READONLY
	align	4
	export	|PlotZoom1|
	=	"PlotZoom1"
	align	4

|PlotZoom1|:
	swi	BPlotChunk
	movs	pc,lr


	AREA	CODE, READONLY
	align	4
	export	|PlotZoom2|
	=	"PlotZoom2"
	align	4

|PlotZoom2|:
	swi	BPlotChunk+1
