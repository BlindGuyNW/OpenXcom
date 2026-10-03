# OpenXcom screen reader fork

A personal accessibility fork of OpenXcom (vanilla, SDL 1.2) for playing with a screen reader through Tolk on Windows. There's one target user, so favour minimum viable over generality. Upstream submission isn't planned.

## Scope

- v1 is the **tactical layer** (Battlescape) plus the menus needed to reach it: main menu → New Battle (`Menu/NewBattleState`). The Geoscape and base building are out of scope until asked for.
- Repurposing keys or degrading the sighted/mouse UX is fine when it's needed.
- Windows only for speech. Other platforms may compile, but speech is a no-op there.

## Building

Visual Studio 2022 is installed; build `src/OpenXcom.2010.sln` with MSBuild (the solution is the primary build, CMake is kept in sync but secondary):

```
"C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" src/OpenXcom.2010.sln -p:Configuration=Release -p:Platform=x64 -m -v:minimal -nologo
```

- A full rebuild takes several minutes; run it in the background.
- Output: `bin/x64/Release/OpenXcom.exe`. The post-build step copies every DLL in `deps/lib/<platform>/` to `bin/<platform>/` (one level above the exe).
- C++17, with `_HAS_AUTO_PTR_ETC=1` defined because the codebase still uses `std::unary_function`/`binary_function`, which MSVC drops in C++17.
- The log is full of yaml-cpp C4251/C4275 and conversion warnings from upstream code; only look at warnings in files you touched.

### Adding a source file

Add it in **three** places: `src/OpenXcom.2010.vcxproj` (ClCompile/ClInclude), `src/OpenXcom.2010.vcxproj.filters` (with a `<Filter>`), and `src/CMakeLists.txt` (the matching `*_src` list).

### File format gotchas

- Source files are checked out with **CRLF**; the `.vcxproj` and `.filters` files are also UTF-8 **with BOM**. Preserve both.
- **Don't use `sed -i` from Git Bash on these files**: it silently strips the CRs and the pattern then fails to match. Use the Edit tool, or a Python script that reads/writes bytes.
- Backslashes in heredoc'd Python get mangled through the Bash tool; write such scripts to a file first.

## Running

