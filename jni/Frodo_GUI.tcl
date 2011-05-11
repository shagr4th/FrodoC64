#!/usr/bin/wish

# Frodo Tk GUI by Lutz Vieweg <lkv@mania.robin.de>
# requires Tk >= 4.1
#
# Frodo (C) 1994-1997,2002-2005 Christian Bauer
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#package require Tk 4.1

set prefname "$env(HOME)/.frodorc"

proc defaults {} {
	global pref
	set pref(NormalCycles) "63"
	set pref(BadLineCycles) "23"
	set pref(CIACycles) "63"
	set pref(FloppyCycles) "64"
	set pref(SkipFrames) "4"
	set pref(DriveType8) "DIR"
	set pref(DrivePath8) "./64prgs"
	set pref(DriveType9) "D64"
	set pref(DrivePath9) "./disk1.d64"
	set pref(DriveType10) "DIR"
	set pref(DrivePath10) ""
	set pref(DriveType11) "DIR"
	set pref(DrivePath11) ""
	set pref(SIDType) "NONE"
	set pref(SpritesOn) "TRUE"
	set pref(SpriteCollisions) "TRUE"
	set pref(Joystick1On) "FALSE"
	set pref(Joystick2On) "TRUE"
	set pref(JoystickSwap) "FALSE"
	set pref(LimitSpeed) "FALSE"
	set pref(FastReset) "FALSE"
	set pref(CIAIRQHack) "FALSE"
	set pref(MapSlash) "TRUE"
	set pref(Emul1541Proc) "FALSE"
	set pref(ShowOptions) "FALSE"
	set pref(SIDFilters) "TRUE"
}

proc s2bool { s } {
	if {$s == "TRUE"} {return 1}
	return 0
}

defaults

if {![catch { set in [open $prefname] }]} {
	while {![eof $in]} {
		set line [gets $in]
		if [regexp {^([^ ]*)[ ]*=[ ]*([^ ]*)$} $line range name val] {
			switch -exact $name {
				"NormalCycles" {
					set pref(NormalCycles) $val
				}
				"BadLineCycles" {
					set pref(BadLineCycles) $val
				}
				"CIACycles" {
					set pref(CIACycles) $val
				}
				"FloppyCycles" {
					set pref(FloppyCycles) $val
				}
				"SkipFrames" {
					set pref(SkipFrames) $val
				}  
				"DriveType8" {
					set pref(DriveType8) $val
				}
				"DrivePath8" {
					set pref(DrivePath8) $val
				}
				"DriveType9" {
					set pref(DriveType9) $val
				}  
				"DrivePath9" {
					set pref(DrivePath9) $val
				}
				"DriveType10" {
					set pref(DriveType10) $val
				} 
				"DrivePath10" {
					set pref(DrivePath10) $val
				}
				"DriveType11" {
					set pref(DriveType11) $val
				} 
				"DrivePath11" {
					set pref(DrivePath11) $val
				}
				"SIDType" {
					set pref(SIDType) $val
				}
				"SpritesOn" {
					set pref(SpritesOn) [s2bool $val]
				}
				"SpriteCollisions" {
					set pref(SpriteCollisions) [s2bool $val]
				}
				"Joystick1On" {
					set pref(Joystick1On) [s2bool $val]
				}
				"Joystick2On" {
					set pref(Joystick2On) [s2bool $val]
				}
				"JoystickSwap" {
					set pref(JoystickSwap) [s2bool $val]
				}
				"LimitSpeed" {
					set pref(LimitSpeed) [s2bool $val]
				}
				"FastReset" {
					set pref(FastReset) [s2bool $val]
				}
				"CIAIRQHack" {
					set pref(CIAIRQHack) [s2bool $val]
				}
				"MapSlash" {
					set pref(MapSlash) [s2bool $val]
				}
				"Emul1541Proc" {
					set pref(Emul1541Proc) [s2bool $val]
				}
				"ShowOptions" {
					set pref(ShowOptions) [s2bool $val]
				}
				"SIDFilters" {
					set pref(SIDFilters) [s2bool $val]
				}
			}
		}	
	}
}

proc bool2s { b } {
	if {$b} { return "TRUE" }
	return "FALSE" 
}

