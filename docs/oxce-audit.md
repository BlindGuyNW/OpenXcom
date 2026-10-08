# OXCE conflict audit (2026-10-05)

An audit of where the accessibility layer assumes vanilla OpenXcom and OXCE disagrees. It came out of the Tab crash: the next-soldier handler dereferenced a null action we passed. Four read-only passes compared the layer with the OXCE source: driving handlers, hotkeys, screen recipes, and game rules. Citations are OXCE file:line at the time of the audit. "Verified" means the finding was re-read in the source after the pass reported it.

Work it top to bottom. Tick items off as they're fixed and played.

Status, re-checked against the code 2026-10-06: every unticked item is still open. Items 1 and 2's fixes, the sea-loss line and wings are in but not yet heard in play (apart from the sort combos).

## Systemic causes

1. [ ] **Clicks bypass the game's visibility gate** (verified). `InteractiveSurface::handle` ignores a surface that is `!_visible || _hidden` (InteractiveSurface.cpp:108). `Controls::click` called `mousePress`/`mouseClick` directly, and OXCE hides buttons to forbid actions. Fix in, untested: `Controls::click`/`clickRow` and the combo and slider adjust refuse hidden surfaces, saying "unavailable". Still to do: explicit recipes should leave hidden widgets out, not list them (dogfight modes and minimize, craft info fixed weapons, the manufacture sell toggle).
2. [ ] **Real modifiers leak into handlers we drive** (verified; sort combos played OK 2026-10-05). OXCE reads `SDL_GetModState()` live (Game.cpp:734-765). Fix in, untested: `Navigator::handleEvent` clears the modifier state while the layer acts (`ModifierMask`) and restores it before the game sees an unclaimed key. Symptoms it fixes:
   - sort combos: Shift reverses the soldier list, Ctrl skips the sort (CraftSoldiersState.cpp:231, 294; SoldiersState.cpp:288, 351; CraftArmorState.cpp:178, 237);
   - Sell/Transfer category combo: Ctrl re-sorts by size (SellState.cpp:1262; TransferItemsState.cpp:1134);
   - Sell: Shift+Right skips a mod's sell warning (SellState.cpp:1144);
   - Intercept: Shift+Enter builds a wing (InterceptState.cpp:444-460);
   - battle: Shift+Enter ignores spotted enemies while walking (BattlescapeGame.cpp:1958, 1996), and Shift+Tab doesn't centre the camera.
   Not covered: OXCE's touch-button flags (`considerTouchButtons`); touch buttons are off on desktop.
3. [ ] **Recipes assume vanilla layouts and flows.** See "Screens" below.
4. [ ] **Backspace = right click, and OXCE gave right click new jobs.** Decide what Backspace does per screen. See "Backspace" below.
5. [ ] **No generic fallback, and unclaimed keys reach OXCE features.** From the code, not yet seen in play: every recipe matches one state type (Screens.cpp `all()`), and with no match `Navigator::sync` returns before speaking (Navigator.cpp:171-172). Keys other than Ctrl+R all go to the game. The typing echo, warnings and dogfight status still speak; `Battle::update`/`Geo::update` stop while the state covers them. See "Keys" and "Uncovered screens".

## Screens

