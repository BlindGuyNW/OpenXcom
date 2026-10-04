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
#include "Dogfight.h"
#include <algorithm>
#include <list>
#include <map>
#include <vector>
#include "Controls.h"
#include "Speech.h"
#include "Vocab.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/State.h"
#include "../Geoscape/DogfightState.h"
#include "../Geoscape/GeoscapeState.h"
#include "../Interface/ImageButton.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/Ufo.h"

namespace OpenXcom
{

namespace Dogfight
{

using namespace Graph;

namespace
{
	/// What the narration remembers of one interception. Strings only: the game deletes
	/// the dogfight, its craft and its UFO without telling anyone.
	struct Watch
	{
		std::string craft;
		bool open;
		int ammo[2];
		bool inRange[2];
	};

	GeoscapeState *_geo = 0;
	std::map<DogfightState *, Watch> _watch;
	/// The last status id each window got, to drop the outrunning message the game re-sets every tick.
	std::map<DogfightState *, std::string> _lastStatus;

	Language *lang()
	{
		return State::getGamePtr()->getLanguage();
	}

	std::string tr(const std::string &id)
	{
		return lang()->getString(id);
	}

	std::vector<DogfightState *> openDogfights(GeoscapeState *geo)
	{
		std::vector<DogfightState *> out;
		for (DogfightState *d : geo->getDogfights())
		{
			if (!d->isMinimized())
				out.push_back(d);
		}
		return out;
	}

	/// Narration for one window: prefixed with its craft when more than one window is open.
	void narrate(DogfightState *d, const std::string &text)
	{
		std::string s = text;
		if (_geo && openDogfights(_geo).size() > 1)
			s = Vocab::format(Vocab::DF_PREFIX, { d->getCraft()->getName(lang()), text });
		Speech::say(s, false);
	}

	CraftWeapon *weaponAt(DogfightState *d, int i)
	{
		Craft *craft = d->getCraft();
		if (i >= (int)craft->getRules()->getWeapons() || i >= (int)craft->getWeapons()->size())
			return 0;
		return craft->getWeapons()->at(i);
	}

	/// The dogfight fires a weapon when the distance is within its range times 8.
	bool inRange(DogfightState *d, CraftWeapon *w)
	{
		return d->getCurrentDistance() <= w->getRules()->getRange() * 8;
	}

	std::string weaponName(CraftWeapon *w)
	{
		return tr(w->getRules()->getType());
	}

	void snapshot(DogfightState *d, Watch &w)
	{
		for (int i = 0; i < 2; ++i)
		{
			CraftWeapon *weapon = weaponAt(d, i);
			w.ammo[i] = weapon ? weapon->getAmmo() : 0;
			w.inRange[i] = weapon ? inRange(d, weapon) : false;
		}
	}

	const char *const MODE_NAMES[5] = { "STR_STANDOFF", "STR_CAUTIOUS_ATTACK", "STR_STANDARD_ATTACK", "STR_AGGRESSIVE_ATTACK", 0 };

	std::string modeName(int i)
	{
		// The game has no string for the disengage button itself, only "DISENGAGING".
		return MODE_NAMES[i] ? tr(MODE_NAMES[i]) : Vocab::get(Vocab::DF_DISENGAGE);
	}

	std::string weaponText(DogfightState *d, int i)
	{
		CraftWeapon *w = weaponAt(d, i);
		std::vector<std::string> parts;
		parts.push_back(Vocab::format(Vocab::DF_WEAPON, { std::to_string(i + 1), weaponName(w), std::to_string(w->getAmmo()) }));
		parts.push_back(Vocab::get(inRange(d, w) ? Vocab::DF_IN_RANGE : Vocab::DF_OUT_OF_RANGE));
		parts.push_back(Vocab::get(d->isWeaponEnabled(i) ? Vocab::ON : Vocab::OFF));
		std::string s;
		for (const std::string &p : parts)
			s += (s.empty() ? "" : ", ") + p;
		return s;
	}

	NodeVtable textNode(std::function<std::string()> text)
	{
		NodeVtable v;
		v.Announcements.push_back(NodeAnnouncement(text, false, AnnouncementKinds::Label));
		return v;
	}

