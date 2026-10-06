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
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "Speech.h"
#include "TerrainNames.h"
#include "Vocab.h"
#include "../Engine/Action.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/Camera.h"
#include "../Battlescape/Map.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/Position.h"
#include "../Battlescape/Projectile.h"
#include "../Battlescape/TileEngine.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Interface/Cursor.h"
#include "../Mod/MapData.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Armor.h"
#include "../Mod/Unit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"

namespace OpenXcom
{

namespace Battle
{

namespace
{
	/// Has a tile (2), or its west (0) or north (1) wall seen from the far side, been seen?
	/// The numbers are vanilla's; OXCE keeps the flags per tile part, the whole tile on the floor.
	bool discovered(const Tile *tile, int part)
	{
		static const TilePart parts[3] = { O_WESTWALL, O_NORTHWALL, O_FLOOR };
		return tile->isDiscovered(parts[part]);
	}

	/// The battle the state below belongs to; a new one resets it.
	SavedBattleGame *_battle = 0;
	Position _cursor;
	/// The differ's memory of the selected unit.
	BattleUnit *_selected = 0;
	/// The last player unit selected. Offsets are measured from it.
	BattleUnit *_soldier = 0;
	/// Hostiles visible last frame, so newly spotted ones are spoken once.
	std::set<BattleUnit *> _spotted;
	/// Every unit shown last frame, so we can say when one we could see goes down.
	std::set<BattleUnit *> _shown;
	/// Our units' health last frame, to say when one is hit.
	std::map<BattleUnit *, int> _health;
	/// The floor named at the cursor's last step.
	/// Was the game waiting for a target last frame?
	bool _wasTargeting = false;
	/// The selected soldier's stance last frame.
	bool _kneeled = false;
	/// The reserve settings last frame (F1 to F4, J).
	BattleActionType _reserve = BA_NONE;
	bool _kneelReserve = false;
	/// Delete was pressed; say the result once the game has handled it.
	bool _zeroPending = false;
	/// The projectile in flight last frame, so each new shot is spoken once.
	Projectile *_projectile = 0;
	/// The current shot hasn't been seen yet: it's spoken when it first comes into view, or never.
	bool _shotPending = false;
	/// The last shot spoken, so an auto shot's bursts aren't repeated.
	Position _shotFrom, _shotAt;
	Uint32 _shotTime = 0;
	const Uint32 BURST_WINDOW = 2000;
	/// Whether the last shot's result is spoken: ours, or one a sighted player watched.
	bool _reportShot = false;
	/// Every unit's state just before a hit or explosion, to say who it hurt.
	struct UnitBefore
	{
		int health, stun;
		bool shown;
	};
	std::map<BattleUnit *, UnitBefore> _before;
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

	/// Which way a hostile faces (its sprite shows it), with "toward you" when the
	/// selected soldier is inside its view cone. Empty for anyone else.
	std::string facingText(BattleUnit *unit)
	{
		if (unit->getFaction() != FACTION_HOSTILE)
			return std::string();
		bool toward = _soldier && !_soldier->isOut() && unit->checkViewSector(_soldier->getPosition());
		return Vocab::format(toward ? Vocab::FACING_YOU : Vocab::FACING, { dirName(unit->getDirection()) });
	}

	/// Sighting numbers for units that aren't ours: given when a unit is first named while
	/// in view, retired when it leaves view, and never reused, so an alien seen again gets a
	/// new number (a sighted player can't tell whether it's the same one either).
	std::map<BattleUnit *, int> _labels;
	std::map<std::string, int> _lastLabel;

	/// What a sighted player can tell a unit by. Ours by name; others by race, since every rank
	/// shares one sprite (the game shows ranks only in panic messages, mind probes and on a
	/// stunned body), plus their sighting number: "Sectoid 2".
	std::string unitLabel(BattleUnit *unit)
	{
		Language *lang = State::getGamePtr()->getLanguage();
		if (unit->getFaction() == FACTION_PLAYER || !unit->getUnitRules())
			return unit->getName(lang);
		std::string race = lang->getString(unit->getUnitRules()->getRace());
		int &n = _labels[unit];
		if (!n)
			n = ++_lastLabel[race];
		return Vocab::format(Vocab::UNIT_NUMBER, { race, num(n) });
	}

	/// A shown unit's label, plus its facing if it's hostile.
	std::string unitName(BattlescapeState *state, BattleUnit *unit)
	{
		return joinComma({ unitLabel(unit), facingText(unit) });
	}

	/// An item's name as the inventory shows it: unresearched alien items are just "Alien Artifact".
	std::string itemName(BattlescapeState *state, BattleItem *item)
	{
		if (!state->getGame()->getSavedGame()->isResearched(item->getRules()->getRequirements()))
			return state->tr("STR_ALIEN_ARTIFACT");
		return state->tr(item->getRules()->getName());
	}

	std::string itemText(BattlescapeState *state, BattleItem *item)
	{
		std::string name = itemName(state, item);
		if (item->isWeaponWithAmmo())
		{
			BattleItem *ammo = item->getAmmoForSlot(0);
			if (!ammo)
				return Vocab::format(Vocab::NO_AMMO, { name });
			return Vocab::format(Vocab::ROUNDS, { name, num(ammo->getAmmoQuantity()) });
		}
		return name;
	}

	/// The edge kinds we describe.
	enum EdgeKind { EDGE_NONE, EDGE_WALL, EDGE_DOOR, EDGE_UFO_DOOR, EDGE_UFO_DOOR_OPEN, EDGE_PASSABLE };

	/// What stands on an edge, and the piece it is (for its name).
	struct Edge
	{
		EdgeKind kind;
		const MapData *piece;
	};

