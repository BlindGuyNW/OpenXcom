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
#include "Screens.h"
#include "Controls.h"
#include <algorithm>
#include <tuple>
#include "../Engine/State.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Frame.h"
#include "../Interface/Slider.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Menu/MainMenuState.h"
#include "../Menu/NewBattleState.h"
#include "../Battlescape/BriefingState.h"
#include "../Battlescape/InventoryState.h"
#include "../Battlescape/NextTurnState.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "Vocab.h"

namespace OpenXcom
{

namespace Screens
{

using namespace Graph;

namespace
{

/// The text of the first visible Text element a state added: its title, on most windows.
std::string firstText(State *state)
{
	for (Surface *s : state->getSurfaces())
	{
		Text *text = dynamic_cast<Text *>(s);
		if (text && text->getVisible() && !text->getText().empty())
			return text->getText();
	}
	return "";
}

/// Every visible text of a state in reading order, top to bottom then left to right, as sentences.
/// For screens that are just something to read.
std::string allText(State *state)
{
	std::vector<Text *> texts;
	for (Surface *s : state->getSurfaces())
	{
		Text *text = dynamic_cast<Text *>(s);
		if (text && text->getVisible() && !text->getText().empty())
			texts.push_back(text);
	}
	std::stable_sort(texts.begin(), texts.end(), [](Text *a, Text *b)
	{
		return std::make_pair(a->getY(), a->getX()) < std::make_pair(b->getY(), b->getX());
	});
	std::string result;
	for (Text *text : texts)
	{
		std::string line = text->getText();
		if (!result.empty())
			result += " ";
		result += line;
		char last = line[line.size() - 1];
		if (last != '.' && last != '!' && last != '?' && last != ':')
			result += ".";
	}
	return result;
}

bool overlaps(int a, int aLen, int b, int bLen)
{
	return a < b + bLen && b < a + aLen;
}

/// The visible frame that holds a surface's centre, if any.
Frame *frameFor(State *state, Surface *target)
{
	int x = target->getX() + target->getWidth() / 2, y = target->getY() + target->getHeight() / 2;
	for (Surface *s : state->getSurfaces())
	{
		Frame *frame = dynamic_cast<Frame *>(s);
		if (frame && frame->getVisible() &&
			x >= frame->getX() && x < frame->getX() + frame->getWidth() &&
			y >= frame->getY() && y < frame->getY() + frame->getHeight())
			return frame;
	}
	return 0;
}

/// The text a sighted player reads as a surface's label: the nearest visible text to its left
/// on the same row and in the same frame, else the one just above it. Empty if there's none.
/// Frames and other headed blocks pass sameRow = false: only text above them is their heading.
std::string labelFor(State *state, Surface *target, bool sameRow = true)
{
	Frame *frame = sameRow ? frameFor(state, target) : 0;
	Text *left = 0, *above = 0;
	for (Surface *s : state->getSurfaces())
	{
		Text *text = dynamic_cast<Text *>(s);
		if (!text || !text->getVisible() || text->getText().empty())
			continue;
		if (overlaps(text->getY(), text->getHeight(), target->getY(), target->getHeight()) && text->getX() < target->getX())
		{
			if (!sameRow || frameFor(state, text) != frame)
				continue;
			if (!left || text->getX() > left->getX())
				left = text;
		}
		else if (overlaps(text->getX(), text->getWidth(), target->getX(), target->getWidth()))
		{
			int gap = target->getY() - (text->getY() + text->getHeight());
			if (gap >= 0 && gap <= 4 && (!above || text->getY() > above->getY()))
				above = text;
		}
	}
	return left ? left->getText() : above ? above->getText() : "";
}

/// Adds every visible button, combo box and slider of a state as a vertical list in reading order:
/// top to bottom, with each frame's controls together under the frame's heading.
/// Keys are the widgets' indices among the state's elements, which only change if the state's code does.
void addWidgets(GraphBuilder &b, State *state)
{
	struct Widget
	{
		size_t index;
		Surface *surface;
		Frame *frame;
		// Framed controls sort at their frame's position, so a frame reads as one block.
		std::tuple<int, int, int, int> order() const
		{
			Surface *anchor = frame ? (Surface *)frame : surface;
			return std::make_tuple(anchor->getY(), anchor->getX(), surface->getY(), surface->getX());
		}
	};
	std::vector<Widget> widgets;
	const std::vector<Surface *> &surfaces = state->getSurfaces();
	for (size_t i = 0; i < surfaces.size(); ++i)
	{
		Surface *s = surfaces[i];
		if (s->getVisible() && (dynamic_cast<TextButton *>(s) || dynamic_cast<ComboBox *>(s) || dynamic_cast<Slider *>(s)))
			widgets.push_back(Widget{ i, s, frameFor(state, s) });
	}
	std::stable_sort(widgets.begin(), widgets.end(), [](const Widget &a, const Widget &b) { return a.order() < b.order(); });

	Frame *context = 0;
	for (const Widget &w : widgets)
	{
		if (w.frame != context)
		{
			if (context)
				b.PopContext();
			context = w.frame;
			if (context)
				b.PushContext(labelFor(state, context, false));
		}
		ControlId id = ControlId::Referenced(w.surface, "widget:" + std::to_string(w.index));
		if (TextButton *btn = dynamic_cast<TextButton *>(w.surface))
			b.AddItem(id, Controls::textButton(state, btn));
		else if (ComboBox *box = dynamic_cast<ComboBox *>(w.surface))
			b.AddItem(id, Controls::comboBox(state, box, labelFor(state, box)));
		else if (Slider *slider = dynamic_cast<Slider *>(w.surface))
			b.AddItem(id, Controls::slider(state, slider, labelFor(state, slider)));
	}
	if (context)
		b.PopContext();
}

AccessScreen mainMenu()
{
	AccessScreen s;
	s.key = "mainMenu";
	s.isActive = [](State *state) { return dynamic_cast<MainMenuState *>(state) != nullptr; };
	s.name = firstText;
	s.build = addWidgets;
	return s;
}

AccessScreen newBattle()
{
	AccessScreen s;
	s.key = "newBattle";
	s.isActive = [](State *state) { return dynamic_cast<NewBattleState *>(state) != nullptr; };
	s.name = firstText;
	s.build = addWidgets;
	return s;
}

AccessScreen briefing()
{
	AccessScreen s;
	s.key = "briefing";
	s.isActive = [](State *state) { return dynamic_cast<BriefingState *>(state) != nullptr; };
	s.name = allText;
	s.build = addWidgets;
	return s;
}

/// The equip screen. Only its navigation buttons for now; the items themselves come later.
AccessScreen inventory()
{
	AccessScreen s;
	s.key = "inventory";
	s.isActive = [](State *state) { return dynamic_cast<InventoryState *>(state) != nullptr; };
	// The unit's name is the state's first text.
	s.name = [](State *state) { return Vocab::format(Vocab::INVENTORY, { firstText(state) }); };
	s.build = [](GraphBuilder &b, State *state)
	{
		// The image buttons are told apart by their tooltips, which name them for the mouse too.
		const char *wanted[] = { "STR_OK", "STR_PREVIOUS_UNIT", "STR_NEXT_UNIT" };
		for (const char *tooltip : wanted)
		{
			for (Surface *surface : state->getSurfaces())
			{
				InteractiveSurface *btn = dynamic_cast<InteractiveSurface *>(surface);
				if (!btn || !btn->getVisible() || btn->getTooltip() != tooltip)
					continue;
				NodeVtable v = Controls::labelledButton(state, btn, state->tr(tooltip));
				if (btn->getTooltip() != "STR_OK")
					v.StateText = [state] { return firstText(state); };
				b.AddItem(ControlId::Referenced(btn, tooltip), v);
			}
		}
	};
	return s;
}

/// "Turn 1, side: X-Com". Any key continues in the game; here Enter on the one item does.
AccessScreen nextTurn()
{
	AccessScreen s;
	s.key = "nextTurn";
	s.isActive = [](State *state) { return dynamic_cast<NextTurnState *>(state) != nullptr; };
	s.name = allText;
	s.build = [](GraphBuilder &b, State *state)
	{
		NodeVtable v;
		v.Type = &Controls::button();
		v.Announcements.push_back(NodeAnnouncement([] { return Vocab::get(Vocab::CONTINUE); }, false, AnnouncementKinds::Label));
		v.OnActivate = [state] { static_cast<NextTurnState *>(state)->close(); };
		b.AddItem(ControlId::Referenced(state, "continue"), v);
	};
	return s;
}

}

const std::vector<AccessScreen> &all()
{
	static const std::vector<AccessScreen> screens = { mainMenu(), newBattle(), briefing(), inventory(), nextTurn() };
	return screens;
}

}

}