- [ ] **Base defense hangs** (verified). OXCE adds Start Firing and Skip Firing (BaseDefenseState.cpp:99-103). OK is hidden (:97), and the timer only starts in `btnStartClick` (:440-447), apart from missile UFOs (:191-201). The recipe wires only OK. With `showUfoPreviewInBaseDefense`, 3 preview rows come first (:164-176) and are cleared on Start (:443), so the tick's `said` count skips the first 3 shots.
- [ ] **Alien containment columns** (verified). Rows are name, sell cost, count, removing, interrogation flag (ManageAlienContainmentState.cpp:262, 270, 615-616). The recipe reads cells 1, 2, 3 as held, removing and interrogation.
- [ ] **Craft info** (verified). The weapon loop is `i < 2` (Screens.cpp:2582), but OXCE allows `WeaponMax` = 4. Fixed weapons have hidden slot buttons (CraftInfoState.cpp:445-448). Missing: the weapon enable/disable toggle (:606-627), "disabled" in the slot text (:418), Pilots (:102, 191) and the shield.
- [ ] **Inventory name** (verified). The name is in the `TextEdit` `_txtName`; `_txtNameStatic` is hidden on desktop (InventoryState.cpp:198-205, Options.cpp:458). `firstText` returns "TUs 54" or similar, which affects the screen name and the prev/next state text. Missing: the armor, stats, templates, ground scroll and quick search buttons (:123-143).
- [ ] **Stores** columns: quantity, size, space used (StoresState.cpp:144-148). The headers label size as space used. The sort arrows start as `ARROW_NONE`, so they're skipped. Enter opens ItemLocations, which is uncovered.
- [ ] **Soldiers** rows say only the name (`tableScreen(..., {})`). OXCE's action combo `_cbxScreenActions` (SoldiersState.cpp:137-179, 656-690) pushes a screen on every Left/Right step and is unlabelled.
- [ ] **Basescape**: Enter on the access lift starts a base-defense preview battle (BasescapeState.cpp:406-425). Backspace on a mind shield toggles it silently (:505-512), and squares don't say "disabled". Backspace on an empty square opens Base Info (:487-490).
- [ ] **Manufacture start**: produced-item and person rows have 2 columns (ManufactureStartState.cpp:166, 201) and are labelled "units required". "Special materials required" is said whenever there are rows.
- [ ] **Manufacture info**: the sell toggle is hidden when `!canAutoSell()` (ManufactureInfoState.cpp:220) but always listed.
- [ ] **Item articles**: ammo is named from `getCompatibleAmmoForSlot(0)[i]`. OXCE uses the page's slot and compacts unavailable ammo (ArticleStateItem.cpp:195-198, 395-417). The melee row of a melee item is skipped. Weight, accuracy modifier and power bonus are ignored.
- [ ] **Build facilities**: disabled facilities are appended (BuildFacilitiesState.cpp:164-180) and not said; Enter does nothing on them. The `SelectStartFacilityState`/`PlaceStartFacilityState` subclasses match our recipes but differ: OK is Reset, there are no costs or days, `isStartFacility` is true and placement is instant.
- [ ] **Crew** refusals: OXCE tests `hasFullHealth()` (health and mana, Soldier.cpp:1091-1117), not `getWoundRecoveryInt`. OXCE's own `ErrorMessageState` refusals get an extra "craft full" from us. The Preview button starts a craft-deployment preview battle.
- [ ] **SkillMenuState** (skills mods only; not reachable with the current setup) matches the action menu recipe, whose name dereferences a weapon that can be null there (SkillMenuState.cpp:125). That's an access violation, which the try/catch won't catch.
- [ ] **New manufacture list**: the filter combo is labelled with the title (`labelFor`). Unlabelled combos: crew sort, craft armor, craft equipment filter, ufopaedia select filter, new research sort.
- [ ] **Place lift** with several lift types (mods): the lift list comes first (PlaceLiftState.cpp:99-130) and is ignored.
- [ ] **Select destination** leaves out bases and friendly craft (Globe.cpp:785-808).
- [ ] **Soldier info**: the Bonuses button reads blank (SoldierInfoState.cpp:287).
- [ ] **Ufopaedia start** scroll arrows (mods only): press starts a timer and release stops it in the same call, so Enter does nothing.

## Backspace (right click on rows)

All silent unless noted:
- Crafts: moves the craft up (Shift: down) and warps the mouse (CraftsState.cpp:179-214).
- Production complete: sells random production (ProductionCompleteState.cpp:210-252).
- Ufopaedia select, new research, new manufacture: cycle new/hidden status (UfopaediaSelectState.cpp:142-167, NewResearchListState.cpp:183-219, NewManufactureListState.cpp:213-276).
- Purchase, Sell, Transfer: tech tree, dependency tree, item locations, hidden toggle (PurchaseState.cpp:1003-1030, SellState.cpp:1050-1120, TransferItemsState.cpp:821-877).
- Soldiers: opens the inventory (SoldiersState.cpp:754-759).
- Intercept: closes and opens craft info or centres on the craft (InterceptState.cpp:492-521).
- Craft armor: applies the last-selected armor (CraftArmorState.cpp:516-530).

## Battle

