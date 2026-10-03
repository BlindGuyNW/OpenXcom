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
#include "Battle.h"
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "Speech.h"
#include "Vocab.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/Camera.h"
#include "../Battlescape/Map.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/Position.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/Cursor.h"
#include "../Mod/MapData.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"

namespace OpenXcom
{

namespace Battle
{

namespace
{
	/// The battle the state below belongs to; a new one resets it.
	SavedBattleGame *_battle = 0;
	Position _cursor;
	/// The differ's memory of the selected unit.
	BattleUnit *_selected = 0;
	/// The last player unit selected. Offsets are measured from it.
	BattleUnit *_soldier = 0;
	/// Hostiles visible last frame, so newly spotted ones are spoken once.
	std::set<BattleUnit *> _spotted;
	/// We started an action; speak the result once the game is idle again.
	bool _awaiting = false;
	/// When Ctrl+E was first pressed; a second press soon after ends the turn.
	Uint32 _endTurnArmed = 0;
	const Uint32 END_TURN_WINDOW = 3000;

	void say(const std::string &text, bool interrupt)
	{
		Speech::say(text, interrupt);
	}

	std::string num(int n)
	{
		return std::to_string(n);
	}

	SavedBattleGame *saveOf(BattlescapeState *state)
	{
		return state->getBattleGame()->getSave();
	}

	/// Compass name for a game direction (0 = north, clockwise).
	const std::string &dirName(int dir)
	{
		static const Vocab::Id names[8] = { Vocab::DIR_N, Vocab::DIR_NE, Vocab::DIR_E, Vocab::DIR_SE, Vocab::DIR_S, Vocab::DIR_SW, Vocab::DIR_W, Vocab::DIR_NW };
		return Vocab::get(names[((dir % 8) + 8) % 8]);
	}

	/// "a", "a and b", "a, b and c".
	std::string joinAnd(const std::vector<std::string> &items)
	{
		if (items.empty())
			return std::string();
		std::string head = items[0];
		for (size_t i = 1; i + 1 < items.size(); ++i)
			head += ", " + items[i];
		return items.size() == 1 ? head : Vocab::format(Vocab::AND_LIST, { head, items.back() });
	}

	std::string joinComma(const std::vector<std::string> &items)
	{
		std::string s;
		for (const std::string &item : items)
		{
			if (item.empty())
				continue;
			if (!s.empty())
				s += ", ";
			s += item;
		}
		return s;
	}

	/// "3 north, 2 east, 1 up" from one position to another.
	std::string offsetText(Position from, Position to)
	{
		std::vector<std::string> parts;
		int dx = to.x - from.x, dy = to.y - from.y, dz = to.z - from.z;
		if (dy)
			parts.push_back(Vocab::format(Vocab::STEPS, { num(std::abs(dy)), dirName(dy < 0 ? 0 : 4) }));
		if (dx)
			parts.push_back(Vocab::format(Vocab::STEPS, { num(std::abs(dx)), dirName(dx > 0 ? 2 : 6) }));
		if (dz)
			parts.push_back(Vocab::format(dz > 0 ? Vocab::LEVELS_UP : Vocab::LEVELS_DOWN, { num(std::abs(dz)) }));
		return parts.empty() ? Vocab::get(Vocab::HERE) : joinComma(parts);
	}

	/// Where offsets are measured from: the last selected soldier, or the cursor if there's none.
	Position anchor()
	{
		return (_soldier && !_soldier->isOut()) ? _soldier->getPosition() : _cursor;
	}

	/// Parity: our own units always, others only while a sighted player could see them.
	bool unitShown(BattleUnit *unit)
	{
		return unit && !unit->isOut() && (unit->getFaction() == FACTION_PLAYER || unit->getVisible());
	}

