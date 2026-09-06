Screen controls require the matching jl1990/libspnav and jl1990/spacenavd forks.
The daemon must be built with `./configure --enable-spacelcd` on Linux.

Build libspnav first, then configure spnavcfg with Qt 6. With both repositories
as sibling directories, `sh tests/run.sh` builds spnavcfg using the sibling
library statically, and tests the UI using Qt's offscreen platform. It does not
modify installed libraries or contact the system daemon. Set SCREEN_PREVIEW to
an absolute PNG path to capture the Screen tab with simulated device information.

Tests cover on/off and title flags, brightness, idle timeout, failed-setting
rollback, unavailable devices/daemon support, refresh, and save result feedback.

The timeout is in seconds; zero is displayed as Never. Changes apply immediately
and Save config persists them. SpaceMouse movement or a button press wakes idle
sleep; manual Off remains off. The control changes the hardware backlight.

For a normal installation, install the matching libspnav first and configure
spnavcfg with the same prefix (ensure its lib directory is in the loader path).
Then install/restart the matching spacenavd. Installing spnavcfg alone does not
add LCD support to an older running daemon.

General also provides an independent LED inactivity timeout, tested through the
same offscreen harness. Zero means Never; motion or a button press restores the
requested LED state, while manual Off remains off. Wayland focus detection uses
the daemon repository's optional GNOME extension and helper, not the GUI.
