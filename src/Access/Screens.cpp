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
#include "../Engine/State.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Menu/MainMenuState.h"

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

/// Adds every visible text button of a state as a vertical list, in the order the state added them.
/// Keys are the buttons' indices among the state's elements, which only change if the state's code does.
void addTextButtons(GraphBuilder &b, State *state)
{
	const std::vector<Surface *> &surfaces = state->getSurfaces();
	for (size_t i = 0; i < surfaces.size(); ++i)
	{
		TextButton *btn = dynamic_cast<TextButton *>(surfaces[i]);
		if (btn && btn->getVisible())
			b.AddItem(ControlId::Referenced(btn, "button:" + std::to_string(i)), Controls::textButton(state, btn));
	}
}

AccessScreen mainMenu()
{
	AccessScreen s;
	s.key = "mainMenu";
	s.isActive = [](State *state) { return dynamic_cast<MainMenuState *>(state) != nullptr; };
	s.name = firstText;
	s.build = addTextButtons;
	return s;
}

}

const std::vector<AccessScreen> &all()
{
	static const std::vector<AccessScreen> screens = { mainMenu() };
	return screens;
}

}

}
