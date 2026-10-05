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

namespace OpenXcom
{

class MapData;

/**
 * Names for terrain pieces, which the game never names: "apple tree", "wooden fence".
 * Keyed by terrain set (the MCD file) and index, from a table labelled by eye
 * off the vanilla sprites (TerrainNames.inc). It's vocabulary data, the one
 * part of the layer's text that lives outside Vocab.
 */
namespace TerrainNames
{
	/// The piece's name, or empty if it has none (unlabelled or invisible).
	std::string get(const MapData *piece);
}

}
