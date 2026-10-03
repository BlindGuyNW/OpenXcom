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
#include <SDL.h>

namespace OpenXcom
{

class Game;

/**
 * The graph navigator and screen manager (graph a11y spec 7 and 9).
 * Matches the top game State against the registered screen recipes, owns the
 * keys while a recipe is attached, and announces focus changes exactly once.
 * Main thread only.
 *
 * Keys on a graph screen: arrows move (Left/Right adjust where a control can),
 * Home/End jump to the ends, Tab/Shift+Tab cycle zones, Enter activates,
 * Backspace is the secondary action, Space reads the tooltip, Ctrl+L says where
 * you are, and Escape goes back where the screen defines it.
 * Everywhere: Ctrl+R repeats the last thing spoken.
 */
namespace Navigator
{
	/// Installs the announcer's wording hooks. Call once after Speech::init.
	void init();
	/// Offers a raw SDL event to the layer before the game sees it.
	/// Returns true when the layer claimed it and the game must not handle it.
	bool handleEvent(Game *game, const SDL_Event &ev);
	/// Per-frame tick: attaches to the top state's screen and announces focus changes.
	void update(Game *game);
}

}
