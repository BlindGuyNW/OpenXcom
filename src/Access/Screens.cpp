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
#include <map>
#include <tuple>
#include "../Engine/State.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Frame.h"
#include "../Interface/Slider.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/TextButton.h"
#include "../Menu/MainMenuState.h"
#include "../Menu/NewBattleState.h"
#include "../Basescape/CraftArmorState.h"
#include "../Basescape/CraftEquipmentState.h"
#include "../Basescape/CraftInfoState.h"
#include "../Basescape/CraftSoldiersState.h"
#include "../Basescape/SoldierArmorState.h"
#include "../Basescape/SoldierInfoState.h"
#include "../Engine/Action.h"
#include "../Battlescape/ActionMenuItem.h"
#include "../Battlescape/ActionMenuState.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BriefingState.h"
#include "../Battlescape/PrimeGrenadeState.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
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
	// Text edits count too: some screens' titles are editable names.
	std::vector<std::pair<Surface *, std::string> > texts;
	for (Surface *s : state->getSurfaces())
	{
		std::string line;
		if (Text *text = dynamic_cast<Text *>(s))
			line = text->getText();
		else if (TextEdit *edit = dynamic_cast<TextEdit *>(s))
			line = edit->getText();
		if (s->getVisible() && !line.empty())
			texts.push_back(std::make_pair(s, line));
	}
	std::stable_sort(texts.begin(), texts.end(), [](const std::pair<Surface *, std::string> &a, const std::pair<Surface *, std::string> &b)
	{
		return std::make_pair(a.first->getY(), a.first->getX()) < std::make_pair(b.first->getY(), b.first->getX());
	});
	std::string result;
	for (const std::pair<Surface *, std::string> &text : texts)
	{
		const std::string &line = text.second;
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

/// Lets a screen add behaviour to its widgets, such as Left/Right on an equipment list.
/// Gets the widget, the row for a list row (NO_ROW otherwise) and the node to change.
typedef std::function<void(Surface *, size_t, NodeVtable &)> Customizer;
const size_t NO_ROW = (size_t)-1;

/// Adds every visible button, combo box, slider and list of a state as a vertical list in reading order:
/// top to bottom, with each frame's controls together under the frame's heading.
/// A list's rows sit together at the list's position, under a "list" heading.
/// Keys are the widgets' indices among the state's elements, which only change if the state's code does.
void addWidgets(GraphBuilder &b, State *state, const Customizer &customize)
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
		if (s->getVisible() && (dynamic_cast<TextButton *>(s) || dynamic_cast<ComboBox *>(s) || dynamic_cast<Slider *>(s) || dynamic_cast<TextList *>(s)))
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
		NodeVtable v;
		if (TextButton *btn = dynamic_cast<TextButton *>(w.surface))
			v = Controls::textButton(state, btn);
		else if (ComboBox *box = dynamic_cast<ComboBox *>(w.surface))
			v = Controls::comboBox(state, box, labelFor(state, box));
		else if (Slider *slider = dynamic_cast<Slider *>(w.surface))
			v = Controls::slider(state, slider, labelFor(state, slider));
		if (!v.Announcements.empty())
		{
			if (customize)
				customize(w.surface, NO_ROW, v);
			b.AddItem(id, v);
		}
		else if (TextList *list = dynamic_cast<TextList *>(w.surface))
		{
			if (list->getTexts() == 0)
				continue;
			b.PushContext(Vocab::get(Vocab::LIST));
			for (size_t row = 0; row < list->getTexts(); ++row)
			{
				NodeVtable v = Controls::listRow(state, list, row);
				if (customize)
					customize(list, row, v);
				b.AddItem(ControlId::Referenced(list, "widget:" + std::to_string(w.index) + ":row:" + std::to_string(row)), v);
			}
			b.PopContext();
		}
	}
	if (context)
		b.PopContext();
}

void addWidgets(GraphBuilder &b, State *state)
{
	addWidgets(b, state, Customizer());
}

/// A screen that reads all its text on arrival and lists its widgets.
AccessScreen simpleScreen(const std::string &key, std::function<bool(State *)> isActive)
{
	AccessScreen s;
	s.key = key;
	s.isActive = isActive;
	s.name = allText;
	s.build = [](GraphBuilder &b, State *state) { addWidgets(b, state); };
	return s;
}

