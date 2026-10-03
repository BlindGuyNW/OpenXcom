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
#include <algorithm>
#include "Vocab.h"
#include "../Engine/Action.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/State.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Slider.h"
#include "../Interface/TextList.h"
#include "../Interface/TextButton.h"

namespace OpenXcom
{

namespace Controls
{

using namespace Graph;

namespace
{
	/// Shift+Left/Right step size for combo boxes and sliders.
	const int LARGE_STEP = 5;

	/// A control type that speaks label, value, then its role word.
	ControlType makeType(const char *key, Vocab::Id role)
	{
		ControlType t;
		t.Key = key;
		t.Order = { AnnouncementKinds::Label, AnnouncementKinds::Value, AnnouncementKinds::Role,
			AnnouncementKinds::Selected, AnnouncementKinds::Enabled };
		t.Common = [role]
		{
			return std::vector<NodeAnnouncement>{ NodeAnnouncement([role] { return Vocab::get(role); }, false, AnnouncementKinds::Role) };
		};
		return t;
	}
}

const ControlType &button()
{
	static const ControlType type = makeType("button", Vocab::ROLE_BUTTON);
	return type;
}

const ControlType &comboBoxType()
{
	static const ControlType type = makeType("comboBox", Vocab::ROLE_COMBO_BOX);
	return type;
}

const ControlType &sliderType()
{
	static const ControlType type = makeType("slider", Vocab::ROLE_SLIDER);
	return type;
}

namespace
{
	/// A synthetic click at a screen point, so handlers that read the
	/// button or the mouse position see what a real click would give them.
	void clickAt(State *state, InteractiveSurface *surface, Uint8 mouseButton, int x, int y)
	{
		SDL_Event ev = {};
		ev.type = SDL_MOUSEBUTTONUP;
		ev.button.button = mouseButton;
		ev.button.x = x;
		ev.button.y = y;
		Action action(&ev, 1.0, 1.0, 0, 0);
		action.setSender(surface);
		action.setMouseAction(ev.button.x, ev.button.y, surface->getX(), surface->getY());
		surface->mousePress(&action, state);
		surface->mouseRelease(&action, state);
		surface->mouseClick(&action, state);
	}
}

void click(State *state, InteractiveSurface *surface, Uint8 mouseButton)
{
	clickAt(state, surface, mouseButton, surface->getX() + surface->getWidth() / 2, surface->getY() + surface->getHeight() / 2);
}

void clickRow(State *state, TextList *list, size_t row, Uint8 mouseButton)
{
	list->setSelectedRow(row);
	clickAt(state, list, mouseButton, list->getX() + 2, list->getY() + 2);
}

std::string rowText(TextList *list, size_t row)
{
	std::string text;
	for (size_t i = 0; i < list->getCellCount(row); ++i)
	{
		std::string cell = list->getCellText(row, i);
		if (cell.empty())
			continue;
		if (!text.empty())
			text += ", ";
		text += cell;
	}
	return text;
}

NodeVtable listRow(State *state, TextList *list, size_t row)
{
	NodeVtable v;
	v.Announcements.push_back(NodeAnnouncement([list, row] { return rowText(list, row); }, false, AnnouncementKinds::Label));
	if (list->isSelectable())
	{
		v.OnActivate = [state, list, row] { clickRow(state, list, row); };
		v.StateText = [list, row] { return rowText(list, row); };
	}
	return v;
}

NodeVtable textButton(State *state, TextButton *btn)
{
	NodeVtable v;
	v.Type = &button();
	v.Announcements.push_back(NodeAnnouncement([btn] { return btn->getText(); }, false, AnnouncementKinds::Label));
	v.OnActivate = [state, btn] { click(state, btn); };
	return v;
}

NodeVtable labelledButton(State *state, InteractiveSurface *btn, const std::string &label)
{
	NodeVtable v;
	v.Type = &button();
	v.Announcements.push_back(NodeAnnouncement([label] { return label; }, false, AnnouncementKinds::Label));
	v.OnActivate = [state, btn] { click(state, btn); };
	return v;
}

NodeVtable comboBox(State *state, ComboBox *box, const std::string &label)
{
	NodeVtable v;
	v.Type = &comboBoxType();
	v.Announcements.push_back(NodeAnnouncement([label] { return label; }, false, AnnouncementKinds::Label));
	v.Announcements.push_back(NodeAnnouncement([box] { return box->getSelectedText(); }, false, AnnouncementKinds::Value));
	v.OnAdjust = [state, box](int sign, bool large)
	{
		int count = (int)box->getOptionCount();
		if (count == 0)
			return;
		int sel = std::max(0, std::min(count - 1, (int)box->getSelected() + sign * (large ? LARGE_STEP : 1)));
		if (sel == (int)box->getSelected())
			return;
		box->setSelected(sel);
		box->notifyChange(state);
	};
	v.StateText = [box] { return box->getSelectedText(); };
	return v;
}

NodeVtable slider(State *state, Slider *slider, const std::string &label)
{
	NodeVtable v;
	v.Type = &sliderType();
	v.Announcements.push_back(NodeAnnouncement([label] { return label; }, false, AnnouncementKinds::Label));
	v.Announcements.push_back(NodeAnnouncement([slider] { return std::to_string(slider->getValue()); }, false, AnnouncementKinds::Value));
	v.OnAdjust = [state, slider](int sign, bool large)
	{
		int value = std::max(slider->getMin(), std::min(slider->getMax(), slider->getValue() + sign * (large ? LARGE_STEP : 1)));
		if (value == slider->getValue())
			return;
		slider->setValue(value);
		slider->notifyChange(state);
	};
	v.StateText = [slider] { return std::to_string(slider->getValue()); };
	return v;
}

}

}
