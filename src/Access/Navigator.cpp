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
#include "Navigator.h"
#include <algorithm>
#include <exception>
#include <map>
#include <memory>
#include <set>
#include "Screens.h"
#include "Speech.h"
#include "Vocab.h"
#include "Graph/GraphAnnouncer.hpp"
#include "Graph/KeyGraph.hpp"
#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/State.h"
#include "../Interface/TextEdit.h"

namespace OpenXcom
{

namespace Navigator
{

using namespace Graph;

namespace
{
	Game *_game = 0;
	/// The attached recipe and the state it's attached to; null when no recipe matches the top state.
	const AccessScreen *_screen = 0;
	State *_state = 0;
	/// Cursor state per live game state, so a covered screen restores where you were.
	std::map<State *, GraphState> _cursors;
	std::unique_ptr<KeyGraph> _graph;
	/// The differ's memory: the focused identity last spoken (spec 7.3). Invalid = nothing yet.
	ControlId _spoken;
	/// Keys whose key-down we swallowed, so their key-up is swallowed too.
	std::set<SDLKey> _swallowed;
	/// One fault log per attach (spec 7.9).
	bool _faultLogged = false;

	void fault(const char *where, const char *what)
	{
		if (_faultLogged)
			return;
		_faultLogged = true;
		Log(LOG_ERROR) << "Access: " << (_screen ? _screen->key : std::string("?")) << " " << where << " threw: " << what;
	}

	/// Runs a host callback, logging instead of letting a throw reach the game loop.
	template <typename F>
	bool guarded(const char *where, F fn)
	{
		try
		{
			fn();
			return true;
		}
		catch (const std::exception &e)
		{
			fault(where, e.what());
		}
		catch (...)
		{
			fault(where, "unknown exception");
		}
		return false;
	}

	std::unique_ptr<GraphRender> render()
	{
		std::unique_ptr<GraphRender> result;
		if (!_screen || !_state)
			return result;
		guarded("build", [&]
		{
			GraphBuilder builder(&_cursors[_state].Expanded);
			_screen->build(builder, _state);
			result = builder.Build();
		});
		return result;
	}

	State *topState()
	{
		const std::list<State *> &states = _game->getStates();
		return states.empty() ? 0 : states.back();
	}

	void say(const std::string &text, bool interrupt)
	{
		Speech::say(text, interrupt);
	}

	/// Attaches to the top state's recipe when the top state or its match changed (spec 9).
	void sync()
	{
		State *top = topState();
		const AccessScreen *match = 0;
		if (top)
		{
			for (const AccessScreen &s : Screens::all())
			{
				bool active = false;
				guarded("isActive", [&] { active = s.isActive(top); });
				if (active)
				{
					match = &s;
					break;
				}
			}
		}
		if (top == _state && match == _screen)
			return;

		// Drop cursors for states that have left the stack.
		const std::list<State *> &states = _game->getStates();
		for (std::map<State *, GraphState>::iterator i = _cursors.begin(); i != _cursors.end();)
		{
			if (std::find(states.begin(), states.end(), i->first) == states.end())
				i = _cursors.erase(i);
			else
				++i;
		}

		_state = top;
		_screen = match;
		_graph.reset();
		_spoken = ControlId();
		_faultLogged = false;
		if (!_screen)
			return;

		Log(LOG_INFO) << "Access: attached " << _screen->key;
		_graph.reset(new KeyGraph(render, &_cursors[_state]));
		if (_screen->name)
		{
			std::string name;
			guarded("name", [&] { name = _screen->name(_state); });
			say(name, false);
		}
	}

	/// Speaks a keypress-driven move, interrupting, and tells the differ it's been said.
	void announce(const MoveResult &r)
	{
		if (!r.Moved || !r.To)
			return;
		std::string text;
		guarded("announce", [&] { text = GraphAnnouncer::Compose(r.From, r.To, r.TransitionLabel); });
		say(text, true);
		_spoken = r.To->Id;
	}

	/// The focused control's state line after an activation or adjust (spec 7.5).
	void stateFeedback()
	{
		if (!_graph || topState() != _state || !_graph->Rerender())
			return;
		GraphNode *node = _graph->CurrentNode();
		if (node && node->Vtable.StateText)
		{
			std::string text;
			guarded("stateText", [&] { text = node->Vtable.StateText(); });
			say(text, true);
		}
	}

