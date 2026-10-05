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
#include "Speech.h"
#include "Vocab.h"
#include "../Engine/Action.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/State.h"
#include "../Interface/ArrowButton.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Slider.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"

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

const ControlType &editType()
{
	static const ControlType type = makeType("edit", Vocab::ROLE_EDIT);
	return type;
}

namespace
{
	/// Set when the last drive was refused; the navigator then skips its state feedback.
	bool _refused = false;

	/// The game ignores input to a hidden surface (InteractiveSurface::handle), and OXCE hides
	/// buttons to forbid their action, so we refuse the same, out loud.
	bool usable(Surface *surface)
	{
		if (surface->getVisible() && !surface->getHidden())
			return true;
		Speech::say(Vocab::get(Vocab::UNAVAILABLE), true);
		_refused = true;
		return false;
	}

	/// A synthetic click at a screen point, so handlers that read the
	/// button or the mouse position see what a real click would give them.
	bool clickAt(State *state, InteractiveSurface *surface, Uint8 mouseButton, int x, int y)
	{
		if (!usable(surface))
			return false;
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
		return true;
	}
}

void withModifiers(SDLMod mod, const std::function<void()> &fn)
{
	struct Restore
	{
		SDLMod before;
		~Restore() { SDL_SetModState(before); }
	} restore{ SDL_GetModState() };
	SDL_SetModState(mod);
	fn();
}

bool takeRefusal()
{
	bool refused = _refused;
	_refused = false;
	return refused;
}

bool click(State *state, InteractiveSurface *surface, Uint8 mouseButton)
{
	return clickAt(state, surface, mouseButton, surface->getX() + surface->getWidth() / 2, surface->getY() + surface->getHeight() / 2);
}

bool clickRow(State *state, TextList *list, size_t row, Uint8 mouseButton)
{
	if (!usable(list))
		return false;
	list->setSelectedRow(row);
	return clickAt(state, list, mouseButton, list->getX() + 2, list->getY() + 2);
}

std::string cellText(TextList *list, size_t row, size_t column)
{
	std::string cell = list->getCellText(row, column);
	if (!list->hasDots())
		return cell;
	// Left-aligned cells get dots after, right-aligned before, centred both.
	size_t first = cell.find_first_not_of(". ");
	if (first == std::string::npos)
		return std::string();
	size_t last = cell.find_last_not_of(". ");
	return cell.substr(first, last - first + 1);
}

std::string rowText(TextList *list, size_t row)
{
	std::string text;
	for (size_t i = 0; i < list->getCellCount(row); ++i)
	{
		std::string cell = cellText(list, row, i);
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
		v.OnSecondary = [state, list, row] { clickRow(state, list, row, SDL_BUTTON_RIGHT); };
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
	// Not the Selected kind: that would make the chosen button where focus lands.
	std::function<std::string()> pressed;
	if (ToggleTextButton *toggle = dynamic_cast<ToggleTextButton *>(btn))
		pressed = [toggle] { return Vocab::get(toggle->getPressed() ? Vocab::ON : Vocab::OFF); };
	else if (btn->getGroup())
		pressed = [btn] { return *btn->getGroup() == btn ? Vocab::get(Vocab::SELECTED) : std::string(); };
	if (pressed)
	{
		v.Announcements.push_back(NodeAnnouncement(pressed, false, "pressed"));
		v.StateText = pressed;
	}
	return v;
}

NodeVtable arrowButton(State *state, ArrowButton *arrow, std::function<std::string()> label)
{
	Vocab::Id direction;
	switch (arrow->getShape())
	{
	case ARROW_BIG_UP:
	case ARROW_SMALL_UP:
		direction = Vocab::ARROW_UP;
		break;
	case ARROW_BIG_DOWN:
	case ARROW_SMALL_DOWN:
		direction = Vocab::ARROW_DOWN;
		break;
	case ARROW_SMALL_LEFT:
		direction = Vocab::ARROW_LEFT;
		break;
	default:
		direction = Vocab::ARROW_RIGHT;
		break;
	}
	NodeVtable v;
	v.Type = &button();
	v.Announcements.push_back(NodeAnnouncement(label, false, AnnouncementKinds::Label));
	v.Announcements.push_back(NodeAnnouncement([direction] { return Vocab::get(direction); }, false, AnnouncementKinds::Value));
	v.OnActivate = [state, arrow] { click(state, arrow); };
	v.OnSecondary = [state, arrow] { click(state, arrow, SDL_BUTTON_RIGHT); };
	v.StateText = label;
	return v;
}

NodeVtable textEdit(State *state, TextEdit *edit)
{
	NodeVtable v;
	v.Type = &editType();
	v.Announcements.push_back(NodeAnnouncement([edit] { return edit->getText().empty() ? Vocab::get(Vocab::BLANK) : edit->getText(); }, false, AnnouncementKinds::Label));
	// The navigator's typing echo says "Editing" once the field has focus.
	v.OnActivate = [state, edit] { click(state, edit); };
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
		if (sel == (int)box->getSelected() || !usable(box))
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
		if (value == slider->getValue() || !usable(slider))
			return;
		slider->setValue(value);
		slider->notifyChange(state);
	};
	v.StateText = [slider] { return std::to_string(slider->getValue()); };
	return v;
}

}

}
