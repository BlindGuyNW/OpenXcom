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
#include "TerrainNames.h"
#include <map>
#include <utility>
#include "../Mod/MapData.h"
#include "../Mod/MapDataSet.h"

namespace OpenXcom
{

namespace TerrainNames
{

namespace
{
	struct Entry
	{
		const char *set;
		int index;
		const char *name;
	};

	const Entry table[] =
	{
#include "TerrainNames.inc"
	};

	typedef std::map<std::pair<std::string, int>, const char *> Lookup;

	const Lookup &lookup()
	{
		static Lookup map;
		if (map.empty())
		{
			for (const Entry &e : table)
				map[std::make_pair(std::string(e.set), e.index)] = e.name;
		}
		return map;
	}
}

std::string get(const MapData *piece)
{
	if (!piece)
		return std::string();
	MapDataSet *set = piece->getDataset();
	// Pieces don't know their own index, and sets are reloaded per battle, so look each time; sets are small.
	for (size_t i = 0; i < set->getSize(); ++i)
	{
		if (set->getObject(i) != piece)
			continue;
		Lookup::const_iterator found = lookup().find(std::make_pair(set->getName(), (int)i));
		return found == lookup().end() ? std::string() : std::string(found->second);
	}
	return std::string();
}

}

}