	std::string itemText(BattlescapeState *state, BattleItem *item)
	{
		std::string name = state->tr(item->getRules()->getName());
		if (!item->getRules()->getCompatibleAmmo()->empty())
		{
			BattleItem *ammo = item->getAmmoItem();
			if (!ammo)
				return Vocab::format(Vocab::NO_AMMO, { name });
			return Vocab::format(Vocab::ROUNDS, { name, num(ammo->getAmmoQuantity()) });
		}
		return name;
	}

	/// The edge kinds we describe, in the order they're read out.
	enum EdgeKind { EDGE_NONE, EDGE_WALL, EDGE_DOOR, EDGE_UFO_DOOR, EDGE_UFO_DOOR_OPEN };

	EdgeKind wallKind(Tile *owner, TilePart part)
	{
		MapData *md = owner->getMapData(part);
		if (!md)
			return EDGE_NONE;
		if (md->isUFODoor())
			return owner->isUfoDoorOpen(part) ? EDGE_UFO_DOOR_OPEN : EDGE_UFO_DOOR;
		if (md->isDoor())
			return EDGE_DOOR;
		// Wall pieces you can walk through (an opened door's replacement, for one) aren't worth a word.
		if (owner->getTUCost(part, MT_WALK) >= 255)
			return EDGE_WALL;
		return EDGE_NONE;
	}

	int bigWall(Tile *tile)
	{
		MapData *md = tile ? tile->getMapData(O_OBJECT) : 0;
		return md ? md->getBigWall() : 0;
	}

	/// Does a big-wall object on a tile block the given side of that tile (0 N, 2 E, 4 S, 6 W)?
	bool bigWallBlocks(int type, int side)
	{
		switch (type)
		{
		case Pathfinding::BIGWALLNORTH: return side == 0;
		case Pathfinding::BIGWALLEAST: return side == 2;
		case Pathfinding::BIGWALLSOUTH: return side == 4;
		case Pathfinding::BIGWALLWEST: return side == 6;
		case Pathfinding::BIGWALLEASTANDSOUTH: return side == 2 || side == 4;
		case Pathfinding::BIGWALLWESTANDNORTH: return side == 6 || side == 0;
		default: return false;
		}
	}

	/// What stands on one side of a tile. Edges belong to whichever tile owns them:
	/// a tile has only west and north walls, so its east wall is the west wall of the tile to the east.
	EdgeKind edgeAt(SavedBattleGame *save, Tile *tile, Position p, int side)
	{
		Tile *owner = 0;
		TilePart part = O_NORTHWALL;
		int seenFlag = 1;
		switch (side)
		{
		case 0: owner = tile; part = O_NORTHWALL; seenFlag = 1; break;
		case 6: owner = tile; part = O_WESTWALL; seenFlag = 0; break;
		case 4: owner = save->getTile(p + Position(0, 1, 0)); part = O_NORTHWALL; seenFlag = 1; break;
		case 2: owner = save->getTile(p + Position(1, 0, 0)); part = O_WESTWALL; seenFlag = 0; break;
		}
		if (owner && (owner->isDiscovered(2) || owner->isDiscovered(seenFlag)))
		{
			EdgeKind kind = wallKind(owner, part);
			if (kind != EDGE_NONE)
				return kind;
		}
		if (bigWallBlocks(bigWall(tile), side))
			return EDGE_WALL;
		Position v;
		Pathfinding::directionToVector(side, &v);
		Tile *neighbour = save->getTile(p + v);
		if (neighbour && neighbour->isDiscovered(2) && bigWallBlocks(bigWall(neighbour), (side + 4) % 8))
			return EDGE_WALL;
		return EDGE_NONE;
	}

