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
#include "Speech.h"
#include <cctype>
#include <chrono>
#include <cstdio>
#include <vector>
#include "../Engine/Logger.h"
#include "../Engine/Unicode.h"
#include "Vocab.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace OpenXcom
{

namespace Speech
{

namespace
{
	/// Some screen readers choke on long payloads; stay well under.
	const size_t MAX_CHUNK = 700;

	std::string _last;
	FILE *_transcript = 0;
	std::chrono::steady_clock::time_point _start;

#ifdef _WIN32
	typedef void (*TolkVoid)();
	typedef void (*TolkBool)(bool);
	typedef const wchar_t *(*TolkDetect)();
	typedef bool (*TolkOutput)(const wchar_t *, bool);
	typedef bool (*TolkSilence)();

	HMODULE _tolk = 0;
	TolkVoid _tolkUnload = 0;
	TolkOutput _tolkOutput = 0;
	TolkSilence _tolkSilence = 0;

	std::wstring toWide(const std::string &utf8)
	{
		if (utf8.empty())
			return std::wstring();
		int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), 0, 0);
		std::wstring wide(len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &wide[0], len);
		return wide;
	}

	std::string toUtf8(const wchar_t *wide)
	{
		if (!wide || !*wide)
			return std::string();
		int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, 0, 0, 0, 0);
		std::string utf8(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wide, -1, &utf8[0], len, 0, 0);
		utf8.resize(len - 1);
		return utf8;
	}

	/// Loads Tolk.dll by full path so it finds its screen reader client DLLs beside it.
	/// Tries the executable's folder, then its parent (Visual Studio builds put the exe
	/// in bin\x64\Release but copy the DLLs to bin\x64), then the normal search order.
	HMODULE loadTolk()
	{
		wchar_t path[MAX_PATH];
		DWORD len = GetModuleFileNameW(0, path, MAX_PATH);
		std::wstring dir(path, len);
		for (int i = 0; i < 2; ++i)
		{
			size_t slash = dir.find_last_of(L"\\/");
			if (slash == std::wstring::npos)
				break;
			dir.resize(slash);
			std::wstring dll = dir + L"\\Tolk.dll";
			HMODULE module = LoadLibraryExW(dll.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
			if (module)
				return module;
		}
		return LoadLibraryW(L"Tolk.dll");
	}
#endif

	void writeTranscript(const char *tag, const std::string &text)
	{
		if (!_transcript)
			return;
		long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _start).count();
		fprintf(_transcript, "%8lld.%03lld %s %s\n", ms / 1000, ms % 1000, tag, text.c_str());
		fflush(_transcript);
	}

	/// Splits on sentence, then whitespace, then codepoint boundaries.
	std::vector<std::string> chunk(const std::string &text)
	{
		std::vector<std::string> chunks;
		size_t pos = 0;
		while (text.size() - pos > MAX_CHUNK)
		{
			size_t end = std::string::npos;
			for (size_t i = pos + MAX_CHUNK; i > pos; --i)
			{
				char c = text[i - 1];
				if ((c == '.' || c == '!' || c == '?') && text[i] == ' ')
				{
					end = i;
					break;
				}
			}
			if (end == std::string::npos)
			{
				size_t space = text.rfind(' ', pos + MAX_CHUNK);
				if (space != std::string::npos && space > pos)
					end = space;
			}
			if (end == std::string::npos)
			{
				end = pos + MAX_CHUNK;
				while (end > pos && (text[end] & 0xC0) == 0x80)
					--end;
				if (end == pos) // not valid UTF-8, cut anywhere
					end = pos + MAX_CHUNK;
			}
			chunks.push_back(text.substr(pos, end - pos));
			pos = end;
			while (pos < text.size() && text[pos] == ' ')
				++pos;
		}
		if (pos < text.size())
			chunks.push_back(text.substr(pos));
		return chunks;
	}
}