	void build(GraphBuilder &b, State *state)
	{
		GeoscapeState *geo = static_cast<GeoscapeState *>(state);
		for (DogfightState *d : openDogfights(geo))
		{
			std::string key = "dogfight:" + std::to_string(d->getInterceptionNumber()) + ":";
			b.PushContext(Vocab::format(Vocab::DF_TITLE, { d->getCraft()->getName(lang()), d->getUfo()->getName(lang()) }));

			b.AddItem(ControlId::Referenced(d, key + "info"), textNode([d]
			{
				return Vocab::format(Vocab::DF_DISTANCE, { std::to_string(d->getCurrentDistance()) }) + ", " +
					Vocab::format(Vocab::DF_DAMAGE, { std::to_string(d->getCraft()->getDamagePercentage()) });
			}));

			std::vector<ImageButton *> modes = d->getModeButtons();
			for (size_t i = 0; i < modes.size(); ++i)
			{
				ImageButton *btn = modes[i];
				NodeVtable v = Controls::labelledButton(d, btn, modeName((int)i));
				// No StateText: the window says the new mode itself, through setStatus.
				v.Announcements.push_back(NodeAnnouncement([d, btn] { return d->getMode() == btn ? Vocab::get(Vocab::SELECTED) : std::string(); }, false, "pressed"));
				b.AddItem(ControlId::Referenced(btn, key + "mode:" + std::to_string(i)), v);
			}

			for (int i = 0; i < 2; ++i)
			{
				if (!weaponAt(d, i))
					continue;
				NodeVtable v;
				v.Type = &Controls::button();
				v.Announcements.push_back(NodeAnnouncement([d, i] { return weaponText(d, i); }, false, AnnouncementKinds::Label));
				InteractiveSurface *btn = d->getWeaponButton(i);
				v.OnActivate = [d, btn] { Controls::click(d, btn); };
				v.StateText = [d, i] { return Vocab::get(d->isWeaponEnabled(i) ? Vocab::ON : Vocab::OFF); };
				b.AddItem(ControlId::Referenced(btn, key + "weapon:" + std::to_string(i)), v);
			}

			b.AddItem(ControlId::Referenced(d->getMinimizeButton(), key + "minimize"),
				Controls::labelledButton(d, d->getMinimizeButton(), Vocab::get(Vocab::DF_MINIMIZE)));
			b.PopContext();
		}
	}
}

AccessScreen screen()
{
	AccessScreen s;
	s.key = "dogfight";
	s.isActive = [](State *state)
	{
		GeoscapeState *geo = dynamic_cast<GeoscapeState *>(state);
		return geo && !openDogfights(geo).empty();
	};
	s.build = build;
	// Nothing on arrival: update() says each interception as it starts.
	s.name = [](State *) { return std::string(); };
	return s;
}

void update(GeoscapeState *geo)
{
	if (geo != _geo)
	{
		_geo = geo;
		_watch.clear();
		_lastStatus.clear();
	}
	const std::list<DogfightState *> &live = geo->getDogfights();
	for (std::map<DogfightState *, Watch>::iterator i = _watch.begin(); i != _watch.end();)
	{
		if (std::find(live.begin(), live.end(), i->first) == live.end())
		{
			if (i->second.open)
				Speech::say(Vocab::format(Vocab::DF_OVER, { i->second.craft }), false);
			_lastStatus.erase(i->first);
			i = _watch.erase(i);
		}
		else
		{
			++i;
		}
	}
	for (DogfightState *d : live)
	{
		bool open = !d->isMinimized();
		std::map<DogfightState *, Watch>::iterator it = _watch.find(d);
		if (it == _watch.end())
		{
			Watch w;
			w.craft = d->getCraft()->getName(lang());
			w.open = open;
			snapshot(d, w);
			_watch[d] = w;
			if (open)
				narrate(d, Vocab::format(Vocab::DF_START, { w.craft, d->getUfo()->getName(lang()) }));
			continue;
		}
		Watch &w = it->second;
		Watch now = w;
		snapshot(d, now);
		w.open = open;
		if (open)
		{
			for (int i = 0; i < 2; ++i)
			{
				CraftWeapon *weapon = weaponAt(d, i);
				if (!weapon)
					continue;
				if (now.ammo[i] == 0 && w.ammo[i] > 0)
					narrate(d, Vocab::format(Vocab::DF_NO_AMMO, { weaponName(weapon) }));
				else if (now.ammo[i] > 0 && now.inRange[i] != w.inRange[i])
					narrate(d, Vocab::format(now.inRange[i] ? Vocab::DF_RANGE_IN : Vocab::DF_RANGE_OUT, { weaponName(weapon) }));
			}
		}
		for (int i = 0; i < 2; ++i)
		{
			w.ammo[i] = now.ammo[i];
			w.inRange[i] = now.inRange[i];
		}
	}
}

void status(DogfightState *d, const std::string &id)
{
	std::string &last = _lastStatus[d];
	bool repeat = id == last && id == "STR_UFO_OUTRUNNING_INTERCEPTOR";
	last = id;
	// A minimized window shows nothing, so says nothing.
	if (repeat || d->isMinimized())
		return;
	narrate(d, tr(id));
}

bool restoreMinimized(GeoscapeState *geo)
{
	for (DogfightState *d : geo->getDogfights())
	{
		if (d->isMinimized())
		{
			Controls::click(d, d->getMinimizedIcon());
			return true;
		}
	}
	return false;
}

}

}