	Edge wallKind(Tile *owner, TilePart part)
	{
		MapData *md = owner->getMapData(part);
		if (!md)
			return { EDGE_NONE, 0 };
		if (md->isUFODoor())
			return { owner->isUfoDoorOpen(part) ? EDGE_UFO_DOOR_OPEN : EDGE_UFO_DOOR, md };
		if (md->isDoor())
			return { EDGE_DOOR, md };
		if (owner->getTUCost(part, MT_WALK) >= 255)
			return { EDGE_WALL, md };
		// Wall pieces you can walk through (an opened door, a broken fence) are only worth a word if they have a name.
		return { EDGE_PASSABLE, md };
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
	Edge edgeAt(SavedBattleGame *save, Tile *tile, Position p, int side)
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
		Edge edge = { EDGE_NONE, 0 };
		if (owner && (discovered(owner, 2) || discovered(owner, seenFlag)))
		{
			edge = wallKind(owner, part);
			// A blocking wall wins over something passable on the same edge.
			if (edge.kind != EDGE_NONE && edge.kind != EDGE_PASSABLE)
				return edge;
		}
		if (bigWallBlocks(bigWall(tile), side))
			return { EDGE_WALL, tile->getMapData(O_OBJECT) };
		Position v;
		Pathfinding::directionToVector(side, &v);
		Tile *neighbour = save->getTile(p + v);
		if (neighbour && discovered(neighbour, 2) && bigWallBlocks(bigWall(neighbour), (side + 4) % 8))
			return { EDGE_WALL, neighbour->getMapData(O_OBJECT) };
		return edge;
	}

	/// What to call an edge: "wall", "wooden fence". Empty if it's not worth saying.
	std::string edgeLabel(const Edge &edge)
	{
		// UFO doors open and close without changing piece, so their state comes from the tile.
		if (edge.kind == EDGE_UFO_DOOR)
			return Vocab::get(Vocab::UFO_DOOR);
		if (edge.kind == EDGE_UFO_DOOR_OPEN)
			return Vocab::get(Vocab::UFO_DOOR_OPEN);
		std::string name = TerrainNames::get(edge.piece);
		if (!name.empty())
			return name;
		if (edge.kind == EDGE_DOOR)
			return Vocab::get(Vocab::DOOR);
		if (edge.kind == EDGE_WALL)
			return Vocab::get(Vocab::WALL);
		return std::string();
	}

	/// "stone wall north and east, wooden door south".
	std::string edgesText(SavedBattleGame *save, Tile *tile, Position p)
	{
		static const int sides[4] = { 0, 2, 4, 6 };
		// Labels in the order first met, each with its sides.
		std::vector<std::pair<std::string, std::vector<std::string> > > byLabel;
		for (int side : sides)
		{
			std::string label = edgeLabel(edgeAt(save, tile, p, side));
			if (label.empty())
				continue;
			size_t i = 0;
			while (i < byLabel.size() && byLabel[i].first != label)
				++i;
			if (i == byLabel.size())
				byLabel.push_back(std::make_pair(label, std::vector<std::string>()));
			byLabel[i].second.push_back(dirName(side));
		}
		std::vector<std::string> parts;
		for (const auto &l : byLabel)
			parts.push_back(Vocab::format(Vocab::EDGE, { l.first, joinAnd(l.second) }));
		return joinComma(parts);
	}

	/// The tile's own contents: object, lift, craft or exit area, smoke and fire. The floor is named last, by describeTile.
	std::vector<std::string> terrainParts(SavedBattleGame *save, Tile *tile, Position p, bool full)
	{
		std::vector<std::string> parts;
		MapData *object = tile->getMapData(O_OBJECT);
		// Big walls along an edge are named with the edges.
		bool edgeObject = object && object->getBigWall() >= Pathfinding::BIGWALLWEST;
		std::string objectName = edgeObject ? std::string() : TerrainNames::get(object);
		if (!objectName.empty())
		{
			parts.push_back(objectName);
			if (full && tile->getTUCost(O_OBJECT, MT_WALK) >= 255)
				parts.push_back(Vocab::get(Vocab::IMPASSABLE));
		}
		else if (object)
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
		if (floor && floor->getSpecialType() == START_POINT)
			parts.push_back(Vocab::get(Vocab::CRAFT_AREA));
		if (floor && floor->getSpecialType() == END_POINT)
			parts.push_back(Vocab::get(Vocab::EXIT_AREA));
		if (tile->getSmoke())
			parts.push_back(Vocab::get(Vocab::SMOKE));
		if (tile->getFire())
			parts.push_back(Vocab::get(Vocab::FIRE));
		return parts;
	}

	/// Where a fall from p would land, the way Pathfinding::calculate settles a click
	/// on open air. Stops above tiles not yet seen, so it never describes hidden ground.
	Position settleDown(SavedBattleGame *save, Position p)
	{
		for (;;)
		{
			Tile *below = save->getTile(p + Position(0, 0, -1));
			if (!below || !discovered(below, 2) || !save->getTile(p)->hasNoFloor(save))
				return p;
			p.z--;
		}
	}

	/// What a sighted player sees on a tile. Brief is for cursor steps, full for Ctrl+L.
	/// The floor comes last, after the edges, so a tile reads the same whichever way the cursor arrives
	/// and a quick next keypress cuts off the least useful part.
	std::string describeTile(BattlescapeState *state, Position p, bool full)
	{
		SavedBattleGame *save = saveOf(state);
		Tile *tile = save->getTile(p);
		if (!tile)
			return Vocab::get(Vocab::MAP_EDGE);
		if (!discovered(tile, 2))
			return Vocab::get(Vocab::UNSEEN);

		std::vector<std::string> parts;
		BattleUnit *unit = tile->getUnit();
		if (unitShown(unit))
			parts.push_back(unitName(state, unit));

		std::vector<BattleItem *> *items = tile->getInventory();
		const size_t shown = full ? items->size() : 2;
		for (size_t i = 0; i < items->size() && i < shown; ++i)
			parts.push_back(itemName(state, (*items)[i]));
		if (items->size() > shown)
			parts.push_back(Vocab::format(Vocab::MORE_ITEMS, { num((int)(items->size() - shown)) }));

		for (const std::string &s : terrainParts(save, tile, p, full))
			parts.push_back(s);
		parts.push_back(edgesText(save, tile, p));
		parts.push_back(TerrainNames::get(tile->getMapData(O_FLOOR)));

		// Open air: say what you'd land on, so the cursor can stay on the level you picked.
		bool openAir = p.z > 0 && tile->hasNoFloor(save);
		// Unless an impassable object fills the tile (a UFO's hull): it's solid, not a hole.
		MapData *object = tile->getMapData(O_OBJECT);
		bool solid = object && object->getBigWall() < Pathfinding::BIGWALLWEST && tile->getTUCost(O_OBJECT, MT_WALK) >= 255;
		if (openAir && solid)
		{
			parts.erase(std::remove(parts.begin(), parts.end(), Vocab::get(Vocab::IMPASSABLE)), parts.end());
			if (std::find(parts.begin(), parts.end(), Vocab::get(Vocab::OBSTACLE)) == parts.end())
				parts.push_back(Vocab::get(Vocab::SOLID));
		}
		else if (openAir)
		{
			Position ground = settleDown(save, p);
			if (ground.z < p.z)
				parts.push_back(Vocab::format(Vocab::NO_FLOOR_BELOW, { num(p.z - ground.z), describeTile(state, ground, full) }));
			else
				parts.push_back(Vocab::get(Vocab::NO_FLOOR));
		}

		std::string text = joinComma(parts);
		if (text.empty())
			text = Vocab::get(Vocab::EMPTY_TILE);
		// Too dark for our soldiers to spot a unit here from more than 9 tiles away; aliens don't care.
		// Open air is skipped since the ground below already says it.
		if (!openAir && tile->getShade() > save->getTileEngine()->getMaxDarknessToSeeUnits())
			text = joinComma({ text, Vocab::get(Vocab::DARK) });
		return text;
	}

