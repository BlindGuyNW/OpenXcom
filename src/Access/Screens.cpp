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
#include <climits>
#include <cstdlib>
#include <map>
#include <tuple>
#include "../Engine/State.h"
#include "../Interface/ArrowButton.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Frame.h"
#include "../Interface/Slider.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Menu/MainMenuState.h"
#include "../Menu/NewBattleState.h"
#include "../Menu/PauseState.h"
#include "../Menu/AbandonGameState.h"
#include "../Menu/ListLoadState.h"
#include "../Menu/ListSaveState.h"
#include "../Menu/DeleteGameState.h"
#include "../Menu/ConfirmLoadState.h"
#include "../Menu/ErrorMessageState.h"
#include "../Menu/NewGameState.h"
#include "../Geoscape/AlienBaseState.h"
#include "../Geoscape/BaseNameState.h"
#include "../Geoscape/BuildNewBaseState.h"
#include "../Geoscape/ConfirmDestinationState.h"
#include "../Geoscape/ConfirmNewBaseState.h"
#include "../Geoscape/CraftErrorState.h"
#include "../Geoscape/CraftPatrolState.h"
#include "../Geoscape/GeoscapeCraftState.h"
#include "../Geoscape/Globe.h"
#include "../Geoscape/ItemsArrivingState.h"
#include "../Geoscape/LowFuelState.h"
#include "../Geoscape/MissionDetectedState.h"
#include "../Geoscape/MultipleTargetsState.h"
#include "../Geoscape/NewPossibleManufactureState.h"
#include "../Geoscape/NewPossibleResearchState.h"
#include "../Geoscape/ProductionCompleteState.h"
#include "../Geoscape/ResearchCompleteState.h"
#include "../Geoscape/ResearchRequiredState.h"
#include "../Geoscape/TargetInfoState.h"
#include "../Geoscape/UfoDetectedState.h"
#include "../Geoscape/UfoLostState.h"
#include "../Ufopaedia/UfopaediaStartState.h"
#include "../Ufopaedia/UfopaediaSelectState.h"
#include "../Mod/City.h"
#include "../Mod/RuleRegion.h"
#include "../Savegame/Region.h"
#include "../Engine/Unicode.h"
#include "../fmath.h"
#include "Geo.h"
#include "Dogfight.h"
#include "../Battlescape/AliensCrashState.h"
#include "../Geoscape/BaseDestroyedState.h"
#include "../Geoscape/ConfirmCydoniaState.h"
#include "../Geoscape/ConfirmLandingState.h"
#include "../Geoscape/DogfightErrorState.h"
#include "../Geoscape/InterceptState.h"
#include "../Geoscape/SelectDestinationState.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Waypoint.h"
#include "../Engine/Options.h"
#include "../Battlescape/AbortMissionState.h"
#include "../Basescape/CraftArmorState.h"
#include "../Basescape/CraftEquipmentState.h"
#include "../Basescape/BaseInfoState.h"
#include "../Basescape/BasescapeState.h"
#include "../Basescape/MonthlyCostsState.h"
#include "../Basescape/NewResearchListState.h"
#include "../Basescape/ResearchInfoState.h"
#include "../Basescape/ResearchState.h"
#include "../Basescape/SoldiersState.h"
#include "../Basescape/StoresState.h"
#include "../Basescape/TransfersState.h"
#include "../Savegame/ResearchProject.h"
#include "../Basescape/CraftInfoState.h"
#include "../Basescape/CraftsState.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Savegame/CraftWeapon.h"
#include "../Basescape/CraftSoldiersState.h"
#include "../Basescape/SoldierArmorState.h"
#include "../Basescape/SoldierInfoState.h"
#include "../Engine/Action.h"
#include "../Battlescape/ActionMenuItem.h"
#include "../Battlescape/ActionMenuState.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BriefingState.h"
#include "../Battlescape/DebriefingState.h"
#include "../Battlescape/PrimeGrenadeState.h"
#include "../Battlescape/MedikitState.h"
#include "../Battlescape/MedikitView.h"
#include "../Engine/Language.h"
#include "../Mod/Unit.h"
#include "../Mod/RuleItem.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Battlescape/InventoryState.h"
#include "../Battlescape/Inventory.h"
#include "../Engine/Game.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInventory.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"
#include "Speech.h"
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

/// Is this a widget addWidgets lists? Arrow buttons without an arrow are the load lists' sort toggles.
bool isListedWidget(Surface *s)
{
	if (ArrowButton *arrow = dynamic_cast<ArrowButton *>(s))
		return arrow->getShape() != ARROW_NONE;
	return dynamic_cast<TextButton *>(s) || dynamic_cast<ComboBox *>(s) || dynamic_cast<Slider *>(s) ||
		dynamic_cast<TextList *>(s) || dynamic_cast<TextEdit *>(s);
}

/// Adds every visible button, arrow button, text field, combo box, slider and list of a state as a vertical list in reading order:
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
		if (s->getVisible() && isListedWidget(s))
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
		else if (ArrowButton *arrow = dynamic_cast<ArrowButton *>(w.surface))
			v = Controls::arrowButton(state, arrow, [state, arrow] { return labelFor(state, arrow); });
		else if (TextEdit *edit = dynamic_cast<TextEdit *>(w.surface))
			v = Controls::textEdit(state, edit);
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

/// allText, then every row of the visible lists: popups whose facts sit in a list (a UFO's size and speed).
std::string allTextWithLists(State *state)
{
	std::string result = allText(state);
	for (Surface *s : state->getSurfaces())
	{
		TextList *list = dynamic_cast<TextList *>(s);
		if (!list || !list->getVisible())
			continue;
		for (size_t row = 0; row < list->getTexts(); ++row)
		{
			std::string line = Controls::rowText(list, row);
			if (line.empty())
				continue;
			if (!result.empty())
				result += " ";
			result += line + ".";
		}
	}
	return result;
}

/// A popup: says all its text and its lists on arrival, then lists its widgets.
AccessScreen popupScreen(const std::string &key, std::function<bool(State *)> isActive)
{
	AccessScreen s = simpleScreen(key, isActive);
	s.name = allTextWithLists;
	return s;
}

template <typename T>
bool is(State *state)
{
	return dynamic_cast<T *>(state) != nullptr;
}

/// New game: says only the title on arrival; the Ironman description is the Ironman button's tooltip (Space).
AccessScreen newGame()
{
	AccessScreen s;
	s.key = "newGame";
	s.isActive = is<NewGameState>;
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state)
	{
		addWidgets(b, state, [state](Surface *surface, size_t, NodeVtable &v)
		{
			if (dynamic_cast<ToggleTextButton *>(surface))
			{
				std::string description = state->tr("STR_IRONMAN_DESC");
				v.OnTooltip = [description] { Speech::say(description, true); };
			}
		});
	};
	return s;
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

