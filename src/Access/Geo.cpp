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
#include "Geo.h"
#include <algorithm>
#include <vector>
#include "../fmath.h"
#include "Dogfight.h"
#include "Speech.h"
#include "Vocab.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/State.h"
#include "../Engine/Unicode.h"
#include "../Geoscape/GeoscapeState.h"
#include "../Geoscape/Globe.h"
#include "../Geoscape/MultipleTargetsState.h"
#include "../Interface/TextButton.h"
#include "../Mod/RuleCountry.h"
#include "../Mod/RuleRegion.h"
#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"

namespace OpenXcom
{

namespace Geo
{

namespace
{
	/// Nautical miles per radian of great circle (60 per degree).
	const double NM_PER_RADIAN = 60.0 * 180.0 / M_PI;

	enum ScanCategory { SCAN_UFOS, SCAN_SITES, SCAN_ALIEN_BASES, SCAN_CRAFT, SCAN_BASES, SCAN_WAYPOINTS, SCAN_COUNT };

	GeoscapeState *_geo = 0;
	/// The speed differ's memory: the pressed speed button's text.
	std::string _speed;
	int _scanCategory = SCAN_UFOS;
	/// The current scanner entry. Compared by address only, never followed unless it's still in a live list.
	const Target *_scanCurrent = 0;

	Game *game()
	{
		return State::getGamePtr();
	}

	SavedGame *save()
	{
		return game()->getSavedGame();
	}

	std::string tr(const std::string &id)
	{
		return game()->getLanguage()->getString(id);
	}

	void say(const std::string &text, bool interrupt)
	{
		Speech::say(text, interrupt);
	}

	std::string joinComma(const std::vector<std::string> &items)
	{
		std::string out;
		for (const std::string &s : items)
		{
			if (s.empty())
				continue;
			if (!out.empty())
				out += ", ";
			out += s;
		}
		return out;
	}

	/// The base distances are measured from: the first placed base (plan decision 5).
	const Base *homeBase()
	{
		for (const Base *b : *save()->getBases())
		{
			if (b->getMarker() != -1)
				return b;
		}
		return 0;
	}

	Vocab::Id categoryName(int category)
	{
		static const Vocab::Id names[SCAN_COUNT] = { Vocab::GEO_UFOS, Vocab::GEO_SITES, Vocab::GEO_ALIEN_BASES,
			Vocab::GEO_CRAFT, Vocab::GEO_BASES, Vocab::GEO_WAYPOINTS };
		return names[category];
	}

	/// What's on the globe in a category, nearest the home base first.
	/// getMarker() is -1 exactly when the globe doesn't draw a target, which is the parity gate.
	std::vector<Target *> scan(int category)
	{
		std::vector<Target *> out;
		SavedGame *s = save();
		switch (category)
		{
		case SCAN_UFOS:
			out.insert(out.end(), s->getUfos()->begin(), s->getUfos()->end());
			break;
		case SCAN_SITES:
			out.insert(out.end(), s->getMissionSites()->begin(), s->getMissionSites()->end());
			break;
		case SCAN_ALIEN_BASES:
			out.insert(out.end(), s->getAlienBases()->begin(), s->getAlienBases()->end());
			break;
		case SCAN_CRAFT:
			for (Base *b : *s->getBases())
				out.insert(out.end(), b->getCrafts()->begin(), b->getCrafts()->end());
			break;
		case SCAN_BASES:
			out.insert(out.end(), s->getBases()->begin(), s->getBases()->end());
			break;
		case SCAN_WAYPOINTS:
			out.insert(out.end(), s->getWaypoints()->begin(), s->getWaypoints()->end());
			break;
		}
		out.erase(std::remove_if(out.begin(), out.end(), [](Target *t) { return t->getMarker() == -1; }), out.end());
		if (const Base *home = homeBase())
		{
			std::stable_sort(out.begin(), out.end(), [home](Target *a, Target *b) { return home->getDistance(a) < home->getDistance(b); });
		}
		return out;
	}

	/// An entry as the scanner reads it: name, where it is, and how far from home.
	std::string entryText(const Target *t)
	{
		const Base *home = homeBase();
		std::string offset = home ? offsetText(home, t->getLongitude(), t->getLatitude()) : std::string();
		return joinComma({ t->getName(game()->getLanguage()), placeName(t->getLongitude(), t->getLatitude()),
			seaText(t->getLongitude(), t->getLatitude()), offset });
	}

	void scanStep(int step)
	{
		std::vector<Target *> list = scan(_scanCategory);
		if (list.empty())
		{
			_scanCurrent = 0;
			say(Vocab::format(Vocab::SCAN_NONE, { Vocab::get(categoryName(_scanCategory)) }), true);
			return;
		}
		int count = (int)list.size();
		int index = (int)(std::find(list.begin(), list.end(), _scanCurrent) - list.begin());
		index = index == count ? (step > 0 ? 0 : count - 1) : ((index + step) % count + count) % count;
		_scanCurrent = list[index];
		say(joinComma({ entryText(list[index]), Vocab::format(Vocab::POSITION, { std::to_string(index + 1), std::to_string(count) }) }), true);
	}

	void scanCategory(int step)
	{
		_scanCategory = ((_scanCategory + step) % SCAN_COUNT + SCAN_COUNT) % SCAN_COUNT;
		std::vector<Target *> list = scan(_scanCategory);
		const std::string &name = Vocab::get(categoryName(_scanCategory));
		if (list.empty())
		{
			_scanCurrent = 0;
			say(Vocab::format(Vocab::SCAN_NONE, { name }), true);
			return;
		}
		_scanCurrent = list[0];
		say(Vocab::format(Vocab::SCAN_CATEGORY, { name, std::to_string(list.size()) }), true);
		say(entryText(list[0]), false);
	}