	/// The reserve setting as the game's tooltip for its button says it.
	std::string reserveText(BattlescapeState *state, BattleActionType reserve)
	{
		switch (reserve)
		{
		case BA_SNAPSHOT: return state->tr("STR_RESERVE_TIME_UNITS_FOR_SNAP_SHOT");
		case BA_AIMEDSHOT: return state->tr("STR_RESERVE_TIME_UNITS_FOR_AIMED_SHOT");
		case BA_AUTOSHOT: return state->tr("STR_RESERVE_TIME_UNITS_FOR_AUTO_SHOT");
		default: return state->tr("STR_DONT_RESERVE_TIME_UNITS");
		}
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
		SavedBattleGame *save = state->getBattleGame()->getSave();
		if (save->getTUReserved() != BA_NONE)
			parts.push_back(reserveText(state, save->getTUReserved()));
		if (save->getKneelReserved())
			parts.push_back(state->tr("STR_RESERVE_TIME_UNITS_FOR_KNEEL"));
		parts.push_back(Vocab::format(Vocab::FACING, { dirName(unit->getDirection()) }));
		BattleItem *left = unit->getLeftHandWeapon(), *right = unit->getRightHandWeapon();
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

	/// Why a soldier can't see a unit, checked in TileEngine::visible's order: too far, too dark,
	/// then outside the 90 degree view cone, then a sight line from the eyes to the unit's middle,
	/// naming what it hits. A sighted player sees the map, so the obstacle is named only if its
	/// tile has been seen. Empty when there's no clear reason (the game also tries other lines).
	std::string outOfViewReason(SavedBattleGame *save, BattleUnit *viewer, BattleUnit *unit)
	{
		TileEngine *engine = save->getTileEngine();
		Position from = viewer->getPosition(), to = unit->getPosition();
		int dist = Position::distance2d(from, to);
		if (dist > engine->getMaxViewDistance())
			return Vocab::get(Vocab::VIEW_TOO_FAR);
		Tile *target = save->getTile(to);
		if (viewer->getFaction() == FACTION_PLAYER && dist > 9 && target->getShade() > engine->getMaxDarknessToSeeUnits())
			return Vocab::get(Vocab::VIEW_TOO_DARK);
		if (!viewer->checkViewSector(to))
			return Vocab::get(Vocab::VIEW_FACING_AWAY);

		Position origin = engine->getSightOriginVoxel(viewer);
		Position aim(to.x * 16 + 8, to.y * 16 + 8, to.z * 24 - target->getTerrainLevel() + unit->getFloatHeight() + unit->getHeight() / 2);
		std::vector<Position> line;
		int hit = engine->calculateLineVoxel(origin, aim, true, &line, viewer);
		if (line.empty())
			return std::string();
		Position voxel = line.back();
		Tile *tile = save->getTile(Position(voxel.x / 16, voxel.y / 16, voxel.z / 24));
		if (hit == V_UNIT)
		{
			BattleUnit *blocker = tile ? tile->getUnit() : 0;
			if (!blocker && tile && voxel.z % 24 < 4)
			{
				Tile *below = save->getTile(tile->getPosition() + Position(0, 0, -1));
				blocker = below ? below->getUnit() : 0;
			}
			if (blocker && blocker != unit)
				return unitShown(blocker) ? Vocab::format(Vocab::VIEW_BLOCKED_BY, { unitLabel(blocker) }) : Vocab::get(Vocab::VIEW_BLOCKED);
			// The line reaches the unit, so it's the smoke along the way.
			for (const Position &p : line)
			{
				Tile *t = save->getTile(Position(p.x / 16, p.y / 16, p.z / 24));
				if (t && t->getSmoke() > 0 && t->getFire() == 0)
					return Vocab::get(Vocab::VIEW_SMOKE);
			}
			return std::string();
		}
		if (hit < V_FLOOR || hit > V_OBJECT || !tile)
			return std::string();
		// A west or north wall can be seen from either side; isDiscovered(0) and (1) are the far side.
		bool seenIt = discovered(tile, 2) || (hit == V_WESTWALL && discovered(tile, 0)) || (hit == V_NORTHWALL && discovered(tile, 1));
		std::string name = seenIt ? TerrainNames::get(tile->getMapData((TilePart)hit)) : std::string();
		return name.empty() ? Vocab::get(Vocab::VIEW_BLOCKED) : Vocab::format(Vocab::VIEW_BLOCKED_BY, { name });
	}

	/// While aiming: whether the soldier can see the unit on a tile. That's what the HUD's
	/// numbered enemy buttons show, so a sighted player knows it too.
	std::string targetText(BattlescapeState *state, Position p)
	{
		BattleAction *action = state->getBattleGame()->getCurrentAction();
		Tile *tile = saveOf(state)->getTile(p);
		if (!action->targeting || !action->actor || !tile || !discovered(tile, 2))
			return std::string();
		BattleUnit *unit = tile->getUnit();
		if (!unitShown(unit) || unit == action->actor)
			return std::string();
		if (unit->getFaction() == FACTION_PLAYER)
			return Vocab::get(Vocab::FRIENDLY);
		std::vector<BattleUnit *> *seen = action->actor->getVisibleUnits();
		if (std::find(seen->begin(), seen->end(), unit) != seen->end())
			return Vocab::get(Vocab::IN_VIEW);
		std::string why = outOfViewReason(saveOf(state), action->actor, unit);
		return why.empty() ? Vocab::get(Vocab::OUT_OF_VIEW) : joinComma({ Vocab::get(Vocab::OUT_OF_VIEW), why });
	}

	void moveCursor(BattlescapeState *state, Position to, bool levelChange, bool interrupt = true)
	{
		if (!saveOf(state)->getTile(to))
		{
			say(Vocab::get(Vocab::MAP_EDGE), true);
			return;
		}
		_cursor = to;
		showCursor(state);
		std::string text = describeTile(state, _cursor, false);
		if (levelChange)
			text = Vocab::format(Vocab::LEVEL, { num(_cursor.z + 1) }) + ", " + text;
		text = joinComma({ text, targetText(state, _cursor) });
		say(text, interrupt);
	}

	/// Total TU cost of the path the game just calculated, walked the way refreshPath does.
	/// Adds the energy it spends to *energy when given.
	int pathCost(SavedBattleGame *save, BattleUnit *unit, int *energy = nullptr)
	{
		Pathfinding *pf = save->getPathfinding();
		const std::vector<int> &path = pf->getPath();
		Position pos = unit->getPosition();
		int total = unit->isKneeled() ? unit->getKneelUpCost() : 0;
		for (std::vector<int>::const_reverse_iterator i = path.rbegin(); i != path.rend(); ++i)
		{
			PathfindingStep step = pf->getTUCost(pos, *i, unit, 0, BAM_NORMAL);
			total += step.cost.time;
			if (energy)
				*energy += step.cost.energy;
			pos = step.pos;
		}
		return total;
	}

	/// Whether a walk costing these TUs and energy turns the preview markers yellow: the test
	/// refreshPath makes, including its stand-in auto-shot reserve when none is set.
	bool eatsIntoReserve(BattlescapeGame *bg, BattleUnit *unit, int tu, int energy)
	{
		bool switchBack = bg->getReservedAction() == BA_NONE;
		if (switchBack)
			bg->setTUReserved(BA_AUTOSHOT);
		bool fits = bg->checkReservedTU(unit, tu, energy, true);
		if (switchBack)
			bg->setTUReserved(BA_NONE);
		return !fits;
	}

	/// "9 steps, the long way round": the path's length, and a detour flag when it takes three or
	/// more steps beyond the fewest possible (diagonals count as one step).
	std::string routeText(Position from, Position to, size_t steps)
	{
		std::string text = steps == 1 ? Vocab::get(Vocab::ONE_STEP) : Vocab::format(Vocab::PATH_STEPS, { num((int)steps) });
		int fewest = std::max({ std::abs(to.x - from.x), std::abs(to.y - from.y), std::abs(to.z - from.z) });
		if ((int)steps >= fewest + 3)
			text = joinComma({ text, Vocab::get(Vocab::DETOUR) });
		return text;
	}

	/// Enter: a left click on the cursor tile.
	/// Blaster Launcher: a click only drops a waypoint, and the HUD's launch button fires.
	/// Enter on the last waypoint stands in for that button.
	void launchStep(BattlescapeState *state, Position target)
	{
		BattlescapeGame *bg = state->getBattleGame();
		BattleAction *action = bg->getCurrentAction();
		if (!action->waypoints.empty() && action->waypoints.back() == target)
		{
			say(Vocab::get(Vocab::LAUNCHED), true);
			bg->launchAction();
			_awaiting = bg->isBusy();
			return;
		}
		size_t before = action->waypoints.size();
		bg->primaryAction(target);
		size_t count = action->waypoints.size();
		if (count == before)
		{
			say(Vocab::get(Vocab::WAYPOINTS_FULL), true);
			return;
		}
		// Same lookup as BattlescapeGame::primaryAction; -1 is unlimited.
		int max = action->weapon->getCurrentWaypoints();
		if (max > 0)
			say(Vocab::format(Vocab::WAYPOINT_SET_OF, { num((int)count), num(max) }), true);
		else
			say(Vocab::format(Vocab::WAYPOINT_SET, { num((int)count) }), true);
	}

	/// Cancels like a right click and says what went: a launcher waypoint, else the aim or preview.
	bool cancelAction(BattlescapeGame *bg)
	{
		BattleAction *action = bg->getCurrentAction();
		bool launch = action->targeting && action->type == BA_LAUNCH;
		size_t before = action->waypoints.size();
		if (!bg->cancelCurrentAction())
			return false;
		size_t after = action->waypoints.size();
		if (launch && after < before)
			say(after ? Vocab::format(Vocab::WAYPOINT_REMOVED, { num((int)after) }) : Vocab::get(Vocab::WAYPOINTS_CLEARED), true);
		else
			say(Vocab::get(Vocab::CANCELLED), true);
		return true;
	}

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
		Position target = _cursor;
		// Aiming at open air: aim at what's below instead, as a sighted player
		// would by looking at that level. Throws at a floorless tile fail outright.
		Tile *tile = save->getTile(target);
		if (targeting && target.z > 0 && !tile->getUnit() && tile->hasNoFloor(save))
			target = settleDown(save, target);
		if (targeting && bg->getCurrentAction()->type == BA_LAUNCH)
		{
			launchStep(state, target);
			return;
		}
		bg->primaryAction(target);
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
		int energy = 0;
		int cost = pathCost(save, before, &energy), tus = before->getTimeUnits();
		// The route a sighted player sees drawn: its length, whether it bends well away from the
		// straight line, and the yellow markers for a walk into reserved TUs.
		std::string route = routeText(before->getPosition(), target, pf->getPath().size());
		if (cost <= tus && eatsIntoReserve(bg, before, cost, energy))
			route = joinComma({ route, Vocab::get(Vocab::INTO_RESERVE) });
		if (cost <= tus)
			say(Vocab::format(Vocab::PATH_COST, { num(cost), num(tus - cost), route }), true);
		else
			say(Vocab::format(Vocab::PATH_TOO_FAR, { num(cost), num(tus), route }), true);
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
		if (cancelAction(bg))
			return;
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
		// OXCE's next-soldier handler reads the action (a right click goes back), so pass a blank left click.
		SDL_Event ev = {};
		ev.type = SDL_MOUSEBUTTONUP;
		ev.button.button = SDL_BUTTON_LEFT;
		Action action(&ev, 1.0, 1.0, 0, 0);
		if (back)
			state->btnPrevSoldierClick(&action);
		else
			state->btnNextSoldierClick(&action);
		// Only one soldier left: the differ won't fire, so answer the keypress here.
		BattleUnit *after = saveOf(state)->getSelectedUnit();
		if (after && after == before && after->getFaction() == FACTION_PLAYER)
			say(unitSummary(state, after), true);
	}