- [x] **Reserve check arguments** (verified). Fixed and played 2026-10-06: `eatsIntoReserve` mirrors the preview's call and auto-shot stand-in. `checkReservedTU(before, cost, true)` (Battle.cpp:751) hits OXCE's `(bu, tu, energy, justChecking=false)`, so it can fire real reserve warnings. The preview's yellow test swaps in the autoshot reserve when it's none and passes energy with `justChecking=true` (Pathfinding.cpp:1248-1310). Mirror that.
- [ ] Path cost ignores stamina, which also turns the path red, and turn-before-first-step (Pathfinding.cpp:1229-1247).
- [ ] **Psi is narrated as "missed"** (verified). Psi sets `_power = 0` even on success (ExplosionBState.cpp:130-141), so `hit` returns early (TileEngine.cpp:3228-3231). `_reportShot` isn't reset on the non-area branch of `endImpact`.
- [ ] A bullet into a body on the floor says "missed, hit floor" (TileEngine.cpp:3239-3250). Shotgun pellets 2+ fall outside the bracket (ProjectileFlyBState.cpp:824).
- [ ] Backspace can't stop a walk: `secondary()` needs `canAct`, but OXCE's right click cancels the running action (BattlescapeState.cpp:1072-1077).
- [ ] Out-of-range Blaster waypoint: OXCE warns out of range (BattlescapeGame.cpp:1737-1741), and we then say "waypoints full".
- [ ] Enter on a tile with a visible non-allied unit (not aiming) speaks the stale preview (BattlescapeGame.cpp:1938-1950).
- [ ] Civilians say "out of view" while aiming: only hostiles are in the visible list (TileEngine.cpp:1494-1496).
- [ ] `outOfViewReason` approximates (it still hard-codes `dist > 9` for the dark test, Battle.cpp:556). The dark threshold is per unit (armour, camouflage, anti-camouflage, burning targets), smoke is graded, and psi vision sees all (TileEngine.cpp:1722-1910).
- [ ] Unconscious allies dying aren't narrated (`update` skips units that are out). The HUD's blue and purple indicators aren't surfaced.
- [ ] Fire-confirm mode (off by default) and spray autoshot aren't handled.
- [ ] Accuracy: the crosshair's per-tile accuracy (Map.cpp:1342-1410) shows only with options we don't use.
- [ ] Naming: action names come from the weapon's config in OXCE. Built-in ammo in slot 0 reads the weapon itself. Unconscious bodies are named after the unit in the inventory.
- [ ] Keyboard inventory drop: ground drop uses `getCost`, not `getMoveToCost`, and skips `canBePlacedIntoInventorySection`. Wrong ammo isn't warned. No Shift quick swap.

## Geoscape and dogfights

