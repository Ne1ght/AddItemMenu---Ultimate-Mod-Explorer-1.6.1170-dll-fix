# AddItemMenu---Ultimate-Mod-Explorer-1.6.1170-dll-fix
Planning to fix the dll to work with version 1.6.1170 by writing it from scratch based on the behaviour and clues left behind in the papyrus files.

Do to never having done this, escpecially c++ i will have to use AI for understnading what i need to do, best pratice for the code and solving problems.

Below are notes for me, summarized by AI for a better overview. I will probably edit it down or specify it further once I start actually working on it

AIM (AddItemMenu) DLL REBUILD - KEY NOTES
==========================================
Items marked [INFERRED] are guesses from script/ini/setting names and
should be verified (xEdit, UIExtensions source, testing).
 

 
 MINIMUM DLL = 2 REAL FUNCTIONS + STUBS
-----------------------------------------
Real work:
  AIM_GetEntries(mode, str)
    Used by: ListScript
    Returns the menu entries (mods) sorted by order mode, filtered by str.
    If the result is empty, the script retries with "".
 
  AIM_GetItems(index, lists, str)
    Used by: ContainerScript
    Fills each FormList in "lists" with items from the selected entry,
    optionally filtered by str. Returns true if anything was found.
    This is the "all items from mod X" function Papyrus can't do.
 
Safe stubs / small functions:
  AIM_GetQuantity(itemType)     -> return 1 (original: default count per
                                   list index, e.g. more for ammo/gold)
  AIM_GetOrderListSize()        -> return 1 (original ini defines 4)
  Four key code functions       -> return 0 (0 disables the feature;
                                   scripts check != 0)
  AIM_GetTranslation            -> return "" (both callers have a fallback)
  AIM_Replace / AIM_ReplaceArray-> return str (only reached if a
                                   translation exists; can also be
                                   rewritten in Papyrus)
  AIM_Get/SetClipboardText      -> return "" / false (only behind the
                                   clipboard key)
  AIM_CloseContainerMenu        -> small C++ UI call, or the Papyrus
                                   AIM_CloseAllMenu as a workaround
                                   (untested)
 
Clipboard, translations and hotkeys can all be switched off by returning
0 or empty. Nothing core is lost.
 
 
 HOW THE SCRIPTS WORK
-----------------------
- Trigger items: the books and the misc item.
    AddItemMenuSEObject      runs on equip
    AddItemMenuSEMagicEffect runs on cast
  Same flow in both. The "searchOption" property decides normal vs
  search version.
- TextEntryScript is a wrapper around UIExtensions' text entry box.
- Search text lives in a Papyrus string (searchText) and in the name of
  searchObject (passed to the list menu via aForm.GetName()). It is NOT
  stored in the clipboard.
- Clipboard is only a convenience:
    text box: hotkey pastes clipboard into the field
    chest:    hotkey copies the selected item's name to the clipboard
- The container script only calls chest.AddItem(list, quantity) on each
  list. The DLL never touches the chest, only the FormLists.
  The player then takes items out like looting a container.
 
 
 THE SPECIAL INDEX 999
------------------------
- In search mode the script calls AIM_GetEntries(0, searchText).
- If exactly one entry comes back, the script skips the list and calls
  GetItems(999, lists, searchText).
- 999 means "search all items in all mods by name".
- If the search text also matches mod names, the list shows those mods
  plus the 999 entry. The script keeps the search active only if the
  picked index is 999.
- So the DLL must return the 999 entry from GetEntries whenever a search
  string is given, with that exact index, and handle GetItems(999, ...)
  as a global name search.
- In the entry format this is simply:  Search...;;-1;;999;;0;;0
 
 
 ENTRY STRING FORMAT (UIExtensions list menu, LM_AddTreeEntries)
------------------------------------------------------------------
Each array element is one entry, five fields separated by ";;":
 
    text;;parent;;id;;callback;;hasChildren
 
  text         what the menu displays. Empty text is skipped (blank
               strings in the array are harmless).
  parent       -1 for top-level entries, otherwise the id of the entry
               it sits under (this is how the tree works).
  id           a number. When a leaf is picked it comes back as
               _resultInt, which is the index passed to GetItems.
  callback     number sent back as numArg.
               MUST NOT be -1. Back sends -1 and the list script treats
               _resultFloat == -1.0 as "cancelled". Use 0.
  hasChildren  0 or 1.
 
Other format facts:
- A child view opens when any other entry has parent == id of the one
  picked.
- First version: flat list, every entry with parent = -1.
- AIM never calls LM_SetSortingEnabled, so the DLL's own sort order is
  the only ordering. Sorting is done in C++.
