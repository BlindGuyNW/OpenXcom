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
#include "Screens.h"

namespace OpenXcom
{

class BattlescapeState;
class BattleUnit;
class SavedBattleGame;

/**
 * The battle map exploration layer: a keyboard tile cursor over the Battlescape.
 * It isn't a graph screen; the navigator hands it keys only while the
 * BattlescapeState is on top and no graph screen claims the keyboard.
 * Main thread only.
 *
 * Keys: arrows move the cursor by compass direction (Up is north),
 * Page Up/Down change level, Home returns to the selected soldier,
 * Enter acts on the cursor tile like a left click (first press previews a move
 * and says its cost, the second moves), Backspace turns to face the tile or
 * opens a door like a right click, Escape cancels targeting,
 * Tab/Shift+Tab cycle soldiers, Space reads the soldier's status,
 * Ctrl+L reads the cursor tile in full, Ctrl+E twice ends the turn.
 *
 * The scanner: Period/Comma step through the current category nearest first,
 * Shift+Period/Comma change category (soldiers, enemies, civilians, items,
 * doors, exits: craft and stage exit tiles, motion contacts: what the motion scanner picked up
 * this turn), Slash jumps the cursor to the current entry.
 */
namespace Battle
{
	/// Acts on a key-down. Returns whether the layer owns the key.
	bool handleKey(BattlescapeState *state, SDLKey key, bool shift, bool ctrl);
	/// Per-frame tick: the selection, spotting and action-result differs.
	void update(BattlescapeState *state);

	/// Shot results. ExplosionBState brackets each bullet hit or explosion with these:
	/// begin snapshots every unit's health, end says who was hit
	/// ("Sectoid Soldier hit", "missed, hit wooden fence", "explosion hits no one").
	void beginImpact(SavedBattleGame *save);
	void endImpact(SavedBattleGame *save, BattleUnit *attacker, bool areaEffect);
	/// A shot that left the map without hitting anything.
	void shotOffMap(SavedBattleGame *save);

	/// The motion scanner (ScannerState): every blip, nearest first, as size and offset from the soldier.
	AccessScreen scannerScreen();
}

}