void init()
{
	_start = std::chrono::steady_clock::now();

	std::string log = Logger::logFile();
	size_t slash = log.find_last_of("\\/");
	std::string path = (slash == std::string::npos ? "" : log.substr(0, slash + 1)) + "speech.log";
	_transcript = fopen(path.c_str(), "w");

#ifdef _WIN32
	_tolk = loadTolk();
	if (!_tolk)
	{
		Log(LOG_WARNING) << "Speech: Tolk.dll not found, speech disabled";
		writeTranscript("!", "Tolk.dll not found");
		return;
	}
	TolkVoid load = (TolkVoid)GetProcAddress(_tolk, "Tolk_Load");
	TolkBool trySapi = (TolkBool)GetProcAddress(_tolk, "Tolk_TrySAPI");
	TolkDetect detect = (TolkDetect)GetProcAddress(_tolk, "Tolk_DetectScreenReader");
	_tolkUnload = (TolkVoid)GetProcAddress(_tolk, "Tolk_Unload");
	_tolkOutput = (TolkOutput)GetProcAddress(_tolk, "Tolk_Output");
	_tolkSilence = (TolkSilence)GetProcAddress(_tolk, "Tolk_Silence");
	if (!load || !_tolkUnload || !_tolkOutput || !_tolkSilence)
	{
		Log(LOG_WARNING) << "Speech: Tolk.dll is missing exports, speech disabled";
		writeTranscript("!", "Tolk.dll is missing exports");
		FreeLibrary(_tolk);
		_tolk = 0;
		_tolkOutput = 0;
		return;
	}
	if (trySapi)
		trySapi(true);
	load();
	std::string reader = detect ? toUtf8(detect()) : "";
	if (reader.empty())
		reader = "none";
	Log(LOG_INFO) << "Speech: Tolk loaded, screen reader: " << reader;
	writeTranscript("!", "Tolk loaded, screen reader: " + reader);
#else
	writeTranscript("!", "No speech backend on this platform");
#endif
}

void shutdown()
{
#ifdef _WIN32
	if (_tolk)
	{
		_tolkUnload();
		FreeLibrary(_tolk);
		_tolk = 0;
		_tolkOutput = 0;
	}
#endif
	if (_transcript)
	{
		fclose(_transcript);
		_transcript = 0;
	}
}

bool isActive()
{
#ifdef _WIN32
	return _tolkOutput != 0;
#else
	return false;
#endif
}

void say(const std::string &text, bool interrupt)
{
	std::string clean = normalize(text);
	if (clean.empty())
		return;
	_last = clean;
	std::vector<std::string> chunks = chunk(clean);
	for (size_t i = 0; i < chunks.size(); ++i)
	{
		// Only the first chunk may cut off what's already playing.
		bool cut = interrupt && i == 0;
		writeTranscript(cut ? "I" : "Q", chunks[i]);
#ifdef _WIN32
		if (_tolkOutput)
			_tolkOutput(toWide(chunks[i]).c_str(), cut);
#endif
	}
}

void silence()
{
	writeTranscript("-", "silence");
#ifdef _WIN32
	if (_tolkSilence)
		_tolkSilence();
#endif
}

void repeatLast()
{
	say(_last.empty() ? Vocab::get(Vocab::NOTHING_TO_REPEAT) : _last, true);
}

std::string normalize(const std::string &text)
{
	std::string out;
	out.reserve(text.size());
	bool space = false;
	for (size_t i = 0; i < text.size(); ++i)
	{
		unsigned char c = text[i];
		bool isSpace = false;
		if (c == Unicode::TOK_COLOR_FLIP)
		{
			continue;
		}
		else if (c < 32 || c == ' ')
		{
			// newlines, TOK_NL_SMALL, tabs and other control bytes
			isSpace = true;
		}
		else if (c == 0xC2 && i + 1 < text.size() && (unsigned char)text[i + 1] == Unicode::TOK_NBSP)
		{
			// UTF-8 non-breaking space. Between digits it's Unicode::formatNumber's thousands
			// separator ("8 000"), which screen readers say as separate numbers, so it goes.
			++i;
			bool digitBefore = !space && !out.empty() && isdigit((unsigned char)out.back());
			bool digitAfter = i + 1 < text.size() && isdigit((unsigned char)text[i + 1]);
			if (digitBefore && digitAfter)
				continue;
			isSpace = true;
		}
		if (isSpace)
		{
			space = !out.empty();
			continue;
		}
		// The game's "label>value" separator ("TURN>1", "COST> $50") reads as a colon.
		// Runs like ">>" are buttons and banners, so they stay.
		bool lone = c == '>' && (i == 0 || text[i - 1] != '>') && (i + 1 == text.size() || text[i + 1] != '>');
		if (lone && !out.empty())
		{
			out += ':';
			space = true;
			continue;
		}
		if (space)
		{
			out += ' ';
			space = false;
		}
		out += (char)c;
	}
	return out;
}

}

}
