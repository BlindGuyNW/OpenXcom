# OpenXcom screen reader fork

A personal accessibility fork of OpenXcom (vanilla, SDL 1.2) for playing with a screen reader through Tolk on Windows. There's one target user, so favour minimum viable over generality. Upstream submission isn't planned.

## Scope

- v1 was the **tactical layer** (Battlescape) plus the menus needed to reach it: main menu → New Battle (`Menu/NewBattleState`). It's playable end to end.
- v2 (asked for 2026-10-04) is the **campaign**: Geoscape, interception, base management. The plan is in `docs/geoscape-plan.md`; read it before starting campaign work.
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

- `src/Access/Speech.{h,cpp}`: `Speech::say(text, interrupt)`, `silence()`, `repeatLast()`, `normalize()`. `normalize` strips `TOK_COLOR_FLIP`, `TOK_NL_SMALL`, newlines and NBSP and collapses whitespace, and turns a lone `>` (the game's "TURN>1" separator) into a colon; runs like `>>` stay. Output is split into chunks of at most 700 bytes. Main thread only.
- `src/Access/Vocab.{h,cpp}`: the `Vocab::Id` enum plus a string table. Keep the two in the same order. `Vocab::format` fills `{0}`, `{1}` placeholders.
- `src/Access/TerrainNames.{h,cpp,inc}`: names for terrain pieces, which the game never names, keyed by MCD set and index (`"CULTIVAT", 9, "apple tree"`). The `.inc` table was labelled by eye from sprite contact sheets made with `scripts/terrain_sheets.py <outdir>` (numbered sprites per set, plus `pieces.csv` with each piece's properties); it covers all 1,228 vanilla pieces. It's the one place mod text lives outside Vocab. Fix a wrong name directly in the `.inc`.
- `src/Access/Graph/`: the graph kernel, copied verbatim from sims2access (deltas: the namespace is `OpenXcom::Graph`, and one C++20 `std::erase_if` is spelled the C++17 way). STL-only, no game headers. Don't edit it for OpenXcom-specific needs; fix upstream and re-copy.
- `tests/graph/`: the kernel conformance suite. Run `tests/graph/build.cmd` (needs C++20, so it builds outside the game; output in `build/graphtests/`).
- `src/Access/Navigator.{h,cpp}`: screen manager plus navigator. Each frame it matches the top `State` against the recipes (poll and diff), keeps a `GraphState` per live `State` so covered screens restore focus, and runs the announce-once differ. Every recipe and layer callback runs guarded: a throw is logged to `openxcom.log` and spoken once per attach ("Accessibility error on newGame, in build: ..."), so a broken screen doesn't just go silent. `handleEvent` is called from `Game::run` before the game sees an event; it swallows claimed key-downs and their key-ups. It stands down while a `TextEdit` is focused, except for the typing echo (`watchEdit`, a differ over the field's text and `TextEdit::getCaretPos`: "Editing X" on focus, then the characters typed or deleted, or the one the caret lands on, "blank" at the end) and Ctrl+L, which reads the whole field, and skips frames until the top state has run `init()` (`Game::isStateInitialized`); many states set up widgets there, so reading earlier announces half-built screens.
- `src/Access/Screens.{h,cpp}`: the `AccessScreen` recipes (`isActive`, `build`, `name`, `back`) and the registry. Recipes read widgets through `State::getSurfaces()`, so most don't need access to private members.
  - `addWidgets` lists the buttons, arrow buttons, text fields, combo boxes, sliders and list rows in reading order (arrow buttons are labelled by `labelFor` plus their direction, a heuristic; `ARROW_NONE` sort toggles are skipped); a `Customizer` lets a recipe change one widget's node. `simpleScreen` = speak `allText` on arrival + `addWidgets`. `addTextLines` makes one item per screen line (label + value).
  - **These rely on layout heuristics**: `labelFor` (text left on the same row in the same frame, else just above), frame grouping by containment, sort by Y then X, line grouping by exact Y, and a few buttons found by their text (`<<`, translated OK). The user distrusts them. Say so when a new screen relies on them, and switch a screen to explicit wiring (accessors or `friend`) if it mislabels or matters a lot.
  - Screens so far: main menu, New Battle (OK speaks a warning if it fails because the craft is empty), briefing, inventory (body slots as one list with each slot as a context, then ground, then buttons; Enter picks up and puts down through keyboard calls added to `Inventory` (`pickUp`, `placeSelected`, `cancelSelected`) that reuse its fit/TU/sound code; held ammo onto a weapon loads it; Escape puts a held item back or presses OK), next turn, craft info, crew, equipment (rows read "Pistol, 4 in stores, 2 on craft", or "unlimited in stores" for New Battle's "-"; Left/Right move one item, Shift five, Ctrl as many as fit or all back, like right-clicking the row's arrows; Enter re-reads the row with a how-to hint, since the game ignores clicks on the row itself), armor, armor picker, soldier info, action menu (Q/E popup; explicit accessors on `ActionMenuItem`/`ActionMenuState`, items sorted top to bottom as drawn), medikit (explicit accessors on `MedikitState` and `MedikitView::setSelectedPart`: says the target, health for our units only, and where the fatal wounds are on arrival; six body parts (Enter selects, Heal works on the selected part), Heal, Stimulant, Pain Killer with what's left and the TU cost, Close; after each use it says the result and the healer's TUs; the game closes it itself when a unit is revived), prime grenade (0 to 23; Escape sends the state a right-button press, since the buttons only take left clicks), debriefing (explicit accessors on `DebriefingState`: says the outcome title, rating and total on arrival, then score rows, recovered items, OK and Stats; Stats switches to each soldier's stat gains named from the column headers' tooltips; in New Battle, OK returns to the main menu, and the campaign follow-ups (promotions, containment) aren't covered). Escape menu (`PauseState`: load, save, abandon, options; the options screens aren't covered), abandon confirmation, abort mission (A), load and save lists (saving: Enter on a row opens the game's name field, the navigator stands down while you type, apart from the typing echo, Enter saves), delete and load confirmations, `ErrorMessageState`: all plain `simpleScreen`s.
  - `AccessScreen::tick` (optional) runs every frame while its recipe is attached, for narration that follows the game.
- `src/Access/Controls.{h,cpp}`: the one `ControlType` registry and drive helpers. `Controls::click` runs a surface's own press/release/click handlers with a synthetic left click, so sounds and side effects match the mouse. `clickRow` selects a `TextList` row the way hovering would, then clicks it, because the game's list handlers read `getSelectedRow()`. Selectable list rows get Enter as left click and Backspace as right click. Text buttons in a group say "selected" on the chosen one and toggle buttons "on"/"off" (a custom announcement kind, not `Selected`, which would move the landing node). Arrow buttons: Enter left-clicks, Backspace right-clicks (as far as it goes on the game's spinners), then the label is read again since it usually holds the value. Text fields: Enter focuses them; the navigator stands down until the game unfocuses them, apart from the typing echo.
- Widget accessors added for the layer: `ComboBox` (`getOptionCount`, `getSelectedText`, `notifyChange`), `Slider` (`getMin`, `getMax`, `notifyChange`), `TextList` (`setSelectedRow`, `getCellCount`, `isSelectable`), `TextButton::getGroup`, `ArrowButton::getShape`. `notifyChange` runs the change handler with a null action.
- `src/Access/Battle.{h,cpp}`: the battle map layer. Not a graph screen: the navigator hands it keys and ticks only when no recipe matches and `BattlescapeState` is on top. It keeps a tile cursor (map coordinates; `Map::setSelectorTile` mirrors it for sighted viewers), describes tiles (unit, items, the object by name, the floor by name only when it changes from the last cursor step (always on Ctrl+L), then edges grouped by name: "stone wall north and east, wooden door south"; unnamed pieces fall back to wording guessed from `MapData` properties), and runs three differs: selected unit (cursor jumps to it), newly visible hostiles (queued), and "our action finished" (queued TUs left). Enter/Backspace call `primaryAction`/`secondaryAction`; it forces `Options::battleNewPreviewPath` on so the first Enter previews and speaks the cost.
  - Map facts: tiles are `x, y, z` with z 0 the ground; direction 0 is north (-y), clockwise. A tile only owns its west and north walls, so east/south walls are the neighbour's west/north. `isDiscovered(2)` = tile seen; `(0)`/`(1)` = its west/north wall seen from the far side. Big-wall objects (`Pathfinding::bigWallTypes`) can block an edge too.
  - `Pathfinding::getTotalTUCost` is wrong after an A* search (it's the last neighbour tried), so `pathCost` walks the path with `getTUCost` like `previewPath` does.
- `src/Access/Geo.{h,cpp}`: the Geoscape layer. Not a graph screen: like `Battle`, the navigator hands it keys and ticks when `GeoscapeState` is on top, no recipe matches and no dogfight window is open (all minimized). It runs a speed differ (`GeoscapeState::getTimeSpeed`, added) and the globe scanner: targets whose `getMarker() != -1` (exactly what the globe draws, so it's the parity gate), nearest the first placed base first, read as "UFO-1, Brazil, 1,230 nautical miles southwest". Enter pushes `MultipleTargetsState` with the one target, which routes it like a globe click. `Geo::placeName` (country, else region) and `Geo::offsetText` are shared with the screens.
- `src/Access/Dogfight.{h,cpp}`: interceptions. Dogfights aren't states, so the recipe (`Dogfight::screen`) matches `GeoscapeState` while any window is open (not minimized): one context per window with distance and damage, the five attack modes ("selected" on the current one; no StateText because the window's own status message says the change), each weapon ("weapon 1, Stingray, ammo 6, in range, on"; Enter toggles), minimize. `Dogfight::update` runs every frame the Geoscape is on top (recipe or not) and narrates, queued, an interception starting and ending, weapons entering or leaving range (distance <= range * 8, the firing test) and running dry; `DogfightState::setStatus` calls `Dogfight::status` (spoken unless minimized; the outrunning message, re-set every tick, only once). Narration keeps strings, never pointers. Accessors added on `DogfightState` (mode buttons, weapon buttons and flags, distance, minimize button and icon), `InterceptState::getCrafts`, `SelectDestinationState::getCraft`.
- Campaign screens: new game, the base city picker (`BuildNewBaseState`: cities grouped by region from `RuleRegion::getCities`, with the base cost after the first base; Enter nudges the point onto land if the city sits just off the polygons, centres the globe there and `Controls::click`s the globe, so the game's own `globeClick` places the base; accessors `getGlobe`/`isFirst` added), base name (the field is focused from the start; the echo says "Editing" after the title), confirm new base, the Geoscape popups as `popupScreen` (all text plus every list row on arrival: UFO detected/lost, mission detected, alien base, craft patrol, low fuel, craft error, research complete/required, new possible research ("No new research topics" when the game shows it empty) and manufacture, production complete, items arriving, multiple targets, target info, craft info, confirm destination), the Ufopaedia start and select lists, intercept (rows from `getCrafts`: "Interceptor-1, READY, base, 2 weapons, 0 soldiers, 0 tanks"; Enter launches, or says why not using the game's own test), select destination (targets the craft can go to, as `Globe::getTargets(craft=true)` allows, nearest the craft first, then cities by region; each with distance from the craft and in or out of the range circle (`Craft::getBaseRange`); Enter pushes `MultipleTargetsState` with the target or a fresh `Waypoint`, as a globe click does), and the confirm landing, aliens crash, base destroyed, confirm Cydonia and dogfight error popups. Escape keeps the game's behaviour on all of them (plan decision 3).
- `State::getGamePtr()` is a static accessor added for the layer, since `State` has no public `getGame()`.
- `Navigator::init` forces `Options::dogfightSpeed` to 50 ms, the slowest the options allow (plan decision 2).
- `src/main.cpp`: `Speech::init()` and `Navigator::init()` after `Options::init`, and `Speech::shutdown()` on exit.

### Keys

- Graph screens: arrows move (Left/Right adjust a slider-like control, Shift for bigger steps, Ctrl all the way: `Controls::ADJUST_LIMIT` as the sign), Home/End jump to the ends, Tab/Shift+Tab cycle zones, Enter activates, Backspace is the secondary action, Space reads the tooltip, Ctrl+L re-reads the focus with its context, Escape is the screen's `back` if it has one (otherwise it goes to the game).
- Battle map (Battlescape on top, no popup): arrows move the cursor by compass (Up = north), Page Up/Down change level, Home returns to the selected soldier, Enter = left click on the cursor tile (preview, then move; fires while targeting), Backspace = right click (cancel preview/targeting, else turn or open a door; this also takes Backspace away from the game's end turn), Escape cancels targeting, Tab/Shift+Tab cycle soldiers, Space = soldier status, Ctrl+L = cursor tile in full with offset and coordinates, Ctrl+E twice = end turn. Scanner: Period/Comma step through the current category nearest first, Shift+Period/Comma change category (soldiers, enemies, civilians, items, doors, exits: the craft's entrance tiles, where soldiers must stand to escape an abort, plus stage exit tiles), Slash jumps the cursor there. While aiming (after picking a shot or throw in the action menu) the layer says the action and accuracy, jumps the cursor to the nearest enemy the soldier can see, and adds "in view"/"out of view" for units under the cursor (parity: the HUD's enemy buttons show the same); Enter fires. Blaster Launcher: Enter drops a waypoint ("waypoint 1 of N"), Enter again on the last waypoint presses the HUD's launch button (`BattlescapeGame::launchAction`); Escape/Backspace remove the last waypoint, then cancel the aim. Stance changes of the selected soldier (K) say "kneeling"/"standing". Visible hostiles carry their facing ("facing south", plus "toward you" when the selected soldier is in their 90° view cone, `BattleUnit::checkViewSector`, which is what reaction fire uses) on the cursor tile, in the scanner and when spotted; the sprite shows the same. Tile descriptions end in "dark" when `Tile::getShade() > TileEngine::MAX_DARKNESS_TO_SEE_UNITS` (our soldiers can't spot a unit there from beyond 9 tiles; aliens ignore light). Open-air tiles leave it to the ground tile they describe. Narration (queued): spotted hostiles, any shown unit killed or knocked out, our soldiers bleeding from fatal wounds at the turn's start ("Vasile Anton bleeding, health 10"; actual hits are spoken at impact), and each shot the player didn't order as its projectile appears (`Map::getProjectile`): aliens' fire and our reaction fire on the aliens' turn. The shooter is `Projectile::getActor()` (added); `getOrigin()` is the trajectory's first tile, often next to the shooter, so it's only used for the bearing: "Sectoid fires at X" if the shooter is visible, else "fire from northeast, above, at X" (compass from target to origin; auto-shot bursts are spoken once). Shot results (queued) follow each impact, for our shots and any shot whose shooter or target was shown: "Sectoid Soldier hit" (our units add their health), "missed, hit wooden fence" (the terrain piece struck; a unit we can't see counts as a miss), "explosion hits no one"; a unit the hit takes down gets only the killed/unconscious line. `ExplosionBState::explode` brackets `TileEngine::hit`/`explode` with `Battle::beginImpact`/`endImpact` (a health and stun snapshot), and `TileEngine::getLastHitUnit`/`getLastHitPart` (added) say what a bullet struck; there's no pain sound in the game, only death screams. Units that aren't ours are named by race, not rank ("Sectoid", not "Sectoid Leader": every rank shares a sprite; the game shows ranks only in panic messages, mind probes and a stunned body in the inventory, which pass the game's text through), plus a per-sighting number (`unitLabel`): given when first named in view, retired with "lost sight of Sectoid 2" when the unit leaves view, never reused, so a returning alien gets a new number. Left Shift alone is swallowed (the game's previous-soldier key). Every `WarningMessage::showMessage` is spoken (battlescape and inventory warnings).
- Geoscape (no popup, no open dogfight): Space = date, time, funds and speed; Period/Comma step through the scanner's category nearest the first base first, Shift+Period/Comma change category (UFOs, alien sites, alien bases, craft in flight, bases, waypoints), Enter opens the entry like clicking it, Ctrl+L re-reads it, D restores a minimized interception window. The game's own keys pass through: 1 to 6 speed (changes are spoken), I intercept, B bases, U Ufopaedia, G graphs, F funding, Escape options.
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
- Warnings: `WarningMessage::showMessage` (spoken). Selection changes: `BattlescapeState::updateSoldierInfo`.

## Roadmap

1. ~~Speech wrapper, text cleanup, transcript, vocabulary module, C++17~~ (done)
2. ~~Graph kernel, navigator, key filter in `Game::run`, screen manager over the `State` stack; prove it on the main menu~~ (done)
3. ~~New Battle setup screen, Equip Craft (crew, equipment, armor, soldier info), briefing, next turn~~ (done)
4. Map exploration layer: tile cursor, confirm via `primaryAction`, selection/cursor differ, parity gating (cursor and scanner in)
5. ~~Action menu~~, aiming, ~~enemy list (the scanner)~~, event narration (in: spotted, down, our soldiers hit, shot results); medikit screen in; still to do: alien turn movement
6. Inventory (first pass in; not yet: Ctrl+click quick move, priming in pre-battle equip)
7. Campaign (v2): milestones M0 to M4 in `docs/geoscape-plan.md`. M0 (infrastructure) is in. M1 (start a campaign, Geoscape layer, popups) is in; the silent-event narration is still to do. M2 first half (intercept, destination, dogfights, landing popups) is in, untested; second half (craft screens, debriefing follow-ups, base defense) next. The user's decisions are in its section 4.

Next up, in order (agreed after the first full playtest):

1. ~~Shot results~~ (in; not covered: melee, shotgun pellets).
2. Alien turn: visible aliens moving (losing sight of one is in). Shooting and the direction of incoming fire from unseen shooters are in.
3. End of mission: abort-mission confirmation (in, untested). Debriefing is in. Note the battle ends when you end the turn after the last alien dies (`Options::battleAutoEnd` is off; leave options for later).
4. ~~Medikit screen~~ (in; not yet: saying when a unit is revived); ~~Blaster Launcher waypoints and launch button~~ (in, untested).
5. Small: "1 time units" grammar, Ctrl+Enter quick move in the inventory, unit stats screen.
6. Terrain names are labelled by eye (agents flagged uncertain ones: e.g. DESERT snakes and skulls are real decals, URBAN 81 "petrol pump", XBASE2 33 to 52 "machinery"); fix names as the user reports them.

## Git

- Work on the `accessibility` branch. `master` tracks upstream. Remotes: `origin` is the user's fork (BlindGuyNW/OpenXcom) and `upstream` is OpenXcom/OpenXcom.
- Never commit until the user has tested the change in game and approved it: build, say what to try, then wait. Once approved, commit promptly.