- [ ] **Keys 1, 2 and 3 during an open dogfight set `dogfightSpeed`** (verified, GeoscapeState.cpp:532-548), undoing our 50 ms for the session.
- [ ] Dogfight modes the game hides (hunter-killer, missile craft; DogfightState.cpp:288-329, 480-514) are still listed. They now say "unavailable" (cause 1) but should be left out. Minimize becomes self-destruct for a defenceless craft (:1377-1398, 1990-2003).
- [ ] Equipment that isn't a weapon (`ammoMax == 0`, not a tractor beam) is listed as a toggle. Tractor beams read "ammo 0" and their range isn't narrated. Distance ignores `getShowDogfightDistanceInKm`.
- [ ] The scanner can reveal an `IGNORE_ME` UFO the globe hides (Globe.cpp:1787).
- [ ] A UFO downed over real water is destroyed right after "UFO CRASH LANDS!" with no further message (DogfightState.cpp:1690-1733); the globe just shows no crash site. In, untested: the window says "UFO-1 lost at sea, no crash site", and "over sea" is added to UFO positions (scanner, destinations, the dogfight's info line) using `Globe::insideLand`, the game's own test.
- [ ] Wings (OXCE Shift+click on Intercept, up to 3 plus the launching craft). In, untested: Shift+Enter goes through `Controls::withModifiers`, rows say "in wing", plain Enter says "launching N craft".

## Keys

- [ ] **F6 is OXCE's InstaSave** (Options.cpp:507), not free. Ctrl+R and Ctrl+L really are free.
- [ ] Ctrl+E (our end turn) swallows OXCE's experience log; Ctrl+Shift+E its overview (BattlescapeState.cpp:2886-2895). The layer's Ctrl branch ignores Shift (Battle.cpp:1178-1193), so Ctrl+Shift+L is also our Ctrl+L.
- [ ] Left Shift swallowing in battle is vestigial: `keyBattlePrevUnit` defaults to none in OXCE (Options.cpp:325).
- [ ] Inventory: digits load equipment layouts, Ctrl+digits save them (InventoryState.cpp:2124-2133).
- [ ] X and Z on Sell, Transfer and crew lists sell, transfer or remove everything (Options.cpp:531-555).
- [ ] End opens the music picker (Geoscape and battle). Ctrl+Home and Ctrl+End in battle change the palette and vision mode.
- [ ] Battle Space also holds night vision (Map.cpp:2449 reads raw key state).
- [ ] Unreachable ctrl-click actions: Ignore UFO (UfoDetectedState.cpp:254-260), Patrol on landing (ConfirmLandingState.cpp:315-322), spray autoshot. Ctrl+Enter is the natural key. Force fire is in (Ctrl+Enter while aiming a gun, 2026-10-07; played).
- [ ] Escape in a focused TextEdit clears the field, then submits (TextEdit.cpp:544-557).

## Free keys (from the hotkey pass)

- Battle, plain: B, C, D, F, G, H, N, O, P, T, V, X, Y, Z, Insert, F8, `-`, `=`, `[`, `]`, `;`, `'`, backquote, numpad. With Ctrl: A, I, O, P, Q, Y, Z, digits, Page Up/Down, Tab (G and N are global; D, K, J, V, W, T, U are debug only).
- Geoscape, plain: A, H, K, L, M, N, O, S, V, W, X, Y, Z, 7, 8, 9, 0, Home, Page Up/Down, Slash, Backspace, Delete, Insert, F1 to F4, F8, F10. With Ctrl: everything except G and N (Ctrl+digits, A, C, D are debug only).
- Graph screens: OXCE's `onKeyboardPress` bindings fire only with no modifier held (InteractiveSurface.cpp:371-372), so Ctrl+Up/Down/Home/End/Enter/Backspace/Space/Tab are free. Page Up/Down scroll a TextList. Letters vary per screen.

## Uncovered screens (nothing read on arrival; see cause 5)

Re-audited 2026-10-06 against every `State` subclass, for the setup in use: only the `xcom1` ruleset, `oxceLinks` off, `customInitialBase` off. Several vanilla screens were missing from the first list.

Recipes added 2026-10-06, untested: Infobox (battle timed messages: panicked, berserk, under alien control, mind control and morale attack successful, killed; every key goes to the game, which closes it), InfoboxOK (died from a fatal wound, unconscious, mission complete text), ConfirmEndMission (fatal wounds left when the battle ends), NewPossiblePurchase, NewPossibleCraft, NewPossibleFacility, ModList and ModConfirmExtended (the mods menu; mod-specific screens stay parked).

Next, in a plain campaign:
- [x] MonthlyReport (played 2026-10-06, through to game over): title on arrival, one item per figure and per paragraph of the council text (`getLines`), OK its own stop; the tick says the failure message. Then the lose slideshow (each caption as it shows; keys go to the game) and Statistics (a list; also the Memorial's Statistics button). PsiTraining follows the report with a Psi Lab and is still uncovered.
- [x] Funding (F; played 2026-10-06): sort buttons (`getSortButtons`) as a stop, then countries with named columns and the total, then OK.
- [ ] Graphs (G, GeoscapeState.cpp:3125): a chart, needs a design.
- [x] UnitInfo (S in battle; played 2026-10-06): name on arrival, one item per stat ("16 of 65" from the bar's maximum, "stun N" from its second value; stun not yet seen), previous/next soldier say the new name. Mind probe not yet seen.
- [ ] ItemLocations (Enter on Stores, Purchase, Sell, Transfer).

Later in a campaign: AllocatePsiTraining and TrainingFinished (Psi Lab), SoldierDiary overview, performance and mission (soldier info's Diary button), the tech tree viewers, the Global research, manufacture and containment screens, UfoTracker, Notes, StatsForNerds, TurnDiary and the hit log (Ctrl+H, an Infobox, now read), ExperienceOverview, AlienInventory, SelectMusicTrack, the inventory and craft-equipment template dialogs, BriefingLight.

Not reachable with this setup: GeoscapeEvent, pilots, transformations, SkillMenu, SoldierBonus (mods); the Extended links menus (`oxceLinks`); SelectStartFacility and PlaceStartFacility (`customInitialBase`); MiniMap (the cursor replaces it); TFTD, unit and soldier articles (not in `xcom1`, probably).

## Checked and fine

- Next/prev soldier, end turn, `launchAction`, `primaryAction`/`secondaryAction`, the inventory OK/prev/next/unload handlers, NextTurn close, the action menu, prime grenade back, medikit.
- Arrow-row `_sel` mapping, the by-value methods, `changeEngineers`/`changeUnits`, the base grid, `selectBase`, the city picker's globe click, `MultipleTargetsState`.
- Dogfight weapon toggles and `range * 8`, load/save rows, text fields, text buttons and groups, toggles, arrow buttons.
- Discovery mapping, `PathfindingStep` costs, big walls, the two-click move, Blaster waypoints, `battleAutoEnd` default, darkness test, the knocked-out test, the impact hook placement, warnings, projectile origin.