	// The scanner: points of interest bucketed by category, rebuilt from live state on each key.

	enum ScanCategory { SCAN_SOLDIERS, SCAN_ENEMIES, SCAN_CIVILIANS, SCAN_ITEMS, SCAN_DOORS, SCAN_EXITS, SCAN_COUNT };

	/// One scanner entry. Units are identified by pointer since they move; everything else by tile and tag.
	struct ScanEntry
	{
		std::string name;
		Position pos;
		BattleUnit *unit;
		int tag;
		bool same(const ScanEntry &o) const { return unit ? unit == o.unit : (!o.unit && pos == o.pos && tag == o.tag); }
	};

	int _scanCategory = SCAN_SOLDIERS;
	ScanEntry _scanCurrent = { std::string(), Position(), 0, -1 };

	Vocab::Id categoryName(int category)
	{
		static const Vocab::Id names[SCAN_COUNT] = { Vocab::SCAN_SOLDIERS, Vocab::SCAN_ENEMIES, Vocab::SCAN_CIVILIANS, Vocab::SCAN_ITEMS, Vocab::SCAN_DOORS, Vocab::SCAN_EXITS };
		return names[category];
	}

	/// Doors on a tile's own west and north edges, with the same discovery rules as the tile description.
	void addDoors(std::vector<ScanEntry> &out, Tile *tile)
	{
		static const TilePart parts[2] = { O_WESTWALL, O_NORTHWALL };
		for (int i = 0; i < 2; ++i)
		{
			if (!discovered(tile, 2) && !discovered(tile, i))
				continue;
			Edge edge = wallKind(tile, parts[i]);
			if (edge.kind != EDGE_DOOR && edge.kind != EDGE_UFO_DOOR && edge.kind != EDGE_UFO_DOOR_OPEN)
				continue;
			out.push_back({ Vocab::format(Vocab::EDGE, { edgeLabel(edge), dirName(i == 0 ? 6 : 0) }), tile->getPosition(), 0, i });
		}
	}