	/// "wall north and east, door south".
	std::string edgesText(SavedBattleGame *save, Tile *tile, Position p)
	{
		static const int sides[4] = { 0, 2, 4, 6 };
		std::map<EdgeKind, std::vector<std::string> > byKind;
		for (int side : sides)
		{
			EdgeKind kind = edgeAt(save, tile, p, side);
			if (kind != EDGE_NONE)
				byKind[kind].push_back(dirName(side));
		}
		std::vector<std::string> parts;
		for (const auto &k : byKind)
		{
			Vocab::Id id = Vocab::WALL;
			switch (k.first)
			{
			case EDGE_DOOR: id = Vocab::DOOR; break;
			case EDGE_UFO_DOOR: id = Vocab::UFO_DOOR; break;
			case EDGE_UFO_DOOR_OPEN: id = Vocab::UFO_DOOR_OPEN; break;
			default: break;
			}
			parts.push_back(Vocab::format(id, { joinAnd(k.second) }));
		}
		return joinComma(parts);
	}

	/// The tile's own contents: object, floor, smoke and fire.
	std::vector<std::string> terrainParts(SavedBattleGame *save, Tile *tile, Position p, bool full)
	{
		std::vector<std::string> parts;
		MapData *object = tile->getMapData(O_OBJECT);
		if (object)
		{
			int big = object->getBigWall();
			if (big == Pathfinding::BLOCK)
				parts.push_back(Vocab::get(Vocab::OBSTACLE));
			else if (big == Pathfinding::BIGWALLNESW || big == Pathfinding::BIGWALLNWSE)
				parts.push_back(Vocab::get(Vocab::DIAGONAL_WALL));
			else if (big == 0)
			{
				if (object->getTerrainLevel() < 0)
					parts.push_back(Vocab::get(Vocab::STAIRS));
				else if (tile->getTUCost(O_OBJECT, MT_WALK) >= 255)
					parts.push_back(Vocab::get(Vocab::OBSTACLE));
				else if (full)
					parts.push_back(Vocab::get(Vocab::OBJECT));
			}
		}
		MapData *floor = tile->getMapData(O_FLOOR);
		if (floor && floor->isGravLift())
			parts.push_back(Vocab::get(Vocab::LIFT));
		if (floor && floor->getSpecialType() == END_POINT)
			parts.push_back(Vocab::get(Vocab::EXIT_AREA));
		if (p.z > 0 && tile->hasNoFloor(save->getTile(p + Position(0, 0, -1))))
			parts.push_back(Vocab::get(Vocab::NO_FLOOR));
		if (tile->getSmoke())
			parts.push_back(Vocab::get(Vocab::SMOKE));
		if (tile->getFire())
			parts.push_back(Vocab::get(Vocab::FIRE));
		return parts;
	}

	/// What a sighted player sees on a tile. Brief is for cursor steps, full for Ctrl+L.
	std::string describeTile(BattlescapeState *state, Position p, bool full)
	{
		SavedBattleGame *save = saveOf(state);
		Tile *tile = save->getTile(p);
		if (!tile)
			return Vocab::get(Vocab::MAP_EDGE);
		if (!tile->isDiscovered(2))
			return Vocab::get(Vocab::UNSEEN);

		std::vector<std::string> parts;
		BattleUnit *unit = tile->getUnit();
		if (unitShown(unit))
			parts.push_back(unit->getName(state->getGame()->getLanguage()));

		std::vector<BattleItem *> *items = tile->getInventory();
		const size_t shown = full ? items->size() : 2;
		for (size_t i = 0; i < items->size() && i < shown; ++i)
			parts.push_back(state->tr((*items)[i]->getRules()->getName()));
		if (items->size() > shown)
			parts.push_back(Vocab::format(Vocab::MORE_ITEMS, { num((int)(items->size() - shown)) }));

		for (const std::string &s : terrainParts(save, tile, p, full))
			parts.push_back(s);
		parts.push_back(edgesText(save, tile, p));

		std::string text = joinComma(parts);
		return text.empty() ? Vocab::get(Vocab::EMPTY_TILE) : text;
	}

