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
#include <memory>
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
#include "../Menu/ListGamesState.h"
#include "../Menu/DeleteGameState.h"
#include "../Menu/ConfirmLoadState.h"
#include "../Menu/ErrorMessageState.h"
#include "../Menu/NewGameState.h"
#include "../Menu/OptionsBaseState.h"
#include "../Menu/OptionsVideoState.h"
#include "../Menu/OptionsAudioState.h"
#include "../Menu/OptionsNoAudioState.h"
#include "../Menu/OptionsControlsState.h"
#include "../Menu/OptionsGeoscapeState.h"
#include "../Menu/OptionsBattlescapeState.h"
#include "../Menu/OptionsAdvancedState.h"
#include "../Menu/OptionsFoldersState.h"
#include "../Menu/OptionsDefaultsState.h"
#include "../Menu/OptionsConfirmState.h"
#include "../Engine/OptionInfo.h"
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
#include "../Geoscape/NewPossiblePurchaseState.h"
#include "../Geoscape/NewPossibleCraftState.h"
#include "../Geoscape/NewPossibleFacilityState.h"
#include "../Geoscape/ProductionCompleteState.h"
#include "../Geoscape/ResearchCompleteState.h"
#include "../Geoscape/ResearchRequiredState.h"
#include "../Geoscape/TargetInfoState.h"
#include "../Geoscape/UfoDetectedState.h"
#include "../Geoscape/UfoLostState.h"
#include "../Ufopaedia/UfopaediaStartState.h"
#include "../Ufopaedia/UfopaediaSelectState.h"
#include "../Ufopaedia/ArticleStateArmor.h"
#include "../Ufopaedia/ArticleStateBaseFacility.h"
#include "../Ufopaedia/ArticleStateCraft.h"
#include "../Ufopaedia/ArticleStateCraftWeapon.h"
#include "../Ufopaedia/ArticleStateItem.h"
#include "../Ufopaedia/ArticleStateText.h"
#include "../Ufopaedia/ArticleStateTextImage.h"
#include "../Ufopaedia/ArticleStateUfo.h"
#include "../Ufopaedia/ArticleStateVehicle.h"
#include "../Mod/City.h"
#include "../Mod/RuleRegion.h"
#include "../Savegame/Region.h"
#include "../Engine/Unicode.h"
#include "../fmath.h"
#include "Geo.h"
#include "Dogfight.h"
#include "../Battlescape/AliensCrashState.h"
#include "../Geoscape/BaseDefenseState.h"
#include "../Geoscape/BaseDestroyedState.h"
#include "../Geoscape/ConfirmCydoniaState.h"
#include "../Geoscape/ConfirmLandingState.h"
#include "../Geoscape/DogfightErrorState.h"
#include "../Geoscape/InterceptState.h"
#include "../Geoscape/SelectDestinationState.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Soldier.h"
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
#include "../Basescape/PurchaseState.h"
#include "../Basescape/SellState.h"
#include "../Basescape/TransferBaseState.h"
#include "../Basescape/TransferConfirmState.h"
#include "../Basescape/TransferItemsState.h"
#include "../Basescape/ManageAlienContainmentState.h"
#include "../Basescape/PlaceLiftState.h"
#include "../Basescape/BuildFacilitiesState.h"
#include "../Basescape/PlaceFacilityState.h"
#include "../Basescape/DismantleFacilityState.h"
#include "../Basescape/ManufactureState.h"
#include "../Basescape/NewManufactureListState.h"
#include "../Basescape/ManufactureStartState.h"
#include "../Basescape/ManufactureInfoState.h"
#include "../Basescape/SackSoldierState.h"
#include "../Basescape/SoldierMemorialState.h"
#include "../Savegame/Production.h"
#include "../Savegame/ItemContainer.h"
#include "../Mod/RuleManufacture.h"
#include "../Basescape/BaseView.h"
#include "../Savegame/BaseFacility.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Savegame/ResearchProject.h"
#include "../Basescape/CraftInfoState.h"
#include "../Basescape/CraftsState.h"
#include "../Mod/RuleCraft.h"
#include "../Mod/RuleCraftWeapon.h"
#include "../Savegame/CraftWeapon.h"
#include "../Basescape/CraftSoldiersState.h"
#include "../Basescape/CraftWeaponsState.h"
#include "../Basescape/SoldierArmorState.h"
#include "../Basescape/SoldierInfoState.h"
#include "../Engine/Action.h"
#include "../Battlescape/ActionMenuItem.h"
#include "../Battlescape/ActionMenuState.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BriefingState.h"
#include "../Battlescape/DebriefingState.h"
#include "../Battlescape/PromotionsState.h"
#include "../Battlescape/CommendationState.h"
#include "../Battlescape/CommendationLateState.h"
#include "../Battlescape/CannotReequipState.h"
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
#include "Navigator.h"
#include "Speech.h"
#include "../Battlescape/NextTurnState.h"
#include "../Battlescape/InfoboxState.h"
#include "../Battlescape/InfoboxOKState.h"
#include "../Battlescape/ConfirmEndMissionState.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "Vocab.h"

namespace OpenXcom
{

/**
 * Reads a Ufopaedia article: a friend of each (UFO Defense) article class, since their
 * fields are protected and their layouts differ. Stats come out as "label: value" lines.
 */
struct ArticleAccess
{
	/// A stats list's rows as "label: value"; the blank spacer rows are skipped.
	static void listLines(TextList *list, std::vector<std::string> &out)
	{
		for (size_t row = 0; row < list->getTexts(); ++row)
		{
			std::string label = list->getCellCount(row) > 0 ? Controls::cellText(list, row, 0) : std::string();
			std::string rest;
			for (size_t c = 1; c < list->getCellCount(row); ++c)
			{
				std::string cell = Controls::cellText(list, row, c);
				if (!cell.empty())
					rest += (rest.empty() ? "" : ", ") + cell;
			}
			if (label.empty() && rest.empty())
				continue;
			out.push_back(rest.empty() ? label : Vocab::format(Vocab::HEADED_CELL, { label, rest }));
		}
	}

	/// The shot table ("Auto Shot, ACCURACY: 35%, TIME UNIT COST: 35%"), then each ammo's damage
	/// ("Rifle Clip, DAMAGE: Armor Piercing, 30"). Ammo is only pictures on screen, so it's named here;
	/// the game leaves ammo whose article isn't available blank, and so does this.
	static void itemLines(ArticleStateItem *a, std::vector<std::string> &out)
	{
		Mod *mod = State::getGamePtr()->getMod();
		RuleItem *item = mod->getItem(a->getId(), true);
		if (item->getBattleType() == BT_FIREARM)
		{
			for (size_t row = 0; row < a->_lstInfo->getTexts(); ++row)
			{
				if (a->_lstInfo->getCellCount(row) < 3)
					continue;
				out.push_back(Controls::cellText(a->_lstInfo, row, 0) + ", " +
					Vocab::format(Vocab::HEADED_CELL, { a->_txtAccuracy->getText(), Controls::cellText(a->_lstInfo, row, 1) }) + ", " +
					Vocab::format(Vocab::HEADED_CELL, { a->_txtTuCost->getText(), Controls::cellText(a->_lstInfo, row, 2) }));
			}
		}
		const std::vector<const RuleItem *> *ammo = item->getCompatibleAmmoForSlot(0);
		for (size_t i = 0; i < 3; ++i)
		{
			std::string type = a->_txtAmmoType[i]->getText(), power = a->_txtAmmoDamage[i]->getText();
			if (type.empty())
				continue;
			std::string line = Vocab::format(Vocab::HEADED_CELL, { std::string(a->tr("STR_DAMAGE_UC")), type + ", " + power });
			if (item->getBattleType() == BT_FIREARM && i < ammo->size())
				line = std::string(a->tr((*ammo)[i]->getName())) + ", " + line;
			out.push_back(line);
		}
	}

	/// Splits a block of lines (the craft article's stats).
	static void textLines(Text *text, std::vector<std::string> &out)
	{
		std::string s = text->getText();
		size_t start = 0;
		while (start <= s.size())
		{
			size_t end = s.find('\n', start);
			if (end == std::string::npos)
				end = s.size();
			std::string line = s.substr(start, end - start);
			if (!line.empty())
				out.push_back(line);
			start = end + 1;
		}
	}

	/// The article's title, stat lines and description. False for article kinds it doesn't know (TFTD's).
	static bool read(State *state, std::string &title, std::vector<std::string> &stats, std::string &info)
	{
		if (ArticleStateCraft *a = dynamic_cast<ArticleStateCraft *>(state))
		{
			title = a->_txtTitle->getText(); info = a->_txtInfo->getText();
			textLines(a->_txtStats, stats);
		}
		else if (ArticleStateCraftWeapon *a = dynamic_cast<ArticleStateCraftWeapon *>(state))
		{
			title = a->_txtTitle->getText(); info = a->_txtInfo->getText();
			listLines(a->_lstInfo, stats);
		}
		else if (ArticleStateItem *a = dynamic_cast<ArticleStateItem *>(state))
		{
			title = a->_txtTitle->getText(); info = a->_txtInfo->getText();
			itemLines(a, stats);
		}
		else if (ArticleStateArmor *a = dynamic_cast<ArticleStateArmor *>(state))
		{
			title = a->_txtTitle->getText(); info = a->_txtInfo->getText();
			listLines(a->_lstInfo, stats);
		}
		else if (ArticleStateBaseFacility *a = dynamic_cast<ArticleStateBaseFacility *>(state))
		{
			title = a->_txtTitle->getText(); info = a->_txtInfo->getText();
			listLines(a->_lstInfo, stats);
		}
		else if (ArticleStateUfo *a = dynamic_cast<ArticleStateUfo *>(state))
		{
			title = a->_txtTitle->getText(); info = a->_txtInfo->getText();
			listLines(a->_lstInfo, stats);
		}
		else if (ArticleStateVehicle *a = dynamic_cast<ArticleStateVehicle *>(state))
		{
			title = a->_txtTitle->getText(); info = a->_txtInfo->getText();
			listLines(a->_lstStats, stats);
		}
		else if (ArticleStateText *a = dynamic_cast<ArticleStateText *>(state))
		{
			title = a->_txtTitle->getText(); info = a->_txtInfo->getText();
		}
		else if (ArticleStateTextImage *a = dynamic_cast<ArticleStateTextImage *>(state))
		{
			title = a->_txtTitle->getText(); info = a->_txtInfo->getText();
		}
		else
		{
			return false;
		}
		return true;
	}
};

/**
 * Reads the options screens: a friend of OptionsBaseState and each category, since their widgets
 * are private and the two-column layout (categories on the left, settings in two columns) would
 * scramble the positional heuristics.
 */
struct OptionsAccess
{
	/// A heading and what sits under it: combo boxes, sliders, toggles and text lines.
	struct Section
	{
		Text *heading;
		std::vector<Surface *> widgets;
	};