/// An item as the inventory names it: unconscious units by name, unresearched alien items as
/// artifacts, then rounds loaded and a primed grenade's timer.
std::string inventoryItemText(State *state, BattleItem *item)
{
	Game *game = State::getGamePtr();
	std::string name;
	if (item->getUnit() && item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
		name = item->getUnit()->getName(game->getLanguage());
	else if (game->getSavedGame()->isResearched(item->getRules()->getRequirements()))
		name = state->tr(item->getRules()->getName());
	else
		name = state->tr("STR_ALIEN_ARTIFACT");
	// needsAmmo() is also true of grenades and the like, so ask whether it takes clips at all.
	if (!item->getRules()->getCompatibleAmmo()->empty())
	{
		BattleItem *ammo = item->getAmmoItem();
		name = ammo ? Vocab::format(Vocab::ROUNDS, { name, std::to_string(ammo->getAmmoQuantity()) }) : Vocab::format(Vocab::NO_AMMO, { name });
	}
	else if (item->getRules()->getBattleType() == BT_AMMO)
	{
		name = Vocab::format(Vocab::ROUNDS, { name, std::to_string(item->getAmmoQuantity()) });
	}
	if (item->getFuseTimer() >= 0)
		name = Vocab::format(Vocab::PRIMED, { name, std::to_string(item->getFuseTimer()) });
	return name;
}

/// The body slots in a fixed reading order, then any a mod adds, then the ground.
std::vector<RuleInventory *> inventorySlots(Mod *mod)
{
	static const char *order[] = { "STR_RIGHT_HAND", "STR_LEFT_HAND", "STR_BELT", "STR_BACK_PACK",
		"STR_RIGHT_SHOULDER", "STR_LEFT_SHOULDER", "STR_RIGHT_LEG", "STR_LEFT_LEG" };
	std::vector<RuleInventory *> slots;
	for (const char *id : order)
	{
		RuleInventory *slot = mod->getInventory(id);
		if (slot)
			slots.push_back(slot);
	}
	RuleInventory *ground = 0;
	for (auto &i : *mod->getInventories())
	{
		if (i.second->getType() == INV_GROUND)
			ground = i.second;
		else if (std::find(slots.begin(), slots.end(), i.second) == slots.end())
			slots.push_back(i.second);
	}
	if (ground)
		slots.push_back(ground);
	return slots;
}

/// Speaks the outcome of a keyboard inventory move, with TUs left when moves cost them.
void sayInventoryResult(Inventory *inv, const std::string &text)
{
	std::string s = text;
	BattleUnit *unit = inv->getSelectedUnit();
	if (inv->getTuMode() && unit)
		s += ", " + Vocab::format(Vocab::TIME_UNITS_LEFT, { std::to_string(unit->getTimeUnits()) });
	Speech::say(s, true);
}

/// Enter on an item or an empty slot: pick the item up, or put the held item here
/// (loading it if this is a weapon that takes it).
std::function<void()> inventoryActivate(State *state, RuleInventory *slot, BattleItem *item)
{
	return [state, slot, item]
	{
		InventoryState *invState = static_cast<InventoryState *>(state);
		Inventory *inv = invState->getInventory();
		BattleItem *held = inv->getSelectedItem();
		if (!held)
		{
			if (item && inv->pickUp(item))
				Speech::say(Vocab::format(Vocab::HOLDING, { inventoryItemText(state, item) }), true);
			return;
		}
		std::string heldName = inventoryItemText(state, held);
		int result = inv->placeSelected(slot, item);
		invState->updateStats();
		if (result == 2)
			sayInventoryResult(inv, Vocab::format(Vocab::LOADED, { inventoryItemText(state, item) }));
		else if (result == 1)
			sayInventoryResult(inv, Vocab::format(Vocab::PLACED, { heldName, state->tr(slot->getId()) }));
	};
}

/// A soldier's inventory: body slots (one list, each slot announced as you enter it),
/// the ground, then the buttons. Enter picks up and puts down; Escape puts a held item back,
/// or closes the screen.
AccessScreen inventory()
{
	AccessScreen s;
	s.key = "inventory";
	s.isActive = [](State *state) { return dynamic_cast<InventoryState *>(state) != nullptr; };
	// The unit's name is the state's first text.
	s.name = [](State *state) { return Vocab::format(Vocab::INVENTORY, { firstText(state) }); };
	s.build = [](GraphBuilder &b, State *state)
	{
		Inventory *inv = static_cast<InventoryState *>(state)->getInventory();
		BattleUnit *unit = inv->getSelectedUnit();
		if (unit)
		{
			std::vector<RuleInventory *> slots = inventorySlots(State::getGamePtr()->getMod());
			b.BeginStop("body");
			for (RuleInventory *slot : slots)
			{
				std::vector<BattleItem *> items;
				if (slot->getType() == INV_GROUND)
				{
					b.BeginStop("ground");
					if (unit->getTile())
						items = *unit->getTile()->getInventory();
				}
				else
				{
					for (BattleItem *item : *unit->getInventory())
					{
						if (item->getSlot() == slot)
							items.push_back(item);
					}
				}
				std::stable_sort(items.begin(), items.end(), [](BattleItem *a, BattleItem *c)
				{
					return a->getSlotY() != c->getSlotY() ? a->getSlotY() < c->getSlotY() : a->getSlotX() < c->getSlotX();
				});
				b.PushContext(state->tr(slot->getId()));
				for (BattleItem *item : items)
				{
					NodeVtable v;
					v.Type = &Controls::button();
					v.Announcements.push_back(NodeAnnouncement([state, inv, item]
					{
						std::string text = inventoryItemText(state, item);
						return inv->getSelectedItem() == item ? Vocab::format(Vocab::HELD, { text }) : text;
					}, false, AnnouncementKinds::Label));
					v.OnActivate = inventoryActivate(state, slot, item);
					b.AddItem(ControlId::Referenced(item, "item:" + std::to_string(item->getId())), v);
				}
				if (items.empty())
				{
					NodeVtable v;
					v.Announcements.push_back(NodeAnnouncement([] { return Vocab::get(Vocab::EMPTY); }, false, AnnouncementKinds::Label));
					v.OnActivate = inventoryActivate(state, slot, 0);
					b.AddItem(ControlId::Referenced(slot, "empty:" + slot->getId()), v);
				}
				b.PopContext();
			}
		}

		b.BeginStop("buttons");
		// The image buttons are told apart by their tooltips, which name them for the mouse too.
		const char *wanted[] = { "STR_OK", "STR_PREVIOUS_UNIT", "STR_NEXT_UNIT", "STR_UNLOAD_WEAPON" };
		for (const char *tooltip : wanted)
		{
			for (Surface *surface : state->getSurfaces())
			{
				InteractiveSurface *btn = dynamic_cast<InteractiveSurface *>(surface);
				if (!btn || !btn->getVisible() || btn->getTooltip() != tooltip)
					continue;
				NodeVtable v = Controls::labelledButton(state, btn, state->tr(tooltip));
				if (btn->getTooltip() == "STR_PREVIOUS_UNIT" || btn->getTooltip() == "STR_NEXT_UNIT")
					v.StateText = [state] { return firstText(state); };
				b.AddItem(ControlId::Referenced(btn, tooltip), v);
			}
		}
	};
	s.back = [](State *state)
	{
		InventoryState *invState = static_cast<InventoryState *>(state);
		Inventory *inv = invState->getInventory();
		if (inv->getSelectedItem())
		{
			inv->cancelSelected();
			Speech::say(Vocab::get(Vocab::PUT_BACK), true);
			return;
		}
		invState->btnOkClick(0);
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
			b.AddItem(ControlId::Referenced(item, "action:" + std::to_string((int)item->getAction())), Controls::labelledButton(state, item, label));
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
			b.AddItem(ControlId::Referenced(button, "timer:" + std::to_string(i)), Controls::labelledButton(state, button, std::to_string(i)));
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

const char *const BODY_PARTS[6] = { "STR_HEAD", "STR_TORSO", "STR_RIGHT_ARM", "STR_LEFT_ARM", "STR_RIGHT_LEG", "STR_LEFT_LEG" };

/// "no fatal wounds", "1 fatal wound", "3 fatal wounds".
std::string woundsText(int n)
{
	if (n == 0)
		return Vocab::get(Vocab::NO_FATAL_WOUNDS);
	return n == 1 ? Vocab::get(Vocab::ONE_FATAL_WOUND) : Vocab::format(Vocab::FATAL_WOUNDS, { std::to_string(n) });
}

std::string joinParts(const std::vector<std::string> &parts)
{
	std::string s;
	for (const std::string &p : parts)
		s += (s.empty() ? "" : ", ") + p;
	return s;
}

/// Health is only ours to know for our own units (the HUD shows it); the medikit screen never shows it.
std::string medikitHealth(BattleUnit *target)
{
	if (target->getOriginalFaction() != FACTION_PLAYER)
		return std::string();
	return Vocab::format(Vocab::HEALTH, { std::to_string(target->getHealth()) });
}

/// After a use: what's left of it and the healer's time units.
std::string medikitUseText(MedikitState *m, int left)
{
	return joinParts({ Vocab::format(Vocab::ITEMS_LEFT, { std::to_string(left) }), Vocab::format(Vocab::TIME_UNITS_LEFT, { std::to_string(m->getHealer()->getTimeUnits()) }) });
}

/// A medikit button: label, then what's left and the TU cost.
NodeVtable medikitButton(State *state, InteractiveSurface *button, const std::string &label, std::function<std::string()> detail, std::function<std::string()> after)
{
	NodeVtable v = Controls::labelledButton(state, button, label);
	v.Announcements.push_back(NodeAnnouncement(detail, false, AnnouncementKinds::Value));
	v.StateText = after;
	return v;
}

/// Using a medikit on the unit in front (or an unconscious one underfoot). On arrival: who and
/// where the fatal wounds are. Then the six body parts (Enter selects one, as clicking the body
/// diagram would; Heal works on the selected part), Heal, Stimulant, Pain Killer and Close.
/// The game closes the screen itself when a stimulant or heal revives an unconscious unit.
/// Wired through MedikitState's accessors.
AccessScreen medikit()
{
	AccessScreen s;
	s.key = "medikit";
	s.isActive = is<MedikitState>;
	s.name = [](State *state)
	{
		BattleUnit *target = static_cast<MedikitState *>(state)->getTarget();
		Language *lang = State::getGamePtr()->getLanguage();
		// Not ours: the race, since the screen doesn't say and ranks share a sprite.
		std::string name = target->getOriginalFaction() == FACTION_PLAYER || !target->getUnitRules()
			? target->getName(lang) : std::string(lang->getString(target->getUnitRules()->getRace()));
		std::vector<std::string> wounded;
		for (int i = 0; i < 6; ++i)
		{
			if (target->getFatalWound(i))
				wounded.push_back(std::string(state->tr(BODY_PARTS[i])) + " " + std::to_string(target->getFatalWound(i)));
		}
		return joinParts({ Vocab::format(Vocab::MEDIKIT_ON, { name }), medikitHealth(target),
			wounded.empty() ? Vocab::get(Vocab::NO_FATAL_WOUNDS) : Vocab::format(Vocab::FATAL_WOUNDS_IN, { joinParts(wounded) }) });
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		MedikitState *m = static_cast<MedikitState *>(state);
		BattleUnit *target = m->getTarget();
		MedikitView *view = m->getView();
		std::string cost = Vocab::format(Vocab::TU_COST, { std::to_string(m->getTUCost()) });
		for (int i = 0; i < 6; ++i)
		{
			std::string part = state->tr(BODY_PARTS[i]);
			NodeVtable v;
			v.Type = &Controls::button();
			v.Announcements.push_back(NodeAnnouncement([part] { return part; }, false, AnnouncementKinds::Label));
			v.Announcements.push_back(NodeAnnouncement([target, view, i]
			{
				std::string text = woundsText(target->getFatalWound(i));
				if (view->getSelectedPart() == i)
					text += ", " + Vocab::get(Vocab::SELECTED);
				return text;
			}, false, AnnouncementKinds::Value));
			v.OnActivate = [view, i] { view->setSelectedPart(i); };
			v.StateText = [part] { return Vocab::format(Vocab::PART_SELECTED, { part }); };
			b.AddItem(ControlId::Referenced(view, "part:" + std::to_string(i)), v);
		}
		BattleItem *item = m->getItem();
		b.AddItem(ControlId::Referenced(m->getHealButton(), "heal"), medikitButton(state, m->getHealButton(), state->tr("STR_HEAL"),
			[state, view, item, cost]
			{
				int part = view->getSelectedPart();
				return joinParts({ part >= 0 ? std::string(state->tr(BODY_PARTS[part])) : std::string(), Vocab::format(Vocab::ITEMS_LEFT, { std::to_string(item->getHealQuantity()) }), cost });
			},
			[m, state, target, view, item]
			{
				int part = view->getSelectedPart();
				std::string where = part >= 0 ? std::string(state->tr(BODY_PARTS[part])) + ": " + woundsText(target->getFatalWound(part)) : std::string();
				return joinParts({ where, medikitHealth(target), medikitUseText(m, item->getHealQuantity()) });
			}));
		b.AddItem(ControlId::Referenced(m->getStimulantButton(), "stimulant"), medikitButton(state, m->getStimulantButton(), state->tr("STR_STIMULANT"),
			[item, cost] { return joinParts({ Vocab::format(Vocab::ITEMS_LEFT, { std::to_string(item->getStimulantQuantity()) }), cost }); },
			[m, item] { return medikitUseText(m, item->getStimulantQuantity()); }));
		b.AddItem(ControlId::Referenced(m->getPainKillerButton(), "painKiller"), medikitButton(state, m->getPainKillerButton(), state->tr("STR_PAIN_KILLER"),
			[item, cost] { return joinParts({ Vocab::format(Vocab::ITEMS_LEFT, { std::to_string(item->getPainKillerQuantity()) }), cost }); },
			[m, item] { return medikitUseText(m, item->getPainKillerQuantity()); }));
		b.AddItem(ControlId::Referenced(m->getEndButton(), "close"), Controls::labelledButton(state, m->getEndButton(), Vocab::get(Vocab::CLOSE)));
	};
	s.back = [](State *state)
	{
		MedikitState *m = static_cast<MedikitState *>(state);
		Controls::click(state, m->getEndButton());
	};
	return s;
}

/// "Pistol, 4 in stores, 2 on craft". New Battle shows "-" for its unlimited stores.
std::string equipRowText(TextList *list, size_t row)
{
	if (list->getCellCount(row) < 3)
		return Controls::rowText(list, row);
	std::string stores = Controls::cellText(list, row, 1);
	return Controls::cellText(list, row, 0) + ", "
		+ (stores == "-" ? Vocab::get(Vocab::STORES_UNLIMITED) : Vocab::format(Vocab::IN_STORES, { stores })) + ", "
		+ Vocab::format(Vocab::ON_CRAFT, { Controls::cellText(list, row, 2) });
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
				// Ctrl is the arrows' right click: as many as fit, or all back to the stores.
				int count = std::abs(sign) >= Controls::ADJUST_LIMIT ? INT_MAX : (large ? 5 : 1);
				if (sign < 0)
					equip->moveLeftByValue(count);
				else
					equip->moveRightByValue(count);
			};
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([list, row] { return equipRowText(list, row); }, false, AnnouncementKinds::Label));
			// The game ignores clicks on the row itself, so Enter re-reads it with how to move items.
			static bool hint = false;
			v.OnActivate = [] { hint = true; };
			v.StateText = [list, row]
			{
				std::string text = equipRowText(list, row);
				if (hint)
					text += ". " + Vocab::get(Vocab::EQUIP_HINT);
				hint = false;
				return text;
			};
		});
	};
	return s;
}