	void whereAmI()
	{
		GraphNode *node = _graph->Rerender() ? _graph->CurrentNode() : 0;
		std::string text;
		if (node)
		{
			guarded("announce", [&] { text = GraphAnnouncer::ComposeFull(node); });
			_spoken = node->Id;
		}
		else if (_screen->name)
		{
			guarded("name", [&] { text = _screen->name(_state); });
		}
		say(text, true);
	}

	/// Is a text field taking typing? Then the layer stands down entirely (spec 8).
	bool textFieldLive()
	{
		for (Surface *s : _state->getSurfaces())
		{
			TextEdit *edit = dynamic_cast<TextEdit *>(s);
			if (edit && edit->isFocused())
				return true;
		}
		return false;
	}

	/// Acts on a key-down for the attached screen. Returns whether the layer owns the key.
	bool dispatch(SDLKey key, bool shift, bool ctrl)
	{
		if (ctrl)
		{
			if (key == SDLK_l)
			{
				whereAmI();
				return true;
			}
			return false;
		}
		switch (key)
		{
		case SDLK_UP:
			announce(_graph->Move(GraphDir::Up));
			return true;
		case SDLK_DOWN:
			announce(_graph->Move(GraphDir::Down));
			return true;
		case SDLK_LEFT:
		case SDLK_RIGHT:
		{
			int sign = key == SDLK_LEFT ? -1 : 1;
			bool adjusted = false;
			guarded("adjust", [&] { adjusted = _graph->TryAdjust(sign, shift); });
			if (adjusted)
				stateFeedback();
			else
				announce(_graph->Move(sign < 0 ? GraphDir::Left : GraphDir::Right));
			return true;
		}
		case SDLK_HOME:
			announce(_graph->MoveToEdge(GraphDir::Up));
			return true;
		case SDLK_END:
			announce(_graph->MoveToEdge(GraphDir::Down));
			return true;
		case SDLK_TAB:
			announce(_graph->MoveStop(shift ? -1 : 1, true));
			return true;
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
		{
			bool activated = false;
			guarded("activate", [&] { activated = _graph->Activate(); });
			if (activated)
				stateFeedback();
			return true;
		}
		case SDLK_BACKSPACE:
		{
			bool activated = false;
			guarded("secondary", [&] { activated = _graph->Secondary(); });
			if (activated)
				stateFeedback();
			return true;
		}
		case SDLK_SPACE:
		{
			bool had = false;
			guarded("tooltip", [&] { had = _graph->Tooltip(); });
			if (!had)
				say(Vocab::get(Vocab::NO_TOOLTIP), true);
			return true;
		}
		case SDLK_ESCAPE:
			if (!_screen->back)
				return false;
			guarded("back", [&] { _screen->back(_state); });
			return true;
		default:
			return false;
		}
	}
}

void init()
{
	GraphAnnouncer::PositionText = [](int index, int count)
	{
		return Vocab::format(Vocab::POSITION, { std::to_string(index), std::to_string(count) });
	};
}

bool handleEvent(Game *game, const SDL_Event &ev)
{
	_game = game;
	SDLKey key = ev.key.keysym.sym;
	if (ev.type == SDL_KEYUP)
		return _swallowed.erase(key) > 0;
	if (ev.type != SDL_KEYDOWN)
		return false;

	SDLMod mod = ev.key.keysym.mod;
	bool shift = (mod & KMOD_SHIFT) != 0, ctrl = (mod & KMOD_CTRL) != 0, alt = (mod & KMOD_ALT) != 0;

	bool claimed = false;
	if (ctrl && !alt && !shift && key == SDLK_r)
	{
		Speech::repeatLast();
		claimed = true;
	}
	else if (!alt)
	{
		sync();
		if (_graph && !textFieldLive())
			claimed = dispatch(key, shift, ctrl);
	}
	if (claimed)
		_swallowed.insert(key);
	return claimed;
}

void update(Game *game)
{
	_game = game;
	// A just-pushed state hasn't run init() yet, and many set up their widgets there.
	// Reading it now would announce half-built screens.
	if (!game->isStateInitialized())
		return;
	sync();
	if (!_graph || !_graph->Rerender())
		return;
	GraphNode *node = _graph->CurrentNode();
	if (!node || node->Id == _spoken)
		return;
	// The differ (spec 7.3): one place announces every focus change that no keypress already spoke.
	std::string text;
	guarded("announce", [&] { text = GraphAnnouncer::Compose(_graph->Current()->NodeAt(_spoken), node); });
	say(text, false);
	_spoken = node->Id;
}

}

}
