/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2003 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

#include "stdafx.h"
#include "gui/browser.h"
#include "gui/chooser.h"
#include "gui/newgui.h"
#include "gui/options.h"
#include "gui/PopUpWidget.h"
#include "gui/TabWidget.h"

#include "backends/fs/fs.h"
#include "base/gameDetector.h"
#include "common/config-manager.h"
#include "common/scaler.h"
#include "sound/mididrv.h"

#if (!( defined(__DC__) || defined(__GP32__)) && !defined(_MSC_VER))
#include <unistd.h>
#endif

// TODO - allow changing options for:
// - the save path (use _browser!)
// - music & graphics driver (but see also the comments on EditGameDialog
//   for some techincal difficulties with this)
// - default volumes (sfx/master/music)
// - aspect ratio, language, platform, subtitles, debug mode/level, cd drive, joystick, multi midi, native mt32

enum {
	kMasterVolumeChanged	= 'mavc',
	kMusicVolumeChanged		= 'muvc',
	kSfxVolumeChanged		= 'sfvc',
	kChooseSaveDirCmd		= 'chos'
};

OptionsDialog::OptionsDialog(const String &domain, int x, int y, int w, int h)
	: Dialog(x, y, w, h),
	_domain(domain),
	_gfxPopUp(0), _fullscreenCheckbox(0), _aspectCheckbox(0),
	_multiMidiCheckbox(0), _mt32Checkbox(0),
	_masterVolumeSlider(0), _masterVolumeLabel(0),
	_musicVolumeSlider(0), _musicVolumeLabel(0),
	_sfxVolumeSlider(0), _sfxVolumeLabel(0) {

}

void OptionsDialog::open() {
	Dialog::open();
	
	// Reset result value
	setResult(0);
	
	// FIXME - disable GFX popup for now
	_gfxPopUp->setSelected(0);
	_gfxPopUp->setEnabled(false);

	// Fullscreen setting
	_fullscreenCheckbox->setState(ConfMan.getBool("fullscreen", _domain));

	// Aspect ratio setting
	_aspectCheckbox->setState(ConfMan.getBool("aspect_ratio", _domain));

	// Music driver
	const MidiDriverDescription *md = getAvailableMidiDrivers();
	const int midiDriver = parseMusicDriver(ConfMan.get("music_driver", _domain));
	int i = 0;
	while (md->name && md->id != midiDriver) {
		i++;
		md++;
	}
	_midiPopUp->setSelected(md->name ? i : 0);

	// Multi midi setting
	_multiMidiCheckbox->setState(ConfMan.getBool("multi_midi", _domain));

	// Native mt32 setting
	_mt32Checkbox->setState(ConfMan.getBool("native_mt32", _domain));

	int vol;

	vol = ConfMan.getInt("master_volume", _domain);
	_masterVolumeSlider->setValue(vol);
	_masterVolumeLabel->setValue(vol);

	vol = ConfMan.getInt("music_volume", _domain);
	_musicVolumeSlider->setValue(vol);
	_musicVolumeLabel->setValue(vol);

	vol = ConfMan.getInt("sfx_volume", _domain);
	_sfxVolumeSlider->setValue(vol);
	_sfxVolumeLabel->setValue(vol);
}

void OptionsDialog::close() {
	if (getResult()) {
		if (_fullscreenCheckbox) {
			ConfMan.set("fullscreen", _fullscreenCheckbox->getState(), _domain);
			ConfMan.set("aspect_ratio", _aspectCheckbox->getState(), _domain);
		}

		if (_masterVolumeSlider) {
			ConfMan.set("master_volume", _masterVolumeSlider->getValue(), _domain);
			ConfMan.set("music_volume", _musicVolumeSlider->getValue(), _domain);
			ConfMan.set("sfx_volume", _sfxVolumeSlider->getValue(), _domain);
		}

		if (_multiMidiCheckbox) {
			ConfMan.set("multi_midi", _multiMidiCheckbox->getState(), _domain);
			ConfMan.set("native_mt32", _mt32Checkbox->getState(), _domain);
	
			const MidiDriverDescription *md = getAvailableMidiDrivers();
			while (md->name && md->id != (int)_midiPopUp->getSelectedTag())
				md++;
			if (md->name)
				ConfMan.set("music_driver", md->name, _domain);
			else
				ConfMan.removeKey("music_driver", _domain);
		}
		
		// Save config file
		ConfMan.flushToDisk();
	}

	Dialog::close();
}

void OptionsDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kMasterVolumeChanged:
		_masterVolumeLabel->setValue(_masterVolumeSlider->getValue());
		_masterVolumeLabel->draw();
		break;
	case kMusicVolumeChanged:
		_musicVolumeLabel->setValue(_musicVolumeSlider->getValue());
		_musicVolumeLabel->draw();
		break;
	case kSfxVolumeChanged:
		_sfxVolumeLabel->setValue(_sfxVolumeSlider->getValue());
		_sfxVolumeLabel->draw();
		break;
	case kOKCmd:
		setResult(1);
		close();
		break;
	default:
		Dialog::handleCommand(sender, cmd, data);
	}
}

int OptionsDialog::addGraphicControls(GuiObject *boss, int yoffset) {
	const int x = 10;
	const int w = _w - 2 * 10;

	// The GFX mode popup
	// TODO - add an API to query the list of available GFX modes, and to get/set the mode
	_gfxPopUp = new PopUpWidget(boss, x-5, yoffset, w+5, kLineHeight, "Graphics mode: ", 100);
	yoffset += 16;

	// FIXME: For the GlobalOptionsDialog, we don't want a <default> here;
	// rather, we want to setSelected to the current global
	_gfxPopUp->appendEntry("<default>");
	_gfxPopUp->appendEntry("");
	_gfxPopUp->appendEntry("Normal (no scaling)", GFX_NORMAL);
	_gfxPopUp->appendEntry("2x", GFX_DOUBLESIZE);
	_gfxPopUp->appendEntry("3x", GFX_TRIPLESIZE);
	_gfxPopUp->appendEntry("2xSAI", GFX_2XSAI);
	_gfxPopUp->appendEntry("Super2xSAI", GFX_SUPER2XSAI);
	_gfxPopUp->appendEntry("SuperEagle", GFX_SUPEREAGLE);
	_gfxPopUp->appendEntry("AdvMAME2x", GFX_ADVMAME2X);
	_gfxPopUp->appendEntry("AdvMAME3x", GFX_ADVMAME3X);
	_gfxPopUp->appendEntry("hq2x", GFX_HQ2X);
	_gfxPopUp->appendEntry("hq3x", GFX_HQ3X);
	_gfxPopUp->appendEntry("TV2x", GFX_TV2X);
	_gfxPopUp->appendEntry("DotMatrix", GFX_DOTMATRIX);

	// Fullscreen checkbox
	_fullscreenCheckbox = new CheckboxWidget(boss, x, yoffset, w, 16, "Fullscreen mode");
	yoffset += 16;

	// Aspect ratio checkbox
	_aspectCheckbox = new CheckboxWidget(boss, x, yoffset, w, 16, "Aspect ratio correction");
	yoffset += 16;

	return yoffset;
}

int OptionsDialog::addMIDIControls(GuiObject *boss, int yoffset) {
	const int x = 10;
	const int w = _w - 20;

	// The MIDI mode popup & a label
	_midiPopUp = new PopUpWidget(boss, x-5, yoffset, w+5, kLineHeight, "Music driver: ", 100);
	yoffset += 16;
	
	// Populate it
	const MidiDriverDescription *md = getAvailableMidiDrivers();
	while (md->name) {
		_midiPopUp->appendEntry(md->description, md->id);
		md++;
	}
	
	// Multi midi setting
	_multiMidiCheckbox = new CheckboxWidget(boss, x, yoffset, w, 16, "Mixed Adlib/MIDI mode");
	yoffset += 16;
	
	// Native mt32 setting
	_mt32Checkbox = new CheckboxWidget(boss, x, yoffset, w, 16, "True Roland MT-32 (disable GM emulation)");
	yoffset += 16;

	return yoffset;
}