	std::string unitSummary(BattlescapeState *state, BattleUnit *unit)
	{
		return Vocab::format(Vocab::UNIT_SUMMARY, { unit->getName(state->getGame()->getLanguage()), num(unit->getTimeUnits()) });
	}

	std::string unitStatus(BattlescapeState *state, BattleUnit *unit)
	{
		UnitStats *stats = unit->getBaseStats();
		std::vector<std::string> parts;
		parts.push_back(Vocab::format(Vocab::UNIT_STATUS, {
			unit->getName(state->getGame()->getLanguage()),
			num(unit->getTimeUnits()), num(stats->tu),
			num(unit->getHealth()), num(stats->health),
			num(unit->getEnergy()), num(stats->stamina),
			num(unit->getMorale()) }));
		if (unit->isKneeled())
			parts.push_back(Vocab::get(Vocab::KNEELING));
		parts.push_back(Vocab::format(Vocab::FACING, { dirName(unit->getDirection()) }));
		BattleItem *left = unit->getItem("STR_LEFT_HAND"), *right = unit->getItem("STR_RIGHT_HAND");
		parts.push_back(Vocab::format(Vocab::RIGHT_HAND, { right ? itemText(state, right) : Vocab::get(Vocab::EMPTY) }));
		parts.push_back(Vocab::format(Vocab::LEFT_HAND, { left ? itemText(state, left) : Vocab::get(Vocab::EMPTY) }));
		return joinComma(parts);
	}

	/// Can the player act right now? Mirrors what BattlescapeState::mapClick accepts.
	bool canAct(BattlescapeState *state)
	{
		BattlescapeGame *bg = state->getBattleGame();
		return state->getGame()->getCursor()->getVisible()
			&& state->getMap()->getCursorType() != CT_NONE
			&& !bg->isBusy()
			&& bg->getSave()->getSide() == FACTION_PLAYER;
	}

	/// Keeps the sighted view on the cursor: centres the camera and moves the tile selector.
	void showCursor(BattlescapeState *state)
	{
		Map *map = state->getMap();
		map->getCamera()->centerOnPosition(_cursor);
		map->setSelectorTile(_cursor);
	}

	/// Settles a position onto the walkable surface the way Pathfinding::calculate settles a click:
	/// down through tiles with no floor, up off a full-height step. Only onto tiles already seen.
	Position settle(SavedBattleGame *save, Position p)
	{
		for (;;)
		{
			Tile *tile = save->getTile(p);
			Tile *below = save->getTile(p + Position(0, 0, -1));
			Tile *above = save->getTile(p + Position(0, 0, 1));
			if (below && below->isDiscovered(2) && tile->hasNoFloor(below))
				p.z--;
			else if (above && above->isDiscovered(2) && tile->getTerrainLevel() == -24)
				p.z++;
			else
				return p;
		}
	}

	void moveCursor(BattlescapeState *state, Position to, bool levelChange)
	{
		SavedBattleGame *save = saveOf(state);
		if (!save->getTile(to))
		{
			say(Vocab::get(Vocab::MAP_EDGE), true);
			return;
		}
		if (!levelChange)
		{
			to = settle(save, to);
			levelChange = to.z != _cursor.z;
		}
		_cursor = to;
		showCursor(state);
		std::string text = describeTile(state, _cursor, false);
		if (levelChange)
			text = Vocab::format(Vocab::LEVEL, { num(_cursor.z + 1) }) + ", " + text;
		say(text, true);
	}

	/// Total TU cost of the path the game just calculated, walked the way previewPath does.
	int pathCost(SavedBattleGame *save, BattleUnit *unit)
	{
		Pathfinding *pf = save->getPathfinding();
		const std::vector<int> &path = pf->getPath();
		Position pos = unit->getPosition(), next;
		int total = unit->isKneeled() ? 8 : 0;
		for (std::vector<int>::const_reverse_iterator i = path.rbegin(); i != path.rend(); ++i)
		{
			total += pf->getTUCost(pos, *i, &next, unit, 0, false);
			pos = next;
		}
		return total;
	}

