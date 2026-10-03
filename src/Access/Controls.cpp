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
#include "Controls.h"
#include "Vocab.h"
#include "../Engine/Action.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/State.h"
#include "../Interface/TextButton.h"

namespace OpenXcom
{

namespace Controls
{

using namespace Graph;

const ControlType &button()
{
	static const ControlType type = []
	{
		ControlType t;
		t.Key = "button";
		t.Order = { AnnouncementKinds::Label, AnnouncementKinds::Value, AnnouncementKinds::Role,
			AnnouncementKinds::Selected, AnnouncementKinds::Enabled };
		t.Common = []
		{
			return std::vector<NodeAnnouncement>{ NodeAnnouncement([] { return Vocab::get(Vocab::ROLE_BUTTON); }, false, AnnouncementKinds::Role) };
		};
		return t;
	}();
	return type;
}

void click(State *state, InteractiveSurface *surface, Uint8 mouseButton)
{
	// A synthetic event at the surface's centre, so handlers that read the
	// button or the mouse position see what a real click would give them.
	SDL_Event ev = {};
	ev.type = SDL_MOUSEBUTTONUP;
	ev.button.button = mouseButton;
	ev.button.x = surface->getX() + surface->getWidth() / 2;
	ev.button.y = surface->getY() + surface->getHeight() / 2;
	Action action(&ev, 1.0, 1.0, 0, 0);
	action.setSender(surface);
	action.setMouseAction(ev.button.x, ev.button.y, surface->getX(), surface->getY());
	surface->mousePress(&action, state);
	surface->mouseRelease(&action, state);
	surface->mouseClick(&action, state);
}

NodeVtable textButton(State *state, TextButton *btn)
{
	NodeVtable v;
	v.Type = &button();
	v.Announcements.push_back(NodeAnnouncement([btn] { return btn->getText(); }, false, AnnouncementKinds::Label));
	v.OnActivate = [state, btn] { click(state, btn); };
	return v;
}

}

}
