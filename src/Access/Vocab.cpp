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