	/// Enter: a left click on the cursor tile.
	void primary(BattlescapeState *state)
	{
		if (!canAct(state))
		{
			say(Vocab::get(Vocab::NOT_NOW), true);
			return;
		}
		BattlescapeGame *bg = state->getBattleGame();
		SavedBattleGame *save = bg->getSave();
		if (!bg->getCurrentAction()->targeting && !state->playableUnitSelected())
		{
			say(Vocab::get(Vocab::NO_SOLDIER), true);
			return;
		}
		BattleUnit *before = save->getSelectedUnit();
		bool targeting = bg->getCurrentAction()->targeting;
		bg->primaryAction(_cursor);
		if (bg->isBusy())
		{
			_awaiting = true;
			return;
		}
		// Selecting another soldier: the selection differ speaks it.
		if (targeting || save->getSelectedUnit() != before)
			return;
		Pathfinding *pf = save->getPathfinding();
		if (!pf->isPathPreviewed())
		{
			say(Vocab::get(Vocab::NO_PATH), true);
			return;
		}
		int cost = pathCost(save, before), tus = before->getTimeUnits();
		if (cost <= tus)
			say(Vocab::format(Vocab::PATH_COST, { num(cost), num(tus - cost) }), true);
		else
			say(Vocab::format(Vocab::PATH_TOO_FAR, { num(cost), num(tus) }), true);
	}

	/// Backspace: a right click on the cursor tile. Cancels a preview or targeting first, like the mouse.
	void secondary(BattlescapeState *state)
	{
		if (!canAct(state))
		{
			say(Vocab::get(Vocab::NOT_NOW), true);
			return;
		}
		BattlescapeGame *bg = state->getBattleGame();
		if (bg->cancelCurrentAction())
		{
			say(Vocab::get(Vocab::CANCELLED), true);
			return;
		}
		if (!state->playableUnitSelected())
		{
			say(Vocab::get(Vocab::NO_SOLDIER), true);
			return;
		}
		bg->secondaryAction(_cursor);
		_awaiting = bg->isBusy();
	}

	void endTurn(BattlescapeState *state)
	{
		Uint32 now = SDL_GetTicks();
		if (_endTurnArmed && now - _endTurnArmed <= END_TURN_WINDOW)
		{
			_endTurnArmed = 0;
			state->btnEndTurnClick(0);
			return;
		}
		_endTurnArmed = now;
		say(Vocab::get(Vocab::END_TURN_CONFIRM), true);
	}

	void cycleSoldier(BattlescapeState *state, bool back)
	{
		BattleUnit *before = saveOf(state)->getSelectedUnit();
		if (back)
			state->btnPrevSoldierClick(0);
		else
			state->btnNextSoldierClick(0);
		// Only one soldier left: the differ won't fire, so answer the keypress here.
		BattleUnit *after = saveOf(state)->getSelectedUnit();
		if (after && after == before && after->getFaction() == FACTION_PLAYER)
			say(unitSummary(state, after), true);
	}