template <typename T>
bool is(State *state)
{
	return dynamic_cast<T *>(state) != nullptr;
}

AccessScreen mainMenu()
{
	AccessScreen s;
	s.key = "mainMenu";
	s.isActive = [](State *state) { return dynamic_cast<MainMenuState *>(state) != nullptr; };
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state) { addWidgets(b, state); };
	return s;
}

AccessScreen newBattle()
{
	AccessScreen s;
	s.key = "newBattle";
	s.isActive = [](State *state) { return dynamic_cast<NewBattleState *>(state) != nullptr; };
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state)
	{
		addWidgets(b, state, [state](Surface *surface, size_t, NodeVtable &v)
		{
			// The game's OK quietly does nothing when the craft has no one aboard.
			// The navigator only speaks this if the screen is still up after Enter, which is exactly that case.
			TextButton *btn = dynamic_cast<TextButton *>(surface);
			if (btn && btn->getText() == state->tr("STR_OK").operator const std::string &())
				v.StateText = [] { return Vocab::get(Vocab::NEW_BATTLE_NO_CREW); };
		});
	};
	return s;
}

AccessScreen briefing()
{
	AccessScreen s;
	s.key = "briefing";
	s.isActive = [](State *state) { return dynamic_cast<BriefingState *>(state) != nullptr; };
	s.name = allText;
	s.build = [](GraphBuilder &b, State *state) { addWidgets(b, state); };
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

/// The text of the first visible text edit of a state: the editable name, on screens that have one.
std::string editText(State *state)
{
	for (Surface *s : state->getSurfaces())
	{
		TextEdit *edit = dynamic_cast<TextEdit *>(s);
		if (edit && edit->getVisible())
			return edit->getText();
	}
	return "";
}

/// Adds a state's visible text as read-only items, one per screen line, so a label and the
/// value beside it ("Time Units", "54") read together.
void addTextLines(GraphBuilder &b, State *state)
{
	std::map<int, std::vector<Text *> > lines;
	std::map<int, size_t> firstIndex;
	const std::vector<Surface *> &surfaces = state->getSurfaces();
	for (size_t i = 0; i < surfaces.size(); ++i)
	{
		Text *text = dynamic_cast<Text *>(surfaces[i]);
		if (!text || !text->getVisible() || text->getText().empty())
			continue;
		lines[text->getY()].push_back(text);
		if (!firstIndex.count(text->getY()))
			firstIndex[text->getY()] = i;
	}
	for (std::map<int, std::vector<Text *> >::iterator i = lines.begin(); i != lines.end(); ++i)
	{
		std::vector<Text *> texts = i->second;
		std::stable_sort(texts.begin(), texts.end(), [](Text *a, Text *b) { return a->getX() < b->getX(); });
		NodeVtable v;
		v.Announcements.push_back(NodeAnnouncement([texts]
		{
			std::string line;
			for (Text *text : texts)
			{
				if (!line.empty())
					line += ", ";
				line += text->getText();
			}
			return line;
		}, false, AnnouncementKinds::Label));
		b.AddItem(ControlId::Referenced(texts.front(), "line:" + std::to_string(firstIndex[i->first])), v);
	}
}

/// A soldier's details: the stat lines, then the buttons. The arrow buttons get names
/// and say who you've switched to.
AccessScreen soldierInfo()
{
	AccessScreen s;
	s.key = "soldierInfo";
	s.isActive = is<SoldierInfoState>;
	s.name = editText;
	s.build = [](GraphBuilder &b, State *state)
	{
		addTextLines(b, state);
		addWidgets(b, state, [state](Surface *surface, size_t, NodeVtable &v)
		{
			TextButton *btn = dynamic_cast<TextButton *>(surface);
			if (!btn || (btn->getText() != "<<" && btn->getText() != ">>"))
				return;
			Vocab::Id label = btn->getText() == "<<" ? Vocab::PREVIOUS_SOLDIER : Vocab::NEXT_SOLDIER;
			v.Announcements[0] = NodeAnnouncement([label] { return Vocab::get(label); }, false, AnnouncementKinds::Label);
			v.StateText = [state] { return editText(state); };
		});
	};
	return s;
}

/// The Q/E popup: what the item in that hand can do, top to bottom as drawn
/// (so Aimed Shot comes first), with accuracy and TU cost. Escape is the game's own cancel.
AccessScreen actionMenu()
{
	AccessScreen s;
	s.key = "actionMenu";
	s.isActive = is<ActionMenuState>;
	s.name = [](State *state)
	{
		BattleAction *action = static_cast<ActionMenuState *>(state)->getAction();
		return std::string(state->tr(action->weapon->getRules()->getName()));
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		BattleAction *action = static_cast<ActionMenuState *>(state)->getAction();
		std::vector<ActionMenuItem *> items;
		for (Surface *surface : state->getSurfaces())
		{
			ActionMenuItem *item = dynamic_cast<ActionMenuItem *>(surface);
			if (item && item->getVisible())
				items.push_back(item);
		}
		std::stable_sort(items.begin(), items.end(), [](ActionMenuItem *a, ActionMenuItem *c) { return a->getY() < c->getY(); });
		for (ActionMenuItem *item : items)
		{
			std::vector<std::string> parts;
			parts.push_back(item->getDescription());
			if (item->getAccuracy() >= 0)
				parts.push_back(Vocab::format(Vocab::ACCURACY, { std::to_string(item->getAccuracy()) }));
			parts.push_back(Vocab::format(Vocab::TU_COST, { std::to_string(item->getTUs()) }));
			if (item->getTUs() > action->actor->getTimeUnits())
				parts.push_back(Vocab::get(Vocab::NOT_ENOUGH_TU));
			std::string label;
			for (const std::string &p : parts)
				label += (label.empty() ? "" : ", ") + p;
			b.AddItem(ControlId::Referenced(item, "action"), Controls::labelledButton(state, item, label));
		}
	};
	return s;
}

/// Setting a grenade's timer: one button per turn count, 0 to 23. Escape cancels like a right click.
AccessScreen primeGrenade()
{
	AccessScreen s;
	s.key = "primeGrenade";
	s.isActive = is<PrimeGrenadeState>;
	s.name = [](State *state) { return static_cast<PrimeGrenadeState *>(state)->getTitle()->getText(); };
	s.build = [](GraphBuilder &b, State *state)
	{
		PrimeGrenadeState *prime = static_cast<PrimeGrenadeState *>(state);
		for (int i = 0; i < 24; ++i)
		{
			InteractiveSurface *button = prime->getButton(i);
			b.AddItem(ControlId::Referenced(button, "timer"), Controls::labelledButton(state, button, std::to_string(i)));
		}
	};
	s.back = [](State *state)
	{
		// The state closes itself on a right button press anywhere; the buttons only take left clicks.
		SDL_Event ev = {};
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_RIGHT;
		Action action(&ev, 1.0, 1.0, 0, 0);
		state->handle(&action);
	};
	return s;
}

/// Moving items between the base's stores and the craft: Left/Right on a row, Shift for five.
AccessScreen craftEquipment()
{
	AccessScreen s = simpleScreen("craftEquipment", is<CraftEquipmentState>);
	s.build = [](GraphBuilder &b, State *state)
	{
		addWidgets(b, state, [state](Surface *surface, size_t row, NodeVtable &v)
		{
			TextList *list = dynamic_cast<TextList *>(surface);
			if (!list || row == NO_ROW)
				return;
			v.OnAdjust = [state, list, row](int sign, bool large)
			{
				// A left press on the row is how the screen learns which item the arrows act on.
				Controls::clickRow(state, list, row);
				CraftEquipmentState *equip = static_cast<CraftEquipmentState *>(state);
				int count = large ? 5 : 1;
				if (sign < 0)
					equip->moveLeftByValue(count);
				else
					equip->moveRightByValue(count);
			};
			v.StateText = [list, row] { return Controls::rowText(list, row); };
		});
	};
	return s;
}

}

const std::vector<AccessScreen> &all()
{
	static const std::vector<AccessScreen> screens = {
		mainMenu(), newBattle(), briefing(), inventory(), nextTurn(),
		simpleScreen("craftInfo", is<CraftInfoState>),
		simpleScreen("craftSoldiers", is<CraftSoldiersState>),
		craftEquipment(),
		simpleScreen("craftArmor", is<CraftArmorState>),
		simpleScreen("soldierArmor", is<SoldierArmorState>),
		soldierInfo(),
		actionMenu(), primeGrenade(),
	};
	return screens;
}

}

}