/// "Aliens killed: 1, score 10": a debriefing score row (item, quantity, score).
std::string debriefRow(TextList *list, size_t row)
{
	if (list->getCellCount(row) < 3)
		return Controls::rowText(list, row);
	return Vocab::format(Vocab::DEBRIEF_ROW, { Controls::cellText(list, row, 0), Controls::cellText(list, row, 1), Controls::cellText(list, row, 2) });
}

/// A read-only item.
void addLine(GraphBuilder &b, const ControlId &id, std::function<std::string()> text)
{
	NodeVtable v;
	v.Announcements.push_back(NodeAnnouncement(text, false, AnnouncementKinds::Label));
	b.AddItem(id, v);
}

/// After a battle: the outcome, rating and total on arrival, then the score rows, the recovered
/// items and the buttons. Stats switches to what each soldier gained ("Time Units +2, Bravery +10").
/// Wired through DebriefingState's accessors, since its two pages overlap on screen.
AccessScreen debriefing()
{
	AccessScreen s;
	s.key = "debriefing";
	s.isActive = is<DebriefingState>;
	s.name = [](State *state)
	{
		DebriefingState *debrief = static_cast<DebriefingState *>(state);
		std::string text = debrief->getTitle()->getText();
		text += ". " + debrief->getRating()->getText();
		if (debrief->getTotalList()->getTexts() > 0)
			text += ". " + Controls::rowText(debrief->getTotalList(), 0);
		return text;
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		DebriefingState *debrief = static_cast<DebriefingState *>(state);
		if (!debrief->isShowingSoldierStats())
		{
			TextList *stats = debrief->getStatsList();
			for (size_t row = 0; row < stats->getTexts(); ++row)
				addLine(b, ControlId::Referenced(stats, "stats:" + std::to_string(row)), [stats, row] { return debriefRow(stats, row); });
			TextList *recovery = debrief->getRecoveryList();
			if (recovery->getTexts() > 0)
			{
				b.PushContext(debrief->getRecoveryHeading()->getText());
				for (size_t row = 0; row < recovery->getTexts(); ++row)
					addLine(b, ControlId::Referenced(recovery, "recovery:" + std::to_string(row)), [recovery, row] { return debriefRow(recovery, row); });
				b.PopContext();
			}
			TextList *total = debrief->getTotalList();
			if (total->getTexts() > 0)
				addLine(b, ControlId::Referenced(total, "total"), [total] { return Controls::rowText(total, 0); });
			Text *rating = debrief->getRating();
			addLine(b, ControlId::Referenced(rating, "rating"), [rating] { return rating->getText(); });
		}
		else
		{
			TextList *soldiers = debrief->getSoldierStatsList();
			std::vector<std::string> statNames;
			for (Text *header : debrief->getSoldierStatHeaders())
				statNames.push_back(state->tr(header->getTooltip()));
			for (size_t row = 0; row < soldiers->getTexts(); ++row)
			{
				addLine(b, ControlId::Referenced(soldiers, "soldier:" + std::to_string(row)), [soldiers, row, statNames]
				{
					// Column 0 is the name, then one column per stat; empty means no gain.
					std::vector<std::string> gains;
					for (size_t i = 0; i < statNames.size() && i + 1 < soldiers->getCellCount(row); ++i)
					{
						std::string cell = Controls::cellText(soldiers, row, i + 1);
						if (!cell.empty())
							gains.push_back(Vocab::format(Vocab::STAT_GAIN, { statNames[i], cell }));
					}
					std::string text = Controls::cellText(soldiers, row, 0) + ": ";
					if (gains.empty())
						return text + Vocab::get(Vocab::NO_STAT_GAINS);
					for (size_t i = 0; i < gains.size(); ++i)
						text += (i ? ", " : "") + gains[i];
					return text;
				});
			}
		}
		b.AddItem(ControlId::Referenced(debrief->getOkButton(), "ok"), Controls::textButton(state, debrief->getOkButton()));
		b.AddItem(ControlId::Referenced(debrief->getStatsButton(), "stats"), Controls::textButton(state, debrief->getStatsButton()));
	};
	return s;
}

}

