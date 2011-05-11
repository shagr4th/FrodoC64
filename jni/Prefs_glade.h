/*
 *  Prefs_glade.h - Global preferences, Glade/Gnome/Gtk+ specific stuff
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

#include <gnome.h>
#include <glade/glade.h>

#include <SDL.h>


// Glade XML tree
static GladeXML *xml = NULL;

// Result of ShowEditor()
static bool result = false;

// Pointer to preferences being edited
static Prefs *prefs = NULL;

// Prefs file name
static char *prefs_path = NULL;

// Prototypes
static void set_values();
static void get_values();
static void ghost_widgets();


/*
 *  Show preferences editor (synchronously)
 *  prefs_name points to the file name of the preferences (which may be changed)
 */

bool Prefs::ShowEditor(bool startup, char *path)
{
	prefs = this;
	prefs_path = path;

	// Load XML user interface file on startup
	if (startup) {
		xml = glade_xml_new(DATADIR "Frodo.glade", NULL, NULL);
		if (xml) {
			glade_xml_signal_autoconnect(xml);
			set_values();
		}
	}

	// No XML means no prefs editor
	if (!xml)
		return startup;

	// Run editor
	result = false;
	gtk_main();
	return result;
}


/*
 *  Set the values of the widgets
 */

static void create_joystick_menu(const char *widget_name)
{
	GtkWidget *w = glade_xml_get_widget(xml, widget_name);
	gtk_option_menu_remove_menu(GTK_OPTION_MENU(w));

	GtkWidget *menu = gtk_menu_new();

	for (int i = -1; i < SDL_NumJoysticks(); ++i) {
		GtkWidget *item = gtk_menu_item_new_with_label(i < 0 ? "None" : SDL_JoystickName(i));
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);
	}

	gtk_option_menu_set_menu(GTK_OPTION_MENU(w), menu);
}

static void set_values()
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "emul1541_proc")), prefs->Emul1541Proc);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "map_slash")), prefs->MapSlash);

	gtk_entry_set_text(GTK_ENTRY(gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(glade_xml_get_widget(xml, "drive8_path")))), prefs->DrivePath[0]);
	gtk_entry_set_text(GTK_ENTRY(gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(glade_xml_get_widget(xml, "drive9_path")))), prefs->DrivePath[1]);
	gtk_entry_set_text(GTK_ENTRY(gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(glade_xml_get_widget(xml, "drive10_path")))), prefs->DrivePath[2]);
	gtk_entry_set_text(GTK_ENTRY(gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(glade_xml_get_widget(xml, "drive11_path")))), prefs->DrivePath[3]);

	gtk_option_menu_set_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "display_type")), prefs->DisplayType);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "sprites_on")), prefs->SpritesOn);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "sprite_collisions")), prefs->SpriteCollisions);

	gtk_option_menu_set_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "sid_type")), prefs->SIDType);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "sid_filters")), prefs->SIDFilters);

	create_joystick_menu("joystick1_port");
	create_joystick_menu("joystick2_port");

	gtk_option_menu_set_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "joystick1_port")), prefs->Joystick1Port);
	gtk_option_menu_set_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "joystick2_port")), prefs->Joystick2Port);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "joystick_swap")), prefs->JoystickSwap);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "skip_frames")), prefs->SkipFrames);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "limit_speed")), prefs->LimitSpeed);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "fast_reset")), prefs->FastReset);

	gtk_option_menu_set_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "reu_size")), prefs->REUSize);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "normal_cycles")), prefs->NormalCycles);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "bad_line_cycles")), prefs->BadLineCycles);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "cia_cycles")), prefs->CIACycles);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "floppy_cycles")), prefs->FloppyCycles);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "cia_irq_hack")), prefs->CIAIRQHack);

	ghost_widgets();
}


/*
 *  Get the values of the widgets
 */

static void get_drive_path(int num, const char *widget_name)
{
	prefs->DrivePath[num][0] = 0;
	const char *path = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(glade_xml_get_widget(xml, widget_name)), false);
	if (path)
		strncpy(prefs->DrivePath[num], path, 255);
	prefs->DrivePath[num][255] = 0;
}

static void get_values()
{
	prefs->Emul1541Proc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "emul1541_proc")));
	prefs->MapSlash = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "map_slash")));

	get_drive_path(0, "drive8_path");
	get_drive_path(1, "drive9_path");
	get_drive_path(2, "drive10_path");
	get_drive_path(3, "drive11_path");

	prefs->DisplayType = gtk_option_menu_get_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "display_type")));
	prefs->SpritesOn = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "sprites_on")));
	prefs->SpriteCollisions = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "sprite_collisions")));

	prefs->SIDType = gtk_option_menu_get_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "sid_type")));
	prefs->SIDFilters = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "sid_filters")));

	prefs->Joystick1Port = gtk_option_menu_get_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "joystick1_port")));
	prefs->Joystick2Port = gtk_option_menu_get_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "joystick2_port")));
	prefs->JoystickSwap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "joystick_swap")));

	prefs->SkipFrames = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "skip_frames")));
	prefs->LimitSpeed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "limit_speed")));
	prefs->FastReset = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "fast_reset")));

	prefs->REUSize = gtk_option_menu_get_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "reu_size")));

	prefs->NormalCycles = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "normal_cycles")));
	prefs->BadLineCycles = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "bad_line_cycles")));
	prefs->CIACycles = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "cia_cycles")));
	prefs->FloppyCycles = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "floppy_cycles")));

	prefs->CIAIRQHack = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "cia_irq_hack")));
}


/*
 *  Ghost/unghost widgets
 */

static void ghost_widget(const char *name, bool ghosted)
{
	gtk_widget_set_sensitive(glade_xml_get_widget(xml, name), !ghosted);
}

static void ghost_widgets()
{
	ghost_widget("drive9_path", prefs->Emul1541Proc);
	ghost_widget("drive10_path", prefs->Emul1541Proc);
	ghost_widget("drive11_path", prefs->Emul1541Proc);

	ghost_widget("sid_filters", prefs->SIDType != SIDTYPE_DIGITAL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "sid_filters")), prefs->SIDType == SIDTYPE_DIGITAL ? prefs->SIDFilters : (prefs->SIDType == SIDTYPE_SIDCARD ? true : false));
}


/*
 *  Signal handlers
 */

extern "C" gboolean on_prefs_win_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	return false;
}

extern "C" void on_ok_clicked(GtkButton *button, gpointer user_data)
{
	result = true;
	get_values();
	prefs->Save(prefs_path);
	gtk_main_quit();
}

extern "C" void on_quit_clicked(GtkButton *button, gpointer user_data)
{
	result = false;
	gtk_main_quit();
}

extern "C" void on_about_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	GladeXML *about_xml = glade_xml_new(DATADIR "Frodo.glade", "about_win", NULL);
	if (about_xml)
		gtk_widget_show(glade_xml_get_widget(about_xml, "about_win"));
}

extern "C" void on_emul1541_proc_toggled(GtkToggleButton *button, gpointer user_data)
{
	prefs->Emul1541Proc = gtk_toggle_button_get_active(button);
	ghost_widgets();
}

extern "C" void on_sid_type_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	prefs->SIDType = gtk_option_menu_get_history(GTK_OPTION_MENU(glade_xml_get_widget(xml, "sid_type")));
	ghost_widgets();
}

extern "C" void on_sid_filters_toggled(GtkToggleButton *button, gpointer user_data)
{
	prefs->SIDFilters = gtk_toggle_button_get_active(button);
}
