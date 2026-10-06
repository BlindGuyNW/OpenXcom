#pragma once
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
#include "../Engine/State.h"
#include "../Engine/ModInfo.h"
#include <vector>
#include <string>
#include <utility>

namespace OpenXcom
{

class Window;
class Text;
class TextList;
class TextButton;
class ComboBox;

/**
 * Mods window to manage the installed mods.
 */
class ModListState : public State
{
private:
	Window *_window;
	Text *_txtMaster;
	ComboBox *_cbxMasters;
	TextList *_lstMods;
	TextButton *_btnOk, *_btnCancel, *_btnOpenFolder;
	Text *_txtTooltip;
	std::string _currentTooltip;
	std::vector<const ModInfo *> _masters;
	std::string _curMasterId;
	std::vector< std::pair<std::string, bool> > _mods;
	size_t _curMasterIdx;
public:
	/// Creates the Mods state.
	ModListState();
	/// Cleans up the Mods state.
	~ModListState();
	std::string makeTooltip(const ModInfo &modInfo);
	void cbxMasterHover(Action *action);
	void cbxMasterChange(Action *action);
	void changeMasterMod();
	void revertMasterMod();
	void lstModsRefresh(size_t scrollLoc);
	void lstModsHover(Action *action);
	/// Handler for clicking an item on the menu.
	void lstModsClick(Action *action);
	void toggleMod();
	/// Handler for clicking the left reordering button.
	void lstModsLeftArrowClick(Action *action);
	/// Moves a mod up.
	void moveModUp(Action *action, unsigned int row, bool max = false);
	/// Handler for clicking the right reordering button.
	void lstModsRightArrowClick(Action *action);
	/// Moves a mod down.
	void moveModDown(Action *action, unsigned int row, bool max = false);
	/// Handler for pressing-down a mouse-button in the list.
	void lstModsMousePress(Action *action);
	/// Handler for showing tooltip.
	void txtTooltipIn(Action *action);
	/// Handler for hiding tooltip.
	void txtTooltipOut(Action *action);
	/// Handler for clicking the OK button.
	void btnOkClick(Action *action);
	/// Handler for clicking the Open Folder button.
	void btnOpenFolderClick(Action *action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action *action);
	/// Toggles the selected mod, asking first if it needs another version.
	void tryToggleMod();

	// Accessibility layer: keyboard entry points that don't depend on the mouse position.
	ComboBox *getMasterCombo() const { return _cbxMasters; }
	const ModInfo *getSelectedMaster() const;
	TextList *getModList() const { return _lstMods; }
	TextButton *getOkButton() const { return _btnOk; }
	TextButton *getCancelButton() const { return _btnCancel; }
	const std::vector< std::pair<std::string, bool> > &getMods() const { return _mods; }
	/// Moves a mod one place up (dir < 0) or down, or to the top or bottom, without warping the mouse.
	void moveModByKey(size_t row, int dir, bool max);
};

}