/// Finds land at or near a point. Coastal cities can sit just off the game's land polygons,
/// and a base can only go on land. Searches rings a quarter degree apart, out to two degrees.
bool landNear(Globe *globe, double &lon, double &lat)
{
	if (globe->insideLand(lon, lat))
		return true;
	const double step = 0.25 * M_PI / 180.0;
	for (int ring = 1; ring <= 8; ++ring)
	{
		for (int dir = 0; dir < 8; ++dir)
		{
			double a = dir * M_PI / 4;
			double tryLon = lon + ring * step * sin(a), tryLat = lat - ring * step * cos(a);
			while (tryLon < 0)
				tryLon += 2 * M_PI;
			while (tryLon >= 2 * M_PI)
				tryLon -= 2 * M_PI;
			if (globe->insideLand(tryLon, tryLat))
			{
				lon = tryLon;
				lat = tryLat;
				return true;
			}
		}
	}
	return false;
}

/// Picking where a new base goes: the game's cities grouped by region (with the base cost,
/// after the first base), each with its country. Enter centres the globe on the city and
/// clicks the globe, so the game's own handler places the base and runs its checks.
AccessScreen buildNewBase()
{
	AccessScreen s;
	s.key = "buildNewBase";
	s.isActive = is<BuildNewBaseState>;
	s.name = allText;
	s.build = [](GraphBuilder &b, State *state)
	{
		BuildNewBaseState *build = static_cast<BuildNewBaseState *>(state);
		Game *game = State::getGamePtr();
		for (Region *region : *game->getSavedGame()->getRegions())
		{
			RuleRegion *rules = region->getRules();
			if (rules->getCities()->empty())
				continue;
			std::string name = state->tr(rules->getType());
			b.PushContext(build->isFirst() ? name : Vocab::format(Vocab::REGION_BASE_COST, { name, Unicode::formatFunding(rules->getBaseCost()) }));
			for (size_t i = 0; i < rules->getCities()->size(); ++i)
			{
				City *city = rules->getCities()->at(i);
				NodeVtable v;
				std::string label = city->getName(game->getLanguage());
				// Country only: the region is already the heading.
				std::string country = Geo::countryName(city->getLongitude(), city->getLatitude());
				if (!country.empty() && country != label)
					label += ", " + country;
				v.Announcements.push_back(NodeAnnouncement([label] { return label; }, false, AnnouncementKinds::Label));
				v.OnActivate = [build, city]
				{
					double lon = city->getLongitude(), lat = city->getLatitude();
					// No land nearby: click the city itself and let the game say it can't build there.
					landNear(build->getGlobe(), lon, lat);
					build->getGlobe()->center(lon, lat);
					Controls::click(build, build->getGlobe());
				};
				b.AddItem(ControlId::Referenced(city, "city:" + rules->getType() + ":" + std::to_string(i)), v);
			}
			b.PopContext();
		}
		// Cancel, when it's there (not for the first base).
		addWidgets(b, state);
	};
	return s;
}