	std::vector<ScanEntry> scan(BattlescapeState *state, int category)
	{
		SavedBattleGame *save = saveOf(state);
		std::vector<ScanEntry> out;
		if (category <= SCAN_CIVILIANS)
		{
			static const UnitFaction factions[3] = { FACTION_PLAYER, FACTION_HOSTILE, FACTION_NEUTRAL };
			for (BattleUnit *unit : *save->getUnits())
			{
				if (unit->getFaction() == factions[category] && unitShown(unit))
					out.push_back({ unitName(state, unit), unit->getPosition(), unit, 0 });
			}
		}
		else
		{
			for (int i = 0; i < save->getMapSizeXYZ(); ++i)
			{
				Tile *tile = save->getTile(i);
				if (category == SCAN_DOORS)
				{
					addDoors(out, tile);
					continue;
				}
				if (!discovered(tile, 2))
					continue;
				if (category == SCAN_ITEMS && !tile->getInventory()->empty())
				{
					std::vector<BattleItem *> *items = tile->getInventory();
					std::string name = itemName(state, items->front());
					if (items->size() > 1)
						name = Vocab::format(Vocab::AND_LIST, { name, Vocab::format(Vocab::MORE_ITEMS, { num((int)items->size() - 1) }) });
					out.push_back({ name, tile->getPosition(), 0, 0 });
				}
				else if (category == SCAN_EXITS)
				{
					MapData *floor = tile->getMapData(O_FLOOR);
					// Entrance tiles (the craft) are where soldiers must stand to escape an abort;
					// exit tiles only lead to the next stage of a multi-stage mission.
					if (floor && floor->getSpecialType() == START_POINT)
						out.push_back({ Vocab::get(Vocab::CRAFT_AREA), tile->getPosition(), 0, 0 });
					else if (floor && floor->getSpecialType() == END_POINT)
						out.push_back({ Vocab::get(Vocab::EXIT_AREA), tile->getPosition(), 0, 0 });
				}
			}
		}
		// Nearest first; a level counts as a few tiles, since climbing costs more than walking.
		Position from = anchor();
		std::stable_sort(out.begin(), out.end(), [&](const ScanEntry &a, const ScanEntry &b)
		{
			Position da = a.pos - from, db = b.pos - from;
			return da.x * da.x + da.y * da.y + 9 * da.z * da.z < db.x * db.x + db.y * db.y + 9 * db.z * db.z;
		});
		return out;
	}

	void sayEntry(const std::vector<ScanEntry> &list, size_t index)
	{
		const ScanEntry &e = list[index];
		say(joinComma({ e.name, offsetText(anchor(), e.pos), Vocab::format(Vocab::POSITION, { num((int)index + 1), num((int)list.size()) }) }), true);
	}