	static OptionsBaseState *base(State *state)
	{
		return static_cast<OptionsBaseState *>(state);
	}

	static std::vector<TextButton *> categories(State *state)
	{
		OptionsBaseState *o = base(state);
		return { o->_btnVideo, o->_btnAudio, o->_btnControls, o->_btnGeoscape, o->_btnBattlescape, o->_btnAdvanced, o->_btnFolders };
	}

	static TextButton *category(State *state)
	{
		return base(state)->_group;
	}

	static std::vector<TextButton *> buttons(State *state)
	{
		OptionsBaseState *o = base(state);
		return { o->_btnOk, o->_btnCancel, o->_btnDefault };
	}

	/// The resolution fields and arrows of the video options; false on other categories.
	static bool resolution(State *state, Text *&heading, TextEdit *&width, TextEdit *&height, ArrowButton *&bigger, ArrowButton *&smaller)
	{
		OptionsVideoState *v = dynamic_cast<OptionsVideoState *>(state);
		if (!v)
			return false;
		heading = v->_txtDisplayResolution;
		width = v->_txtDisplayWidth;
		height = v->_txtDisplayHeight;
		bigger = v->_btnDisplayResolutionUp;
		smaller = v->_btnDisplayResolutionDown;
		return true;
	}

	/// The category's settings in reading order, left column first: each column runs top to bottom.
	static std::vector<Section> sections(State *state)
	{
		if (OptionsVideoState *v = dynamic_cast<OptionsVideoState *>(state))
		{
			return {
				{ v->_txtLanguage, { v->_cbxLanguage } },
				{ v->_txtGeoScale, { v->_cbxGeoScale } },
				{ v->_txtBattleScale, { v->_cbxBattleScale } },
				{ v->_txtMode, { v->_cbxDisplayMode } },
				{ v->_txtFilter, { v->_cbxFilter } },
				{ v->_txtOptions, { v->_btnLetterbox, v->_btnLockMouse, v->_btnRootWindowedMode } },
			};
		}
		if (OptionsAudioState *a = dynamic_cast<OptionsAudioState *>(state))
		{
			return {
				{ a->_txtMusicVolume, { a->_slrMusicVolume } },
				{ a->_txtSoundVolume, { a->_slrSoundVolume } },
				{ a->_txtUiVolume, { a->_slrUiVolume } },
				{ a->_txtOptions, { a->_btnBackgroundMute } },
				{ a->_txtVideoFormat, { a->_cbxVideoFormat } },
				{ a->_txtMusicFormat, { a->_cbxMusicFormat, a->_txtCurrentMusic } },
				{ a->_txtSoundFormat, { a->_cbxSoundFormat, a->_txtCurrentSound } },
			};
		}
		if (OptionsGeoscapeState *g = dynamic_cast<OptionsGeoscapeState *>(state))
		{
			return {
				{ g->_txtScrollSpeed, { g->_slrScrollSpeed } },
				{ g->_txtClockSpeed, { g->_slrClockSpeed } },
				{ g->_txtGlobeDetails, { g->_btnGlobeCountries, g->_btnGlobeRadars, g->_btnGlobePaths } },
				{ g->_txtDragScroll, { g->_cbxDragScroll } },
				{ g->_txtDogfightSpeed, { g->_slrDogfightSpeed } },
				{ g->_txtOptions, { g->_btnShowFunds } },
			};
		}
		if (OptionsBattlescapeState *b = dynamic_cast<OptionsBattlescapeState *>(state))
		{
			return {
				{ b->_txtEdgeScroll, { b->_cbxEdgeScroll } },
				{ b->_txtScrollSpeed, { b->_slrScrollSpeed } },
				{ b->_txtXcomSpeed, { b->_slrXcomSpeed } },
				{ b->_txtPathPreview, { b->_btnArrows, b->_btnTuCost, b->_btnEnergyCost } },
				{ b->_txtDragScroll, { b->_cbxDragScroll } },
				{ b->_txtFireSpeed, { b->_slrFireSpeed } },
				{ b->_txtAlienSpeed, { b->_slrAlienSpeed } },
				{ b->_txtOptions, { b->_btnTooltips, b->_btnDeaths } },
			};
		}
		if (OptionsFoldersState *f = dynamic_cast<OptionsFoldersState *>(state))
		{
			return {
				{ f->_txtDataFolder, { f->_txtDataFolderPath1, f->_txtDataFolderPath2 } },
				{ f->_txtUserFolder, { f->_txtUserFolderPath } },
				{ f->_txtSaveFolder, { f->_txtSaveFolderPath } },
				{ f->_txtConfigFolder, { f->_txtConfigFolderPath } },
			};
		}
		if (OptionsNoAudioState *n = dynamic_cast<OptionsNoAudioState *>(state))
		{
			return { { 0, { n->_txtError } } };
		}
		return {};
	}

	/// The OXC and OXCE buttons above the advanced settings and key bindings (a third is hidden).
	static std::vector<TextButton *> owners(State *state)
	{
		if (OptionsAdvancedState *a = dynamic_cast<OptionsAdvancedState *>(state))
			return { a->_btnOXC, a->_btnOXCE, a->_btnOTHER };
		if (OptionsControlsState *c = dynamic_cast<OptionsControlsState *>(state))
			return { c->_btnOXC, c->_btnOXCE, c->_btnOTHER };
		return {};
	}

	static TextList *advancedList(State *state)
	{
		return static_cast<OptionsAdvancedState *>(state)->_lstOptions;
	}

	/// The setting on a row of the advanced list; null on the section headings and spacers.
	static OptionInfo *advancedSetting(State *state, size_t row)
	{
		return static_cast<OptionsAdvancedState *>(state)->getSetting(row);
	}

	static TextList *controlsList(State *state)
	{
		return static_cast<OptionsControlsState *>(state)->_lstControls;
	}

	/// The key binding on a row of the controls list; null on the section headings and spacers.
	static OptionInfo *control(State *state, size_t row)
	{
		return static_cast<OptionsControlsState *>(state)->getControl(row);
	}

