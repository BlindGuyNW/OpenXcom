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
#include "Vocab.h"

namespace OpenXcom
{

namespace Vocab
{

namespace
{
	// Must match the order of Vocab::Id.
	const std::string _strings[COUNT] =
	{
		"OpenXcom accessibility loaded",
		"Nothing to repeat",
		"button",
		"combo box",
		"slider",
		"{0} of {1}",
		"No tooltip",
		"Inventory, {0}",
		"Continue",
		"list",
		"The craft has no soldiers or tanks. Add some under Equip Craft, Crew.",
		"Previous soldier",
		"Next soldier",
		"unseen",
		"empty",
		"edge of map",
		"level {0}",
		"north",
		"northeast",
		"east",
		"southeast",
		"south",
		"southwest",
		"west",
		"northwest",
		"{0} and {1}",
		"{0} {1}",
		"{0} up",
		"{0} down",
		"here",
		"x {0}, y {1}",
		"{0} {1}",
		"wall",
		"door",
		"UFO door",
		"open UFO door",
		"obstacle",
		"diagonal wall",
		"object",
		"stairs",
		"impassable",
		"no floor",
		"lift",
		"exit area",
		"craft",
		"smoke",
		"fire",
		"{0} more items",
		"{0}, {1} time units",
		"{0}, {1} of {2} time units, health {3} of {4}, energy {5} of {6}, morale {7}",
		"kneeling",
		"standing",
		"facing {0}",
		"left hand {0}",
		"right hand {0}",
		"empty",
		"{0}, {1} rounds",
		"{0}, no ammo",
		"{0} time units, {1} left. Enter again to move.",
		"{0} time units, you have {1}. Enter again to move as far as you can.",
		"No path",
		"Not now",
		"Cancelled",
		"Press Control E again to end the turn",
		"{0} time units left",
		"{0} spotted, {1}",
		"No soldier selected",
		"no floor, {0} below: {1}",
		"Soldiers",
		"Enemies",
		"Civilians",
		"Items",
		"Doors",
		"Exits",
		"{0}, {1}",
		"{0}, none",
		"That's gone",
		"accuracy {0}%",
		"{0} time units",
		"not enough time units",
		"{0}, choose a target",
		"in view",
		"out of view",
		"friendly",
		"{0} killed",
		"{0} unconscious",
		"{0} hit, health {1}",
		"{0}, primed, {1}",
		"holding {0}",
		"{0}, held",
		"{0} loaded",
		"{0} to {1}",
		"put back",
		"fire from {0}",
		"fire from {0} at {1}",
		"{0} fires",
		"{0} fires at {1}",
		"above",
		"below",
		"facing {0}, toward you",
		"dark",
		"{0}: {1}, score {2}",
		"{0} {1}",
		"no change",
		"{0} hit",
		"missed",
		"missed, hit {0}",
		"explosion hits no one",
		"solid",
		"{0} {1}",
		"lost sight of {0}",
		"{0} bleeding, health {1}",
		"medikit on {0}",
		"health {0}",
		"no fatal wounds",
		"1 fatal wound",
		"{0} fatal wounds",
		"fatal wounds: {0}",
		"selected",
		"{0} selected",
		"{0} left",
		"close",
		"Right moves one to the craft, Left one back to the stores. Shift for five, Ctrl for all",
		"{0} in stores",
		"unlimited in stores",
		"{0} on craft",
		"waypoint {0}, Enter again here to launch",
		"waypoint {0} of {1}, Enter again here to launch",
		"no more waypoints, Enter on the last one to launch",
		"waypoint removed, {0} left",
		"waypoints cleared, still aiming",
		"launch",
		"on",
		"off",
		"up arrow",
		"down arrow",
		"left arrow",
		"right arrow",
		"edit",
		"Editing {0}. Type, then Enter to finish. Ctrl+L reads the field",
		"blank",
		"space",
		"UFOs",
		"alien sites",
		"alien bases",
		"craft in flight",
		"bases",
		"waypoints",
		"{0} nautical miles {1}",
		"{0}, {1}, funds {2}, speed {3}",
		"speed {0}",
		"Nothing picked. Period and Comma step through what's on the globe, Shift changes category",
		"{0}, a base costs {1}",
		"No new research topics.",
		"Accessibility error on {0}, in {1}: {2}. Details in openxcom.log",
		"{0}: {1}",
		"disengage",
		"weapon {0}, {1}, ammo {2}",
		"in range",
		"out of range",
		"distance {0}",
		"damage {0} percent",
		"minimize",
		"{0} versus {1}",
		"{0} intercepting {1}",
		"{0}, interception over",
		"{0} in range",
		"{0} out of range",
		"{0} out of ammo",
		"No minimized interceptions",
		"{0}, {1}, {2}, {3} weapons, {4} soldiers, {5} tanks",
		"Can't launch: {0}",
		"low on fuel",
		"returning to base",
		"targets",
		"weapon {0}, {1}, ammo {2} of {3}",
		"weapon {0}, none",
		"{0}, {1} soldiers, {2} spaces free",
		"{0}, {1} tanks, {2} items",
		"{0}, {1}, {2} of {3} weapons, {4} soldiers, {5} tanks",
		"Can't open: {0} is out",
		"bases",
		"{0} of {1}",
		"{0}: {1}",
		"empty",
		"previous article",
		"next article",
	};
}

const std::string &get(Id id)
{
	static const std::string empty;
	return (id >= 0 && id < COUNT) ? _strings[id] : empty;
}

std::string format(Id id, const std::vector<std::string> &args)
{
	std::string s = get(id);
	for (size_t i = 0; i < args.size(); ++i)
	{
		std::string token = "{" + std::to_string(i) + "}";
		for (size_t pos = s.find(token); pos != std::string::npos; pos = s.find(token, pos + args[i].size()))
		{
			s.replace(pos, token.size(), args[i]);
		}
	}
	return s;
}

}

}
