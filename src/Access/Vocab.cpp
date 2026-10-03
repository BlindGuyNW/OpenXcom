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