/// An Intercept row: "Interceptor-1, READY, Base 1, 2 weapons, 0 soldiers, 0 tanks".
std::string interceptRow(State *state, Craft *c)
{
	return Vocab::format(Vocab::INTERCEPT_ROW, { c->getName(State::getGamePtr()->getLanguage()), state->tr(c->getStatus()), c->getBase()->getName(),
		std::to_string(c->getNumWeapons()), std::to_string(c->getNumSoldiers()), std::to_string(c->getNumVehicles()) });
}

/// Why InterceptState::lstCraftsLeftClick won't launch a craft (same test), or empty if it will.
std::string launchRefusal(State *state, Craft *c)
{
	if (c->getStatus() == "STR_READY" || ((c->getStatus() == "STR_OUT" || Options::craftLaunchAlways) && !c->getLowFuel() && !c->getMissionComplete()))
		return "";
	std::string reason;
	if (c->getLowFuel())
		reason = Vocab::get(Vocab::LOW_FUEL);
	else if (c->getMissionComplete())
		reason = Vocab::get(Vocab::RETURNING);
	else
		reason = state->tr(c->getStatus());
	return Vocab::format(Vocab::CANT_LAUNCH, { reason });
}

/// Launch interception: one row per craft from the state's own craft list; Enter launches,
/// or says why not. Backspace (right click) centres the globe on a craft in flight, as the game does.
AccessScreen intercept()
{
	AccessScreen s = simpleScreen("intercept", is<InterceptState>);
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state)
	{
		InterceptState *ic = static_cast<InterceptState *>(state);
		addWidgets(b, state, [state, ic](Surface *, size_t row, NodeVtable &v)
		{
			if (row == NO_ROW || row >= ic->getCrafts().size())
				return;
			Craft *c = ic->getCrafts()[row];
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([state, c] { return interceptRow(state, c); }, false, AnnouncementKinds::Label));
			// Only heard if the screen is still up after Enter, which is exactly a refusal.
			v.StateText = [state, c] { return launchRefusal(state, c); };
		});
	};
	return s;
}

/// A destination as the picker reads it: where it is, how far from the craft, and whether it's
/// inside the range circle the globe draws (half the craft's fuel, so it can get home).
std::string destinationText(Craft *craft, const std::string &name, double lon, double lat)
{
	std::vector<std::string> parts = { name, Geo::placeName(lon, lat), Geo::offsetText(craft, lon, lat),
		Vocab::get(craft->getDistance(lon, lat) <= craft->getBaseRange() ? Vocab::DF_IN_RANGE : Vocab::DF_OUT_OF_RANGE) };
	std::string s;
	for (const std::string &p : parts)
	{
		if (!p.empty())
			s += (s.empty() ? "" : ", ") + p;
	}
	return s;
}

