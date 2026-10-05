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
#include <vector>
#include <SDL.h>

namespace OpenXcom
{

class GeoscapeState;
class Target;

/**
 * The Geoscape layer. Like the battle map, it isn't a graph screen: the navigator
 * hands it keys and ticks while the GeoscapeState is on top, no recipe matches
 * and no dogfight window is open. Main thread only.
 *
 * Keys: Space reads the date, time, funds and speed. The game's own keys pass
 * through: 1 to 6 set the speed, I intercept, B bases, U Ufopaedia, Escape options.
 *
 * The scanner, a list over what's on the globe: Period/Comma step through the
 * current category nearest the first base first, Shift+Period/Comma change
 * category (UFOs, alien sites, alien bases, craft in flight, bases, waypoints),
 * Enter opens the current entry as clicking it would, Ctrl+L reads it again.
 * D restores a minimized interception window.
 * Speed changes are spoken, queued.
 */
namespace Geo
{
	/// Acts on a key-down. Returns whether the layer owns the key.
	bool handleKey(GeoscapeState *state, SDLKey key, bool shift, bool ctrl);
	/// Per-frame tick: the speed differ.
	void update(GeoscapeState *state);
	/// What a craft can be sent to, as the globe shows it: UFOs, alien sites, alien bases, waypoints.
	std::vector<Target *> destinations();
	/// The country a point is in, or empty.
	std::string countryName(double lon, double lat);
	/// Where a point is, for speech: its country, else its region. Empty over open sea outside every region.
	std::string placeName(double lon, double lat);
	/// "over sea" when the globe shows water at a point (Globe::insideLand, the test the
	/// game uses to sink a downed UFO instead of leaving a crash site), else empty.
	std::string seaText(double lon, double lat);
	/// Distance and compass bearing from one target to a point: "1,230 nautical miles northeast".
	std::string offsetText(const Target *from, double lon, double lat);
}

}
