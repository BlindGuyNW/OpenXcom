# Campaign accessibility plan (Geoscape, interception, bases)

Written 2026-10-04 from a three-part survey of `src/Geoscape`, `src/Basescape`, `src/Menu`, `src/Ufopaedia` and the campaign paths in `src/Battlescape`. Line numbers are from that tree and will drift; trust function names over numbers.

Read `CLAUDE.md` first (layer architecture, axioms, build). This doc says what to build for the campaign, in what order, and what the survey found that isn't obvious from the code.

## 1. Facts that shape the design

### Time and popups
- **Only the top state thinks** (`Game::run`). Any state pushed over `GeoscapeState` freezes game time. There's no separate pause flag to manage: every popup pauses the game for free.
- `GeoscapeState::popup(State*)` queues popups in `_popups`. `think()` pushes **one per frame**, and only when no dogfight or zoom effect is running. So the navigator sees popups one at a time, in event order (research complete, then research required, then new possible research, and so on).
- Some states bypass the queue: lose-game cutscene, `BriefingState` from `handleBaseDefense`, `MultipleTargetsState` from a globe click, quick save/load.
- Speed: `_timeSpeed` (private `TextButton*`) is one of `_btn5Secs`..`_btn1Day`. Keys 1 to 6 already work through `btnTimerClick`. `timerReset()` drops back to 5 seconds on many events (research complete, landing, monthly report). The speed buttons have no click handler; the group is set in `TextButton::mousePress`, which `Controls::click` triggers.
- Each `timeAdvance` tick runs N five-second steps; `GameTime::advance` returns the trigger and the handlers fall through month → day → hour → 30 min → 10 min → 5 s.

### What pops up when (GeoscapeState.cpp)
| Event | Popup |
|---|---|
| 5 s: UFO reaches waypoint | `UfoLostState` (if followed), `MissionDetectedState`, `BaseDefenseState` / `handleBaseDefense` |
| 5 s: craft arrives | flying UFO → dogfight; landed/crashed UFO, site or alien base with troops → `ConfirmLandingState`; waypoint → `CraftPatrolState`; lost target → `GeoscapeCraftState` |
| 10 min | `LowFuelState` |
| 30 min | `CraftErrorState` (refuel), `UfoDetectedState`, `UfoLostState` |
| 1 h | `CraftErrorState` (rearm), `ItemsArrivingState`, `ProductionCompleteState`, `ErrorMessageState` + `SellState` (storage full), `MissionDetectedState` |
| 1 day | `ProductionCompleteState` (construction), `ResearchCompleteState`, `ResearchRequiredState`, `NewPossibleResearchState` (**always pushed, even empty with an empty title**), `NewPossibleManufactureState`; autosave on days 10 and 20 |
| month | `MonthlyReportState`, `AlienBaseState`; after its OK: `CommendationState`, `PsiTrainingState`, save |

**Silent events** (no popup; candidates for queued narration): craft returning or arriving home, UFO landing or taking off, UFO crash site expiring, alien base found by a craft's sight (`time10Minutes`), a craft's target lost while not followed.