proc WritePrefs { } {
	global pref prefname
	
	if [catch { set out [open "$prefname" "w"] }] {
		puts stderr "unable to write preferences file '$prefname'"
	} else {
		puts $out "NormalCycles = $pref(NormalCycles)"
		puts $out "BadLineCycles = $pref(BadLineCycles)"
		puts $out "CIACycles = $pref(CIACycles)"
		puts $out "FloppyCycles = $pref(FloppyCycles)"
		puts $out "SkipFrames = $pref(SkipFrames)"
		puts $out "DriveType8 = $pref(DriveType8)"
		puts $out "DrivePath8 = $pref(DrivePath8)"
		puts $out "DriveType9 = $pref(DriveType9)"
		puts $out "DrivePath9 = $pref(DrivePath9)"
		puts $out "DriveType10 = $pref(DriveType10)"
		puts $out "DrivePath10 = $pref(DrivePath10)"
		puts $out "DriveType11 = $pref(DriveType11)"
		puts $out "DrivePath11 = $pref(DrivePath11)"
		puts $out "SIDType = $pref(SIDType)"
		puts $out "SpritesOn = [bool2s $pref(SpritesOn)]"
		puts $out "SpriteCollisions = [bool2s $pref(SpriteCollisions)]"
		puts $out "Joystick1On = [bool2s $pref(Joystick1On)]"
		puts $out "Joystick2On = [bool2s $pref(Joystick2On)]"
		puts $out "JoystickSwap = [bool2s $pref(JoystickSwap)]"
		puts $out "LimitSpeed = [bool2s $pref(LimitSpeed)]"
		puts $out "FastReset = [bool2s $pref(FastReset)]"
		puts $out "CIAIRQHack = [bool2s $pref(CIAIRQHack)]"
		puts $out "MapSlash = [bool2s $pref(MapSlash)]"
		puts $out "Emul1541Proc = [bool2s $pref(Emul1541Proc)]"
		puts $out "ShowOptions = [bool2s $pref(ShowOptions)]"
		puts $out "SIDFilters = [bool2s $pref(SIDFilters)]"
		
		close $out
		
		puts -nonewline "p"
		flush stdout
	}	
}

proc Quit {} {
	puts -nonewline "q"
	flush stdout
	exit 0
}

# =============================================================

frame .cmds
pack .cmds -expand false -fill both

button .cmds.quit -text "Quit" -command "Quit"
pack .cmds.quit -side left -expand true -fill both

button .cmds.reset -text "Reset" -command {puts -nonewline "r" ; flush stdout}
pack .cmds.reset -side left -expand true -fill both

# =============================================================

proc Change { {dummy1 ""} {dummy2 ""}} {
	WritePrefs
}

#====================== begin of fs-box ========================

proc check_file_type {filename var} {
  global pref
  switch [file extension $filename] {
     .d64 - .x64 { set $var D64; Change }
     .lnx - .t64 { set $var T64; Change }
  }
}

