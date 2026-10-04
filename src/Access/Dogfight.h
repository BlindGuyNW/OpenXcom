#pragma once
/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string>
#include "Screens.h"

namespace OpenXcom
{

class DogfightState;
class GeoscapeState;

/**
 * Interceptions. Dogfights aren't game states: they're windows the GeoscapeState
 * draws and ticks itself, so the Geoscape stays on top throughout.
 *
 * The screen recipe matches the Geoscape while any interception window is open
 * (not minimized), with one context per window: distance and damage, the five
 * attack modes (the current one "selected"), each weapon (Enter switches it
 * on or off), minimize.
 *
 * Narration, queued: the window's status messages ("UFO HIT!"), a weapon coming
 * into or going out of range or running out of ammo, an interception starting
 * and ending. Prefixed with the craft when more than one window is open.
 * The game deletes dogfights, crafts and UFOs silently, so only strings are kept.
 */
namespace Dogfight
{
	/// The interception windows recipe.
	AccessScreen screen();
	/// Per-frame tick while the Geoscape is on top, popup or not open: start, end, range and ammo narration.
	void update(GeoscapeState *state);
	/// Called by DogfightState::setStatus with the status string's id.
	void status(DogfightState *dogfight, const std::string &id);
	/// Restores the first minimized interception, as clicking its icon would. Returns false if there's none.
	bool restoreMinimized(GeoscapeState *state);
}

}
