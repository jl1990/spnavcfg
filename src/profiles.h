#ifndef PROFILES_UI_H_
#define PROFILES_UI_H_
#include <QKeySequence>
#include <QWidget>
#include <spnav_profiles.h>
class QComboBox;
class QLabel;
class QLineEdit;
class QTableWidget;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
class QTabWidget;
class QTimer;
int profileShortcutKeys(const QKeySequence &seq, unsigned int *keys);
QKeySequence profileShortcutSequence(const spnav_profile_button &button);
class ProfileEditor : public QWidget
{
    spnav_profile_set draft{}, baseline{};
    int naxes, nbuttons, selected = 0;
    bool loading = false, unsaved = false;
    QComboBox *choice;
    QLabel *active, *state;
    QLineEdit *match;
    QDoubleSpinBox *sensitivity;
    QCheckBox *inheritSensitivity, *swap, *inheritSwap;
    QTableWidget *axes, *buttons;
    QTabWidget *tabs;
    QPushButton *applyButton, *saveButton;
    QTimer *poll;
    void render();
    void changed();
    void createProfile(bool duplicate);
    void chooseApplication();
    void detectApplication();
    void refreshInherited();

  public:
    ProfileEditor(int axes, int buttons, QWidget *parent = nullptr);
    bool load();
    void setSnapshot(const spnav_profile_set &snapshot);
    bool dirty() const;
    bool pending() const { return dirty() || unsaved; }
    void setActiveProfile(int index);
    void selectButton(int index);
    void axisValue(int index,int value);
    bool apply();
    bool save();
    bool confirmClose();
};
#endif
