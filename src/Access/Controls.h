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
#include "Graph/GraphTypes.hpp"

namespace OpenXcom
{

class State;
class InteractiveSurface;
class TextButton;

/**
 * Building blocks for graph screen recipes: the one control type registry
 * (graph a11y spec 3.3) and helpers that drive the game's own widgets (A5).
 */
namespace Controls
{
	/// The "button" control type.
	const Graph::ControlType &button();
	/// Clicks a surface the way the mouse would: press, release, click.
	/// Runs the surface's own handlers, so sounds and side effects match a real click.
	void click(State *state, InteractiveSurface *surface, Uint8 mouseButton = SDL_BUTTON_LEFT);
	/// A node for a text button: its visible text as the label, Enter clicks it.
	Graph::NodeVtable textButton(State *state, TextButton *button);
}

}