/// Select destination: what's on the globe a craft can go to, nearest first, then the cities
/// by region for flying to a point. Enter goes through MultipleTargetsState as a globe click does,
/// so the game's own confirmation (and waypoint bookkeeping) follows.
AccessScreen selectDestination()
{
	AccessScreen s;
	s.key = "selectDestination";
	s.isActive = is<SelectDestinationState>;
	s.name = allText;
	s.build = [](GraphBuilder &b, State *state)
	{
		Craft *craft = static_cast<SelectDestinationState *>(state)->getCraft();
		Game *game = State::getGamePtr();
		std::vector<Target *> targets = Geo::destinations();
		std::stable_sort(targets.begin(), targets.end(), [craft](Target *a, Target *b) { return craft->getDistance(a) < craft->getDistance(b); });
		if (!targets.empty())
		{
			b.PushContext(Vocab::get(Vocab::DEST_TARGETS));
			for (Target *t : targets)
			{
				NodeVtable v;
				v.Announcements.push_back(NodeAnnouncement([craft, t, game]
				{
					return destinationText(craft, t->getName(game->getLanguage()), t->getLongitude(), t->getLatitude());
				}, false, AnnouncementKinds::Label));
				v.OnActivate = [craft, t, game] { game->pushState(new MultipleTargetsState(std::vector<Target *>(1, t), craft, 0)); };
				b.AddItem(ControlId::Referenced(t, "target:" + t->getType() + ":" + std::to_string(t->getId())), v);
			}
			b.PopContext();
		}
		for (Region *region : *game->getSavedGame()->getRegions())
		{
			RuleRegion *rules = region->getRules();
			if (rules->getCities()->empty())
				continue;
			b.PushContext(state->tr(rules->getType()));
			for (size_t i = 0; i < rules->getCities()->size(); ++i)
			{
				City *city = rules->getCities()->at(i);
				NodeVtable v;
				v.Announcements.push_back(NodeAnnouncement([craft, city, game]
				{
					return destinationText(craft, city->getName(game->getLanguage()), city->getLongitude(), city->getLatitude());
				}, false, AnnouncementKinds::Label));
				v.OnActivate = [craft, city, game]
				{
					// What clicking an empty spot on the globe does: a fresh waypoint, registered only if confirmed.
					Waypoint *w = new Waypoint();
					w->setLongitude(city->getLongitude());
					w->setLatitude(city->getLatitude());
					game->pushState(new MultipleTargetsState(std::vector<Target *>(1, w), craft, 0));
				};
				b.AddItem(ControlId::Referenced(city, "city:" + rules->getType() + ":" + std::to_string(i)), v);
			}
			b.PopContext();
		}
		// Cancel, and Cydonia when it's offered.
		addWidgets(b, state);
	};
	return s;
}

/// The Basescape's menu: says the base, its region and the funds on arrival, then the buttons
/// (and the base name field), then the other bases to switch to. The facility grid isn't covered yet.
/// The game's number keys switch bases too; either way the tick says the new base.
AccessScreen basescape()
{
	static Base *shown = 0;
	AccessScreen s;
	s.key = "basescape";
	s.isActive = is<BasescapeState>;
	s.name = [](State *state)
	{
		shown = static_cast<BasescapeState *>(state)->getBase();
		return allText(state);
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		BasescapeState *bs = static_cast<BasescapeState *>(state);
		addWidgets(b, state);
		std::vector<Base *> *bases = State::getGamePtr()->getSavedGame()->getBases();
		if (bases->size() < 2)
			return;
		b.PushContext(Vocab::get(Vocab::BASES));
		for (Base *base : *bases)
		{
			NodeVtable v;
			v.Type = &Controls::button();
			v.Announcements.push_back(NodeAnnouncement([base] { return base->getName(); }, false, AnnouncementKinds::Label));
			v.Announcements.push_back(NodeAnnouncement([bs, base] { return bs->getBase() == base ? Vocab::get(Vocab::SELECTED) : std::string(); }, false, "pressed"));
			v.OnActivate = [bs, base] { bs->selectBase(base); };
			b.AddItem(ControlId::Referenced(base, "base:" + base->getName()), v);
		}
		b.PopContext();
	};
	s.tick = [](State *state)
	{
		Base *base = static_cast<BasescapeState *>(state)->getBase();
		if (base == shown)
			return;
		shown = base;
		Speech::say(allText(state), true);
	};
	return s;
}

/// A craft as the base's craft list shows it, with the columns named.
std::string craftsRow(State *state, Craft *c)
{
	return Vocab::format(Vocab::CRAFTS_ROW, { c->getName(State::getGamePtr()->getLanguage()), state->tr(c->getStatus()),
		std::to_string(c->getNumWeapons()), std::to_string(c->getRules()->getWeapons()),
		std::to_string(c->getNumSoldiers()), std::to_string(c->getNumVehicles()) });
}

/// The base's craft list. Enter opens a craft's info; the game ignores craft that are out, so that's said.
AccessScreen crafts()
{
	AccessScreen s = simpleScreen("crafts", is<CraftsState>);
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state)
	{
		Base *base = static_cast<CraftsState *>(state)->getBase();
		addWidgets(b, state, [state, base](Surface *, size_t row, NodeVtable &v)
		{
			if (row == NO_ROW || row >= base->getCrafts()->size())
				return;
			Craft *c = base->getCrafts()->at(row);
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([state, c] { return craftsRow(state, c); }, false, AnnouncementKinds::Label));
			// Only heard if the screen is still up after Enter.
			v.StateText = [c]
			{
				return c->getStatus() == "STR_OUT" ? Vocab::format(Vocab::CRAFT_OUT, { c->getName(State::getGamePtr()->getLanguage()) }) : std::string();
			};
		});
	};
	return s;
}