	/// Next or previous entry in the current category, keeping your place by identity as distances change.
	void scanStep(BattlescapeState *state, int step)
	{
		std::vector<ScanEntry> list = scan(state, _scanCategory);
		if (list.empty())
		{
			_scanCurrent.tag = -1;
			_scanCurrent.unit = 0;
			say(Vocab::format(Vocab::SCAN_NONE, { Vocab::get(categoryName(_scanCategory)) }), true);
			return;
		}
		int index = -1;
		for (size_t i = 0; i < list.size(); ++i)
		{
			if (list[i].same(_scanCurrent))
			{
				index = (int)i;
				break;
			}
		}
		int count = (int)list.size();
		index = index < 0 ? (step > 0 ? 0 : count - 1) : ((index + step) % count + count) % count;
		_scanCurrent = list[index];
		sayEntry(list, index);
	}

	/// Next or previous category; says its name and count, then its nearest entry.
	void scanCategory(BattlescapeState *state, int step)
	{
		_scanCategory = ((_scanCategory + step) % SCAN_COUNT + SCAN_COUNT) % SCAN_COUNT;
		std::vector<ScanEntry> list = scan(state, _scanCategory);
		const std::string &name = Vocab::get(categoryName(_scanCategory));
		if (list.empty())
		{
			_scanCurrent.tag = -1;
			_scanCurrent.unit = 0;
			say(Vocab::format(Vocab::SCAN_NONE, { name }), true);
			return;
		}
		_scanCurrent = list[0];
		say(Vocab::format(Vocab::SCAN_CATEGORY, { name, num((int)list.size()) }), true);
		const ScanEntry &e = list[0];
		say(joinComma({ e.name, offsetText(anchor(), e.pos) }), false);
	}

	/// Which of a unit's tiles to put the cursor on. A large unit (2 by 2) is anchored on its north-west
	/// tile, which may be the one behind cover: while aiming, prefer a tile the shooter has a line of
	/// fire to (the game's own test, TileEngine::canTargetUnit), then the one nearest the selected soldier.
	Position unitTile(BattlescapeState *state, BattleUnit *unit)
	{
		Position anchorPos = unit->getPosition();
		int size = unit->getArmor()->getSize();
		if (size <= 1)
			return anchorPos;
		SavedBattleGame *save = saveOf(state);
		BattleAction *action = state->getBattleGame()->getCurrentAction();
		BattleUnit *shooter = action->targeting ? action->actor : 0;
		Position origin;
		if (shooter)
			origin = save->getTileEngine()->getOriginVoxel(*action, save->getTile(shooter->getPosition()));
		Position from = anchor();
		Position best = anchorPos;
		int bestScore = INT_MAX;
		for (int x = 0; x < size; ++x)
		{
			for (int y = 0; y < size; ++y)
			{
				Position p = anchorPos + Position(x, y, 0);
				Tile *tile = save->getTile(p);
				if (!tile)
					continue;
				int dx = p.x - from.x, dy = p.y - from.y;
				int score = dx * dx + dy * dy;
				Position scanVoxel;
				if (shooter && !save->getTileEngine()->canTargetUnit(&origin, tile, &scanVoxel, shooter, false))
					score += 1000000;
				if (score < bestScore)
				{
					bestScore = score;
					best = p;
				}
			}
		}
		return best;
	}

	/// Moves the cursor onto the current entry, if it's still there.
	void scanJump(BattlescapeState *state)
	{
		for (const ScanEntry &e : scan(state, _scanCategory))
		{
			if (e.same(_scanCurrent))
			{
				Position p = e.unit ? unitTile(state, e.unit) : e.pos;
				moveCursor(state, p, p.z != _cursor.z);
				return;
			}
		}
		say(Vocab::get(Vocab::SCAN_GONE), true);
	}

	const char *actionNameId(BattleActionType type)
	{
		switch (type)
		{
		case BA_AIMEDSHOT: return "STR_AIMED_SHOT";
		case BA_SNAPSHOT: return "STR_SNAP_SHOT";
		case BA_AUTOSHOT: return "STR_AUTO_SHOT";
		case BA_THROW: return "STR_THROW";
		case BA_LAUNCH: return "STR_LAUNCH_MISSILE";
		case BA_MINDCONTROL: return "STR_MIND_CONTROL";
		case BA_PANIC: return "STR_PANIC_UNIT";
		case BA_USE: return "STR_USE_MIND_PROBE";
		default: return 0;
		}
	}

	void startAiming(BattlescapeState *state)
	{
		BattleAction *action = state->getBattleGame()->getCurrentAction();
		std::vector<std::string> parts;
		const char *id = actionNameId(action->type);
		if (id)
			parts.push_back(state->tr(id));
		if (action->actor && action->weapon)
		{
			int acc = -1;
			BattleActionType t = action->type;
			if (t == BA_THROW || t == BA_AIMEDSHOT || t == BA_SNAPSHOT || t == BA_AUTOSHOT || t == BA_LAUNCH || t == BA_HIT)
				acc = BattleUnit::getFiringAccuracy(BattleActionAttack::GetBeforeShoot(t, action->actor, action->weapon), State::getGamePtr()->getMod());
			if (acc >= 0)
				parts.push_back(Vocab::format(Vocab::ACCURACY, { num(acc) }));
		}
		say(Vocab::format(Vocab::AIMING, { joinComma(parts) }), true);

		// Already on a visible enemy? Leave it there.
		Tile *tile = saveOf(state)->getTile(_cursor);
		BattleUnit *there = tile ? tile->getUnit() : 0;
		if (unitShown(there) && there->getFaction() == FACTION_HOSTILE)
		{
			// A large one may be better hit through another of its tiles.
			Position p = unitTile(state, there);
			if (p != _cursor)
				moveCursor(state, p, false, false);
			else
				say(joinComma({ describeTile(state, _cursor, false), targetText(state, _cursor) }), false);
			return;
		}
		// Nearest enemy the soldier can see, else the nearest one shown at all.
		std::vector<ScanEntry> enemies = scan(state, SCAN_ENEMIES);
		if (enemies.empty())
			return;
		const ScanEntry *pick = &enemies[0];
		std::vector<BattleUnit *> *seen = action->actor ? action->actor->getVisibleUnits() : 0;
		for (const ScanEntry &e : enemies)
		{
			if (seen && std::find(seen->begin(), seen->end(), e.unit) != seen->end())
			{
				pick = &e;
				break;
			}
		}
		Position p = unitTile(state, pick->unit);
		moveCursor(state, p, p.z != _cursor.z, false);
	}