### The globe
- Coordinates are radians. Longitude 0..2π east of Greenwich. **Latitude is negative for north** (London is −51.5°). For speech: north = −lat; lon > 180° means west.
- **The click trick:** `Controls::click` clicks a surface's centre, and the globe's centre pixel maps back to exactly `_cenLon/_cenLat` (`cartToPolar`'s `rho == 0` branch). So `globe->center(lon, lat); Controls::click(topState, globe)` reproduces a click at that point with no new code. It must be called with the top state, because the globe's click handler is swapped between `GeoscapeState`, `BuildNewBaseState` and `SelectDestinationState`.
- **Opening an object** like a click: `pushState(new MultipleTargetsState({target}, 0, geoscapeState))`. It routes base → `InterceptState`, craft → `GeoscapeCraftState`, UFO → `UfoDetectedState`, else `TargetInfoState`.
- **Sending a craft:** from `SelectDestinationState`, push `MultipleTargetsState(v, craft, 0)` with the chosen targets, or a fresh `Waypoint` (id 0) for an arbitrary point. Don't call `Craft::setDestination` directly: it skips waypoint registration and status, and unregistered waypoints are deleted. `ConfirmDestinationState`'s OK pops twice, so only use it from `SelectDestinationState`.
- **Parity, what's on the globe:**

  | Object | Shown when |
  |---|---|
  | Base | placed (not at 0, 0) |
  | Craft | status `STR_OUT` only |
  | UFO | `getDetected()` (name changes to landing/crash site with status) |
  | Mission site | `getDetected()`. **`Globe::getTargets` doesn't check this**; gate on it yourself |
  | Alien base | `isDiscovered()` |
  | Waypoint | always |

  Cities, country names and radar ranges are static geography; listing them is fair.
- **Cities for base placement:** `SavedGame::getRegions()` → `Region::getRules()->getCities()`; vanilla has 51 (the terror-site list). Validate with `Globe::insideLand` (coastal cities may fail; unverified). Cost and area: `SavedGame::locateRegion(lon, lat)` → `RuleRegion::getBaseCost()`. Country: `RuleCountry::insideCountry(lon, lat)`.

### Dogfights aren't states
- `GeoscapeState` owns `_dogfights` (private, up to 4), draws them, forwards events to them and ticks them from `_dogfightTimer`. **During a dogfight the top state is still `GeoscapeState`**, and Geoscape's own widgets only get events when every dogfight is minimized. Time runs only when all are minimized.
- No dogfight button has text: the five modes are `ImageButton`s, the rest bare `InteractiveSurface`s. `addWidgets` sees nothing.
- `Controls::click` must get the **`DogfightState*`** as its state, since the handlers are its members.
- Status messages go through `DogfightState::setStatus` and vanish after 50 animation ticks. Diffing the text misses repeats ("UFO hit" twice), and "outrunning" is re-set every tick. **Hook `setStatus`** and dedupe.
- Range bars and the craft damage silhouette are graphics only. UFO damage isn't shown to the player at all (only a hit flash and the crash shrink): **don't speak it**.
- Ticks are 20 to 50 ms (`Options::dogfightSpeed`). It moves fast for speech.
- Dogfights are deleted silently in `handleDogfights`, and their craft or UFO can be deleted too: **narration must store strings, not pointers.**

### Generic layer gaps the campaign hits everywhere
- **`ErrorMessageState` has no recipe** and is pushed all over (placement, purchase limits, transfers, storage full, no containment after a battle, craft equipment). Essential; `simpleScreen` is enough.
- **`AccessScreen` has no per-frame tick.** Base defense rows, dogfight narration and arrival popups that update in place need one (or a module ticked from `Navigator::update`, like `Battle::update`).
- **Button group state isn't spoken.** `TextButton::_group` is private with no getter. Time speed, difficulty, diary tabs need "selected". `ToggleTextButton::getPressed()` exists but isn't read either.
- **`addWidgets` skips `ArrowButton` and `TextEdit`.** Research/manufacture +/- are invisible; base/craft/soldier rename is unreachable. Clicking a `TextEdit` focuses it and the navigator already stands down while it's focused.
- **The navigator always claims Left/Right**, which blocks the Ufopaedia's prev/next (`keyGeoLeft/Right`) and globe rotation.
- **Escape semantics vary, sometimes dangerously.** Recipes without `back` pass Escape to the game's `keyCancel`:
  - `ConfirmLandingState`: Escape = No = **the craft returns to base**.
  - `DogfightErrorState`: Escape = continue, Enter = return to base (inverted).
  - `ManufactureInfoState` (new production): Escape = OK = **starts** production.
  - `ResearchInfoState` (new project): Escape cancels the project.
  - `SelectStartFacilityState`: Escape does nothing. `PlaceLiftState`: no exit at all.
  - Popups where Enter is the "do something" button and Escape is plain OK: LowFuel, CraftError, ResearchComplete (Enter = view reports), NewPossibleResearch/Manufacture, ItemsArriving, CraftPatrol (Enter = redirect).
- **`>` separators** ("TURN>", "STATUS>", "SOLDIERS>", "COST>", "INCOME>") are everywhere in the campaign. See open question 1.
- **Heuristic mislabels found:** Purchase and Sell category combos get labelled with the funds/sales text above them; the New Manufacture combo gets the title; ManufactureInfo's two "Increase"/"Decrease" pairs get identical labels; Ufopaedia item articles interleave the right-hand damage column with the title; Intercept's two-line header sorts before the others; CraftInfo interleaves weapon names and ammo.

### Arrow-row lists (Purchase, Sell, Transfer, Containment)
- Same mechanism as craft equipment: `Controls::clickRow` sets `_sel` through the list's press handler, then call the state's public by-value method. Shift = 5 and Ctrl = `INT_MAX` already match the right-click meaning in every state.

  | State | Increase | Decrease | Columns |
  |---|---|---|---|
  | Purchase | `increaseByValue` | `decreaseByValue` | name, price, in base, buying |
  | Sell | `changeByValue(n, 1)` | `changeByValue(n, -1)` | name, left in base, selling, value |
  | TransferItems | `increaseByValue` | `decreaseByValue` | name, here, sending, at destination |
  | ManageAlienContainment | `increaseByValue` | `decreaseByValue` | alien, held, to remove, interrogating |
  | CraftEquipment (existing) | `moveRightByValue` | `moveLeftByValue` | name, stores, craft |

- **Map keys semantically**: Right = increase. The on-screen arrows disagree between states (vertical lists put increase on the left/up arrow).
- Use the by-value methods, not synthetic arrow clicks: the row arrows aren't in `getSurfaces()`, and Shift+5 clicks could stack five error popups.
- Generic header-to-column mapping won't work (Purchase has no header over "in base"; Transfer headers are two lines). Write a row formatter per state.

### The base grid
- `BaseView`: 6×6, `_facilities[6][6]` (private; a size-2 facility fills four squares with the same pointer, origin top-left). Selection is mouse-only (`mouseOver` sets `_gridX/_gridY/_selFacility` and the selector box). Add `BaseView::selectSquare(x, y)` mirroring `mouseOver`, plus `getFacilityAt(x, y)`.
- Placement: `isPlaceable(rule)` returns a bool only. Rules: footprint inside the grid and empty; at least one orthogonal neighbour along the perimeter holds a facility, finished unless `Options::allowBuildingQueue`. The layer computes its own reason (off grid, occupied by X, not connected). Dismantle connectivity: `Base::getDisconnectedFacilities`.
- Build days and hangar craft are painted into the view, not text. `BaseFacility::getCraft()` is only assigned during `BaseView::draw()`.
- Basescape left click = dismantle flow; right click = jump to the facility's screen (lab → Research, hangar → CraftInfo, stores → Sell, and so on).
- **The first base comes prebuilt** unless `Options::customInitialBase` (default off), so the grid isn't needed to start a campaign.

## 2. Architecture decisions

1. **Geoscape is a layer, not a graph** (like `Battle`): `src/Access/Geo.{h,cpp}`, handed keys and ticks when `GeoscapeState` is on top, no recipe matches and no dogfight window is open. Its HUD and clock would be mangled by the heuristics (ten clock fragments, two blank side buttons).
2. **The globe is a list, not a cursor.** A scanner over what's on the globe, same keys as the battle map: Period/Comma step through the current category nearest first (nearest to the selected base, see decision 5), Shift+Period/Comma change category (UFOs, alien sites, alien bases, our craft in flight, bases, waypoints), Enter opens it through `MultipleTargetsState`, Space speaks date, time, funds and speed. Keys 1 to 6 and the game's letter keys (I intercept, B bases, G graphs, U ufopaedia, F funding) already work and pass through. The layer narrates speed changes (a differ on `_timeSpeed`) and the silent events listed above, queued.
3. **Picking a place** (new base, craft destination) is a graph screen recipe on `BuildNewBaseState` / `SelectDestinationState`: cities grouped by region, each with country, cost (bases) or distance and in-range (craft: `Craft::getBaseRange`), plus current targets for destinations. Activation: centre the globe on it and `Controls::click` the globe, which runs the game's own handler and its checks. An arbitrary-point cursor can wait.
4. **Dogfights:** a `Dogfight` module plus a recipe that matches `GeoscapeState` while a dogfight window is open. Needs `GeoscapeState::getDogfights()` and a `friend` on `DogfightState` (or accessors) for `_mode`, the mode buttons, `_btnUfo`, `_btnMinimize`, `_btnMinimizedIcon`, `_weapon1/2`, `_weapon1Enabled/2Enabled`, `_currentDist`, `_targetDist`, `_ufoBreakingOff`. One context per dogfight: the five modes (selected one marked), each weapon (name, ammo, in range, on/off), distance, craft damage %, minimize. Narration (queued, prefixed with the craft when there's more than one): `setStatus` messages, weapon out of ammo, entering/leaving weapon range, dogfight over with why.
5. **Infrastructure first** (milestone 0) so every later screen benefits: `ErrorMessageState` recipe, `AccessScreen::tick`, group/toggle state in button announcements, `ArrowButton` and `TextEdit` ("rename") nodes in `addWidgets`, Escape decisions for the dangerous popups.
6. **Arrow-row lists** become one table-driven recipe generalising `craftEquipment`: per state `isActive`, increase, decrease, row formatter, and a totals line spoken after each change (funds and cost, sales, space).
7. **The base grid** is a recipe with a raw 6×6 zone (arrows move, announce facility, construction days, hangar craft, which part of a large facility) sharing code across Basescape, PlaceFacility, PlaceStartFacility and PlaceLift. Mirror the cursor with `selectSquare` for sighted viewers.
8. Prefer explicit wiring (accessors / `friend`) over heuristics for anything that mislabels or matters: CraftInfo, Intercept, ManufactureInfo, ResearchInfo, Ufopaedia articles, MonthlyCosts.

## 3. Milestones

### M0: infrastructure (done 2026-10-04, played 2026-10-04)
- `ErrorMessageState` recipe.
- `AccessScreen::tick` (called by the navigator each frame while the recipe is active).
- `TextButton` group accessor (`getGroup()`); speak "selected" for grouped buttons and pressed state for `ToggleTextButton`.
- `addWidgets`: `ArrowButton` nodes (label from the text to its left) and a "rename" node for `TextEdit`s.
- `>` to colon in `Speech::normalize` (decision 1).
- Force the slowest `Options::dogfightSpeed` (decision 2).

### M1: start a campaign and let time run (done 2026-10-04, played 2026-10-04)
Not done yet: narrating the silent events (decision 2's queued narration: craft home, UFO landing or taking off, and so on). The ConfirmNewBase, BaseName and popup recipes are plain `simpleScreen`/`popupScreen`; check their wording in play.
- `NewGameState`: difficulty group (selected state), ironman toggle, OK.
- `BuildNewBaseState`: city picker recipe (first base: no cost, no cancel). Then `BaseNameState` (TextEdit focused from the start; speak the title; Enter commits). `ConfirmNewBaseState` is `simpleScreen`.
- `Geo` layer: time/date/funds readout, speed differ, scanner, open target.
- Popups as `simpleScreen` with Escape reviewed: UfoDetected, UfoLost, MissionDetected, AlienBase, CraftPatrol, LowFuel, CraftError, ResearchComplete, ResearchRequired, NewPossibleResearch (say "no new topics" when empty), NewPossibleManufacture, ProductionComplete, ItemsArriving, MultipleTargets, TargetInfo, GeoscapeCraft, ConfirmDestination.
- The Ufopaedia start and select lists come free as `simpleScreen`; articles wait for M4.

### M2: intercept, fight, land, come home
First half in (2026-10-04, played and working 2026-10-04; AliensCrash, BaseDestroyed, ConfirmCydonia and DogfightError not yet seen in play): Intercept, SelectDestination, the dogfight module and recipe, ConfirmLanding, AliensCrash, BaseDestroyed, ConfirmCydonia, DogfightError. Still to do: CraftWeapons, CraftSoldiers refusals, Sell/containment after a battle (arrow-row batch), BaseDefense. Debriefing follow-ups (promotions, medals, lost in service, cannot reequip) in and played 2026-10-04. Done since: a new interception starts focus at the top of its window; the Basescape menu (buttons, base switcher; no grid), the craft list and CraftInfo (pulled forward from M3 so craft screens can be reached). Then M3 batch 1 (played 2026-10-04): Soldiers, BaseInfo, Stores, Transfers, MonthlyCosts, Research, NewResearchList, ResearchInfo. Arrow-row batch in 2026-10-05, not yet played: Purchase, Sell, TransferBase, TransferItems, TransferConfirm, ManageAlienContainment, with craft equipment moved onto the shared recipe. PlaceLift (found in play: building a second base left the keyboard dead) got the first cut of the grid: `addBaseGrid`, `BaseView::selectSquare`/`getFacilityAt`. Next batches: Manufacture, Build Facilities and the grid. The user asked for weapon stats; those are only in Ufopaedia articles, so the M4 article recipe was pulled forward and played 2026-10-04.
- `InterceptState`: rows as "name, status, base, N weapons, N soldiers, N HWPs" (accessor for its craft list or parse), refusal feedback when a craft can't go.
- `SelectDestinationState` recipe (targets + cities, range).
- Dogfight module and recipe.
- `ConfirmLandingState` (guard Escape), `AliensCrashState`, `BaseDestroyedState`, `ConfirmCydoniaState`, `DogfightErrorState`: `simpleScreen`.
- `CraftInfoState` explicit wiring: "Weapon 1: Avalanche, ammo 3 of 3" or "Weapon 1: none", crew/HWP/item counts (graphics only today), damage and fuel, rename. `CraftWeaponsState`: name says which slot and what's mounted; rows "Cannon, 2 in stores, 50 rounds available".
- `CraftSoldiersState`: speak refusals (wounded, craft full, craft out).
- Debriefing follow-ups, shown in this order: storage/containment error, `SellState` or containment, `CannotReequipState`, `PromotionsState`, `CommendationState`, `CommendationLateState`. All `simpleScreen` except containment (arrow-row recipe). The existing debriefing recipe works as is.
- `BaseDefenseState`: narrate rows as they appear (tick), announce OK when it shows.

### M3: running the bases
- Base grid layer (Basescape, BuildFacilities → PlaceFacility, DismantleFacility). Basescape buttons, base switcher as a list of bases (keys 1 to 8 already work), rename.
- Research: `ResearchState` row formatter; `ResearchInfoState` scientists as one adjustable node via public `moreByValue/lessByValue`, with available and lab space.
- Manufacture: `ManufactureState` row formatter; `NewManufactureListState` combo label; `ManufactureStartState` says why Start is missing; `ManufactureInfoState` engineers and units as adjustable nodes (`friend`, or click its `ArrowButton`s left/right to get the infinity semantics), sell toggle, profit.
- Arrow-row recipe: Purchase, Sell, TransferItems, ManageAlienContainment (move craftEquipment onto it).
- `simpleScreen` (some with row formatters): BaseInfo ("5:7" → "5 of 7"), MonthlyCosts (explicit sections), Stores, Transfers, TransferBase, TransferConfirm, Soldiers, SackSoldier, Crafts, Memorial (join the date cells).

### M4: the rest
- Monthly report, funding (name the columns), graphs as data read from the save (the UI is drawn lines and unlabelled icons), psi training and allocation.
- Ufopaedia article recipe: title, text, stat rows as "label: value" (skip empty armor rows, split craft stats on newlines, item articles: shot table rows from headers, then per ammo "name: damage type, power"; ammo names come from `getCompatibleAmmo()` since they're only sprites). Fix prev/next paging (Left/Right conflict).
- Soldier diary (tabs, commendation descriptions via the public `lstInfoMouseOver`).
- Custom initial base (`PlaceLiftState`, `SelectStartFacilityState`), only if wanted.
- Cutscenes and the end-game slideshow: probably just speak the text.

## 4. Decisions (answered by the user 2026-10-04)

1. **`>` becomes a colon** in `Speech::normalize`: `>` followed by a space becomes `:`, otherwise `: `. Do it in M0. It applies everywhere, battle text included ("TURN: 1").
2. **Dogfights run at the slowest speed**: force `Options::dogfightSpeed` to its slowest value at layer init, the way `Battle` forces `battleNewPreviewPath`. No hold key for now. The player's choices in a fight are few (attack mode, weapon on/off, minimize, view the UFO); firing and hits are automatic, so narration plus the slow speed should be enough. Revisit after a playtest.
3. **Escape keeps the game's behaviour** on every popup for now, including `ConfirmLandingState` (Escape = the craft goes home). Don't add `back` overrides; do make sure each popup's arrival text makes the buttons' meaning clear.
4. **No typing echo yet.** Reversed after the first try: the user couldn't tell what was in the field, so typing is now echoed.
5. **The globe scanner sorts nearest to the selected base** (the first base until there's a way to choose). Base placement isn't the scanner: it's the city picker, grouped by region and country.

## 5. Not covered by the survey
- The Geoscape's Options button leads to the options screens, which aren't spoken (same as the battle).
- Save/load already works from the escape menu recipes; the Geoscape's Options button opens the same `PauseState`.
- TFTD-only paths (water-only craft) can be ignored.