proc fs_FileSelect {w {title {FileSelector}} {filter {*}} {name {}} typevar} {
	global fs_priv

	if {[info commands tk_getOpenFile] != ""} {
	   switch $filter {
	      "*.{t64,lnx}" {
	         set types {
		    {{t64/lnx archive files} {.t64 .lnx}}
		    {{disk files} {.d64 .x64}}
		    {{all files} *}
		 }
	      }
	      "*.{d64,x64}" {
	         set types {
		    {{disk files} {.d64 .x64}}
		    {{t64/lnx archive files} {.t64 .lnx}}
		    {{all files} *}
		 }
	      }
	      default {
	         set types {
		    {{all files} *}
		    {{disk files} {.d64 .x64}}
		    {{t64/lnx archive files} {.t64 .lnx}}
		 }
	      }
	   }
	   if {[file isdir $name]} {
		set name $name
	   } else {
		set name "[file dirname $name]"
	   }
	   set fs_priv(result) [tk_getOpenFile -filetypes $types -initialdir  $name]
	   check_file_type $fs_priv(result) $typevar
	   return $fs_priv(result)
	}

	# remainder of the code is for pre-tk8.0

	if {$name != ""} {
		if {[file isdir $name]} {
			set filter "$name/$filter"
		} else {
			set filter "[file dirname $name]/$filter"
		}
	}

	set fs_priv(window) $w
	set fs_priv(filter) $filter
	set fs_priv(name) ""
	set fs_priv(result) ""
	 
	# if this window already exists, destroy it
	catch {destroy $w}

	# create new toplevel
	toplevel $w
	wm title $w $title

	# create frames	

	# create filter-entry
	frame $w.filter
	pack $w.filter -side top -fill x
	label $w.filter.lbl -text "Filter"
	pack $w.filter.lbl -side top -anchor w
	entry $w.filter.et -textvar fs_priv(filter)
	pack $w.filter.et -side top -fill x -expand true
	bind $w.filter.et <Return> { fs_newpath }
	button $w.filter.up -text "Up" -command {
	        set f [file dirname $fs_priv(filter)]
	        set t [file tail $fs_priv(filter)]
		if {$f == "."} {set f [pwd]}
		set f [file dirname $f]
		if {$f == "/"} {set f ""}
		set fs_priv(filter) "$f/$t"
		fs_newpath
	}
	pack $w.filter.up -side top -anchor w
	
	#create list-frames
	frame $w.l
	pack $w.l -side top -fill both -expand true
	frame $w.l.d
	pack $w.l.d -side left -fill both -expand true
	frame $w.l.f
	pack $w.l.f -side left -fill both -expand true
	
	fs_slist $w.l.d Directories single

	fs_slist $w.l.f Files single
	bind $w.l.f.top.lst <ButtonRelease-1> {
		focus %W
		global fs_priv
		set sel [%W curselection]
		if {$sel != ""} {
			set fs_priv(name) [%W get [%W curselection]]
		}
	}

	bind $w.l.f.top.lst <Double-Button-1> "$w.bts.ok flash; $w.bts.ok invoke"
	bind $w.l.d.top.lst <Double-Button-1> {
		global fs_priv
	        set f [file dirname $fs_priv(filter)]
	        set t [file tail $fs_priv(filter)]
		set d [%W get active]
		switch $d {
		   "." { }
		   ".." {
		      if {$f == "."} {set f [pwd]}
		      set fs_priv(filter) "[file dirname $f]/$t"
		   }
		   default { 
		      if {$f == "/"} {set f ""}
		      set fs_priv(filter) "$f/$d/$t"
		   }
		}
		fs_newpath
	}
	
	#create name-entry

	frame $w.name
	pack $w.name -side top -fill x
	label $w.name.lbl -text "Filename"
	pack $w.name.lbl -side top -anchor w
	entry $w.name.et -textvar fs_priv(name)
	pack $w.name.et -side top -fill x
	bind $w.name.et <FocusOut> {
		global fs_priv
		set fs_priv(filter) \
		    "[file dirname $fs_priv(name)]/*[file extension $fs_priv(filter)]"
		fs_newpath
	}
	bind $w.name.et <Return> {
		global fs_priv
		set n $fs_priv(name)
		
		if {[string index $n 0] != "/" && [string index $n 0] != "~"} {
			set n "[file dirname $fs_priv(filter)]/$n"
		}

		set n "[file dirname $n]/[file tail $n]"
		
		set fs_priv(result) $n
	}
	
	# create buttons
	frame $w.bts
	pack $w.bts -side top -fill x
	button $w.bts.ok -text "OK" -command {
		global fs_priv
	        set w $fs_priv(window)
		set sel [$w.l.f.top.lst curselection]
		if {$sel != ""} {
		   set val [$w.l.f.top.lst get $sel]
		   set fs_priv(result) "[file dirname $fs_priv(filter)]/$val"
		}
	}
	pack $w.bts.ok -side left -expand true
		
	button $w.bts.cancel -text "Cancel" -command {
		global fs_priv
		set fs_priv(result) ""
	}
	pack $w.bts.cancel -side left -expand true
	
	fs_newpath

	set oldfocus [focus]
	grab $w
	focus $w
	
	tkwait variable fs_priv(result)

	if { "$oldfocus" != "" } { focus $oldfocus }

	destroy $w

	check_file_type $fs_priv(result) $typevar

	return $fs_priv(result)	
}