	/// Rough compass direction from one tile to another, plus above/below: "northeast, above".
	std::string bearingText(Position from, Position to)
	{
		std::vector<std::string> parts;
		int dx = to.x - from.x, dy = to.y - from.y, dz = to.z - from.z;
		if (dx || dy)
		{
			// Game directions: 0 is north (-y), clockwise in eighths.
			double angle = std::atan2((double)dx, (double)-dy);
			parts.push_back(dirName((int)std::lround(angle / (3.14159265358979 / 4))));
		}
		if (dz)
			parts.push_back(Vocab::get(dz > 0 ? Vocab::ABOVE : Vocab::BELOW));
		return joinComma(parts);
	}

	/// A shot or throw the player didn't order (an alien's, or our reaction fire) has come into view:
	/// say who fired if we can see them; else, from where it was first seen (seenAt, a tile),
	/// the direction it came in from if it's aimed at someone shown, or where it was seen and
	/// which way it was heading. Never the hidden shooter's position.
	void narrateShot(BattlescapeState *state, Projectile *projectile, Position seenAt)
	{
		SavedBattleGame *save = saveOf(state);
		// The origin is where the trajectory starts, often the tile next to the shooter, so it's only good for a bearing.
		Position from = projectile->getOrigin(), at = projectile->getTarget();
		BattleUnit *shooter = projectile->getActor();
		// Our shots on our turn are the player's own keypresses. On the aliens' turn they're reaction fire, worth saying.
		if (shooter && shooter->getFaction() == FACTION_PLAYER && save->getSide() == FACTION_PLAYER)
			return;
		Uint32 now = SDL_GetTicks();
		if (from == _shotFrom && at == _shotAt && now - _shotTime < BURST_WINDOW)
		{
			_shotTime = now;
			return;
		}
		_shotFrom = from;
		_shotAt = at;
		_shotTime = now;

		Tile *atTile = save->getTile(at);
		BattleUnit *target = atTile ? atTile->getUnit() : 0;
		if (!unitShown(target))
			target = 0;
		if (unitShown(shooter))
		{
			std::string name = unitLabel(shooter);
			say(target ? Vocab::format(Vocab::UNIT_FIRES_AT, { name, unitLabel(target) }) : Vocab::format(Vocab::UNIT_FIRES, { name }), false);
			return;
		}
		if (target)
		{
			// Where it came into view; if that's the target's own tile, the way it travelled.
			std::string bearing = bearingText(at, seenAt);
			if (bearing.empty())
				bearing = bearingText(at, from);
			say(bearing.empty() ? Vocab::format(Vocab::UNIT_FIRES_AT, { Vocab::get(Vocab::SHOT_UNSEEN_SHOOTER), unitLabel(target) })
				: Vocab::format(Vocab::FIRE_FROM_AT, { bearing, unitLabel(target) }), false);
			return;
		}
		std::string heading = bearingText(Position(from.x, from.y, 0), Position(at.x, at.y, 0));
		std::string text = Vocab::format(Vocab::SHOT_SEEN, { offsetText(anchor(), seenAt) });
		if (!heading.empty())
			text += ", " + Vocab::format(Vocab::SHOT_HEADING, { heading });
		say(text, false);
	}

	void reset(SavedBattleGame *save)
	{
		_battle = save;
		_selected = 0;
		_soldier = 0;
		_spotted.clear();
		_shown.clear();
		_health.clear();
		_wasTargeting = false;
		_projectile = 0;
		_shotPending = false;
		_shotTime = 0;
		_reportShot = false;
		_before.clear();
		_labels.clear();
		_lastLabel.clear();
		_scanCategory = SCAN_SOLDIERS;
		_scanCurrent.unit = 0;
		_scanCurrent.tag = -1;
		_awaiting = false;
		_endTurnArmed = 0;
		_reserve = save->getTUReserved();
		_kneelReserve = save->getKneelReserved();
		_zeroPending = false;
		_cursor =Position(save->getMapSizeX() / 2, save->getMapSizeY() / 2, 0);
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
			cancelAction(state->getBattleGame());
			return true;
		}
		return false;
	case SDLK_TAB:
		cycleSoldier(state, shift);
		return true;
	case SDLK_PERIOD:
	case SDLK_COMMA:
	{
		int step = key == SDLK_PERIOD ? 1 : -1;
		if (shift)
			scanCategory(state, step);
		else
			scanStep(state, step);
		return true;
	}
	case SDLK_SLASH:
		scanJump(state);
		return true;
	case SDLK_LSHIFT:
		// The game binds Left Shift alone to previous soldier, which fires on every Shift+key.
		return true;
	case SDLK_DELETE:
		// The game's expend-all-TUs key: let it through and say the result next frame.
		_zeroPending = true;
		return false;
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
			_kneeled = selected->isKneeled();
			_cursor = selected->getPosition();
			showCursor(state);
			say(unitSummary(state, selected), true);
		}
	}

	// Kneeling or standing up, almost always a reply to K.
	if (_soldier && !_soldier->isOut() && _soldier->isKneeled() != _kneeled)
	{
		_kneeled = _soldier->isKneeled();
		say(Vocab::get(_kneeled ? Vocab::KNEELING : Vocab::STANDING), true);
	}