/// Craft info, wired explicitly: the weapons, crew and equipment are only pictures on screen.
/// Arrival says the craft, its status, damage and fuel (the game's lines, with repair and refuel times).
AccessScreen craftInfo()
{
	AccessScreen s;
	s.key = "craftInfo";
	s.isActive = is<CraftInfoState>;
	s.name = [](State *state)
	{
		CraftInfoState *ci = static_cast<CraftInfoState *>(state);
		Craft *c = ci->getCraft();
		return c->getName(State::getGamePtr()->getLanguage()) + ". " + std::string(state->tr(c->getStatus())) + ". " +
			ci->getDamageText()->getText() + ". " + ci->getFuelText()->getText() + ".";
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		CraftInfoState *ci = static_cast<CraftInfoState *>(state);
		Craft *c = ci->getCraft();
		b.AddItem(ControlId::Referenced(ci->getNameEdit(), "name"), Controls::textEdit(state, ci->getNameEdit()));
		{
			NodeVtable v;
			v.Announcements.push_back(NodeAnnouncement([state, ci, c]
			{
				return std::string(state->tr(c->getStatus())) + ", " + ci->getDamageText()->getText() + ", " + ci->getFuelText()->getText();
			}, false, AnnouncementKinds::Label));
			b.AddItem(ControlId::Referenced(c, "status"), v);
		}
		for (int i = 0; i < 2 && i < (int)c->getRules()->getWeapons(); ++i)
		{
			CraftWeapon *w = i < (int)c->getWeapons()->size() ? c->getWeapons()->at(i) : 0;
			std::string n = std::to_string(i + 1);
			std::string label = w ? Vocab::format(Vocab::CRAFT_WEAPON, { n, state->tr(w->getRules()->getType()),
					std::to_string(w->getAmmo()), std::to_string(w->getRules()->getAmmoMax()) })
				: Vocab::format(Vocab::CRAFT_WEAPON_NONE, { n });
			TextButton *btn = ci->getWeaponButton(i);
			b.AddItem(ControlId::Referenced(btn, "weapon:" + n), Controls::labelledButton(state, btn, label));
		}
		if (c->getRules()->getSoldiers() > 0)
		{
			TextButton *crew = ci->getCrewButton(), *equip = ci->getEquipButton(), *armor = ci->getArmorButton();
			b.AddItem(ControlId::Referenced(crew, "crew"), Controls::labelledButton(state, crew, Vocab::format(Vocab::CRAFT_CREW,
				{ crew->getText(), std::to_string(c->getNumSoldiers()), std::to_string(c->getSpaceAvailable()) })));
			b.AddItem(ControlId::Referenced(equip, "equip"), Controls::labelledButton(state, equip, Vocab::format(Vocab::CRAFT_EQUIPMENT,
				{ equip->getText(), std::to_string(c->getNumVehicles()), std::to_string(c->getNumEquipment()) })));
			b.AddItem(ControlId::Referenced(armor, "armor"), Controls::textButton(state, armor));
		}
		b.AddItem(ControlId::Referenced(ci->getOkButton(), "ok"), Controls::textButton(state, ci->getOkButton()));
	};
	return s;
}

/// A table row with its columns named: "Pistol, QUANTITY: 2, SPACE USED: 1". The first cell names the row;
/// headers are the game's string ids for the other columns, in order.
std::string headedRow(State *state, TextList *list, size_t row, const std::vector<std::string> &headers)
{
	std::string s = Controls::cellText(list, row, 0);
	for (size_t i = 0; i < headers.size() && i + 1 < list->getCellCount(row); ++i)
		s += ", " + Vocab::format(Vocab::HEADED_CELL, { std::string(state->tr(headers[i])), Controls::cellText(list, row, i + 1) });
	return s;
}

/// A screen with one table: says its title, then lists the rows with their columns named, and the buttons.
AccessScreen tableScreen(const std::string &key, std::function<bool(State *)> isActive, const std::vector<std::string> &headers)
{
	AccessScreen s = simpleScreen(key, isActive);
	s.name = firstText;
	s.build = [headers](GraphBuilder &b, State *state)
	{
		addWidgets(b, state, [state, headers](Surface *surface, size_t row, NodeVtable &v)
		{
			TextList *list = dynamic_cast<TextList *>(surface);
			if (!list || row == NO_ROW)
				return;
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([state, list, row, headers] { return headedRow(state, list, row, headers); }, false, AnnouncementKinds::Label));
		});
	};
	return s;
}

/// "5:7" as the base info screen writes its counts, said "5 of 7".
std::string ofText(const std::string &value)
{
	size_t colon = value.find(':');
	if (colon == std::string::npos)
		return value;
	return Vocab::format(Vocab::VALUE_OF, { value.substr(0, colon), value.substr(colon + 1) });
}

/// Base information, wired explicitly: the base name on arrival, then the personnel, space and
/// defense lines (one context each), then the name field and the buttons. Its number keys switch
/// bases like the Basescape's, and the tick says the new one.
AccessScreen baseInfo()
{
	static Base *shown = 0;
	AccessScreen s;
	s.key = "baseInfo";
	s.isActive = is<BaseInfoState>;
	s.name = [](State *state)
	{
		shown = static_cast<BaseInfoState *>(state)->getBase();
		return shown->getName();
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		BaseInfoState *bi = static_cast<BaseInfoState *>(state);
		bool open = false;
		for (const std::pair<Text *, Text *> &line : bi->getLines())
		{
			Text *label = line.first, *value = line.second;
			if (!value)
			{
				if (open)
					b.PopContext();
				b.PushContext(label->getText());
				open = true;
				continue;
			}
			addLine(b, ControlId::Referenced(label, "line:" + label->getText()), [label, value]
			{
				return label->getText() + ", " + ofText(value->getText());
			});
		}
		if (open)
			b.PopContext();
		addWidgets(b, state);
	};
	s.tick = [](State *state)
	{
		Base *base = static_cast<BaseInfoState *>(state)->getBase();
		if (base == shown)
			return;
		shown = base;
		Speech::say(base->getName(), true);
	};
	return s;
}

/// Monthly costs: the title and income on arrival, then craft rental and salaries with their
/// columns named, then maintenance and the total.
AccessScreen monthlyCosts()
{
	AccessScreen s;
	s.key = "monthlyCosts";
	s.isActive = is<MonthlyCostsState>;
	s.name = [](State *state)
	{
		return firstText(state) + ". " + static_cast<MonthlyCostsState *>(state)->getIncomeText()->getText();
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		MonthlyCostsState *mc = static_cast<MonthlyCostsState *>(state);
		std::vector<std::string> headers = { "STR_COST_PER_UNIT", "STR_QUANTITY", "STR_TOTAL" };
		std::pair<const char *, TextList *> tables[] = { { "STR_CRAFT_RENTAL", mc->getCraftList() }, { "STR_SALARIES", mc->getSalaryList() } };
		for (const std::pair<const char *, TextList *> &t : tables)
		{
			TextList *list = t.second;
			b.PushContext(state->tr(t.first));
			for (size_t row = 0; row < list->getTexts(); ++row)
				addLine(b, ControlId::Referenced(list, std::string(t.first) + ":" + std::to_string(row)), [state, list, row, headers] { return headedRow(state, list, row, headers); });
			b.PopContext();
		}
		TextList *rest[] = { mc->getMaintenanceList(), mc->getTotalList() };
		for (size_t i = 0; i < 2; ++i)
		{
			TextList *list = rest[i];
			for (size_t row = 0; row < list->getTexts(); ++row)
				addLine(b, ControlId::Referenced(list, "rest:" + std::to_string(i) + ":" + std::to_string(row)), [list, row] { return Controls::rowText(list, row); });
		}
		b.AddItem(ControlId::Referenced(mc->getOkButton(), "ok"), Controls::textButton(state, mc->getOkButton()));
	};
	return s;
}