	/// The row waiting for a new key, or -1.
	static int waitingRow(State *state)
	{
		return static_cast<OptionsControlsState *>(state)->_selected;
	}
};

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

/// Every visible text of a state in reading order, top to bottom then left to right, as sentences,
/// leaving out skip (a hover tooltip).
std::string allTextExcept(State *state, Surface *skip)
{
	// Text edits count too: some screens' titles are editable names.
	std::vector<std::pair<Surface *, std::string> > texts;
	for (Surface *s : state->getSurfaces())
	{
		if (s == skip)
			continue;
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

/// Every visible text of a state in reading order, as sentences. For screens that are just something to read.
std::string allText(State *state)
{
	return allTextExcept(state, 0);
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
/// Tab-stops: each list and each frame is a stop of its own, and the loose widgets between them share one,
/// so Tab skips a long list or a group of settings. Leading loose widgets stay in the recipe's current stop,
/// and anything the recipe adds after a list or frame starts a fresh stop.
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
	// The list or frame whose stop is being filled; null for loose widgets.
	const Surface *group = 0;
	bool first = true;
	for (const Widget &w : widgets)
	{
		const Surface *g = dynamic_cast<TextList *>(w.surface) ? w.surface : (const Surface *)w.frame;
		if (first ? g != 0 : g != group)
			b.BeginStop("widgets:" + std::to_string(w.index));
		group = g;
		first = false;
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
			// A customizer clears the announcements to leave a widget out (the recipe wires it itself).
			if (!v.Announcements.empty())
				b.AddItem(id, v);
		}
		else if (TextList *list = dynamic_cast<TextList *>(w.surface))
		{
			b.PushContext(Vocab::get(Vocab::LIST));
			if (list->getTexts() == 0)
			{
				NodeVtable empty;
				empty.Announcements.push_back(NodeAnnouncement([] { return Vocab::get(Vocab::LIST_EMPTY); }, false, AnnouncementKinds::Label));
				b.AddItem(ControlId::Referenced(list, "widget:" + std::to_string(w.index) + ":empty"), empty);
				b.PopContext();
				continue;
			}
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
	if (group)
		b.BeginStop("widgets:end");
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

/// The save list's order: "A to Z", "newest first".
std::string saveOrderText()
{
	switch (Options::saveOrder)
	{
	case SORT_NAME_ASC: return Vocab::get(Vocab::SAVES_A_TO_Z);
	case SORT_NAME_DESC: return Vocab::get(Vocab::SAVES_Z_TO_A);
	case SORT_DATE_ASC: return Vocab::get(Vocab::SAVES_OLDEST_FIRST);
	default: return Vocab::get(Vocab::SAVES_NEWEST_FIRST);
	}
}

/// The load and save lists: "sort by name" and "sort by date" first, the active one saying the order
/// (Enter sorts by it, again to reverse), then the saves and the buttons. The game hides the inactive
/// sort arrow by giving it no shape, so the generic lister can't be trusted with them.
AccessScreen listGames(const std::string &key, std::function<bool(State *)> isActive)
{
	AccessScreen s = simpleScreen(key, isActive);
	s.build = [](GraphBuilder &b, State *state)
	{
		ListGamesState *lg = static_cast<ListGamesState *>(state);
		ArrowButton *byName = lg->getSortNameButton(), *byDate = lg->getSortDateButton();
		if (lg->isSortable())
		{
			std::pair<ArrowButton *, const char *> sorts[] = { { byName, "STR_NAME" }, { byDate, "STR_DATE" } };
			for (const std::pair<ArrowButton *, const char *> &sort : sorts)
			{
				ArrowButton *btn = sort.first;
				NodeVtable v = Controls::labelledButton(state, btn, Vocab::format(Vocab::SAVES_SORT_BY, { std::string(state->tr(sort.second)) }));
				std::function<std::string()> order = [btn] { return btn->getShape() != ARROW_NONE ? saveOrderText() : std::string(); };
				v.Announcements.push_back(NodeAnnouncement(order, false, "pressed"));
				v.StateText = order;
				b.AddItem(ControlId::Referenced(btn, std::string("sort:") + sort.second), v);
			}
		}
		addWidgets(b, state, [byName, byDate](Surface *surface, size_t, NodeVtable &v)
		{
			if (surface == byName || surface == byDate)
				v.Announcements.clear();
		});
	};
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
	if (item->isWeaponWithAmmo())
	{
		BattleItem *ammo = item->getAmmoForSlot(0);
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
			if (target->getFatalWound((UnitBodyPart)i))
				wounded.push_back(std::string(state->tr(BODY_PARTS[i])) + " " + std::to_string(target->getFatalWound((UnitBodyPart)i)));
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
				std::string text = woundsText(target->getFatalWound((UnitBodyPart)i));
				if (view->getSelectedPart() == i)
					text += ", " + Vocab::get(Vocab::SELECTED);
				return text;
			}, false, AnnouncementKinds::Value));
			v.OnActivate = [view, i] { view->setSelectedPart(i); };
			v.StateText = [part] { return Vocab::format(Vocab::PART_SELECTED, { part }); };
			b.AddItem(ControlId::Referenced(view, "part:" + std::to_string(i)), v);
		}
		BattleItem *item = m->getItem();
		b.BeginStop("treatments");
		b.AddItem(ControlId::Referenced(m->getHealButton(), "heal"), medikitButton(state, m->getHealButton(), state->tr("STR_HEAL"),
			[state, view, item, cost]
			{
				int part = view->getSelectedPart();
				return joinParts({ part >= 0 ? std::string(state->tr(BODY_PARTS[part])) : std::string(), Vocab::format(Vocab::ITEMS_LEFT, { std::to_string(item->getHealQuantity()) }), cost });
			},
			[m, state, target, view, item]
			{
				int part = view->getSelectedPart();
				std::string where = part >= 0 ? std::string(state->tr(BODY_PARTS[part])) + ": " + woundsText(target->getFatalWound((UnitBodyPart)part)) : std::string();
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

/// How one arrow-row screen works: the list's rows have little arrows that add to or take from an order.
struct ArrowRows
{
	/// Changes the selected row's order: sign > 0 adds, count is how many (INT_MAX for as many as possible).
	std::function<void(State *, int sign, int count)> change;
	/// Says a row, its columns named.
	std::function<std::string(TextList *, size_t)> row;
	/// The lines that change with the order (cost, space), or null.
	std::function<std::string(State *)> totals;
	/// The category filter, or null.
	std::function<ComboBox *(State *)> category;
	Vocab::Id hint;
};

/// A list whose rows are changed with Left/Right (Shift five, Ctrl as many as possible), the way the
/// rows' arrows do. Each change says the row again, then the totals. Right always adds, whichever way
/// the screen's arrows point. Uses the states' public by-value methods; the rows' arrows aren't widgets.
AccessScreen arrowRowScreen(const std::string &key, std::function<bool(State *)> isActive, const ArrowRows &rows)
{
	AccessScreen s = simpleScreen(key, isActive);
	s.build = [rows](GraphBuilder &b, State *state)
	{
		addWidgets(b, state, [state, rows](Surface *surface, size_t row, NodeVtable &v)
		{
			if (ComboBox *box = dynamic_cast<ComboBox *>(surface))
			{
				// The heuristic would label it with the funds line above it.
				if (rows.category && rows.category(state) == box)
					v = Controls::comboBox(state, box, Vocab::get(Vocab::CATEGORY));
				return;
			}
			TextList *list = dynamic_cast<TextList *>(surface);
			if (!list || row == NO_ROW)
				return;
			v.OnAdjust = [state, list, row, rows](int sign, bool large)
			{
				// A left press on the row is how the screen learns which row the arrows act on.
				Controls::clickRow(state, list, row);
				// Ctrl is the arrows' right click: as many as possible.
				int count = std::abs(sign) >= Controls::ADJUST_LIMIT ? INT_MAX : (large ? 5 : 1);
				rows.change(state, sign, count);
			};
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([list, row, rows] { return rows.row(list, row); }, false, AnnouncementKinds::Label));
			// The game ignores clicks on the row itself, so Enter re-reads it with how to change it.
			static bool hint = false;
			v.OnActivate = [] { hint = true; };
			v.StateText = [state, list, row, rows]
			{
				std::string text = rows.row(list, row);
				if (rows.totals)
				{
					std::string totals = rows.totals(state);
					if (!totals.empty())
						text += ". " + totals;
				}
				if (hint)
					text += ". " + Vocab::get(rows.hint);
				hint = false;
				return text;
			};
		});
	};
	return s;
}

/// The visible ones of some text lines, as sentences.
std::string visibleTexts(const std::vector<Text *> &texts)
{
	std::string result;
	for (Text *t : texts)
	{
		if (!t->getVisible())
			continue;
		std::string line = t->getText();
		if (line.empty())
			continue;
		if (!result.empty())
			result += ". ";
		result += line;
	}
	return result;
}

/// The totals, plus a warning when the screen has hidden its OK button for lack of space.
std::string totalsWithOk(const std::vector<Text *> &texts, TextButton *ok)
{
	std::string text = visibleTexts(texts);
	if (!ok->getVisible())
		text += ". " + Vocab::get(Vocab::NO_ROOM_OK);
	return text;
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

/// A four-column row read through a format: the cells in order.
std::function<std::string(TextList *, size_t)> fourColumnRow(Vocab::Id id)
{
	return [id](TextList *list, size_t row)
	{
		if (list->getCellCount(row) < 4)
			return Controls::rowText(list, row);
		return Vocab::format(id, { Controls::cellText(list, row, 0), Controls::cellText(list, row, 1), Controls::cellText(list, row, 2), Controls::cellText(list, row, 3) });
	};
}

/// Moving items between the base's stores and the craft: Right to the craft, Left back to the stores.
AccessScreen craftEquipment()
{
	ArrowRows rows;
	rows.change = [](State *state, int sign, int count)
	{
		CraftEquipmentState *equip = static_cast<CraftEquipmentState *>(state);
		if (sign < 0)
			equip->moveLeftByValue(count);
		else
			equip->moveRightByValue(count);
	};
	rows.row = equipRowText;
	rows.hint = Vocab::EQUIP_HINT;
	return arrowRowScreen("craftEquipment", is<CraftEquipmentState>, rows);
}

/// Buying: "Pistol, $800 each, 4 in base, buying 2"; each change says the cost of purchases and the stores.
AccessScreen purchase()
{
	ArrowRows rows;
	rows.change = [](State *state, int sign, int count)
	{
		PurchaseState *p = static_cast<PurchaseState *>(state);
		if (sign < 0)
			p->decreaseByValue(count);
		else
			p->increaseByValue(count);
	};
	rows.row = fourColumnRow(Vocab::BUY_ROW);
	rows.totals = [](State *state) { return visibleTexts(static_cast<PurchaseState *>(state)->getTotals()); };
	rows.category = [](State *state) { return static_cast<PurchaseState *>(state)->getCategory(); };
	rows.hint = Vocab::ROW_HINT;
	AccessScreen s = arrowRowScreen("purchase", is<PurchaseState>, rows);
	s.name = [](State *state)
	{
		PurchaseState *p = static_cast<PurchaseState *>(state);
		std::vector<Text *> lines = { p->getFundsText() };
		for (Text *t : p->getTotals())
			lines.push_back(t);
		return firstText(state) + ". " + visibleTexts(lines);
	};
	return s;
}

/// Selling and sacking: "Pistol, 4 in base, selling 2, $560 each"; each change says the sales value and the stores.
AccessScreen sell()
{
	ArrowRows rows;
	rows.change = [](State *state, int sign, int count)
	{
		static_cast<SellState *>(state)->changeByValue(count, sign < 0 ? -1 : 1);
	};
	rows.row = fourColumnRow(Vocab::SELL_ROW);
	rows.totals = [](State *state)
	{
		SellState *sell = static_cast<SellState *>(state);
		return totalsWithOk(sell->getTotals(), sell->getOkButton());
	};
	rows.category = [](State *state) { return static_cast<SellState *>(state)->getCategory(); };
	rows.hint = Vocab::ROW_HINT;
	AccessScreen s = arrowRowScreen("sell", is<SellState>, rows);
	s.name = [rows](State *state)
	{
		SellState *sell = static_cast<SellState *>(state);
		return firstText(state) + ". " + sell->getFundsText()->getText() + ". " + rows.totals(state);
	};
	return s;
}

/// Transferring to another base: "Pistol, 4 here, sending 2, 1 at destination"; each change says the cost.
AccessScreen transferItems()
{
	ArrowRows rows;
	rows.change = [](State *state, int sign, int count)
	{
		TransferItemsState *t = static_cast<TransferItemsState *>(state);
		if (sign < 0)
			t->decreaseByValue(count);
		else
			t->increaseByValue(count);
	};
	rows.row = fourColumnRow(Vocab::TRANSFER_ROW);
	rows.totals = [](State *state)
	{
		return Vocab::format(Vocab::TRANSFER_COST, { Unicode::formatFunding(static_cast<TransferItemsState *>(state)->getTotal()) });
	};
	rows.category = [](State *state) { return static_cast<TransferItemsState *>(state)->getCategory(); };
	rows.hint = Vocab::ROW_HINT;
	AccessScreen s = arrowRowScreen("transferItems", is<TransferItemsState>, rows);
	s.name = firstText;
	return s;
}

/// Containment: "Sectoid Soldier, 3 held, removing 1, under interrogation"; each change says the space.
AccessScreen alienContainment()
{
	ArrowRows rows;
	rows.change = [](State *state, int sign, int count)
	{
		ManageAlienContainmentState *c = static_cast<ManageAlienContainmentState *>(state);
		if (sign < 0)
			c->decreaseByValue(count);
		else
			c->increaseByValue(count);
	};
	rows.row = [](TextList *list, size_t row)
	{
		if (list->getCellCount(row) < 4)
			return Controls::rowText(list, row);
		std::string text = Vocab::format(Vocab::CONTAINMENT_ROW, { Controls::cellText(list, row, 0), Controls::cellText(list, row, 1), Controls::cellText(list, row, 2) });
		if (Controls::cellText(list, row, 3) != "0")
			text += ", " + Vocab::get(Vocab::UNDER_INTERROGATION);
		return text;
	};
	rows.totals = [](State *state)
	{
		ManageAlienContainmentState *c = static_cast<ManageAlienContainmentState *>(state);
		return totalsWithOk(c->getTotals(), c->getOkButton());
	};
	rows.hint = Vocab::ROW_HINT;
	AccessScreen s = arrowRowScreen("alienContainment", is<ManageAlienContainmentState>, rows);
	s.name = [rows](State *state) { return firstText(state) + ". " + rows.totals(state); };
	return s;
}

/// A base square: "Living Quarters, row 2, column 3", or "empty, ...". Facilities still being built say the days left.
std::string baseSquareText(State *state, BaseView *view, int x, int y)
{
	std::string content = Vocab::get(Vocab::LIST_EMPTY);
	if (BaseFacility *fac = view->getFacilityAt(x, y))
	{
		content = state->tr(fac->getRules()->getType());
		if (fac->getBuildTime() > 0)
			content = Vocab::format(Vocab::UNDER_CONSTRUCTION, { content, std::to_string(fac->getBuildTime()) });
		// The hangar's craft, as the Basescape's hover text names it (assigned when the view draws).
		else if (fac->getRules()->getCrafts() > 0 && fac->getCraftForDrawing())
			content += ", " + std::string(state->tr("STR_CRAFT_").arg(fac->getCraftForDrawing()->getName(State::getGamePtr()->getLanguage())));
	}
	return Vocab::format(Vocab::GRID_SQUARE, { content, std::to_string(y + 1), std::to_string(x + 1) });
}

/// What a grid screen adds to the shared grid: more to say about a square, what Enter does there
/// (by default select it and click the view), and Backspace (by default nothing).
struct GridHooks
{
	std::function<std::string(int x, int y)> extra;
	std::function<void(int x, int y)> activate;
	std::function<void(int x, int y)> secondary;
	std::string tooltip;
};

/// The base's 6 by 6 grid as rows of squares: arrows move in two dimensions, Enter clicks the square
/// as the mouse would, after selecting it the way hovering does (which also moves the selector box).
void addBaseGrid(GraphBuilder &b, State *state, BaseView *view, const GridHooks &hooks = GridHooks())
{
	const int size = 6;
	b.PushContext(Vocab::get(Vocab::BASE_GRID));
	for (int y = 0; y < size; ++y)
	{
		b.StartRow("grid");
		for (int x = 0; x < size; ++x)
		{
			NodeVtable v;
			v.Announcements.push_back(NodeAnnouncement([state, view, hooks, x, y]
			{
				std::string text = baseSquareText(state, view, x, y);
				if (hooks.extra)
				{
					std::string more = hooks.extra(x, y);
					if (!more.empty())
						text += ", " + more;
				}
				return text;
			}, false, AnnouncementKinds::Label));
			if (hooks.activate)
			{
				v.OnActivate = [hooks, x, y] { hooks.activate(x, y); };
			}
			else
			{
				v.OnActivate = [state, view, x, y]
				{
					view->selectSquare(x, y);
					Controls::click(state, view);
				};
			}
			if (hooks.secondary)
				v.OnSecondary = [hooks, x, y] { hooks.secondary(x, y); };
			if (!hooks.tooltip.empty())
			{
				std::string tip = hooks.tooltip;
				v.OnTooltip = [tip] { Speech::say(tip, true); };
			}
			b.AddItem(ControlId::Referenced(view, "square:" + std::to_string(x) + ":" + std::to_string(y)), v);
		}
		b.EndRow();
	}
	b.PopContext();
}

/// Placing a new base's access lift: the title, then the grid. Enter on any square places it there.
AccessScreen placeLift()
{
	AccessScreen s;
	s.key = "placeLift";
	s.isActive = is<PlaceLiftState>;
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state) { addBaseGrid(b, state, static_cast<PlaceLiftState *>(state)->getView()); };
	return s;
}

/// The facilities to build: "Living Quarters, $400,000, 16 days, $10,000 a month"; Enter picks one to place.
AccessScreen buildFacilities()
{
	AccessScreen s = simpleScreen("buildFacilities", is<BuildFacilitiesState>);
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state)
	{
		BuildFacilitiesState *bf = static_cast<BuildFacilitiesState *>(state);
		addWidgets(b, state, [state, bf](Surface *surface, size_t row, NodeVtable &v)
		{
			if (!dynamic_cast<TextList *>(surface) || row == NO_ROW || row >= bf->getFacilities().size())
				return;
			RuleBaseFacility *rule = bf->getFacilities()[row];
			std::string text = Vocab::format(Vocab::FACILITY_ROW, { std::string(state->tr(rule->getType())),
				Unicode::formatFunding(rule->getBuildCost()), std::to_string(rule->getBuildTime()), Unicode::formatFunding(rule->getMonthlyCost()) });
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([text] { return text; }, false, AnnouncementKinds::Label));
		});
	};
	return s;
}

/// What a square says about placing a facility there, from OXCE's own placement errors. OXCE folds
/// "off the grid" and "taken" into "not connected", so the grid tells those apart; its rarer reasons
/// (upgrades, facilities in use, mod rules) are "can't build here", and Enter gives the game's message.
std::string placementText(BaseView *view, const RuleBaseFacility *rule, int x, int y)
{
	BasePlacementErrors error = view->getPlacementErrorAt(rule, x, y);
	if (error == BPE_None)
		return Vocab::get(Vocab::PLACE_OK);
	if (error == BPE_NotConnected)
	{
		for (int dy = 0; dy < rule->getSizeY(); ++dy)
			for (int dx = 0; dx < rule->getSizeX(); ++dx)
			{
				if (x + dx >= 6 || y + dy >= 6)
					return Vocab::get(Vocab::PLACE_OFF_GRID);
				if (view->getFacilityAt(x + dx, y + dy))
					return Vocab::get(Vocab::PLACE_OCCUPIED);
			}
		return Vocab::get(Vocab::PLACE_UNCONNECTED);
	}
	if (error == BPE_Queue)
		return Vocab::get(Vocab::PLACE_UNCONNECTED);
	return Vocab::get(Vocab::PLACE_OTHER);
}

/// Placing a facility: its cost, time and upkeep on arrival (and, for a big one, that it's placed by its
/// top left square), then the grid, each square saying whether it can go there and why not, then Cancel.
/// Enter clicks the square, so the game's own checks run; a placement is confirmed by name.
AccessScreen placeFacility()
{
	AccessScreen s;
	s.key = "placeFacility";
	s.isActive = is<PlaceFacilityState>;
	s.name = [](State *state)
	{
		std::string text = allText(state);
		const RuleBaseFacility *rule = static_cast<PlaceFacilityState *>(state)->getRule();
		if (rule->getSizeX() > 1 || rule->getSizeY() > 1)
			text += ". " + Vocab::format(Vocab::PLACE_SIZE, { std::to_string(rule->getSizeX()), std::to_string(rule->getSizeY()) });
		return text;
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		PlaceFacilityState *pf = static_cast<PlaceFacilityState *>(state);
		BaseView *view = pf->getView();
		const RuleBaseFacility *rule = pf->getRule();
		Base *base = pf->getBase();
		GridHooks grid;
		grid.extra = [view, rule](int x, int y) { return placementText(view, rule, x, y); };
		grid.activate = [state, view, rule, base](int x, int y)
		{
			std::string placed = Vocab::format(Vocab::FACILITY_PLACED, { std::string(state->tr(rule->getType())), std::to_string(rule->getBuildTime()) });
			size_t before = base->getFacilities()->size();
			view->selectSquare(x, y);
			Controls::click(state, view);
			// Success pops back to the list without a word; refusals push an error message, which speaks itself.
			if (base->getFacilities()->size() > before)
				Speech::say(placed, true);
		};
		addBaseGrid(b, state, view, grid);
		b.AddItem(ControlId::Referenced(pf->getCancelButton(), "cancel"), Controls::textButton(state, pf->getCancelButton()));
	};
	return s;
}

/// A production line as the Manufacture screen shows it, columns named:
/// "Laser Rifle, 10 engineers, 2 of 5 made, selling, $8,000 each, 3 days 4 hours left".
std::string productionRow(State *state, Production *p)
{
	std::string made = p->getInfiniteAmount()
		? Vocab::format(Vocab::MAN_MADE_ENDLESS, { std::to_string(p->getAmountProduced()) })
		: Vocab::format(Vocab::MAN_MADE, { std::to_string(p->getAmountProduced()), std::to_string(p->getAmountTotal()) });
	if (p->getSellItems())
		made += ", " + Vocab::get(Vocab::MAN_SELLING);
	int engineers = p->getAssignedEngineers();
	std::string left;
	if (p->getInfiniteAmount())
	{
		left = Vocab::get(Vocab::MAN_NO_END);
	}
	else if (engineers > 0)
	{
		// The game's sum: a part of an hour's work takes the whole hour.
		int timeLeft = p->getAmountTotal() * p->getRules()->getManufactureTime() - p->getTimeSpent();
		int hoursLeft = (timeLeft + engineers - 1) / engineers;
		left = Vocab::format(Vocab::MAN_TIME_LEFT, { std::to_string(hoursLeft / 24), std::to_string(hoursLeft % 24) });
	}
	else
	{
		left = Vocab::get(Vocab::MAN_IDLE);
	}
	return Vocab::format(Vocab::MAN_ROW, { std::string(state->tr(p->getRules()->getName())), std::to_string(engineers),
		made, Unicode::formatFunding(p->getRules()->getManufactureCost()), left });
}

/// Current production: engineers, workshop space and funds on arrival, then the production lines
/// (read from the base's productions, which the list shows in order; Enter opens one), New Production and OK.
AccessScreen manufacture()
{
	AccessScreen s = simpleScreen("manufacture", is<ManufactureState>);
	s.name = [](State *state)
	{
		return firstText(state) + ". " + visibleTexts(static_cast<ManufactureState *>(state)->getInfoTexts());
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		Base *base = static_cast<ManufactureState *>(state)->getBase();
		addWidgets(b, state, [state, base](Surface *surface, size_t row, NodeVtable &v)
		{
			if (!dynamic_cast<TextList *>(surface) || row == NO_ROW || row >= base->getProductions().size())
				return;
			Production *p = base->getProductions()[row];
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([state, p] { return productionRow(state, p); }, false, AnnouncementKinds::Label));
		});
	};
	return s;
}

/// What can be made: the title, the category filter (labelled explicitly; the heuristic picks the title),
/// then "Laser Pistol, Weapons" rows. Enter opens one.
AccessScreen newManufactureList()
{
	AccessScreen s = simpleScreen("newManufactureList", is<NewManufactureListState>);
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state)
	{
		ComboBox *category = static_cast<NewManufactureListState *>(state)->getCategory();
		addWidgets(b, state, [state, category](Surface *surface, size_t, NodeVtable &v)
		{
			if (surface == category)
				v = Controls::comboBox(state, category, Vocab::get(Vocab::CATEGORY));
		});
	};
	return s;
}

/// Why Start Production is hidden, by the game's own tests, or nothing.
std::string manufactureRefusal(ManufactureStartState *ms)
{
	if (ms->getStartButton()->getVisible())
		return std::string();
	Game *game = State::getGamePtr();
	Base *base = ms->getBase();
	RuleManufacture *rule = ms->getRule();
	std::vector<std::string> why;
	if (!rule->haveEnoughMoneyForOneMoreUnit(game->getSavedGame()->getFunds()))
		why.push_back(Vocab::get(Vocab::MAN_NO_MONEY));
	bool missing = false;
	for (const auto &needed : rule->getRequiredCrafts())
		missing |= base->getCraftCountForProduction(needed.first) < needed.second;
	for (const auto &needed : rule->getRequiredItems())
		missing |= base->getStorageItems()->getItem(needed.first) < needed.second;
	if (missing)
		why.push_back(Vocab::get(Vocab::MAN_NO_MATERIALS));
	if (!rule->getSpawnedPersonType().empty() && base->getAvailableQuarters() <= base->getUsedQuarters())
		why.push_back(Vocab::get(Vocab::MAN_NO_QUARTERS));
	return why.empty() ? std::string() : Vocab::format(Vocab::MAN_CANT_START, { joinParts(why) });
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

/// Before starting production: the item, engineer hours, cost and space, every special material
/// ("Elerium-115, UNITS REQUIRED: 4, UNITS AVAILABLE: 10") and, if Start is missing, why. Then the
/// materials, Cancel and Start.
AccessScreen manufactureStart()
{
	static const std::vector<std::string> headers = { "STR_UNITS_REQUIRED", "STR_UNITS_AVAILABLE" };
	AccessScreen s;
	s.key = "manufactureStart";
	s.isActive = is<ManufactureStartState>;
	s.name = [](State *state)
	{
		ManufactureStartState *ms = static_cast<ManufactureStartState *>(state);
		RuleManufacture *rule = ms->getRule();
		std::string text = firstText(state) + ". " +
			std::string(state->tr("STR_ENGINEER_HOURS_TO_PRODUCE_ONE_UNIT").arg(rule->getManufactureTime())) + ". " +
			std::string(state->tr("STR_COST_PER_UNIT_").arg(Unicode::formatFunding(rule->getManufactureCost()))) + ". " +
			std::string(state->tr("STR_WORK_SPACE_REQUIRED").arg(rule->getRequiredSpace())) + ".";
		TextList *list = ms->getRequiredList();
		if (list->getVisible() && list->getTexts() > 0)
		{
			text += " " + std::string(state->tr("STR_SPECIAL_MATERIALS_REQUIRED")) + ".";
			for (size_t row = 0; row < list->getTexts(); ++row)
				text += " " + headedRow(state, list, row, headers) + ".";
		}
		std::string refusal = manufactureRefusal(ms);
		if (!refusal.empty())
			text += " " + refusal + ".";
		return text;
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		addWidgets(b, state, [state](Surface *surface, size_t row, NodeVtable &v)
		{
			TextList *list = dynamic_cast<TextList *>(surface);
			if (!list || row == NO_ROW)
				return;
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([state, list, row] { return headedRow(state, list, row, headers); }, false, AnnouncementKinds::Label));
		});
	};
	return s;
}

/// A production's engineers: allocated, then what's free, in the game's words.
std::string engineersText(ManufactureInfoState *mi)
{
	Base *base = mi->getBase();
	return std::string(mi->tr("STR_ENGINEERS_ALLOCATED").arg(mi->getProduction()->getAssignedEngineers())) + ", " +
		std::string(mi->tr("STR_ENGINEERS_AVAILABLE_UC").arg(base->getAvailableEngineers())) + ", " +
		std::string(mi->tr("STR_WORKSHOP_SPACE_AVAILABLE_UC").arg(base->getFreeWorkshops()));
}

/// "UNITS TO PRODUCE: 5", or "no limit".
std::string unitsText(ManufactureInfoState *mi)
{
	Production *p = mi->getProduction();
	return Vocab::format(Vocab::HEADED_CELL, { std::string(mi->tr("STR_UNITS_TO_PRODUCE")),
		p->getInfiniteAmount() ? Vocab::get(Vocab::NO_LIMIT) : std::to_string(p->getAmountTotal()) });
}

/// Setting up a production: the item, engineers, units and monthly profit on arrival, then engineers
/// and units as adjustable nodes (Left/Right one, Shift five; Ctrl+Right all engineers or no unit limit,
/// Ctrl+Left none or the fewest, as the arrows' right clicks do), each change saying the profit,
/// then the sell toggle, Stop Production and OK. Escape is the game's OK, which starts a new production.
AccessScreen manufactureInfo()
{
	AccessScreen s;
	s.key = "manufactureInfo";
	s.isActive = is<ManufactureInfoState>;
	s.name = [](State *state)
	{
		ManufactureInfoState *mi = static_cast<ManufactureInfoState *>(state);
		return firstText(state) + ". " + engineersText(mi) + ". " + unitsText(mi) + ". " + mi->getProfitText()->getText();
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		ManufactureInfoState *mi = static_cast<ManufactureInfoState *>(state);
		std::function<int(int, bool)> amount = [](int sign, bool large)
		{
			return std::abs(sign) >= Controls::ADJUST_LIMIT ? INT_MAX : (large ? 5 : 1);
		};

		NodeVtable engineers;
		engineers.Type = &Controls::sliderType();
		engineers.Announcements.push_back(NodeAnnouncement([mi]
		{
			return std::string(mi->tr("STR_ENGINEERS_ALLOCATED").arg(mi->getProduction()->getAssignedEngineers()));
		}, false, AnnouncementKinds::Label));
		engineers.OnAdjust = [mi, amount](int sign, bool large) { mi->changeEngineers(sign, amount(sign, large)); };
		engineers.StateText = [mi] { return engineersText(mi) + ". " + mi->getProfitText()->getText(); };
		b.AddItem(ControlId::Referenced(mi->getProduction(), "engineers"), engineers);

		NodeVtable units;
		units.Type = &Controls::sliderType();
		units.Announcements.push_back(NodeAnnouncement([mi] { return unitsText(mi); }, false, AnnouncementKinds::Label));
		units.OnAdjust = [mi, amount](int sign, bool large) { mi->changeUnits(sign, amount(sign, large)); };
		units.StateText = [mi] { return unitsText(mi) + ". " + mi->getProfitText()->getText(); };
		b.AddItem(ControlId::Referenced(mi->getProduction(), "units"), units);

		NodeVtable sell = Controls::textButton(state, mi->getSellButton());
		std::function<std::string()> pressed = sell.StateText;
		sell.StateText = [pressed, mi] { return pressed() + ". " + mi->getProfitText()->getText(); };
		b.AddItem(ControlId::Referenced(mi->getSellButton(), "sell"), sell);
		b.AddItem(ControlId::Referenced(mi->getStopButton(), "stop"), Controls::textButton(state, mi->getStopButton()));
		b.AddItem(ControlId::Referenced(mi->getOkButton(), "ok"), Controls::textButton(state, mi->getOkButton()));
	};
	return s;
}

/// A craft's weapon slot: "weapon 1, STINGRAY, ammo 6 of 6", or "weapon 1, none".
std::string weaponSlotText(State *state, Craft *c, size_t slot)
{
	CraftWeapon *w = slot < c->getWeapons()->size() ? c->getWeapons()->at(slot) : 0;
	std::string n = std::to_string(slot + 1);
	if (!w)
		return Vocab::format(Vocab::CRAFT_WEAPON_NONE, { n });
	return Vocab::format(Vocab::CRAFT_WEAPON, { n, state->tr(w->getRules()->getType()),
		std::to_string(w->getAmmo()), std::to_string(w->getRules()->getAmmoMax()) });
}

/// Picking a craft weapon: says the title and what the slot holds now, then the weapons in stores
/// ("Stingray, 2 in stores, ammunition 30"); Enter mounts one (None takes it off), as clicking the row does.
AccessScreen craftWeapons()
{
	AccessScreen s = simpleScreen("craftWeapons", is<CraftWeaponsState>);
	s.name = [](State *state)
	{
		CraftWeaponsState *cw = static_cast<CraftWeaponsState *>(state);
		Craft *c = cw->getCraft();
		return firstText(state) + ". " + c->getName(State::getGamePtr()->getLanguage()) + ", " + weaponSlotText(state, c, cw->getSlot());
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		addWidgets(b, state, [](Surface *surface, size_t row, NodeVtable &v)
		{
			TextList *list = dynamic_cast<TextList *>(surface);
			if (!list || row == NO_ROW || list->getCellCount(row) < 3)
				return;
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([list, row]
			{
				return Vocab::format(Vocab::ARMAMENT_ROW, { Controls::cellText(list, row, 0), Controls::cellText(list, row, 1), Controls::cellText(list, row, 2) });
			}, false, AnnouncementKinds::Label));
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

/// After a battle: the outcome, rating and total on arrival, then the page showing and the buttons.
/// The Stats button cycles OXCE's three pages: the score rows and recovered aliens and artifacts, what
/// each soldier gained ("Time Units +2, Bravery +10"), and the loot list (with Sell and Transfer).
/// Wired through DebriefingState's accessors, since the pages overlap on screen.
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
		int page = debrief->getPageNumber();
		if (page == 0)
		{
			TextList *stats = debrief->getStatsList();
			for (size_t row = 0; row < stats->getTexts(); ++row)
				addLine(b, ControlId::Referenced(stats, "stats:" + std::to_string(row)), [stats, row] { return debriefRow(stats, row); });
			TextList *recovery = debrief->getRecoveryList();
			if (recovery->getTexts() > 0)
			{
				b.BeginStop("recovered");
				b.PushContext(debrief->getRecoveryHeading()->getText());
				for (size_t row = 0; row < recovery->getTexts(); ++row)
					addLine(b, ControlId::Referenced(recovery, "recovery:" + std::to_string(row)), [recovery, row] { return debriefRow(recovery, row); });
				b.PopContext();
				b.BeginStop("totals");
			}
			TextList *total = debrief->getTotalList();
			if (total->getTexts() > 0)
				addLine(b, ControlId::Referenced(total, "total"), [total] { return Controls::rowText(total, 0); });
			Text *rating = debrief->getRating();
			addLine(b, ControlId::Referenced(rating, "rating"), [rating] { return rating->getText(); });
		}
		else if (page == 2)
		{
			TextList *loot = debrief->getRecoveredItemsList();
			for (size_t row = 0; row < loot->getTexts(); ++row)
				addLine(b, ControlId::Referenced(loot, "loot:" + std::to_string(row)), [loot, row] { return Controls::rowText(loot, row); });
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
		b.BeginStop("buttons");
		b.AddItem(ControlId::Referenced(debrief->getOkButton(), "ok"), Controls::textButton(state, debrief->getOkButton()));
		b.AddItem(ControlId::Referenced(debrief->getStatsButton(), "stats"), Controls::textButton(state, debrief->getStatsButton()));
		if (debrief->getSellButton()->getVisible())
			b.AddItem(ControlId::Referenced(debrief->getSellButton(), "sell"), Controls::textButton(state, debrief->getSellButton()));
		if (debrief->getTransferButton()->getVisible())
			b.AddItem(ControlId::Referenced(debrief->getTransferButton(), "transfer"), Controls::textButton(state, debrief->getTransferButton()));
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
			b.BeginStop("region:" + rules->getType());
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
		b.BeginStop("buttons");
		addWidgets(b, state);
	};
	return s;
}

/// An Intercept row: "Interceptor-1, READY, Base 1, 2 weapons, 0 soldiers, 0 tanks".
std::string interceptRow(State *state, Craft *c)
{
	return Vocab::format(Vocab::INTERCEPT_ROW, { c->getName(State::getGamePtr()->getLanguage()), state->tr(c->getStatus()), c->getBase()->getName(),
		std::to_string(c->getNumWeapons()), std::to_string(c->getNumTotalSoldiers()), std::to_string(c->getNumTotalVehicles()) });
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

bool inWing(InterceptState *ic, Craft *c)
{
	const std::vector<Craft *> &wing = ic->getSelectedCrafts();
	return std::find(wing.begin(), wing.end(), c) != wing.end();
}

/// Launch interception: one row per craft from the state's own craft list; Enter launches,
/// or says why not. Backspace (right click) centres the globe on a craft in flight, as the game does.
/// Shift+Enter is OXCE's Shift+click: puts the craft in the wing or takes it out
/// (InterceptState::lstCraftsLeftClick, at most 3); then Enter on any craft launches it with the wing.
AccessScreen intercept()
{
	AccessScreen s = simpleScreen("intercept", is<InterceptState>);
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state)
	{
		InterceptState *ic = static_cast<InterceptState *>(state);
		addWidgets(b, state, [state, ic](Surface *surface, size_t row, NodeVtable &v)
		{
			if (row == NO_ROW || row >= ic->getCrafts().size())
				return;
			TextList *list = static_cast<TextList *>(surface);
			Craft *c = ic->getCrafts()[row];
			v.Announcements.clear();
			v.Announcements.push_back(NodeAnnouncement([state, ic, c]
			{
				std::string text = interceptRow(state, c);
				return inWing(ic, c) ? text + ", " + Vocab::get(Vocab::IN_WING) : text;
			}, false, AnnouncementKinds::Label));
			// What the last Shift+Enter did to the wing; empty after a plain Enter.
			std::shared_ptr<std::string> wingChange = std::make_shared<std::string>();
			v.OnActivate = [state, ic, list, row, c, wingChange]
			{
				wingChange->clear();
				bool before = inWing(ic, c);
				if (Navigator::keyModifiers() & KMOD_SHIFT)
				{
					Controls::withModifiers(KMOD_LSHIFT, [&] { Controls::clickRow(state, list, row); });
					bool after = inWing(ic, c);
					std::string count = std::to_string(ic->getSelectedCrafts().size());
					if (after != before)
						*wingChange = Vocab::format(after ? Vocab::WING_ADDED : Vocab::WING_REMOVED, { count });
					else
					{
						std::string refusal = launchRefusal(state, c);
						*wingChange = refusal.empty() ? Vocab::format(Vocab::WING_FULL, { count }) : refusal;
					}
					return;
				}
				// The game adds this craft to the wing and launches them all.
				size_t total = ic->getSelectedCrafts().size() + (before ? 0 : 1);
				if (total > 1 && launchRefusal(state, c).empty())
					Speech::say(Vocab::format(Vocab::WING_LAUNCH, { std::to_string(total) }), true);
				Controls::clickRow(state, list, row);
			};
			// A plain Enter is only heard if the screen is still up after it, which is exactly a refusal.
			v.StateText = [state, c, wingChange] { return wingChange->empty() ? launchRefusal(state, c) : *wingChange; };
		});
	};
	return s;
}

/// A destination as the picker reads it: where it is, how far from the craft, and whether it's
/// inside the range circle the globe draws (half the craft's fuel, so it can get home).
std::string destinationText(Craft *craft, const std::string &name, double lon, double lat)
{
	std::vector<std::string> parts = { name, Geo::placeName(lon, lat), Geo::seaText(lon, lat), Geo::offsetText(craft, lon, lat),
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
		std::vector<Craft *> crafts = static_cast<SelectDestinationState *>(state)->getCrafts();
		Game *game = State::getGamePtr();
		std::vector<Target *> targets = Geo::destinations();
		std::stable_sort(targets.begin(), targets.end(), [craft](Target *a, Target *b) { return craft->getDistance(a) < craft->getDistance(b); });
		if (!targets.empty())
		{
			b.BeginStop("targets");
			b.PushContext(Vocab::get(Vocab::DEST_TARGETS));
			for (Target *t : targets)
			{
				NodeVtable v;
				v.Announcements.push_back(NodeAnnouncement([craft, t, game]
				{
					return destinationText(craft, t->getName(game->getLanguage()), t->getLongitude(), t->getLatitude());
				}, false, AnnouncementKinds::Label));
				v.OnActivate = [crafts, t, game] { game->pushState(new MultipleTargetsState(std::vector<Target *>(1, t), crafts, 0, false)); };
				b.AddItem(ControlId::Referenced(t, "target:" + t->getType() + ":" + std::to_string(t->getId())), v);
			}
			b.PopContext();
		}
		for (Region *region : *game->getSavedGame()->getRegions())
		{
			RuleRegion *rules = region->getRules();
			if (rules->getCities()->empty())
				continue;
			b.BeginStop("region:" + rules->getType());
			b.PushContext(state->tr(rules->getType()));
			for (size_t i = 0; i < rules->getCities()->size(); ++i)
			{
				City *city = rules->getCities()->at(i);
				NodeVtable v;
				v.Announcements.push_back(NodeAnnouncement([craft, city, game]
				{
					return destinationText(craft, city->getName(game->getLanguage()), city->getLongitude(), city->getLatitude());
				}, false, AnnouncementKinds::Label));
				v.OnActivate = [crafts, city, game]
				{
					// What clicking an empty spot on the globe does: a fresh waypoint, registered only if confirmed.
					Waypoint *w = new Waypoint();
					w->setLongitude(city->getLongitude());
					w->setLatitude(city->getLatitude());
					game->pushState(new MultipleTargetsState(std::vector<Target *>(1, w), crafts, 0, false));
				};
				b.AddItem(ControlId::Referenced(city, "city:" + rules->getType() + ":" + std::to_string(i)), v);
			}
			b.PopContext();
		}
		// Cancel, and Cydonia when it's offered.
		b.BeginStop("buttons");
		addWidgets(b, state);
	};
	return s;
}

/// The Basescape's menu: says the base, its region and the funds on arrival, then the buttons
/// (and the base name field), the facility grid (Enter dismantles, Backspace opens the facility's
/// screen, as the mouse buttons do), then the other bases to switch to.
/// The game's number keys switch bases too; either way the tick says the new base.
AccessScreen basescape()
{
	static Base *shown = 0;
	AccessScreen s;
	s.key = "basescape";
	s.isActive = is<BasescapeState>;
	s.name = [](State *state)
	{
		BasescapeState *bs = static_cast<BasescapeState *>(state);
		shown = bs->getBase();
		// The hover tooltip names whatever facility the mouse happens to be over.
		return allTextExcept(state, bs->getFacilityText());
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		BasescapeState *bs = static_cast<BasescapeState *>(state);
		// Three Tab-stops: the menu, the grid, the other bases.
		b.BeginStop("menu");
		addWidgets(b, state);
		b.BeginStop("grid");
		// The game's clicks: left starts dismantling (with a confirmation), right opens the facility's screen.
		BaseView *view = bs->getView();
		GridHooks grid;
		grid.secondary = [state, view](int x, int y)
		{
			view->selectSquare(x, y);
			Controls::click(state, view, SDL_BUTTON_RIGHT);
		};
		grid.tooltip = Vocab::get(Vocab::BASE_GRID_HINT);
		addBaseGrid(b, state, view, grid);
		b.BeginStop("bases");
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
		Speech::say(allTextExcept(state, static_cast<BasescapeState *>(state)->getFacilityText()), true);
	};
	return s;
}

/// A craft as the base's craft list shows it, with the columns named.
std::string craftsRow(State *state, Craft *c)
{
	return Vocab::format(Vocab::CRAFTS_ROW, { c->getName(State::getGamePtr()->getLanguage()), state->tr(c->getStatus()),
		std::to_string(c->getNumWeapons()), std::to_string(c->getRules()->getWeapons()),
		std::to_string(c->getNumTotalSoldiers()), std::to_string(c->getNumTotalVehicles()) });
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

/// What a crew row's Enter did: on or off the craft, or why the game refused (it does so silently).
std::string crewChange(Craft *c, Soldier *s, Craft *before)
{
	Language *lang = State::getGamePtr()->getLanguage();
	Craft *after = s->getCraft();
	if (after == c && before != c)
		return Vocab::format(Vocab::CREW_ADDED, { c->getName(lang), std::to_string(c->getSpaceAvailable()) });
	if (before == c && after != c)
		return Vocab::format(Vocab::CREW_REMOVED, { std::to_string(c->getSpaceAvailable()) });
	if (before && before->getStatus() == "STR_OUT")
		return Vocab::format(Vocab::CREW_CRAFT_OUT, { before->getName(lang) });
	if (s->getWoundRecoveryInt() > 0)
		return Vocab::format(Vocab::CREW_WOUNDED, { std::to_string(s->getWoundRecoveryInt()) });
	return Vocab::format(Vocab::CREW_FULL, { c->getName(lang) });
}

/// Picking a craft's crew: rows as the game shows them (name, rank, craft). Enter puts a soldier
/// on or off the craft through the game's own click, then says what happened or why it refused.
AccessScreen craftSoldiers()
{
	AccessScreen s = simpleScreen("craftSoldiers", is<CraftSoldiersState>);
	s.build = [](GraphBuilder &b, State *state)
	{
		CraftSoldiersState *cs = static_cast<CraftSoldiersState *>(state);
		Base *base = cs->getBase();
		Craft *c = base->getCrafts()->at(cs->getCraftIndex());
		// Outlives the rebuild between Enter and the state line.
		static std::string result;
		addWidgets(b, state, [state, base, c](Surface *surface, size_t row, NodeVtable &v)
		{
			TextList *list = dynamic_cast<TextList *>(surface);
			if (!list || row == NO_ROW || row >= base->getSoldiers()->size())
				return;
			v.OnActivate = [state, list, row, base, c]
			{
				Soldier *soldier = base->getSoldiers()->at(row);
				Craft *before = soldier->getCraft();
				Controls::clickRow(state, list, row);
				result = crewChange(c, soldier, before);
			};
			v.StateText = [] { return result; };
		});
	};
	return s;
}

/// A cell with only spaces in it (the game pads unused base defense cells with " ").
bool blankCell(const std::string &cell)
{
	return cell.find_first_not_of(' ') == std::string::npos;
}

std::string defenseCell(TextList *list, size_t row, size_t column)
{
	return column < list->getCellCount(row) ? list->getCellText(row, column) : std::string();
}

/// A base defense row: "Missile Defenses, HIT", or a message ("Grav shield repels UFO").
std::string defenseRow(TextList *list, size_t row)
{
	std::string name = defenseCell(list, row, 0), firing = defenseCell(list, row, 1), result = defenseCell(list, row, 2);
	if (!blankCell(result))
		return Vocab::format(Vocab::DEFENSE_SHOT, { name, result });
	if (!blankCell(firing))
		return Vocab::format(Vocab::DEFENSE_SHOT, { name, firing });
	return name;
}

/// A row is finished once its shot has a result, it's one of the two messages, or a later row exists.
bool defenseRowDone(State *state, TextList *list, size_t row)
{
	std::string name = defenseCell(list, row, 0);
	return row + 1 < list->getTexts() || !blankCell(defenseCell(list, row, 2))
		|| name == std::string(state->tr("STR_GRAV_SHIELD_REPELS_UFO")) || name == std::string(state->tr("STR_UFO_DESTROYED"));
}

/// A UFO attacking a base with defenses: the title on arrival, then each defense's shot (queued)
/// as it resolves, from the game's own list. OK only exists once the attack is over; focus moves there.
AccessScreen baseDefense()
{
	AccessScreen s;
	s.key = "baseDefense";
	s.isActive = is<BaseDefenseState>;
	s.name = firstText;
	s.build = [](GraphBuilder &b, State *state)
	{
		BaseDefenseState *bd = static_cast<BaseDefenseState *>(state);
		Text *init = bd->getInitText();
		TextList *list = bd->getList();
		addLine(b, ControlId::Referenced(init, "init"), [init] { return init->getText(); });
		for (size_t row = 0; row < list->getTexts(); ++row)
			addLine(b, ControlId::Referenced(list, "row:" + std::to_string(row)), [list, row] { return defenseRow(list, row); });
		TextButton *ok = bd->getOkButton();
		if (ok->getVisible())
			b.AddItem(ControlId::Referenced(ok, "ok"), Controls::textButton(state, ok));
	};
	s.tick = [](State *state)
	{
		static State *watched = 0;
		static size_t said = 0;
		static bool okFocused = false;
		if (state != watched)
		{
			watched = state;
			said = 0;
			okFocused = false;
		}
		BaseDefenseState *bd = static_cast<BaseDefenseState *>(state);
		TextList *list = bd->getList();
		for (; said < list->getTexts() && defenseRowDone(state, list, said); ++said)
			Speech::say(defenseRow(list, said), false);
		TextButton *ok = bd->getOkButton();
		if (ok->getVisible() && !okFocused)
		{
			okFocused = true;
			Navigator::focus(state, ControlId::Referenced(ok, "ok"));
		}
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
			TextButton *btn = ci->getWeaponButton(i);
			b.AddItem(ControlId::Referenced(btn, "weapon:" + std::to_string(i + 1)), Controls::labelledButton(state, btn, weaponSlotText(state, c, i)));
		}
		if (c->getRules()->getMaxUnits() > 0)
		{
			TextButton *crew = ci->getCrewButton(), *equip = ci->getEquipButton(), *armor = ci->getArmorButton();
			b.AddItem(ControlId::Referenced(crew, "crew"), Controls::labelledButton(state, crew, Vocab::format(Vocab::CRAFT_CREW,
				{ crew->getText(), std::to_string(c->getNumTotalSoldiers()), std::to_string(c->getSpaceAvailable()) })));
			b.AddItem(ControlId::Referenced(equip, "equip"), Controls::labelledButton(state, equip, Vocab::format(Vocab::CRAFT_EQUIPMENT,
				{ equip->getText(), std::to_string(c->getNumTotalVehicles()), std::to_string(c->getNumEquipment()) })));
			b.AddItem(ControlId::Referenced(armor, "armor"), Controls::textButton(state, armor));
		}
		b.AddItem(ControlId::Referenced(ci->getOkButton(), "ok"), Controls::textButton(state, ci->getOkButton()));
	};
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

/// A popup holding one short table: says its title and every row with its columns named on arrival,
/// then lists the rows and buttons like tableScreen.
AccessScreen tablePopup(const std::string &key, std::function<bool(State *)> isActive, const std::vector<std::string> &headers)
{
	AccessScreen s = tableScreen(key, isActive, headers);
	s.name = [headers](State *state)
	{
		std::string result = firstText(state);
		if (!result.empty())
			result += ".";
		for (Surface *surface : state->getSurfaces())
		{
			TextList *list = dynamic_cast<TextList *>(surface);
			if (!list || !list->getVisible())
				continue;
			for (size_t row = 0; row < list->getTexts(); ++row)
			{
				std::string line = headedRow(state, list, row, headers);
				if (line.empty())
					continue;
				if (!result.empty())
					result += " ";
				result += line + ".";
			}
		}
		return result;
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
				b.BeginStop("section:" + label->getText());
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
		b.BeginStop("buttons");
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
			b.BeginStop(t.first);
			b.PushContext(state->tr(t.first));
			for (size_t row = 0; row < list->getTexts(); ++row)
				addLine(b, ControlId::Referenced(list, std::string(t.first) + ":" + std::to_string(row)), [state, list, row, headers] { return headedRow(state, list, row, headers); });
			b.PopContext();
		}
		b.BeginStop("totals");
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

/// A Ufopaedia article: says the title on arrival, then lists the stats ("DAMAGE: 70"), the
/// description, OK and the previous and next buttons. Left/Right page through the articles from
/// anywhere, as the game's own arrow keys do. Other kinds (TFTD's) fall back to reading the screen.
AccessScreen article()
{
	AccessScreen s;
	s.key = "article";
	s.isActive = is<ArticleState>;
	s.name = [](State *state)
	{
		std::string title, info;
		std::vector<std::string> stats;
		return ArticleAccess::read(state, title, stats, info) ? title : allText(state);
	};
	s.build = [](GraphBuilder &b, State *state)
	{
		ArticleState *a = static_cast<ArticleState *>(state);
		std::function<void(int, bool)> page = [state, a](int sign, bool)
		{
			Controls::click(state, sign < 0 ? a->getPrevButton() : a->getNextButton());
		};
		std::string title, info;
		std::vector<std::string> stats;
		if (!ArticleAccess::read(state, title, stats, info))
		{
			addWidgets(b, state, [page](Surface *, size_t, NodeVtable &v) { v.OnAdjust = page; });
			return;
		}
		for (size_t i = 0; i < stats.size(); ++i)
		{
			NodeVtable v;
			std::string line = stats[i];
			v.Announcements.push_back(NodeAnnouncement([line] { return line; }, false, AnnouncementKinds::Label));
			v.OnAdjust = page;
			b.AddItem(ControlId::Referenced(a, "stat:" + std::to_string(i)), v);
		}
		b.BeginStop("text");
		if (!info.empty())
		{
			NodeVtable v;
			v.Announcements.push_back(NodeAnnouncement([info] { return info; }, false, AnnouncementKinds::Label));
			v.OnAdjust = page;
			b.AddItem(ControlId::Structural("info"), v);
		}
		NodeVtable ok = Controls::textButton(state, a->getOkButton());
		ok.OnAdjust = page;
		b.AddItem(ControlId::Referenced(a->getOkButton(), "ok"), ok);
		NodeVtable prev = Controls::labelledButton(state, a->getPrevButton(), Vocab::get(Vocab::ARTICLE_PREV));
		prev.OnAdjust = page;
		b.AddItem(ControlId::Referenced(a->getPrevButton(), "prev"), prev);
		NodeVtable next = Controls::labelledButton(state, a->getNextButton(), Vocab::get(Vocab::ARTICLE_NEXT));
		next.OnAdjust = page;
		b.AddItem(ControlId::Referenced(a->getNextButton(), "next"), next);
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

/// A widget's game tooltip (the options' descriptions), spoken on Space.
void addTooltip(State *state, Surface *surface, NodeVtable &v)
{
	InteractiveSurface *widget = dynamic_cast<InteractiveSurface *>(surface);
	if (!widget || widget->getTooltip().empty())
		return;
	std::string text = state->tr(widget->getTooltip());
	v.OnTooltip = [text] { Speech::say(text, true); };
}

/// "VIDEO options": the category showing.
std::string optionsName(State *state)
{
	TextButton *category = OptionsAccess::category(state);
	return category ? Vocab::format(Vocab::OPTIONS_NAME, { category->getText() }) : std::string();
}

/// The categories down the left, as the first stop. The one showing is where the stop lands,
/// so focus stays on it when Enter switches category (each category is a new state).
void addOptionCategories(GraphBuilder &b, State *state)
{
	b.BeginStop("categories");
	for (TextButton *btn : OptionsAccess::categories(state))
	{
		if (!btn->getVisible())
			continue;
		NodeVtable v = Controls::textButton(state, btn);
		for (NodeAnnouncement &a : v.Announcements)
		{
			if (a.Kind == "pressed")
				a.Kind = AnnouncementKinds::Selected;
		}
		b.AddItem(ControlId::ForObject(btn), v);
	}
}

/// OK, Cancel and Restore Defaults, the last stop.
void addOptionButtons(GraphBuilder &b, State *state)
{
	b.BeginStop("buttons");
	for (TextButton *btn : OptionsAccess::buttons(state))
	{
		if (btn->getVisible())
			b.AddItem(ControlId::ForObject(btn), Controls::textButton(state, btn));
	}
}

/// One heading's settings. A lone combo box or slider takes the heading as its label
/// ("Music volume, 75 percent"); toggle groups and folder paths sit under it as their context.
void addOptionSection(GraphBuilder &b, State *state, const OptionsAccess::Section &section)
{
	std::vector<Surface *> shown;
	int labelled = 0;
	for (Surface *w : section.widgets)
	{
		if (!w || !w->getVisible())
			continue;
		Text *text = dynamic_cast<Text *>(w);
		if (text && text->getText().empty())
			continue;
		if (dynamic_cast<ComboBox *>(w) || dynamic_cast<Slider *>(w))
			++labelled;
		shown.push_back(w);
	}
	if (shown.empty())
		return;
	std::string heading = section.heading ? section.heading->getText() : std::string();
	bool context = labelled != 1 && !heading.empty();
	if (context)
		b.PushContext(heading);
	for (Surface *w : shown)
	{
		NodeVtable v;
		if (ComboBox *box = dynamic_cast<ComboBox *>(w))
			v = Controls::comboBox(state, box, heading);
		else if (Slider *slider = dynamic_cast<Slider *>(w))
			v = Controls::slider(state, slider, heading);
		else if (TextButton *btn = dynamic_cast<TextButton *>(w))
			v = Controls::textButton(state, btn);
		else if (Text *text = dynamic_cast<Text *>(w))
		{
			v.Announcements.push_back(NodeAnnouncement([text] { return text->getText(); }, false, AnnouncementKinds::Label));
			// The folder paths (the texts with tooltips) open in Explorer when clicked.
			if (!text->getTooltip().empty())
				v.OnActivate = [state, text] { Controls::click(state, text); };
		}
		addTooltip(state, w, v);
		b.AddItem(ControlId::ForObject(w), v);
	}
	if (context)
		b.PopContext();
}

/// The video options' resolution: width and height fields ("width, 1280, edit"), then the
/// arrows, each saying the new resolution ("1280 by 800").
void addResolution(GraphBuilder &b, State *state)
{
	Text *heading;
	TextEdit *width, *height;
	ArrowButton *bigger, *smaller;
	if (!OptionsAccess::resolution(state, heading, width, height, bigger, smaller))
		return;
	b.PushContext(heading->getText());
	std::pair<TextEdit *, Vocab::Id> edits[] = { { width, Vocab::RES_WIDTH }, { height, Vocab::RES_HEIGHT } };
	for (const std::pair<TextEdit *, Vocab::Id> &edit : edits)
	{
		NodeVtable v = Controls::textEdit(state, edit.first);
		// The field's text is its value; its name goes first.
		v.Announcements[0].Kind = AnnouncementKinds::Value;
		std::string label = Vocab::get(edit.second);
		v.Announcements.push_back(NodeAnnouncement([label] { return label; }, false, AnnouncementKinds::Label));
		b.AddItem(ControlId::ForObject(edit.first), v);
	}
	std::function<std::string()> value = [width, height]
	{
		return Vocab::format(Vocab::RES_VALUE, { width->getText(), height->getText() });
	};
	std::pair<ArrowButton *, Vocab::Id> arrows[] = { { bigger, Vocab::RES_BIGGER }, { smaller, Vocab::RES_SMALLER } };
	for (const std::pair<ArrowButton *, Vocab::Id> &arrow : arrows)
	{
		if (!arrow.first->getVisible())
			continue;
		NodeVtable v = Controls::labelledButton(state, arrow.first, Vocab::get(arrow.second));
		v.StateText = value;
		b.AddItem(ControlId::ForObject(arrow.first), v);
	}
	b.PopContext();
}

/// The options categories laid out in sections: categories, settings, then OK, Cancel and Restore Defaults.
/// Covers video, audio (or the no-audio notice), Geoscape, Battlescape and folders; the advanced
/// settings and key bindings have recipes of their own, matched first.
AccessScreen options()
{
	AccessScreen s;
	s.key = "options";
	s.isActive = is<OptionsBaseState>;
	s.name = optionsName;
	s.build = [](GraphBuilder &b, State *state)
	{
		addOptionCategories(b, state);
		b.BeginStop("settings");
		addResolution(b, state);
		for (const OptionsAccess::Section &section : OptionsAccess::sections(state))
			addOptionSection(b, state, section);
		addOptionButtons(b, state);
	};
	return s;
}

/// The OXC and OXCE buttons that switch the advanced settings and key bindings, a stop of their own.
void addOptionOwners(GraphBuilder &b, State *state)
{
	b.BeginStop("owners");
	for (TextButton *btn : OptionsAccess::owners(state))
	{
		if (btn->getVisible())
			b.AddItem(ControlId::ForObject(btn), Controls::textButton(state, btn));
	}
}

/// A settings list split at its headings (General, Geoscape, Basescape...): each heading starts
/// a stop and is the context of the rows under it; the spacer rows are skipped.
void addHeadedList(GraphBuilder &b, TextList *list, std::function<bool(size_t)> isItem, std::function<NodeVtable(size_t)> node)
{
	bool open = false;
	for (size_t row = 0; row < list->getTexts(); ++row)
	{
		if (isItem(row))
		{
			b.AddItem(ControlId::Referenced(list, "row:" + std::to_string(row)), node(row));
			continue;
		}
		std::string heading = Controls::cellText(list, row, 0);
		if (heading.empty())
			continue;
		if (open)
			b.PopContext();
		b.BeginStop("section:" + std::to_string(row));
		b.PushContext(heading);
		open = true;
	}
	if (open)
		b.PopContext();
}

/// An advanced setting: "Autosave, YES" or a number. Enter is the game's left click (a yes/no
/// flips, a number goes up), Backspace its right click (a number goes down); Left/Right step
/// numbers too. The game wraps numbers at their ends. Settings the mod fixes say so and don't change.
NodeVtable advancedRow(State *state, TextList *list, size_t row)
{
	OptionInfo *setting = OptionsAccess::advancedSetting(state, row);
	bool fixed = State::getGamePtr()->getMod()->getFixedUserOptions().count(setting->id()) > 0;
	std::function<std::string()> value = [list, row, fixed]
	{
		std::string v = Controls::cellText(list, row, 1);
		return fixed ? v + ", " + Vocab::get(Vocab::OPTION_FIXED) : v;
	};
	NodeVtable v;
	v.Announcements.push_back(NodeAnnouncement([list, row] { return Controls::cellText(list, row, 0); }, false, AnnouncementKinds::Label));
	v.Announcements.push_back(NodeAnnouncement(value, false, AnnouncementKinds::Value));
	v.OnActivate = [state, list, row] { Controls::clickRow(state, list, row); };
	v.OnSecondary = [state, list, row] { Controls::clickRow(state, list, row, SDL_BUTTON_RIGHT); };
	if (setting->type() == OPTION_INT)
	{
		v.OnAdjust = [state, list, row](int sign, bool)
		{
			Controls::clickRow(state, list, row, sign > 0 ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT);
		};
	}
	v.StateText = value;
	std::string description = state->tr(setting->description() + "_DESC");
	v.OnTooltip = [description] { Speech::say(description, true); };
	return v;
}

/// Advanced options: categories, the OXC/OXCE switch, one stop per section of settings, then the buttons.
AccessScreen optionsAdvanced()
{
	AccessScreen s;
	s.key = "optionsAdvanced";
	s.isActive = is<OptionsAdvancedState>;
	s.name = optionsName;
	s.build = [](GraphBuilder &b, State *state)
	{
		addOptionCategories(b, state);
		addOptionOwners(b, state);
		TextList *list = OptionsAccess::advancedList(state);
		addHeadedList(b, list,
			[state](size_t row) { return OptionsAccess::advancedSetting(state, row) != 0; },
			[state, list](size_t row) { return advancedRow(state, list, row); });
		addOptionButtons(b, state);
	};
	return s;
}

/// A key binding: "Quick save, F6", or "no key". Enter starts rebinding (the game then takes the
/// next key, whatever it is; see passKeys), Backspace clears the binding.
NodeVtable keyRow(State *state, TextList *list, size_t row)
{
	std::function<std::string()> name = [list, row] { return Controls::cellText(list, row, 0); };
	std::function<std::string()> key = [list, row]
	{
		std::string k = Controls::cellText(list, row, 1);
		return k.empty() ? Vocab::get(Vocab::KEY_NONE) : k;
	};
	NodeVtable v;
	v.Announcements.push_back(NodeAnnouncement(name, false, AnnouncementKinds::Label));
	v.Announcements.push_back(NodeAnnouncement(key, false, AnnouncementKinds::Value));
	v.OnActivate = [state, list, row] { Controls::clickRow(state, list, row); };
	v.OnSecondary = [state, list, row] { Controls::clickRow(state, list, row, SDL_BUTTON_RIGHT); };
	v.StateText = [state, row, name, key]
	{
		if (OptionsAccess::waitingRow(state) == (int)row)
			return Vocab::format(Vocab::KEY_WAITING, { name() });
		return name() + ", " + key();
	};
	addTooltip(state, list, v);
	return v;
}

/// Key bindings: categories, the OXC/OXCE switch, one stop per section of keys, then the buttons.
/// While a row waits for its new key the navigator stands down so the game gets that key,
/// and the tick says the new binding once the game has taken it.
AccessScreen optionsControls()
{
	AccessScreen s;
	s.key = "optionsControls";
	s.isActive = is<OptionsControlsState>;
	s.name = optionsName;
	s.build = [](GraphBuilder &b, State *state)
	{
		addOptionCategories(b, state);
		addOptionOwners(b, state);
		TextList *list = OptionsAccess::controlsList(state);
		addHeadedList(b, list,
			[state](size_t row) { return OptionsAccess::control(state, row) != 0; },
			[state, list](size_t row) { return keyRow(state, list, row); });
		addOptionButtons(b, state);
	};
	s.passKeys = [](State *state) { return OptionsAccess::waitingRow(state) != -1; };
	s.tick = [](State *state)
	{
		static State *watched = 0;
		static int waiting = -1;
		int now = OptionsAccess::waitingRow(state);
		if (state == watched && waiting != -1 && now == -1)
		{
			TextList *list = OptionsAccess::controlsList(state);
			if ((size_t)waiting < list->getTexts())
			{
				std::string key = Controls::cellText(list, waiting, 1);
				Speech::say(Controls::cellText(list, waiting, 0) + ", " + (key.empty() ? Vocab::get(Vocab::KEY_NONE) : key), true);
			}
		}
		watched = state;
		waiting = now;
	};
	return s;
}

/// The battle's timed message box ("X has panicked", "Mind control successful"): says the message,
/// queued so it follows the narration. Any key closes it, so every key goes to the game. It closes
/// itself after two seconds; the speech carries on. A sound-only box has no visible text and says nothing.
AccessScreen infobox()
{
	AccessScreen s = simpleScreen("infobox", is<InfoboxState>);
	s.build = [](GraphBuilder &, State *) {};
	s.passKeys = [](State *) { return true; };
	return s;
}

const std::vector<AccessScreen> &all()
{
	static const std::vector<AccessScreen> screens = {
		mainMenu(), newBattle(), briefing(), inventory(), nextTurn(),
		craftInfo(),
		craftWeapons(),
		craftSoldiers(),
		baseDefense(),
		craftEquipment(),
		simpleScreen("craftArmor", is<CraftArmorState>),
		simpleScreen("soldierArmor", is<SoldierArmorState>),
		soldierInfo(),
		actionMenu(), primeGrenade(), medikit(), debriefing(),
		tablePopup("promotions", is<PromotionsState>, { "STR_NEW_RANK", "STR_BASE" }),
		popupScreen("commendations", is<CommendationState>),
		popupScreen("commendationsLate", is<CommendationLateState>),
		tablePopup("cannotReequip", is<CannotReequipState>, { "STR_QUANTITY_UC", "STR_CRAFT" }),
		simpleScreen("pause", is<PauseState>),
		simpleScreen("abandonGame", is<AbandonGameState>),
		simpleScreen("abortMission", is<AbortMissionState>),
		simpleScreen("confirmEndMission", is<ConfirmEndMissionState>),
		infobox(),
		simpleScreen("infoboxOK", is<InfoboxOKState>),
		listGames("listLoad", is<ListLoadState>),
		listGames("listSave", is<ListSaveState>),
		simpleScreen("deleteGame", is<DeleteGameState>),
		simpleScreen("confirmLoad", is<ConfirmLoadState>),
		simpleScreen("errorMessage", is<ErrorMessageState>),
		optionsAdvanced(),
		optionsControls(),
		options(),
		simpleScreen("optionsDefaults", is<OptionsDefaultsState>),
		simpleScreen("optionsConfirm", is<OptionsConfirmState>),
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
		popupScreen("newPossiblePurchase", is<NewPossiblePurchaseState>),
		popupScreen("newPossibleCraft", is<NewPossibleCraftState>),
		popupScreen("newPossibleFacility", is<NewPossibleFacilityState>),
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
		purchase(),
		sell(),
		tableScreen("transferBase", is<TransferBaseState>, { "STR_AREA" }),
		transferItems(),
		popupScreen("transferConfirm", is<TransferConfirmState>),
		alienContainment(),
		placeLift(),
		buildFacilities(),
		placeFacility(),
		simpleScreen("dismantleFacility", is<DismantleFacilityState>),
		manufacture(),
		newManufactureList(),
		manufactureStart(),
		manufactureInfo(),
		simpleScreen("sackSoldier", is<SackSoldierState>),
		simpleScreen("memorial", is<SoldierMemorialState>),
		tableScreen("soldiers", is<SoldiersState>, {}),
		research(),
		tableScreen("newResearchList", is<NewResearchListState>, {}),
		researchInfo(),
		article(),
	};
	return screens;
}

}

}