proc fs_DirSelect {w {title {FileSelector}} {filter {*}} {name {}} } {
	global fs_priv

	if {$name != ""} {
		if {[file isdir $name]} {
			set filter $name
		} else {
			set filter [file dirname $name]
		}
	}

	if {[info commands tk_chooseDirectory] != ""} {
	   return [tk_chooseDirectory -initialdir $filter]
	}

	# remainder of the code is for pre-tk8.3

	set fs_priv(window) $w
	set fs_priv(filter) $filter
	set fs_priv(name) $name
	set fs_priv(result) ""
	 
	# if this window already exists, destroy it
	catch {destroy $w}

	# create new toplevel
	toplevel $w
	wm title $w $title

	# create frames	

	# create filter-entry
	frame $w.filter
	pack $w.filter -side top -fill x
	label $w.filter.lbl -text "Directory"
	pack $w.filter.lbl -side top -anchor w
	entry $w.filter.et -textvar fs_priv(filter)
	pack $w.filter.et -side top -fill x -expand true
	bind $w.filter.et <Return> { fs_dir_newpath }
	button $w.filter.up -text "Up" -command {
	        set f $fs_priv(filter)
		if {$f == "."} {set f [pwd]}
		set fs_priv(filter) [file dirname $f]
		fs_dir_newpath
	}
	pack $w.filter.up -side top -anchor w
	
	#create list-frames
	frame $w.l
	pack $w.l -side top -fill both -expand true
	frame $w.l.d
	pack $w.l.d -side left -fill both -expand true
	
	fs_slist $w.l.d "Sub Directories" single

	bind $w.l.d.top.lst <Double-Button-1> {
		global fs_priv
	        set f [string trimright $fs_priv(filter) /]
		set d [%W get active]
		switch $d {
		   "." { }
		   ".." {
		      if {$f == "."} {set f [pwd]}
		      set fs_priv(filter) [file dirname $f]
		   }
		   default {
		      if {$f == "/"} {set f ""}
		      set fs_priv(filter) "$f/$d"
		   }
		}
		fs_dir_newpath
	}
	
	# create buttons
	frame $w.bts
	pack $w.bts -side top -fill x
	button $w.bts.ok -text "OK" -command {
		global fs_priv
	        set w $fs_priv(window)
		set sel [$w.l.d.top.lst curselection]
		if {$sel != ""} {
		   set val [$w.l.d.top.lst get $sel]
		   set fs_priv(result) "$fs_priv(filter)/$val"
		} else {
		   set fs_priv(result) $fs_priv(filter)
		}
	}
	pack $w.bts.ok -side left -expand true
		
	button $w.bts.cancel -text "Cancel" -command {
		global fs_priv
		set fs_priv(result) ""
	}
	pack $w.bts.cancel -side left -expand true
	
	fs_dir_newpath

	set oldfocus [focus]
	grab $w
	focus $w
	
	tkwait variable fs_priv(result)

	if { "$oldfocus" != "" } { focus $oldfocus }

	destroy $w

	return $fs_priv(result)	
}

proc fs_slist {w title mode} {

	if {$title != ""} {
		label $w.lbl -text $title
		pack $w.lbl -side top -anchor w
	}

	frame $w.top
	pack $w.top -side top -fill both -expand true
	frame $w.bot
	pack $w.bot -side top -fill x
	
	listbox $w.top.lst -relief sunken -bd 2 -yscrollcommand "$w.top.vs set" \
	        -xscrollcommand "$w.bot.hs set" -selectmode $mode \
	        -font -*-courier-medium-r-normal--14-*-*-*-*-*-*
	pack $w.top.lst -side left -fill both -expand true

	scrollbar $w.top.vs -relief sunken -bd 2 -command "$w.top.lst yview" \
	          -orient vertical
	pack $w.top.vs -side left -fill y
	
	scrollbar $w.bot.hs -relief sunken -bd 2 -command "$w.top.lst xview" \
	          -orient horizontal
	pack $w.bot.hs -side left -fill x -expand true
	
	frame $w.bot.pad -width [expr [lindex [$w.top.vs config -width] 4] + \
	                        [lindex [$w.top.vs config -bd] 4] *2]

	pack $w.bot.pad -side left
		
}

