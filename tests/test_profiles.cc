#define SPNAV_CONFIG_H_
#include "profiles.h"
#include <QApplication>
#include <QComboBox>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QTableWidget>
#include <cassert>
#include <spnav.h>
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    spnav_profile_set s = {};
    s.version = 1;
    s.count = 2;
    strcpy(s.profiles[0].name, "Default");
    strcpy(s.profiles[1].name, "Blender");
    strcpy(s.profiles[1].match, "=blender.desktop");
    s.profiles[0].sensitivity = 1000;
    s.profiles[1].sensitivity = 1000;
    ProfileEditor editor(6, 31);
    editor.setSnapshot(s);
    auto *choice = editor.findChild<QComboBox *>("profileChoice");
    assert(choice);
    choice->setCurrentIndex(1);
    auto *table = editor.findChild<QTableWidget *>("profileButtons");
    assert(table && table->rowCount() == 31);
    table->item(0, 3)->setText("Frame selected");
    assert(editor.dirty());
    editor.setActiveProfile(0);
    assert(choice->currentIndex() == 1 && table->item(0, 3)->text() == "Frame selected");
    choice->setCurrentIndex(0);
    choice->setCurrentIndex(1);
    assert(table->item(0, 3)->text() == "Frame selected");
    unsigned int keys[8] = {};
    assert(profileShortcutKeys(QKeySequence("Ctrl+Shift+S"), keys) == 3);
    assert(keys[0] == 0xffe3 && keys[1] == 0xffe1 && keys[2] == 0x73);
    assert(profileShortcutKeys(QKeySequence(QKeyCombination(Qt::ControlModifier | Qt::KeypadModifier, Qt::Key_1)), keys) == 2);
    assert(keys[1] == 0xffb1);
    choice->setCurrentIndex(0);
    auto *assignment=qobject_cast<QComboBox*>(table->cellWidget(0,1));assert(assignment);assignment->setCurrentIndex(1);
    auto *record=table->cellWidget(0,2)->findChild<QKeySequenceEdit*>();assert(record && record->keySequence().isEmpty());
    editor.selectButton(5);
    assert(table->currentRow() == 5);
    if (argc > 1) {
        editor.resize(1050, 750);
        editor.show();
        app.processEvents();
        editor.grab().save(argv[1]);
    }
    puts("Profile draft isolation, button selection and shortcut recording passed");
}