	void reset(SavedBattleGame *save)
	{
		_battle = save;
		_selected = 0;
		_soldier = 0;
		_spotted.clear();
		_awaiting = false;
		_endTurnArmed = 0;
		_cursor = Position(save->getMapSizeX() / 2, save->getMapSizeY() / 2, 0);
		// Enter previews a move before making it, so the game's two-click move must be on.
		if (Options::battleNewPreviewPath == PATH_NONE)
			Options::battleNewPreviewPath = PATH_FULL;
	}
}

bool handleKey(BattlescapeState *state, SDLKey key, bool shift, bool ctrl)
{
	SavedBattleGame *save = saveOf(state);
	if (save != _battle)
		reset(save);

	if (ctrl)
	{
		switch (key)
		{
		case SDLK_l:
		{
			std::string text = describeTile(state, _cursor, true);
			text += ", " + Vocab::format(Vocab::LEVEL, { num(_cursor.z + 1) });
			text += ", " + offsetText(anchor(), _cursor);
			text += ", " + Vocab::format(Vocab::COORDS, { num(_cursor.x), num(_cursor.y) });
			say(text, true);
			return true;
		}
		case SDLK_e:
			endTurn(state);
			return true;
		default:
			return false;
		}
	}

	switch (key)
	{
	case SDLK_UP:
		moveCursor(state, _cursor + Position(0, -1, 0), false);
		return true;
	case SDLK_DOWN:
		moveCursor(state, _cursor + Position(0, 1, 0), false);
		return true;
	case SDLK_LEFT:
		moveCursor(state, _cursor + Position(-1, 0, 0), false);
		return true;
	case SDLK_RIGHT:
		moveCursor(state, _cursor + Position(1, 0, 0), false);
		return true;
	case SDLK_PAGEUP:
		moveCursor(state, _cursor + Position(0, 0, 1), true);
		return true;
	case SDLK_PAGEDOWN:
		moveCursor(state, _cursor + Position(0, 0, -1), true);
		return true;
	case SDLK_HOME:
		if (_soldier && !_soldier->isOut())
		{
			moveCursor(state, _soldier->getPosition(), _soldier->getPosition().z != _cursor.z);
		}
		else
		{
			say(Vocab::get(Vocab::NO_SOLDIER), true);
		}
		return true;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		primary(state);
		return true;
	case SDLK_BACKSPACE:
		// The game binds Backspace to end turn; here it's the right click instead.
		secondary(state);
		return true;
	case SDLK_ESCAPE:
		if (state->getBattleGame()->getCurrentAction()->targeting && canAct(state))
		{
			state->getBattleGame()->cancelCurrentAction();
			say(Vocab::get(Vocab::CANCELLED), true);
			return true;
		}
		return false;
	case SDLK_TAB:
		cycleSoldier(state, shift);
		return true;
	case SDLK_LSHIFT:
		// The game binds Left Shift alone to previous soldier, which fires on every Shift+key.
		return true;
	case SDLK_SPACE:
		if (_soldier && !_soldier->isOut())
			say(unitStatus(state, _soldier), true);
		else
			say(Vocab::get(Vocab::NO_SOLDIER), true);
		return true;
	default:
		return false;
	}
}

void update(BattlescapeState *state)
{
	SavedBattleGame *save = saveOf(state);
	if (save != _battle)
		reset(save);

	// The selection differ: the cursor follows the selected soldier and says who it is.
	BattleUnit *selected = save->getSelectedUnit();
	if (selected != _selected)
	{
		_selected = selected;
		if (selected && selected->getFaction() == FACTION_PLAYER && save->getSide() == FACTION_PLAYER)
		{
			_soldier = selected;
			_cursor = selected->getPosition();
			showCursor(state);
			say(unitSummary(state, selected), true);
		}
	}

	// Newly spotted hostiles, queued since they're narration rather than a reply to a key.
	std::set<BattleUnit *> visible;
	for (BattleUnit *unit : *save->getUnits())
	{
		if (unit->getFaction() != FACTION_HOSTILE || !unitShown(unit))
			continue;
		visible.insert(unit);
		if (_spotted.find(unit) == _spotted.end())
		{
			say(Vocab::format(Vocab::SPOTTED, { unit->getName(state->getGame()->getLanguage()), offsetText(anchor(), unit->getPosition()) }), false);
		}
	}
	_spotted.swap(visible);

	// An action we started has finished.
	if (_awaiting && canAct(state))
	{
		_awaiting = false;
		if (_soldier && !_soldier->isOut())
			say(Vocab::format(Vocab::TIME_UNITS_LEFT, { num(_soldier->getTimeUnits()) }), false);
	}
}

}

}