/// Current research: the scientists and lab space on arrival, then the projects with their columns named
/// (Enter opens one to change its scientists), New Project and OK.
AccessScreen research()
{
	AccessScreen s = tableScreen("research", is<ResearchState>, { "STR_SCIENTISTS_ALLOCATED_UC", "STR_PROGRESS" });
	s.name = [](State *state)
	{
		Base *base = static_cast<ResearchState *>(state)->getBase();
		return firstText(state) + ". " +
			std::string(state->tr("STR_SCIENTISTS_AVAILABLE").arg(base->getAvailableScientists())) + ". " +
			std::string(state->tr("STR_SCIENTISTS_ALLOCATED").arg(base->getAllocatedScientists())) + ". " +
			std::string(state->tr("STR_LABORATORY_SPACE_AVAILABLE").arg(base->getFreeLaboratories())) + ".";
	};
	return s;
}

/// A research project's scientists: allocated, then what's free, in the game's words.
std::string scientistsText(ResearchInfoState *ri)
{
	Base *base = ri->getBase();
	return std::string(ri->tr("STR_SCIENTISTS_ALLOCATED").arg(ri->getProject()->getAssigned())) + ", " +
		std::string(ri->tr("STR_SCIENTISTS_AVAILABLE_UC").arg(base->getAvailableScientists())) + ", " +
		std::string(ri->tr("STR_LABORATORY_SPACE_AVAILABLE_UC").arg(base->getFreeLaboratories()));
}

/// Allocating scientists to a project: the project and the counts on arrival, then the scientists
/// as one adjustable node (Left/Right one, Shift five, Ctrl all), then OK (or Start Project) and Cancel.
AccessScreen researchInfo()
{
	AccessScreen s;
	s.key = "researchInfo";
	s.isActive = is<ResearchInfoState>;
	s.name = [](State *state)
	{
		return firstText(state) + ". " + scientistsText(static_cast<ResearchInfoState *>(state));
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		ResearchInfoState *ri = static_cast<ResearchInfoState *>(state);
		NodeVtable v;
		v.Type = &Controls::sliderType();
		v.Announcements.push_back(NodeAnnouncement([ri]
		{
			return std::string(ri->tr("STR_SCIENTISTS_ALLOCATED").arg(ri->getProject()->getAssigned()));
		}, false, AnnouncementKinds::Label));
		v.OnAdjust = [ri](int sign, bool large)
		{
			int count = std::abs(sign) >= Controls::ADJUST_LIMIT ? INT_MAX : (large ? 5 : 1);
			if (sign < 0)
				ri->lessByValue(count);
			else
				ri->moreByValue(count);
		};
		v.StateText = [ri] { return scientistsText(ri); };
		b.AddItem(ControlId::Referenced(ri->getProject(), "scientists"), v);
		b.AddItem(ControlId::Referenced(ri->getOkButton(), "ok"), Controls::textButton(state, ri->getOkButton()));
		b.AddItem(ControlId::Referenced(ri->getCancelButton(), "cancel"), Controls::textButton(state, ri->getCancelButton()));
	};
	return s;
}

AccessScreen newPossibleResearch()
{
	AccessScreen s = popupScreen("newPossibleResearch", is<NewPossibleResearchState>);
	// The game shows this even when nothing is new, with no title and an empty list.
	s.name = [](State *state)
	{
		std::string text = allTextWithLists(state);
		return text.empty() ? Vocab::get(Vocab::NO_NEW_RESEARCH) : text;
	};
	return s;
}

const std::vector<AccessScreen> &all()
{
	static const std::vector<AccessScreen> screens = {
		mainMenu(), newBattle(), briefing(), inventory(), nextTurn(),
		craftInfo(),
		simpleScreen("craftSoldiers", is<CraftSoldiersState>),
		craftEquipment(),
		simpleScreen("craftArmor", is<CraftArmorState>),
		simpleScreen("soldierArmor", is<SoldierArmorState>),
		soldierInfo(),
		actionMenu(), primeGrenade(), medikit(), debriefing(),
		simpleScreen("pause", is<PauseState>),
		simpleScreen("abandonGame", is<AbandonGameState>),
		simpleScreen("abortMission", is<AbortMissionState>),
		simpleScreen("listLoad", is<ListLoadState>),
		simpleScreen("listSave", is<ListSaveState>),
		simpleScreen("deleteGame", is<DeleteGameState>),
		simpleScreen("confirmLoad", is<ConfirmLoadState>),
		simpleScreen("errorMessage", is<ErrorMessageState>),
		newGame(),
		buildNewBase(),
		simpleScreen("baseName", is<BaseNameState>),
		simpleScreen("confirmNewBase", is<ConfirmNewBaseState>),
		popupScreen("ufoDetected", is<UfoDetectedState>),
		popupScreen("ufoLost", is<UfoLostState>),
		popupScreen("missionDetected", is<MissionDetectedState>),
		popupScreen("alienBase", is<AlienBaseState>),
		popupScreen("craftPatrol", is<CraftPatrolState>),
		popupScreen("lowFuel", is<LowFuelState>),
		popupScreen("craftError", is<CraftErrorState>),
		popupScreen("researchComplete", is<ResearchCompleteState>),
		popupScreen("researchRequired", is<ResearchRequiredState>),
		newPossibleResearch(),
		popupScreen("newPossibleManufacture", is<NewPossibleManufactureState>),
		popupScreen("productionComplete", is<ProductionCompleteState>),
		popupScreen("itemsArriving", is<ItemsArrivingState>),
		popupScreen("multipleTargets", is<MultipleTargetsState>),
		popupScreen("targetInfo", is<TargetInfoState>),
		popupScreen("geoscapeCraft", is<GeoscapeCraftState>),
		popupScreen("confirmDestination", is<ConfirmDestinationState>),
		simpleScreen("ufopaediaStart", is<UfopaediaStartState>),
		intercept(),
		selectDestination(),
		Dogfight::screen(),
		popupScreen("confirmLanding", is<ConfirmLandingState>),
		popupScreen("aliensCrash", is<AliensCrashState>),
		popupScreen("baseDestroyed", is<BaseDestroyedState>),
		popupScreen("confirmCydonia", is<ConfirmCydoniaState>),
		popupScreen("dogfightError", is<DogfightErrorState>),
		simpleScreen("ufopaediaSelect", is<UfopaediaSelectState>),
		basescape(),
		crafts(),
		baseInfo(),
		monthlyCosts(),
		tableScreen("stores", is<StoresState>, { "STR_QUANTITY_UC", "STR_SPACE_USED_UC" }),
		tableScreen("transfers", is<TransfersState>, { "STR_QUANTITY_UC", "STR_ARRIVAL_TIME_HOURS" }),
		tableScreen("soldiers", is<SoldiersState>, {}),
		research(),
		tableScreen("newResearchList", is<NewResearchListState>, {}),
		researchInfo(),
	};
	return screens;
}

}

}
