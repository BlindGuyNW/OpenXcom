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
#include <functional>
#include <string>
#include <vector>
#include <SDL.h>
#include "Graph/GraphBuilder.hpp"

namespace OpenXcom
{

class State;

/**
 * A graph screen recipe (graph a11y spec 9): an accessible view over one game State.
 * Only the top of the game's state stack gets input, so the navigator only ever
 * matches recipes against that state.
 */
struct AccessScreen
{
	/// Stable name for logs.
	std::string key;
	/// Does this recipe handle the given state? Usually a dynamic_cast.
	std::function<bool(State *)> isActive;
	/// Declares the screen's nodes from live game state. Runs several times per keypress, so keep it cheap.
	std::function<void(Graph::GraphBuilder &, State *)> build;
	/// The name spoken when the screen comes up. Optional.
	std::function<std::string(State *)> name;
	/// Escape. Optional; without it Escape goes to the game as usual.
	std::function<void(State *)> back;
	/// Called every frame while the recipe is attached, for narration that follows the game. Optional.
	std::function<void(State *)> tick;
	/// While this returns true the navigator stands down and every key goes to the game,
	/// as for a focused text field: the key binding list waiting for its new key. Optional.
	std::function<bool(State *)> passKeys;
	/// Offered each key-down before the navigator's own keys, with the focused node: returns true
	/// to claim it. For controls the graph keys can't express, like the globe cursor's four-way moves. Optional.
	std::function<bool(State *, const Graph::ControlId &, SDLKey, bool shift, bool ctrl)> keys;
};

namespace Screens
{
	/// Every registered recipe, in match priority order.
	const std::vector<AccessScreen> &all();
}

}
