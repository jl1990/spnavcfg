# Profiles

The new **Profiles** tab appears with a matching daemon/library build. Create a
profile, choose an application, and edit its Axes or Buttons. **Use Default** means
that the setting follows Default, including future Default changes. Uncheck it to
override a setting; recheck it to reset that setting.

**Identify a button** selects the physical button's row without running its
binding for ten seconds. The shortcut recorder supports modifier combinations
and distinct numpad keys. Use **…** for key names or modifier-only bindings.
Screen labels customize the Enterprise's first twelve button captions.

Edits remain a draft until **Apply** or **Save**. Apply affects this session; Save
also persists after restart. **Revert draft** reloads the running configuration.
Switching applications never switches the profile you are editing. The active
profile indicator updates independently. Closing with pending edits offers Save,
Discard or Cancel.

**Detect application** captures the focused application after five seconds; keep
its window focused until the capture finishes. GNOME Wayland uses the installed
focus extension/helper. The application match field supports `=id` for exact
matching and plain text for substring matching.

Build libspnav first, then run `sh tests/run.sh` here. The tests exercise screen
controls and profile drafts offscreen. Set PROFILE_PREVIEW to a PNG path to save
the profile editor test window. The daemon's `doc/profile-editor.md` documents
persistence, limits, keyboard-layout considerations and protocol validation.