- Original game data lives in `bin/UFO/` (GEODATA, GEOGRAPH, MAPS, ROUTES, SOUND, TERRAIN, UFOGRAPH, UFOINTRO, UNITS), copied from the Steam install at `C:\Program Files (x86)\Steam\steamapps\common\XCom UFO Defense\XCOM`. It's gitignored.
- Launch with `run.cmd` at the repo root (double-clickable), or from `bin/x64`: `./Release/OpenXcom.exe -data ../`
- User/config folder, `openxcom.log` and `speech.log`: `C:\Users\zklin\OneDrive\Documents\OpenXcom\`.
- **`speech.log` is the main debugging tool**: a timestamped transcript of everything sent to the screen reader (`I` = interrupting, `Q` = queued, `!` = backend status, `-` = silence).

## Tolk

- Loaded at runtime (`LoadLibraryExW` + `GetProcAddress`), so there's no header or import lib; a missing DLL means silence, not a crash. It tries the exe folder, then its parent, then the normal search order.
- DLLs in `deps/lib/x64/` (Tolk, nvdaControllerClient64, SAAPI64) and `deps/lib/Win32/`. They're **not tracked by git** (`*.dll` is ignored, same as SDL). Sources: `C:\git\glfrontier-extended\tolk\x64\` and `C:\git\dangerous-access\libs\tolk\` (32-bit).
- SAPI is enabled as a fallback, so if NVDA isn't running at launch, speech goes to SAPI.

## Accessibility architecture

The design follows the Graph A11y Kernel spec at `C:\git\sims2access\docs\graph-a11y-spec.md`. Read it before building navigation or screens.

- **Menus and popups** become graph screens: an immediate-mode parallel UI tree rebuilt from live game state on each operation. The plan is to reuse the C++ kernel from `C:\git\sims2access\src\Graph\` (with tests in `tests\graph\`).
- **The battle map is not a graph.** It's a separate exploration layer (keyboard tile cursor) that's active only when no graph screen claims the keyboard.
- Axioms to follow everywhere:
  - **Announce once:** a differ over the focused identity (for example the selected unit or cursor tile), not hand-placed announce calls.
  - **Parity:** never reveal what a sighted player can't see. Gate on `Tile::isDiscovered` and visible-unit lists.
  - **Interrupt by provenance:** responses to the player's keypresses interrupt; event narration queues.
  - **One vocabulary module:** every string the mod authors goes through `Access/Vocab`. Game text is already localized and is passed through as is.
  - **Drive the game's own handlers** rather than reimplementing flows. Many `ActionHandler`s ignore their `Action*` argument, so call them with null.

### Code layout

- `src/Access/Speech.{h,cpp}`: `Speech::say(text, interrupt)`, `silence()`, `repeatLast()`, `normalize()`. `normalize` strips `TOK_COLOR_FLIP`, `TOK_NL_SMALL`, newlines and NBSP and collapses whitespace. Output is split into chunks of at most 700 bytes. Main thread only.
- `src/Access/Vocab.{h,cpp}`: the `Vocab::Id` enum plus a string table. Keep the two in the same order. `Vocab::format` fills `{0}`, `{1}` placeholders.
- `src/Access/Graph/`: the graph kernel, copied verbatim from sims2access (deltas: the namespace is `OpenXcom::Graph`, and one C++20 `std::erase_if` is spelled the C++17 way). STL-only, no game headers. Don't edit it for OpenXcom-specific needs; fix upstream and re-copy.
- `tests/graph/`: the kernel conformance suite. Run `tests/graph/build.cmd` (needs C++20, so it builds outside the game; output in `build/graphtests/`).
- `src/Access/Navigator.{h,cpp}`: screen manager plus navigator. Each frame it matches the top `State` against the recipes (poll and diff), keeps a `GraphState` per live `State` so covered screens restore focus, and runs the announce-once differ. `handleEvent` is called from `Game::run` before the game sees an event; it swallows claimed key-downs and their key-ups. It stands down while a `TextEdit` is focused, and skips frames until the top state has run `init()` (`Game::isStateInitialized`); many states set up widgets there, so reading earlier announces half-built screens.
- `src/Access/Screens.{h,cpp}`: the `AccessScreen` recipes (`isActive`, `build`, `name`, `back`) and the registry. Recipes read widgets through `State::getSurfaces()`, so most don't need access to private members.
  - `addWidgets` lists the buttons, combo boxes, sliders and list rows in reading order; a `Customizer` lets a recipe change one widget's node. `simpleScreen` = speak `allText` on arrival + `addWidgets`. `addTextLines` makes one item per screen line (label + value).
  - **These rely on layout heuristics**: `labelFor` (text left on the same row in the same frame, else just above), frame grouping by containment, sort by Y then X, line grouping by exact Y, and a few buttons found by their text (`<<`, translated OK). The user distrusts them. Say so when a new screen relies on them, and switch a screen to explicit wiring (accessors or `friend`) if it mislabels or matters a lot.
  - Screens so far: main menu, New Battle (OK speaks a warning if it fails because the craft is empty), briefing, battle inventory (OK/prev/next only), next turn, craft info, crew, equipment (Left/Right move items), armor, armor picker, soldier info.
- `src/Access/Controls.{h,cpp}`: the one `ControlType` registry and drive helpers. `Controls::click` runs a surface's own press/release/click handlers with a synthetic left click, so sounds and side effects match the mouse. `clickRow` selects a `TextList` row the way hovering would, then clicks it, because the game's list handlers read `getSelectedRow()`. Selectable list rows get Enter as left click and Backspace as right click.
- Widget accessors added for the layer: `ComboBox` (`getOptionCount`, `getSelectedText`, `notifyChange`), `Slider` (`getMin`, `getMax`, `notifyChange`), `TextList` (`setSelectedRow`, `getCellCount`, `isSelectable`). `notifyChange` runs the change handler with a null action.
- `src/Access/Battle.{h,cpp}`: the battle map layer. Not a graph screen: the navigator hands it keys and ticks only when no recipe matches and `BattlescapeState` is on top. It keeps a tile cursor (map coordinates; `Map::setSelectorTile` mirrors it for sighted viewers), describes tiles (unit, items, terrain from `MapData` properties since pieces have no names, then walls per edge), and runs three differs: selected unit (cursor jumps to it), newly visible hostiles (queued), and "our action finished" (queued TUs left). Enter/Backspace call `primaryAction`/`secondaryAction`; it forces `Options::battleNewPreviewPath` on so the first Enter previews and speaks the cost.
  - Map facts: tiles are `x, y, z` with z 0 the ground; direction 0 is north (-y), clockwise. A tile only owns its west and north walls, so east/south walls are the neighbour's west/north. `isDiscovered(2)` = tile seen; `(0)`/`(1)` = its west/north wall seen from the far side. Big-wall objects (`Pathfinding::bigWallTypes`) can block an edge too.
  - `Pathfinding::getTotalTUCost` is wrong after an A* search (it's the last neighbour tried), so `pathCost` walks the path with `getTUCost` like `previewPath` does.
- `src/main.cpp`: `Speech::init()` and `Navigator::init()` after `Options::init`, and `Speech::shutdown()` on exit.

### Keys

- Graph screens: arrows move (Left/Right adjust a slider-like control), Home/End jump to the ends, Tab/Shift+Tab cycle zones, Enter activates, Backspace is the secondary action, Space reads the tooltip, Ctrl+L re-reads the focus with its context, Escape is the screen's `back` if it has one (otherwise it goes to the game).
- Battle map (Battlescape on top, no popup): arrows move the cursor by compass (Up = north), Page Up/Down change level, Home returns to the selected soldier, Enter = left click on the cursor tile (preview, then move; fires while targeting), Backspace = right click (cancel preview/targeting, else turn or open a door; this also takes Backspace away from the game's end turn), Escape cancels targeting, Tab/Shift+Tab cycle soldiers, Space = soldier status, Ctrl+L = cursor tile in full with offset and coordinates, Ctrl+E twice = end turn. Scanner: Period/Comma step through the current category nearest first, Shift+Period/Comma change category (soldiers, enemies, civilians, items, doors, exit area), Slash jumps the cursor there. Left Shift alone is swallowed (the game's previous-soldier key). Game warnings are spoken from `BattlescapeState::warning`.
- Everywhere: Ctrl+R repeats the last speech.
- Free in OpenXcom's own bindings: F6, Ctrl+R, Ctrl+L. F1 to F5 and F7 to F12 are taken.

### Key hook points (tactical)

- Main loop and global keys: `Engine/Game.cpp` `Game::run`. The SDL event poll is where to filter keys the layer claims, before `_states.back()->handle()`.
- Screen stack: `Game::pushState`/`popState`. A screen recipe's `IsActive` is a `dynamic_cast` on the top state.
- Battlescape input: `BattlescapeState::handle`. It ignores input while the cursor is hidden. Bindings are in `Engine/Options.inc.h`; the arrow keys belong to `Camera::keyboardPress`.
- Tile cursor: `Map::setSelectorPosition` takes screen pixels, so add a map-coordinate setter. `BattlescapeGame::primaryAction(Position)` and `secondaryAction(Position)` are what a click does.
- Path cost: `Pathfinding::calculate`, then sum `getTUCost` along `getPath()` (not `getTotalTUCost`). Hit chance: `BattleUnit::getFiringAccuracy`.
- Newly spotted units: `BattleUnit::addToVisibleUnits` returns true on first sight.
- Events: the `BattleState` subclasses (`UnitWalkBState`, `ProjectileFlyBState`, `ExplosionBState`, `UnitDieBState`, and so on), plus `BattlescapeGame::checkForCasualties` and `endTurn`.
- Warnings: `BattlescapeState::warning`. Selection changes: `BattlescapeState::updateSoldierInfo`.

## Roadmap

1. ~~Speech wrapper, text cleanup, transcript, vocabulary module, C++17~~ (done)
2. ~~Graph kernel, navigator, key filter in `Game::run`, screen manager over the `State` stack; prove it on the main menu~~ (done)
3. ~~New Battle setup screen, Equip Craft (crew, equipment, armor, soldier info), briefing, next turn~~ (done)
4. Map exploration layer: tile cursor, confirm via `primaryAction`, selection/cursor differ, parity gating (cursor and scanner in)
5. Action menu, unit and enemy list overlays, event narration
6. Inventory

## Git

- Work on the `accessibility` branch. `master` tracks upstream. Remotes: `origin` is the user's fork (BlindGuyNW/OpenXcom) and `upstream` is OpenXcom/OpenXcom.
- Commit as work lands. An earlier attempt at this fork was lost because it was never committed.