	// Reserve changes, replies to F1 to F4 and J. The texts are the buttons' tooltips.
	if (save->getTUReserved() != _reserve)
	{
		_reserve = save->getTUReserved();
		say(reserveText(state, _reserve), true);
	}
	if (save->getKneelReserved() != _kneelReserve)
	{
		_kneelReserve = save->getKneelReserved();
		say(joinComma({ state->tr("STR_RESERVE_TIME_UNITS_FOR_KNEEL"), Vocab::get(_kneelReserve ? Vocab::ON : Vocab::OFF) }), true);
	}
	if (_zeroPending)
	{
		_zeroPending = false;
		if (_soldier && !_soldier->isOut() && _soldier->getTimeUnits() == 0)
			say(Vocab::get(Vocab::TUS_SPENT), true);
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
			say(joinComma({ Vocab::format(Vocab::SPOTTED, { unitLabel(unit), offsetText(anchor(), unit->getPosition()) }), facingText(unit) }), false);
		}
	}
	_spotted.swap(visible);

	// Units we could see going down, and our own being hit.
	std::set<BattleUnit *> shown;
	for (BattleUnit *unit : *save->getUnits())
	{
		if (unitShown(unit))
			shown.insert(unit);
		else if (_shown.count(unit) && unit->isOut())
			say(Vocab::format(unit->getStatus() == STATUS_DEAD ? Vocab::UNIT_KILLED : Vocab::UNIT_UNCONSCIOUS, { unitLabel(unit) }), false);
		if (unit->getFaction() == FACTION_PLAYER && !unit->isOut())
		{
			std::map<BattleUnit *, int>::iterator h = _health.find(unit);
			if (h != _health.end() && unit->getHealth() < h->second && unit->getHealth() > 0)
				// Hits are spoken at impact, so a drop here is mostly fatal wounds bleeding at the turn's start.
				say(Vocab::format(unit->getFatalWounds() > 0 ? Vocab::UNIT_BLEEDING : Vocab::UNIT_WOUNDED, { unitLabel(unit), num(unit->getHealth()) }), false);
			_health[unit] = unit->getHealth();
		}
	}
	_shown.swap(shown);

	// Retire the numbers of units that left view; ones that went down were just spoken.
	for (std::map<BattleUnit *, int>::iterator l = _labels.begin(); l != _labels.end();)
	{
		if (unitShown(l->first))
		{
			++l;
			continue;
		}
		if (!l->first->isOut())
			say(Vocab::format(Vocab::OUT_OF_SIGHT, { unitLabel(l->first) }), false);
		l = _labels.erase(l);
	}

	// Shots, spoken as they leave so they come before any hit or death.
	Projectile *projectile = state->getMap()->getProjectile();
	if (projectile && projectile != _projectile)
	{
		BattleUnit *shooter = projectile->getActor();
		Tile *targetTile = save->getTile(projectile->getTarget());
		_reportShot = (shooter && shooter->getFaction() == FACTION_PLAYER) || unitShown(shooter) || (targetTile && unitShown(targetTile->getUnit()));
		_shotPending = true;
	}
	if (!projectile)
		_shotPending = false;
	if (_shotPending)
	{
		// What the Map draws (Map.cpp, _projectileInFOV): on our turn every shot; on the aliens'
		// turn a shot only while it's on a tile we can see, else the hidden movement screen.
		// A visible shooter counts from the start: the screen shows it firing.
		Position now = projectile->getPosition().toTile();
		Tile *tile = save->getTile(now);
		if (save->getSide() == FACTION_PLAYER || unitShown(projectile->getActor()) || (tile && tile->getVisible()))
		{
			_shotPending = false;
			narrateShot(state, projectile, now);
		}
	}
	_projectile = projectile;

	// Aiming started: say what and how well, then put the cursor on the nearest enemy.
	bool targeting = state->getBattleGame()->getCurrentAction()->targeting && save->getSide() == FACTION_PLAYER;
	if (targeting && !_wasTargeting)
		startAiming(state);
	_wasTargeting = targeting;

	// An action we started has finished.
	if (_awaiting && canAct(state))
	{
		_awaiting = false;
		if (_soldier && !_soldier->isOut())
			say(Vocab::format(Vocab::TIME_UNITS_LEFT, { num(_soldier->getTimeUnits()) }), false);
	}
}

void beginImpact(SavedBattleGame *save)
{
	_before.clear();
	if (save != _battle)
		return;
	for (BattleUnit *unit : *save->getUnits())
		_before[unit] = { unit->getHealth(), unit->getStunlevel(), unitShown(unit) };
}

namespace
{
	/// One line for a shown unit an impact reached, or nothing if it went down
	/// (the killed/unconscious narration follows). Our units' wounds come with their health.
	std::string hitText(BattleUnit *unit, const UnitBefore &before)
	{
		if (unit->getHealth() == 0 || unit->getStunlevel() >= unit->getHealth())
			return std::string();
		std::string name = unitLabel(unit);
		if (unit->getFaction() == FACTION_PLAYER && unit->getHealth() < before.health)
		{
			_health[unit] = unit->getHealth();
			return Vocab::format(Vocab::UNIT_WOUNDED, { name, num(unit->getHealth()) });
		}
		return Vocab::format(Vocab::UNIT_HIT, { name });
	}
}

void endImpact(SavedBattleGame *save, BattleUnit *attacker, bool areaEffect)
{
	if (save != _battle || _before.empty())
		return;
	bool report = _reportShot || (attacker && attacker->getFaction() == FACTION_PLAYER);
	if (areaEffect)
	{
		// Everyone shown that the blast hurt; a sighted player sees them flinch or fall.
		bool anyone = false;
		for (const auto &b : _before)
		{
			BattleUnit *unit = b.first;
			if (!b.second.shown || (unit->getHealth() >= b.second.health && unit->getStunlevel() <= b.second.stun))
				continue;
			anyone = true;
			std::string text = hitText(unit, b.second);
			if (!text.empty())
				say(text, false);
		}
		if (!anyone && report)
			say(Vocab::get(Vocab::BLAST_NO_ONE), false);
		_reportShot = false;
	}
	else
	{
		TileEngine *engine = save->getTileEngine();
		BattleUnit *unit = engine->getLastHitUnit();
		std::map<BattleUnit *, UnitBefore>::const_iterator b = unit ? _before.find(unit) : _before.end();
		if (b != _before.end() && b->second.shown)
		{
			std::string text = hitText(unit, b->second);
			if (!text.empty())
				say(text, false);
		}
		else if (report)
		{
			// A unit we can't see counts as a miss: all a sighted player sees is the impact.
			std::string piece = unit ? std::string() : TerrainNames::get(engine->getLastHitPart());
			say(piece.empty() ? Vocab::get(Vocab::MISSED) : Vocab::format(Vocab::MISSED_INTO, { piece }), false);
		}
	}
	_before.clear();
}

void shotOffMap(SavedBattleGame *save)
{
	if (save == _battle && _reportShot)
		say(Vocab::get(Vocab::MISSED), false);
}

}

}
