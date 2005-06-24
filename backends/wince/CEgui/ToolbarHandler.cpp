/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001-2005 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "common/stdafx.h"
#include "ToolbarHandler.h"

namespace CEGUI {

	ToolbarHandler::ToolbarHandler():
	_current(""), _active(NULL) {
	}


	bool ToolbarHandler::add(const String &name, const Toolbar &toolbar) {
		_toolbarMap.addKey(name);
		_toolbarMap[name] = (Toolbar*)&toolbar;

		
		if (!_active) {
			_active = &((Toolbar&)toolbar);
			_current = name;
		}

		return true;
	}

	String ToolbarHandler::activeName() {
		return _current;
	}

	bool ToolbarHandler::setActive(const String &name) {
		if (!_toolbarMap.contains(name))
			return false;
		if (_current == name)
			return true;
		_current = name;
		_active = _toolbarMap[name];
		_active->forceRedraw();
		return true;
	}

	bool ToolbarHandler::action(int x, int y, bool pushed) {
		if (_active) {
			// FIXME !
			if (_offset > 240)
				return _active->action(x / 2, (y - _offset) / 2, pushed);
			else
				return _active->action(x, y - _offset, pushed);			
		}
		else
			return false;
	}

	void ToolbarHandler::setVisible(bool visible) {
		if (_active)
			_active->setVisible(visible);
	}

	bool ToolbarHandler::visible() {
		if (_active)
			return _active->visible();
		else
			return false;
	}

	void ToolbarHandler::forceRedraw() {
		if (_active)
			_active->forceRedraw();
	}

	bool ToolbarHandler::drawn() {
		if (_active)
			return _active->drawn();
		else
			return false;
	}

	bool ToolbarHandler::draw(SDL_Surface *surface, SDL_Rect *rect) {
		if (_active) {
			bool result = _active->draw(surface);
			if (result) {
				rect->x = _active->x();
				rect->y = _active->y();
				rect->w = _active->width();
				rect->h = _active->height();
			}
			return result;
		}
		else
			return false;
	}

	void ToolbarHandler::setOffset(int offset) {
		_offset = offset;
	}

	int ToolbarHandler::getOffset() {
		return _offset;
	}

	Toolbar* ToolbarHandler::active() {
		return _active;
	}

	ToolbarHandler::~ToolbarHandler() {
		_toolbarMap.clear();
	}
}