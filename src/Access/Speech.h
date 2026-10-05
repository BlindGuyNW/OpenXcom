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
#include <string>

namespace OpenXcom
{

/**
 * Screen reader output (graph a11y spec P5), text cleanup (P10)
 * and the speech transcript (P9).
 * Backed by Tolk.dll, loaded at runtime so a missing DLL means silence, not a crash.
 * Only call from the main thread.
 */
namespace Speech
{
	/// Loads Tolk and opens the transcript next to the game log. Call after Options::init.
	void init();
	/// Unloads Tolk and closes the transcript.
	void shutdown();
	/// Is a speech backend loaded?
	bool isActive();
	/// Speaks UTF-8 text. interrupt=true cuts off current speech, false queues.
	void say(const std::string &text, bool interrupt);
	/// Stops all current speech.
	void silence();
	/// Speaks the last non-empty text again, interrupting.
	void repeatLast();
	/// Strips game formatting codes and collapses whitespace into one line.
	/// A lone ">" (the game's label separator) becomes a colon.
	std::string normalize(const std::string &text);
}

}