int OptionsDialog::addVolumeControls(GuiObject *boss, int yoffset) {
	// Volume controllers
	_masterVolumeSlider = new SliderWidget(boss, 5, yoffset, 185, 12,  "Master volume: ", 100, kMasterVolumeChanged);
	_masterVolumeLabel = new StaticTextWidget(boss, 200, yoffset+2, 24, kLineHeight, "100%", kTextAlignLeft);
	_masterVolumeSlider->setMinValue(0); _masterVolumeSlider->setMaxValue(255);
	_masterVolumeLabel->setFlags(WIDGET_CLEARBG);
	yoffset += 16;

	_musicVolumeSlider = new SliderWidget(boss, 5, yoffset, 185, 12, "Music volume: ", 100, kMusicVolumeChanged);
	_musicVolumeLabel = new StaticTextWidget(boss, 200, yoffset+2, 24, kLineHeight, "100%", kTextAlignLeft);
	_musicVolumeSlider->setMinValue(0); _musicVolumeSlider->setMaxValue(255);
	_musicVolumeLabel->setFlags(WIDGET_CLEARBG);
	yoffset += 16;

	_sfxVolumeSlider = new SliderWidget(boss, 5, yoffset, 185, 12, "SFX volume: ", 100, kSfxVolumeChanged);
	_sfxVolumeLabel = new StaticTextWidget(boss, 200, yoffset+2, 24, kLineHeight, "100%", kTextAlignLeft);
	_sfxVolumeSlider->setMinValue(0); _sfxVolumeSlider->setMaxValue(255);
	_sfxVolumeLabel->setFlags(WIDGET_CLEARBG);
	yoffset += 16;

	return yoffset;
}

#pragma mark -


GlobalOptionsDialog::GlobalOptionsDialog(GameDetector &detector)
	: OptionsDialog(Common::ConfigManager::kApplicationDomain, 10, 20, 320 - 2 * 10, 200 - 2 * 20) {

	const int vBorder = 5;
	int yoffset;

	// The tab widget
	TabWidget *tab = new TabWidget(this, 0, vBorder, _w, _h - 24 - 2*vBorder);

	//
	// 1) The graphics tab
	//
	tab->addTab("Graphics");
	yoffset = vBorder;
	yoffset = addGraphicControls(tab, yoffset);


	//
	// 2) The audio tab
	//
	tab->addTab("Audio");
	yoffset = vBorder;
	yoffset = addMIDIControls(tab, yoffset);
	yoffset = addVolumeControls(tab, yoffset);
	// TODO: cd drive setting
	

	//
	// 3) The miscellaneous tab
	//
	tab->addTab("Misc");
	yoffset = vBorder;

#if !( defined(__DC__) || defined(__GP32__) )
	// Save game path
	new StaticTextWidget(tab, 5, yoffset+2, 100, kLineHeight, "Savegame path: ", kTextAlignRight);
	_savePath = new StaticTextWidget(tab, 105, yoffset+2, 180, kLineHeight, "/foo/bar", kTextAlignLeft);
	new ButtonWidget(tab, 105, yoffset+14, 64, 16, "Choose...", kChooseSaveDirCmd, 0);
#endif
	// TODO: joystick setting

	
	// Activate the first tab
	tab->setActiveTab(0);

	// Add OK & Cancel buttons
	addButton(_w - 2 * (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
	addButton(_w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);

	// Create file browser dialog
	_browser = new BrowserDialog("Select directory for savegames");
}

GlobalOptionsDialog::~GlobalOptionsDialog() {
	delete _browser;
}

void GlobalOptionsDialog::open() {
	OptionsDialog::open();

#if !( defined(__DC__) || defined(__GP32__) )
	// Set _savePath to the current save path
	Common::String dir(ConfMan.get("savepath", _domain));
	if (!dir.isEmpty()) {
		_savePath->setLabel(dir);
	} else {
		// Default to the current directory...
		char buf[256];
		getcwd(buf, sizeof(buf));
		_savePath->setLabel(buf);
	}
#endif
}

void GlobalOptionsDialog::close() {
	if (getResult()) {
		// Savepath
		ConfMan.set("savepath", _savePath->getLabel(), _domain);
	}
	OptionsDialog::close();
}

void GlobalOptionsDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kChooseSaveDirCmd:
		if (_browser->runModal()) {
			// User made his choice...
			FilesystemNode *dir = _browser->getResult();
			_savePath->setLabel(dir->path());
			// TODO - we should check if the directory is writeable before accepting it
		}
		break;
	default:
		OptionsDialog::handleCommand(sender, cmd, data);
	}
}