	/// The current entry if it's still on the globe, else null.
	Target *currentEntry()
	{
		if (!_scanCurrent)
			return 0;
		for (Target *t : scan(_scanCategory))
		{
			if (t == _scanCurrent)
				return t;
		}
		return 0;
	}

	/// Opens the current entry the way clicking it on the globe does.
	void openCurrent(GeoscapeState *state)
	{
		if (!_scanCurrent)
		{
			say(Vocab::get(Vocab::GEO_NOTHING_PICKED), true);
			return;
		}
		Target *t = currentEntry();
		if (!t)
		{
			say(Vocab::get(Vocab::SCAN_GONE), true);
			return;
		}
		game()->pushState(new MultipleTargetsState(std::vector<Target *>(1, t), std::vector<Craft *>(), state, true));
	}

	void rereadCurrent()
	{
		Target *t = currentEntry();
		say(t ? entryText(t) : Vocab::get(_scanCurrent ? Vocab::SCAN_GONE : Vocab::GEO_NOTHING_PICKED), true);
	}

	std::string twoDigits(int n)
	{
		return (n < 10 ? "0" : "") + std::to_string(n);
	}

	/// Date, time, funds and speed, as the Geoscape's side panel shows them.
	std::string statusText(GeoscapeState *state)
	{
		GameTime *time = save()->getTime();
		std::string date = tr(time->getWeekdayString()) + " " + time->getDayString(game()->getLanguage()) + " " +
			tr(time->getMonthString()) + " " + std::to_string(time->getYear());
		std::string clock = twoDigits(time->getHour()) + ":" + twoDigits(time->getMinute());
		return Vocab::format(Vocab::GEO_STATUS, { date, clock, Unicode::formatFunding(save()->getFunds()), state->getTimeSpeed()->getText() });
	}
}

std::vector<Target *> destinations()
{
	std::vector<Target *> out;
	for (int category : { SCAN_UFOS, SCAN_SITES, SCAN_ALIEN_BASES, SCAN_WAYPOINTS })
	{
		std::vector<Target *> part = scan(category);
		out.insert(out.end(), part.begin(), part.end());
	}
	return out;
}

std::string countryName(double lon, double lat)
{
	for (Country *c : *save()->getCountries())
	{
		if (c->getRules()->insideCountry(lon, lat))
			return tr(c->getRules()->getType());
	}
	return "";
}

std::string placeName(double lon, double lat)
{
	std::string country = countryName(lon, lat);
	if (!country.empty())
		return country;
	for (Region *r : *save()->getRegions())
	{
		if (r->getRules()->insideRegion(lon, lat))
			return tr(r->getRules()->getType());
	}
	return "";
}

std::string seaText(double lon, double lat)
{
	// The Geoscape sits under every campaign screen, so its globe is always on the stack.
	for (State *s : game()->getStates())
	{
		if (GeoscapeState *geo = dynamic_cast<GeoscapeState *>(s))
			return geo->getGlobe()->insideLand(lon, lat) ? std::string() : Vocab::get(Vocab::OVER_SEA);
	}
	return "";
}

std::string offsetText(const Target *from, double lon, double lat)
{
	double distance = from->getDistance(lon, lat);
	int miles = (int)std::lround(distance * NM_PER_RADIAN / 10.0) * 10;
	if (miles == 0)
		return "";
	// The game's latitude is negative to the north; flip it for the usual bearing formula.
	double lat1 = -from->getLatitude(), lat2 = -lat, dLon = lon - from->getLongitude();
	double bearing = atan2(sin(dLon) * cos(lat2), cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon));
	int octant = ((int)std::lround(bearing / (M_PI / 4)) % 8 + 8) % 8;
	return Vocab::format(Vocab::GEO_OFFSET, { Unicode::formatNumber(miles), Vocab::get((Vocab::Id)(Vocab::DIR_N + octant)) });
}

bool handleKey(GeoscapeState *state, SDLKey key, bool shift, bool ctrl)
{
	if (ctrl)
	{
		if (key == SDLK_l)
		{
			rereadCurrent();
			return true;
		}
		return false;
	}
	switch (key)
	{
	case SDLK_SPACE:
		say(statusText(state), true);
		return true;
	case SDLK_PERIOD:
	case SDLK_COMMA:
	{
		int step = key == SDLK_PERIOD ? 1 : -1;
		if (shift)
			scanCategory(step);
		else
			scanStep(step);
		return true;
	}
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		openCurrent(state);
		return true;
	case SDLK_d:
		if (!Dogfight::restoreMinimized(state))
			say(Vocab::get(Vocab::GEO_NO_MINIMIZED), true);
		return true;
	default:
		return false;
	}
}

void update(GeoscapeState *state)
{
	std::string speed = state->getTimeSpeed() ? state->getTimeSpeed()->getText() : std::string();
	if (state != _geo)
	{
		// A new Geoscape (new or loaded game): start fresh and don't announce the speed it starts at.
		_geo = state;
		_speed = speed;
		_scanCategory = SCAN_UFOS;
		_scanCurrent = 0;
		return;
	}
	if (speed != _speed)
	{
		_speed = speed;
		say(Vocab::format(Vocab::GEO_SPEED, { speed }), false);
	}
}

}

}
