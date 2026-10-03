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

namespace OpenXcom
{

/**
 * Every string the accessibility layer authors itself (graph a11y spec A8).
 * Game content is already localized and never goes through here.
 * English only for now; the table in Vocab.cpp is the one place to translate.
 */
namespace Vocab
{
	enum Id
	{
		STARTUP,
		NOTHING_TO_REPEAT,
		ROLE_BUTTON,
		ROLE_COMBO_BOX,
		ROLE_SLIDER,
		POSITION,
		NO_TOOLTIP,
		INVENTORY,
		CONTINUE,
		LIST,
		NEW_BATTLE_NO_CREW,
		PREVIOUS_SOLDIER,
		NEXT_SOLDIER,
		UNSEEN,
		EMPTY_TILE,
		MAP_EDGE,
		LEVEL,
		DIR_N,
		DIR_NE,
		DIR_E,
		DIR_SE,
		DIR_S,
		DIR_SW,
		DIR_W,
		DIR_NW,
		AND_LIST,
		STEPS,
		LEVELS_UP,
		LEVELS_DOWN,
		HERE,
		COORDS,
		WALL,
		DOOR,
		UFO_DOOR,
		UFO_DOOR_OPEN,
		OBSTACLE,
		DIAGONAL_WALL,
		OBJECT,
		STAIRS,
		NO_FLOOR,
		LIFT,
		EXIT_AREA,
		SMOKE,
		FIRE,
		MORE_ITEMS,
		UNIT_SUMMARY,
		UNIT_STATUS,
		KNEELING,
		FACING,
		LEFT_HAND,
		RIGHT_HAND,
		EMPTY,
		ROUNDS,
		NO_AMMO,
		PATH_COST,
		PATH_TOO_FAR,
		NO_PATH,
		NOT_NOW,
		CANCELLED,
		END_TURN_CONFIRM,
		TIME_UNITS_LEFT,
		SPOTTED,
		NO_SOLDIER,
		COUNT
	};

	/// Gets the text for a vocabulary entry.
	const std::string &get(Id id);
	/// Gets the text for a vocabulary entry with {0}, {1}... replaced by the arguments.
	std::string format(Id id, const std::vector<std::string> &args);
}

}
