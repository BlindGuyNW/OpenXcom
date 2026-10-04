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
		EDGE,
		WALL,
		DOOR,
		UFO_DOOR,
		UFO_DOOR_OPEN,
		OBSTACLE,
		DIAGONAL_WALL,
		OBJECT,
		STAIRS,
		IMPASSABLE,
		NO_FLOOR,
		LIFT,
		EXIT_AREA,
		CRAFT_AREA,
		SMOKE,
		FIRE,
		MORE_ITEMS,
		UNIT_SUMMARY,
		UNIT_STATUS,
		KNEELING,
		STANDING,
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
		NO_FLOOR_BELOW,
		SCAN_SOLDIERS,
		SCAN_ENEMIES,
		SCAN_CIVILIANS,
		SCAN_ITEMS,
		SCAN_DOORS,
		SCAN_EXITS,
		SCAN_CATEGORY,
		SCAN_NONE,
		SCAN_GONE,
		ACCURACY,
		TU_COST,
		NOT_ENOUGH_TU,
		AIMING,
		IN_VIEW,
		OUT_OF_VIEW,
		FRIENDLY,
		UNIT_KILLED,
		UNIT_UNCONSCIOUS,
		UNIT_WOUNDED,
		PRIMED,
		HOLDING,
		HELD,
		LOADED,
		PLACED,
		PUT_BACK,
		FIRE_FROM,
		FIRE_FROM_AT,
		UNIT_FIRES,
		UNIT_FIRES_AT,
		ABOVE,
		BELOW,
		FACING_YOU,
		DARK,
		DEBRIEF_ROW,
		STAT_GAIN,
		NO_STAT_GAINS,
		UNIT_HIT,
		MISSED,
		MISSED_INTO,
		BLAST_NO_ONE,
		SOLID,
		UNIT_NUMBER,
		OUT_OF_SIGHT,
		UNIT_BLEEDING,
		MEDIKIT_ON,
		HEALTH,
		NO_FATAL_WOUNDS,
		ONE_FATAL_WOUND,
		FATAL_WOUNDS,
		FATAL_WOUNDS_IN,
		SELECTED,
		PART_SELECTED,
		ITEMS_LEFT,
		CLOSE,
		EQUIP_HINT,
		IN_STORES,
		STORES_UNLIMITED,
		ON_CRAFT,
		WAYPOINT_SET,
		WAYPOINT_SET_OF,
		WAYPOINTS_FULL,
		WAYPOINT_REMOVED,
		WAYPOINTS_CLEARED,
		LAUNCHED,
		ON,
		OFF,
		ARROW_UP,
		ARROW_DOWN,
		ARROW_LEFT,
		ARROW_RIGHT,
		ROLE_EDIT,
		EDITING,
		COUNT
	};

	/// Gets the text for a vocabulary entry.
	const std::string &get(Id id);
	/// Gets the text for a vocabulary entry with {0}, {1}... replaced by the arguments.
	std::string format(Id id, const std::vector<std::string> &args);
}

}