proc fs_newpath {} {

	global fs_priv

	if {$fs_priv(filter) == ""} {
		set fs_priv(filter) "./*"
	}

	if {[file isdir $fs_priv(filter)]} {
		set fs_priv(filter) "$fs_priv(filter)/*"
	}

	set w $fs_priv(window)
	set filter $fs_priv(filter)
	
	$w.l.d.top.lst delete 0 end

	$w.l.f.top.lst delete 0 end

	# update dirs
	set dwidth 5
	set files [lsort "[glob -nocomplain "[file dirname $filter]/*" ]  \
	                  [glob -nocomplain "[file dirname $filter]/.*"]" ]
	foreach j $files {
		if [file isdir $j] {
			set name [file tail $j]
			$w.l.d.top.lst insert end $name
			if {[string length $name] > $dwidth} { set dwidth [string length $name] }
		}
	}

	#update files
	set pos 0
	set fwidth 5
	set files [lsort [glob -nocomplain "$filter"]]
	foreach j $files {
		if [file isfile $j] {
			set name [file tail $j]
			$w.l.f.top.lst insert end $name
			if {[string length $name] > $fwidth} {
				set pos [string length [file dirname $j]]
				set fwidth [string length $name]
			}
		}
	}
	
	if {$fwidth < 20} { set fwidth 20 }
	$w.l.f.top.lst configure -width [expr $fwidth+1]

	if {$dwidth < 20} { set dwidth 20 }
        $w.l.d.top.lst configure -width [expr $dwidth+1]


	if {$pos == 1} { set pos 0 }
	
	update idletasks
	
	$w.l.f.top.lst xview $pos
	
}

proc fs_dir_newpath {} {

	global fs_priv

	if {$fs_priv(filter) == ""} {
		set fs_priv(filter) "."
	}

	set w $fs_priv(window)
	set filter $fs_priv(filter)
	
	$w.l.d.top.lst delete 0 end

	# update dirs
	set dwidth 5
	set files [lsort "[glob -nocomplain "$filter/*" ]  \
	                  [glob -nocomplain "$filter/.*"]" ]
	foreach j $files {
		if [file isdir $j] {
			set name [file tail $j]
			$w.l.d.top.lst insert end $name
			if {[string length $name] > $dwidth} { set dwidth [string length $name] }
		}
	}

	
	if {$dwidth < 20} { set dwidth 20 }
        $w.l.d.top.lst configure -width [expr $dwidth+1]

	update idletasks
	
}

#====================== end of fs-box ========================

set num(1) "ABCDEFGHIJKLMNOPQRSTUVWXYZA"
set num(2) "abcdefghijklmnopqrstuvwxyza"
set num(3) "12345678901"

proc NDname { name } {
	global num
	if [string match *.?64 $name] {
		set len [string length $name]
		set z [string index $name [expr $len-5]]
		
		foreach i "1 2 3" {
			set c [string first $z $num($i)]
			if {$c >= 0} { break }
		}
		incr c
		set nname "[string range $name 0 [expr $len-6]][string index $num($i) $c][string range $name [expr $len-4] end]"
		if [file exists $nname] { return $nname }
		set nname "[string range $name 0 [expr $len-6]][string index $num($i) 0][string range $name [expr $len-4] end]"
		if [file exists $nname] { return $nname }
	}
	return $name
}

# ===========================================================

frame .drives -borderwidth 0
pack .drives -side top -expand false -fill x

label .drives.l -text "Disk Drive Controls" -height 2
pack .drives.l -side top -expand true -fill both

checkbutton .drives.ef -text "Emulate 1541 CPU (Drive 8 only)" -variable pref(Emul1541Proc) -command "Change" -bg "dark grey" -anchor w
pack .drives.ef -side top -expand true -fill both

frame .drives.d8 -borderwidth 0
pack .drives.d8 -side top -expand true -fill x

label .drives.d8.l -text "8" -width 2
pack .drives.d8.l -side left -expand false 
radiobutton .drives.d8.d64 -text "D64" -variable pref(DriveType8) -value "D64" \
    -bg "dark grey" -command {
		set erg [fs_FileSelect .fs "Choose D64 image file" "*.{d64,x64}" $pref(DrivePath8) pref(DriveType8)]
		if {$erg != ""} { set pref(DrivePath8) $erg ; Change }
    }
pack .drives.d8.d64 -side left -expand false -fill y

radiobutton .drives.d8.dir -text "DIR" -variable pref(DriveType8) -value "DIR" \
    -command {
		set erg [fs_DirSelect .fs "Choose directory" "*" $pref(DrivePath8)]
		if {$erg != ""} { set pref(DrivePath8) $erg ; Change }
    }
pack .drives.d8.dir -side left -expand false

radiobutton .drives.d8.t64 -text "T64" -variable pref(DriveType8) -value "T64" \
    -command {
		set erg [fs_FileSelect .fs "Choose T64/LYNX archive file" "*.{t64,lnx}" $pref(DrivePath8) pref(DriveType8)]
		if {$erg != ""} { set pref(DrivePath8) $erg ; Change }
    }
pack .drives.d8.t64 -side left -expand false

entry .drives.d8.name -textvar pref(DrivePath8)
bind .drives.d8.name <Return> "Change"
bind .drives.d8.name <Double-1> {
	set erg [fs_FileSelect .fs "Choose A File" "*" $pref(DrivePath8) pref(DriveType8)]
	if {$erg != ""} { set pref(DrivePath8) $erg ; Change }
}
pack .drives.d8.name -side left -expand true -fill x

button .drives.d8.n -text "N" -command { set pref(DrivePath8) [NDname $pref(DrivePath8)]; Change }
pack .drives.d8.n -side left -expand false

frame .drives.d9
pack .drives.d9 -side top -expand true -fill x

label .drives.d9.l -text "9" -width 2
pack .drives.d9.l -side left -expand false 
radiobutton .drives.d9.d64 -text "D64" -variable pref(DriveType9) -value "D64" \
    -command {
		set erg [fs_FileSelect .fs "Choose D64 image file" "*.{d64,x64}" $pref(DrivePath9) pref(DriveType9)]
		if {$erg != ""} { set pref(DrivePath9) $erg ; Change }
    }
pack .drives.d9.d64 -side left -expand false

radiobutton .drives.d9.dir -text "DIR" -variable pref(DriveType9) -value "DIR" \
    -command {
		set erg [fs_DirSelect .fs "Choose directory" "*" $pref(DrivePath9)]
		if {$erg != ""} { set pref(DrivePath9) $erg ; Change }
    }
pack .drives.d9.dir -side left -expand false

radiobutton .drives.d9.t64 -text "T64" -variable pref(DriveType9) -value "T64" \
    -command {
		set erg [fs_FileSelect .fs "Choose T64/LYNX archive file" "*.{t64,lnx}" $pref(DrivePath9) pref(DriveType9)]
		if {$erg != ""} { set pref(DrivePath9) $erg ; Change }
    }
pack .drives.d9.t64 -side left -expand false

entry .drives.d9.name -textvar pref(DrivePath9)
bind .drives.d9.name <Return> "Change"
bind .drives.d9.name <Double-1> {
	set erg [fs_FileSelect .fs "Choose A File" "*" $pref(DrivePath9) pref(DriveType9)]
	if {$erg != ""} { set pref(DrivePath9) $erg ; Change }
}
pack .drives.d9.name -side left -expand true -fill x

button .drives.d9.n -text "N" -command { set pref(DrivePath9) [NDname $pref(DrivePath9)]; Change }
pack .drives.d9.n -side left -expand false


frame .drives.d10
pack .drives.d10 -side top -expand true -fill x

label .drives.d10.l -text "10" -width 2
pack .drives.d10.l -side left -expand false 
radiobutton .drives.d10.d64 -text "D64" -variable pref(DriveType10) -value "D64" \
    -command {
		set erg [fs_FileSelect .fs "Choose D64 image file" "*.{d64,x64}" $pref(DrivePath10) pref(DriveType10)]
		if {$erg != ""} { set pref(DrivePath10) $erg ; Change }
    }
pack .drives.d10.d64 -side left -expand false

radiobutton .drives.d10.dir -text "DIR" -variable pref(DriveType10) -value "DIR" \
    -command {
		set erg [fs_DirSelect .fs "Choose directory" "*" $pref(DrivePath10)]
		if {$erg != ""} { set pref(DrivePath10) $erg ; Change }
    }
pack .drives.d10.dir -side left -expand false

radiobutton .drives.d10.t64 -text "T64" -variable pref(DriveType10) -value "T64" \
    -command {
		set erg [fs_FileSelect .fs "Choose T64/LYNX archive file" "*.{t64,lnx}" $pref(DrivePath10) pref(DriveType10)]
		if {$erg != ""} { set pref(DrivePath10) $erg ; Change }
    }
pack .drives.d10.t64 -side left -expand false

entry .drives.d10.name -textvar pref(DrivePath10)
bind .drives.d10.name <Return> "Change"
bind .drives.d10.name <Double-1> {
	set erg [fs_FileSelect .fs "Choose A File" "*" $pref(DrivePath10) pref(DriveType10)]
	if {$erg != ""} { set pref(DrivePath10) $erg ; Change }
}
pack .drives.d10.name -side left -expand true -fill x

button .drives.d10.n -text "N" -command { set pref(DrivePath10) [NDname $pref(DrivePath10)]; Change }
pack .drives.d10.n -side left -expand false


frame .drives.d11
pack .drives.d11 -side top -expand true -fill x

label .drives.d11.l -text "11" -width 2
pack .drives.d11.l -side left -expand false 
radiobutton .drives.d11.d64 -text "D64" -variable pref(DriveType11) -value "D64" \
    -command {
		set erg [fs_FileSelect .fs "Choose D64 image file" "*.{d64,x64}" $pref(DrivePath11) pref(DriveType11)]
		if {$erg != ""} { set pref(DrivePath11) $erg ; Change }
    }
pack .drives.d11.d64 -side left -expand false

radiobutton .drives.d11.dir -text "DIR" -variable pref(DriveType11) -value "DIR" \
    -command {
		set erg [fs_DirSelect .fs "Choose directory" "*" $pref(DrivePath11)]
		if {$erg != ""} { set pref(DrivePath11) $erg ; Change }
    }
pack .drives.d11.dir -side left -expand false

radiobutton .drives.d11.t64 -text "T64" -variable pref(DriveType11) -value "T64" \
    -command {
		set erg [fs_FileSelect .fs "Choose T64/LYNX archive file" "*.{t64,lnx}" $pref(DrivePath11) pref(DriveType11)]
		if {$erg != ""} { set pref(DrivePath11) $erg ; Change }
    }
pack .drives.d11.t64 -side left -expand false

entry .drives.d11.name -textvar pref(DrivePath11)
bind .drives.d11.name <Return> "Change"
bind .drives.d11.name <Double-1> {
	set erg [fs_FileSelect .fs "Choose A File" "*" $pref(DrivePath11) pref(DriveType11)]
	if {$erg != ""} { set pref(DrivePath11) $erg ; Change }
}
pack .drives.d11.name -side left -expand true -fill x

button .drives.d11.n -text "N" -command { set pref(DrivePath11) [NDname $pref(DrivePath11)]; Change }
pack .drives.d11.n -side left -expand false


# =============================================================

global show_options_string

checkbutton .more_options -borderwidth 3 -relief raised -textvariable show_options_string -variable pref(ShowOptions) -command "Change"
pack .more_options -side top -expand false -fill x

frame .nums -borderwidth 3 -relief raised

scale .nums.nc -from 1 -to 200 -orient horizontal -variable pref(NormalCycles) \
               -label "Normal Cycles"
pack .nums.nc -side top -expand false -fill x

scale .nums.bc -from 1 -to 200 -orient horizontal -variable pref(BadLineCycles) \
               -label "Bad Line Cycles"
pack .nums.bc -side top -expand false -fill x

scale .nums.cc -from 1 -to 200 -orient horizontal -variable pref(CIACycles) \
               -label "CIA Cycles"
pack .nums.cc -side top -expand false -fill x

scale .nums.fc -from 1 -to 200 -orient horizontal -variable pref(FloppyCycles) \
               -label "Floppy Cycles"
pack .nums.fc -side top -expand false -fill x

scale .nums.sf -from 1 -to 10 -orient horizontal -variable pref(SkipFrames) \
               -label "Skip Frames"
pack .nums.sf -side top -expand false -fill x

# =============================================================

frame .bools1 -borderwidth 3 -relief raised

frame .bools1.sprites
pack .bools1.sprites -side left -expand true -fill both

checkbutton .bools1.sprites.o -text "Sprites" -variable pref(SpritesOn) -command "Change"
pack .bools1.sprites.o -anchor nw -expand false -fill y

checkbutton .bools1.sprites.c -text "Sprite Collisions" \
            -variable pref(SpriteCollisions) -command "Change"
pack .bools1.sprites.c -anchor nw -expand false -fill y


frame .bools1.joy
pack .bools1.joy -side left -expand true -fill both

checkbutton .bools1.joy.j1 -text "Joy 1" -variable pref(Joystick1On) -command "Change"
pack .bools1.joy.j1 -anchor nw -expand false -fill y

checkbutton .bools1.joy.j2 -text "Joy 2" -variable pref(Joystick2On) -command "Change"
pack .bools1.joy.j2 -anchor nw -expand false -fill y

checkbutton .bools1.joy.swap -text "Swap 1<->2" -variable pref(JoystickSwap) -command "Change"
pack .bools1.joy.swap -anchor nw -expand false -fill y


frame .bools2 -borderwidth 3 -relief raised

frame .bools2.m1
pack .bools2.m1 -side left -expand true -fill both

checkbutton .bools2.m1.ls -text "Limit Speed" -variable pref(LimitSpeed) -command "Change"
pack .bools2.m1.ls -anchor nw -expand false -fill y

checkbutton .bools2.m1.fr -text "Fast Reset" -variable pref(FastReset) -command "Change"
pack .bools2.m1.fr -anchor nw -expand false -fill y


frame .bools2.m2
pack .bools2.m2 -side left -expand true -fill both

checkbutton .bools2.m2.ch -text "CIA IRQ Hack" -variable pref(CIAIRQHack) -command "Change"
pack .bools2.m2.ch -anchor nw -expand false -fill y

checkbutton .bools2.m2.ms -text "Map '/'" -variable pref(MapSlash) -command "Change"
pack .bools2.m2.ms -anchor nw -expand false -fill y


frame .bools4 -relief raised -borderwidth 3

frame .bools4.st
pack .bools4.st -side left -expand true -fill both

label .bools4.st.l -text "SID Emulation"
pack .bools4.st.l -anchor nw 
radiobutton .bools4.st.none -text "None" -variable pref(SIDType) -value "NONE" \
            -command {Change}
pack .bools4.st.none -anchor nw 

radiobutton .bools4.st.digi -text "Digital" -variable pref(SIDType) -value "DIGITAL" \
            -command {Change}
pack .bools4.st.digi -anchor nw 

frame .bools4.sf
pack .bools4.sf -side left -expand true -fill both

checkbutton .bools4.sf.sf -text "SID Filters" -variable pref(SIDFilters) -command "Change"
pack .bools4.sf.sf -side top -expand false -fill y


# =============================================================

frame .pcmd
pack .pcmd -side top -expand false -fill both

button .pcmd.apply -text "Apply" -command "Change" 
pack .pcmd.apply -side left -expand true -fill both

button .pcmd.default -text "Defaults" -command "defaults ; Change" 
pack .pcmd.default -side left -expand false -fill both

# =============================================================

set ledcolors(0) "#d9d9d9"
set ledcolors(1) "red"
set ledcolors(2) "brown"

proc ListenToFrodo {} {
	set line [gets stdin]
	set cmd [lindex $line 0]
	switch -exact $cmd {
		"speed" {
			.speed.v configure -text "[lindex $line 1]%"
		}
		"ping" {
			puts -nonewline "o"
			flush stdout
		}
		"quit" {
			exit 0
		}
		"leds" {
			global ledcolors
			.drives.d8.l configure -background $ledcolors([lindex $line 1])
			.drives.d9.l configure -background $ledcolors([lindex $line 2])
			.drives.d10.l configure -background $ledcolors([lindex $line 3])
			.drives.d11.l configure -background $ledcolors([lindex $line 4])
		}
		default {
			puts stderr "line = $line"
		}
	}
}


proc set_Emul1541Proc args {
   global pref

   if {$pref(Emul1541Proc)} {
      set state disabled
      set pref(DriveType8) "D64"
   } else {
      set state normal
   }
   .drives.d8.dir configure -state $state
   .drives.d8.t64 configure -state $state
   foreach i {9 10 11} {
      .drives.d${i}.d64 configure -state $state
      .drives.d${i}.dir configure -state $state
      .drives.d${i}.t64 configure -state $state
      .drives.d${i}.name configure -state $state
      .drives.d${i}.n configure -state $state
   }
}  

proc set_ShowOptions args {
   global pref show_options_string

   if {$pref(ShowOptions)} {
      pack .nums -side top -expand false -fill x -after .more_options
      pack .bools1 -side top -expand true -fill both -after .nums
      pack .bools2 -side top -expand true -fill both -after .bools1
      pack .bools4 -side top -expand true -fill both -after .bools2
      set show_options_string "Hide Advanced Options"
   } else {
      pack forget .nums .bools1 .bools2 .bools4
      set show_options_string "Show Advanced Options"
   }
}

fileevent stdin readable { ListenToFrodo }

# =============================================================

wm title . "Frodo Preferences Menu"

# set trace and trigger it now
trace variable pref(Emul1541Proc) w set_Emul1541Proc
set pref(Emul1541Proc) $pref(Emul1541Proc)

# set trace and trigger it now
trace variable pref(ShowOptions) w set_ShowOptions
set pref(ShowOptions) $pref(ShowOptions)

tkwait window .
