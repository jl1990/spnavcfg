#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# Build against the sibling libspnav checkout, without changing installed libraries.
make -j4 add_cflags="-I../libspnav/src" LDFLAGS="$(pkg-config --libs Qt6Widgets) ../libspnav/libspnav.a -lX11"
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT HUP INT TERM
${CXX:-c++} -g -std=c++17 -fPIC -Isrc -I../libspnav/src $(pkg-config --cflags Qt6Widgets) \
 tests/test_screen.cc src/ui.o src/ui.moc.o src/spnavcfg.o res.cc \
 ../libspnav/libspnav.a $(pkg-config --libs Qt6Widgets) -lX11 \
 -Wl,--wrap=spnav_cfg_set_lcd,--wrap=spnav_cfg_get_lcd,--wrap=spnav_lcd_refresh,--wrap=spnav_cfg_save,--wrap=spnav_cfg_set_led_idle,--wrap=spnav_cfg_get_led_idle,--wrap=spnav_cfg_set_lcd_brightness,--wrap=spnav_cfg_get_lcd_brightness,--wrap=spnav_cfg_set_lcd_idle,--wrap=spnav_cfg_get_lcd_idle \
 -o "$build/test_screen"
QT_QPA_PLATFORM=offscreen "$build/test_screen" ${SCREEN_PREVIEW:+"$SCREEN_PREVIEW"}
