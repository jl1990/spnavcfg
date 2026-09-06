#include <cassert>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QAction>
#include <QTimer>
#include <QMessageBox>
#include <QStatusBar>
#include <QPushButton>
#include <QTabWidget>
#define SPNAV_CONFIG_H_
#include <spnav.h>
#include "spnavcfg.h"
#include "ui.h"
MainWin *mainwin;
static int save_result, led_idle;
extern "C" int __wrap_spnav_cfg_set_led_idle(int v) { led_idle=v; return 0; }
extern "C" int __wrap_spnav_cfg_get_led_idle(void) { return led_idle; }
extern "C" int __wrap_spnav_cfg_save(void) { return save_result; }
static int accepted = 3, fail_write, writes, refreshes, brightness=65, idle=120;
extern "C" int __wrap_spnav_cfg_set_lcd_brightness(int v) { if(fail_write) return -1; brightness=v; return 0; }
extern "C" int __wrap_spnav_cfg_get_lcd_brightness(void) { return brightness; }
extern "C" int __wrap_spnav_cfg_set_lcd_idle(int v) { if(fail_write) return -1; idle=v; return 0; }
extern "C" int __wrap_spnav_cfg_get_lcd_idle(void) { return idle; }
extern "C" int __wrap_spnav_cfg_set_lcd(int flags)
{ writes++; if(fail_write) return -1; accepted = flags; return 0; }
extern "C" int __wrap_spnav_cfg_get_lcd(void) { return accepted; }
extern "C" int __wrap_spnav_lcd_refresh(void) { refreshes++; return fail_write ? -1 : 0; }
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	MainWin w; mainwin = &w; assert(w.init());
	devinfo.name = strdup("3Dconnexion SpaceMouse Enterprise");
	devinfo.path = strdup("/dev/input/event22");
	devinfo.type = SPNAV_DEV_SMENT; devinfo.naxes = 6; devinfo.nbuttons = 31;
	cfg.sens = 1; cfg.led = 2; cfg.grab = 1; cfg.repeat = -1; cfg.lcd_flags = 3; cfg.lcd_brightness=65; cfg.lcd_idle_seconds=120;
	for(int i=0;i<6;i++) { cfg.sens_axis[i]=1; cfg.map_axis[i]=i; cfg.dead_thres[i]=2; }
	for(int i=0;i<31;i++) cfg.map_bn[i]=i;
	w.updateui();
	auto ledTimer=w.findChild<QSpinBox*>("spin_led_idle"); assert(ledTimer);
	ledTimer->setValue(60); assert(led_idle==60 && cfg.led_idle_seconds==60);
	auto mode=w.findChild<QComboBox*>("combo_screen");
	auto title=w.findChild<QCheckBox*>("chk_screen_title");
	auto refresh=w.findChild<QPushButton*>("bn_screen_refresh");
	auto status=w.findChild<QLabel*>("lb_screen_status");
	auto bright=w.findChild<QSpinBox*>("spin_screen_brightness");
	auto timer=w.findChild<QSpinBox*>("spin_screen_idle");
	assert(mode && title && refresh && status && bright && timer);
	assert(bright->value()==65 && timer->value()==120);
	bright->setValue(40); timer->setValue(300);
	assert(brightness==40 && idle==300 && cfg.lcd_idle_seconds==300);
	fail_write=1; timer->setValue(600); bright->setValue(80);
	assert(timer->value()==300 && bright->value()==40);
	fail_write=0;
	assert(mode->isEnabled() && mode->currentIndex()==1 && writes==0);
	mode->setCurrentIndex(0);
	assert(accepted==2 && cfg.lcd_flags==2 && !title->isEnabled());
	fail_write=1; mode->setCurrentIndex(1);
	assert(cfg.lcd_flags==2 && mode->currentIndex()==0 && status->text().contains("Could not"));
	fail_write=0; mode->setCurrentIndex(1); title->setChecked(false);
	assert(cfg.lcd_flags==1 && accepted==1);
	refresh->click(); assert(refreshes==1);
	cfg.lcd_flags=-1; w.updateui(); assert(!mode->isEnabled() && !refresh->isEnabled());
	cfg.lcd_flags=3; devinfo.type=SPNAV_DEV_SNAV; w.updateui(); assert(!mode->isEnabled());
	devinfo.type=SPNAV_DEV_SMENT; w.updateui();
	auto tabs=w.findChild<QTabWidget*>("tabWidget_2");
	for(int i=0;i<tabs->count();i++) if(tabs->tabText(i)=="Screen") tabs->setCurrentIndex(i);
	auto save=w.findChild<QAction*>("act_savecfg"); assert(save);
	for(int result : {0, -1}) {
		save_result=result;
		QTimer::singleShot(0, [] {
			for(QWidget *top : QApplication::topLevelWidgets())
				if(auto dialog=qobject_cast<QMessageBox*>(top)) dialog->button(QMessageBox::Yes)->click();
		});
		save->trigger();
		assert(w.statusBar()->currentMessage().contains(result ? "Could not" : "saved"));
	}
	w.statusBar()->clearMessage();
	w.resize(960,800); w.show(); app.processEvents();
	if(argc==2) assert(w.grab().save(argv[1]));
	puts("Screen UI tests passed");
	return 0;
}