- Delimiter risk: a plugin name containing ";;" would break parsing
  (very unlikely).
 
 
 ID ENCODING (SUGGESTED DESIGN, NOT THE ORIGINAL'S)
-----------------------------------------------------
- The id is yours to choose. If it is stable, GetItems can decode it
  without remembering the last GetEntries list (the list order changes
  with sort mode, so a stored list would otherwise be needed).
- Suggestion:
    regular plugins -> load-order index
    ESL plugins     -> 1000 + light index (so they can't collide
                       with 999)
- Per-item-type sub-entries (for big masters) would need ids encoding
  both plugin and type. Leave out of version one.
 
 
 ITEM CATEGORIES (FROM THE .INI)
----------------------------------
[QuantitySettings] lists 14 types:
  Weapon, Armor, Book, Key, Misc, Light, SoulGem, Ammo, Scroll,
  SpellTome, Ingredient, Potion, Note, Ingot
 
- [INFERRED] These match the 14 FormLists in the quest's "lists"
  property, in that order. AIM_GetQuantity(index) looks up a count by
  list index. VERIFY THE ORDER IN xEDIT.
- Your DLL must sort items into these categories.
  Easy: weapon, armor, ammo, potion.
  Harder: Note, Ingot, SpellTome (need book flags or keywords; the
  original's rules are unknown).
 
 
 FORMLIST HANDLING WARNINGS
-----------------------------
- Clear the lists on every GetItems call. FormLists changed at runtime
  can end up in the save, and stale entries would carry over to the next
  search.
- Check how CommonLibSSE-NG handles BGSListForm additions before relying
  on any approach.
- Huge results (e.g. "all items" on a big mod list) make the chest UI
  sluggish. Limit not tested. Probably why the original had sort and
  search modes.
- ContainerSize=512 in the ini suggests the original capped items per
  chest. You'd probably want a similar cap.
- The 128 limit applies to Papyrus arrays; it matters little for the
  14-entry "lists" property.
 
 
 OTHER INI SETTINGS
---------------------
- [OrderSettings]: four sorts (load order asc/desc, name asc/desc), so
  the original's AIM_GetOrderListSize returns 4.
- SearchItemOnTop: where the 999 "search everything" entry is placed.
- ExclusionPlugins: hides plugins from the list.
- ItemTypeMenuPlugins: vanilla masters. [INFERRED] these expand into
  per-item-type sub-entries (tree menu). Without that, a flat entry for
  Skyrim.esm is a huge dump.
- CheckResourceFiles / IgnoreCheckResourcePlugins: [INFERRED] the
  original hides items whose model/resource files don't exist. Real
  extra work (loose files + BSAs). Skip in version one.
- CreateItemDataBeforeMainMenu=1: [INFERRED] the original builds its
  item index before the menu opens, probably for speed.
- Key codes, quantity and order list size are read from the .ini
  (matches GetPrivateProfileSectionA in the imports).
- AIM_GetTranslation reads the translation txt files.
 
 
 GIFT SCRIPT / INITALIAS
---------------------------
- InitAlias gives the player one Gift item the first time the quest
  starts. Works when added to an existing save, no new game needed.
- Equipping the Gift item calls GiftActor.ShowGiftMenu(false, listForm)
  [INFERRED: a take-items menu from a hidden actor holding the four
  items, filtered by listForm. Confirm in xEdit.]
- When the actor is empty, the script removes the Gift item and
  disables the actor.
- Needs no DLL. It only calls AIM_CloseAllMenu (plain Papyrus), so it
  should keep working as is.
- Hotkey/MCM idea: a quest script calling the same flow as
  AddItemMenuSEObject, compiled in the Creation Kit. Do it after the
  DLL works.
 
 
 OPEN RISKS / FIRST TESTS
----------------------------
1. Array size (test first): the stock script allocates 128-slot arrays,
   but the AIM DLL builds its own. Unknown whether a native can return
   more than 128 strings, or whether UI.InvokeStringA handles that many.
   A typical load order can have 200+ plugins.
   Fallbacks: return entries in pages, or filter out plugins with no
   items (the original may have done this, unverified).
2. Category rules for Note, Ingot, SpellTome are unknown.
3. FormList persistence in saves (see section 8).
4. AIM_CloseContainerMenu via Papyrus AIM_CloseAllMenu is untested.
5. Verify in xEdit: order/count of the "lists" property, and the Gift
   actor setup.
 
 
 SUGGESTED BUILD ORDER
-------------------------
1. DLL with stubs for everything except GetEntries and GetItems.
2. Flat entry list (one entry per plugin, parent = -1, callback = 0,
   hasChildren = 0) plus the 999 search entry.
3. GetItems: fill the 14 FormLists by category, clear them each call,
   respect a 512 item cap.
4. Test with a large load order for the 128-entry array limit.
5. Later: sorting modes, tree sub-menus, exclusions, resource checks,
   hotkeys/MCM, translations, clipboard.

