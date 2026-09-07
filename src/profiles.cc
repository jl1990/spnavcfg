#define SPNAV_CONFIG_H_
#include "profiles.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDirIterator>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QProgressBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <X11/Xlib.h>
#include <algorithm>
#include <cstring>
#include <spnav.h>
#undef None
#undef KeyPress
#undef KeyRelease

static void copyText(char *dst, size_t size, const QString &text)
{
    QByteArray b = text.toUtf8();
    std::memset(dst, 0, size);
    if (b.size() < (int)size)
        std::memcpy(dst, b.constData(), b.size());
}
static unsigned int qtSym(int key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        return key - 'A' + 'a';
    if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
        return 0xffbe + key - Qt::Key_F1;
    switch (key) {
    case Qt::Key_Escape:
        return 0xff1b;
    case Qt::Key_Tab:
        return 0xff09;
    case Qt::Key_Backtab:
        return 0xff09;
    case Qt::Key_Backspace:
        return 0xff08;
    case Qt::Key_Return:
        return 0xff0d;
    case Qt::Key_Enter:
        return 0xff8d;
    case Qt::Key_Insert:
        return 0xff63;
    case Qt::Key_Delete:
        return 0xffff;
    case Qt::Key_Home:
        return 0xff50;
    case Qt::Key_End:
        return 0xff57;
    case Qt::Key_Left:
        return 0xff51;
    case Qt::Key_Up:
        return 0xff52;
    case Qt::Key_Right:
        return 0xff53;
    case Qt::Key_Down:
        return 0xff54;
    case Qt::Key_PageUp:
        return 0xff55;
    case Qt::Key_PageDown:
        return 0xff56;
    default:
        return key >= 32 && key <= 126 ? key : 0;
    }
}
int profileShortcutKeys(const QKeySequence &seq, unsigned int *keys)
{
    std::memset(keys, 0, 8 * sizeof *keys);
    if (seq.isEmpty())
        return 0;
    if (seq.count() != 1)
        return -1;
    auto chord = seq[0];
    int n = 0;
    auto mods = chord.keyboardModifiers();
    if (mods & Qt::ControlModifier)
        keys[n++] = 0xffe3;
    if (mods & Qt::ShiftModifier)
        keys[n++] = 0xffe1;
    if (mods & Qt::AltModifier)
        keys[n++] = 0xffe9;
    if (mods & Qt::MetaModifier)
        keys[n++] = 0xffeb;
    unsigned int sym = qtSym(chord.key());
    if (mods & Qt::KeypadModifier) {
        int key = chord.key();
        if (key >= Qt::Key_0 && key <= Qt::Key_9) sym = 0xffb0 + key - Qt::Key_0;
        else if(key == Qt::Key_Plus) sym=0xffab;
        else if(key == Qt::Key_Minus) sym=0xffad;
        else if(key == Qt::Key_Asterisk) sym=0xffaa;
        else if(key == Qt::Key_Slash) sym=0xffaf;
        else if(key == Qt::Key_Period || key == Qt::Key_Comma) sym=0xffae;
        else if(key == Qt::Key_Return || key == Qt::Key_Enter) sym=0xff8d;
        else return -1;
    }
    if (!sym)
        return -1;
    keys[n++] = sym;
    return n;
}
QKeySequence profileShortcutSequence(const spnav_profile_button &b)
{
    QStringList parts;
    for (int i = 0; i < b.count; i++) {
        if(b.keys[i]>=0xffb0 && b.keys[i]<=0xffb9){parts<<"Num"<<QString::number(b.keys[i]-0xffb0);continue;}
        switch (b.keys[i]) {
        case 0xffe3:
        case 0xffe4:
            parts << "Ctrl";
            break;
        case 0xffe1:
        case 0xffe2:
            parts << "Shift";
            break;
        case 0xffe9:
        case 0xffea:
            parts << "Alt";
            break;
        case 0xffeb:
        case 0xffec:
            parts << "Meta";
            break;
        default:
            for (int k = Qt::Key_Escape; k <= Qt::Key_PageDown; k++)
                if (qtSym(k) == b.keys[i]) {
                    parts << QKeySequence(k).toString(QKeySequence::PortableText);
                    goto found;
                }
            if (b.keys[i] >= 0xffbe && b.keys[i] <= 0xffe0)
                parts << QString("F%1").arg(b.keys[i] - 0xffbe + 1);
            else if (b.keys[i] >= 32 && b.keys[i] <= 126)
                parts << QString(QChar(b.keys[i])).toUpper();
            else
                return QKeySequence();
        found:
            break;
        }
    }
    return QKeySequence(parts.join('+'), QKeySequence::PortableText);
}
ProfileEditor::ProfileEditor(int ax, int bn, QWidget *parent)
    : QWidget(parent), naxes(std::clamp(ax, 0, 64)), nbuttons(std::clamp(bn, 0, 64))
{
    auto *layout = new QVBoxLayout(this);
    auto *bar = new QHBoxLayout;
    bar->addWidget(new QLabel(tr("Editing:")));
    choice = new QComboBox;
    choice->setObjectName("profileChoice");
    bar->addWidget(choice, 1);
    for (auto entry :
         {std::pair<const char *, int>("New", 0), {"Duplicate", 1}, {"Rename", 2}, {"Delete", 3}}) {
        auto *b = new QPushButton(tr(entry.first));
        bar->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, op = entry.second] {
            if (op < 2) {
                createProfile(op == 1);
                return;
            }
            if (!selected) {
                QMessageBox::information(this, tr("Default profile"),
                                         tr("Default cannot be renamed or deleted."));
                return;
            }
            if (op == 2) {
                bool ok;
                QString name =
                    QInputDialog::getText(this, tr("Rename profile"), tr("Name"), QLineEdit::Normal,
                                          QString::fromUtf8(draft.profiles[selected].name), &ok);
                if (ok && !name.trimmed().isEmpty() && name.toUtf8().size() < 64 &&
                    !name.contains('"')) {
                    copyText(draft.profiles[selected].name, 64, name.trimmed());
                    render();
                    changed();
                }
            } else if (QMessageBox::question(
                           this, tr("Delete profile"),
                           tr("Delete %1 from this draft?")
                               .arg(QString::fromUtf8(draft.profiles[selected].name))) ==
                       QMessageBox::Yes) {
                for (int i = selected; i < draft.count - 1; i++)
                    draft.profiles[i] = draft.profiles[i + 1];
                draft.profiles[--draft.count] = {};
                selected = 0;
                render();
                changed();
            }
        });
    }
    layout->addLayout(bar);
    active = new QLabel(tr("Currently active: unknown"));
    layout->addWidget(active);
    auto *appbar = new QHBoxLayout;
    appbar->addWidget(new QLabel(tr("Application match:")));
    match = new QLineEdit;
    match->setPlaceholderText(tr("=blender.desktop (exact), or blender (contains)"));
    match->setMaxLength(255);
    appbar->addWidget(match, 1);
    auto *choose = new QPushButton(tr("Choose application…"));
    auto *detect = new QPushButton(tr("Detect application…"));
    appbar->addWidget(choose);
    appbar->addWidget(detect);
    layout->addLayout(appbar);
    connect(choose, &QPushButton::clicked, this, &ProfileEditor::chooseApplication);
    connect(detect, &QPushButton::clicked, this, &ProfileEditor::detectApplication);
    connect(match, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (!loading && selected) {
            copyText(draft.profiles[selected].match, 256, text);
            changed();
        }
    });
    tabs = new QTabWidget;
    layout->addWidget(tabs, 1);
    auto *axesPage = new QWidget;
    auto *al = new QVBoxLayout(axesPage);
    auto *controls = new QHBoxLayout;
    controls->addWidget(new QLabel(tr("Sensitivity")));
    sensitivity = new QDoubleSpinBox;
    sensitivity->setRange(0, 100);
    sensitivity->setDecimals(3);
    sensitivity->setSingleStep(.1);
    controls->addWidget(sensitivity);
    inheritSensitivity = new QCheckBox(tr("Use Default"));
    controls->addWidget(inheritSensitivity);
    swap = new QCheckBox(tr("Swap Y–Z"));
    controls->addWidget(swap);
    inheritSwap = new QCheckBox(tr("Use Default"));
    controls->addWidget(inheritSwap);
    controls->addStretch();
    al->addLayout(controls);
    connect(sensitivity, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (!loading) {
            draft.profiles[selected].sensitivity = qRound(v * 1000);
            changed();
        }
    });
    connect(swap, &QCheckBox::toggled, this, [this](bool v) {
        if (!loading) {
            draft.profiles[selected].swapyz = v;
            changed();
        }
    });
    connect(inheritSensitivity, &QCheckBox::toggled, this, [this](bool v) {
        if (!loading) {
            auto &p = draft.profiles[selected];
            p.overrides = (p.overrides & ~1) | (v ? 0 : 1);
            render();
            changed();
        }
    });
    connect(inheritSwap, &QCheckBox::toggled, this, [this](bool v) {
        if (!loading) {
            auto &p = draft.profiles[selected];
            p.overrides = (p.overrides & ~2) | (v ? 0 : 2);
            render();
            changed();
        }
    });
    axes = new QTableWidget(naxes, 7);
    axes->setHorizontalHeaderLabels({tr("Axis"), tr("Sensitivity"), tr("Deadzone"), tr("Invert"),
                                     tr("Maps to"), tr("Use Default"), tr("Live input")});
    axes->verticalHeader()->hide();
    axes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    al->addWidget(axes);
    tabs->addTab(axesPage, tr("Axes"));
    auto *bp = new QWidget;
    auto *bl = new QVBoxLayout(bp);
    auto *learn = new QPushButton(tr("Identify a button (10 seconds)"));
    bl->addWidget(learn);
    connect(learn, &QPushButton::clicked, this, [this] {
        if (spnav_profile_capture(1) < 0)
            state->setText(tr("Button identification is unavailable."));
        else
            state->setText(
                tr("Press a device button now. Mapped actions are suppressed for 10 seconds."));
    });
    auto *hint =
        new QLabel(tr("Press a SpaceMouse button to select its row. Shortcuts are one key or a "
                      "modifier combination. Labels appear on the Enterprise screen."));
    hint->setWordWrap(true);
    bl->addWidget(hint);
    buttons = new QTableWidget(nbuttons, 5);
    buttons->setObjectName("profileButtons");
    buttons->setHorizontalHeaderLabels({tr("Button"), tr("Assignment"), tr("Shortcut / button"),
                                        tr("Screen label"), tr("Use Default")});
    buttons->verticalHeader()->hide();
    buttons->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    buttons->setSelectionBehavior(QAbstractItemView::SelectRows);
    bl->addWidget(buttons);
    tabs->addTab(bp, tr("Buttons"));
    connect(buttons, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (loading || item->column() != 3)
            return;
        QByteArray label = item->text().toUtf8();
        if (label.size() > 63) {
            QSignalBlocker b(buttons);
            item->setText(QString::fromUtf8(draft.profiles[selected].buttons[item->row()].label));
            return;
        }
        copyText(draft.profiles[selected].buttons[item->row()].label, 64, item->text());
        draft.profiles[selected].button_override[item->row()] = 1;
        if (auto *cb = qobject_cast<QCheckBox *>(buttons->cellWidget(item->row(), 4))) {
            QSignalBlocker block(cb);
            cb->setChecked(false);
        }
        changed();
    });
    auto *foot = new QHBoxLayout;
    state = new QLabel;
    state->setWordWrap(true);
    foot->addWidget(state, 1);
    auto *revert = new QPushButton(tr("Revert draft"));
    applyButton = new QPushButton(tr("Apply"));
    saveButton = new QPushButton(tr("Save"));
    foot->addWidget(revert);
    foot->addWidget(applyButton);
    foot->addWidget(saveButton);
    layout->addLayout(foot);
    connect(revert, &QPushButton::clicked, this, [this] {
        if (!dirty() ||
            QMessageBox::question(
                this, tr("Revert draft"),
                tr("Discard your unapplied edits and reload the running configuration?")) ==
                QMessageBox::Yes)
            load();
    });
    connect(applyButton, &QPushButton::clicked, this, [this] { apply(); });
    connect(saveButton, &QPushButton::clicked, this, [this] { save(); });
    connect(choice, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int i) {
        if (!loading && i >= 0 && i < draft.count) {
            selected = i;
            render();
        }
    });
    poll = new QTimer(this);
    connect(poll, &QTimer::timeout, this, [this] { setActiveProfile(spnav_profile_active()); });
}
bool ProfileEditor::dirty() const { return std::memcmp(&draft, &baseline, sizeof draft) != 0; }
void ProfileEditor::changed()
{
    applyButton->setEnabled(dirty());
    saveButton->setEnabled(pending());
    state->setText(
        dirty()
            ? tr("Unsaved draft — Apply changes this session; Save keeps changes after restart.")
        : unsaved ? tr("Applied for this session. Save to keep these changes after restart.")
                  : tr("No pending edits."));
}
void ProfileEditor::setSnapshot(const spnav_profile_set &s)
{
    draft = baseline = s;
    selected = std::min(selected, s.count - 1);
    render();
    baseline = draft;
    changed();
}
bool ProfileEditor::load()
{
    spnav_profile_set s{};
    if (spnav_profiles_read(&s) < 0)
        return false;
    setSnapshot(s);
    poll->start(2000);
    setActiveProfile(spnav_profile_active());
    return true;
}
void ProfileEditor::setActiveProfile(int index)
{
    QString name = index >= 0 && index < baseline.count
                       ? QString::fromUtf8(baseline.profiles[index].name)
                       : tr("unavailable");
    active->setText(tr("Currently active: %1").arg(name));
}
void ProfileEditor::refreshInherited()
{
    auto &base = draft.profiles[0];
    for (int i = 1; i < draft.count; i++) {
        auto &p = draft.profiles[i];
        if (!(p.overrides & 1))
            p.sensitivity = base.sensitivity;
        if (!(p.overrides & 2))
            p.swapyz = base.swapyz;
        for (int j = 0; j < 64; j++) {
            if (!p.axis_override[j])
                p.axes[j] = base.axes[j];
            if (!p.button_override[j])
                p.buttons[j] = base.buttons[j];
        }
    }
}
void ProfileEditor::render()
{
    loading = true;
    refreshInherited();
    choice->clear();
    for (int i = 0; i < draft.count; i++)
        choice->addItem(QString::fromUtf8(draft.profiles[i].name));
    choice->setCurrentIndex(selected);
    auto &p = draft.profiles[selected];
    match->setText(QString::fromUtf8(p.match));
    match->setEnabled(selected > 0);
    sensitivity->setValue(p.sensitivity / 1000.0);
    swap->setChecked(p.swapyz);
    inheritSensitivity->setEnabled(selected > 0);
    inheritSensitivity->setChecked(selected && !(p.overrides & 1));
    sensitivity->setEnabled(!selected || (p.overrides & 1));
    inheritSwap->setEnabled(selected > 0);
    inheritSwap->setChecked(selected && !(p.overrides & 2));
    swap->setEnabled(!selected || (p.overrides & 2));
    const char *names[] = {"TX", "TY", "TZ", "RX", "RY", "RZ"};
    for (int i = 0; i < naxes; i++) {
        auto *item = new QTableWidgetItem(i < 6 ? QString(names[i]) : QString::number(i));
        item->setFlags(Qt::ItemIsEnabled);
        axes->setItem(i, 0, item);
        auto *live=new QProgressBar;live->setRange(-512,512);live->setValue(0);live->setFormat("%v");axes->setCellWidget(i,6,live);
        auto *sens = new QDoubleSpinBox;
        sens->setRange(0, 100);
        sens->setDecimals(3);
        sens->setValue(p.axes[i].sensitivity / 1000.0);
        axes->setCellWidget(i, 1, sens);
        auto *dead = new QSpinBox;
        dead->setRange(0, 32767);
        dead->setValue(p.axes[i].deadzone);
        axes->setCellWidget(i, 2, dead);
        auto *inv = new QCheckBox;
        inv->setChecked(p.axes[i].invert);
        axes->setCellWidget(i, 3, inv);
        auto *mapping = new QSpinBox;
        mapping->setRange(0, 5);
        mapping->setValue(p.axes[i].map);
        axes->setCellWidget(i, 4, mapping);
        auto *inherit = new QCheckBox;
        inherit->setChecked(selected && !p.axis_override[i]);
        inherit->setEnabled(selected);
        axes->setCellWidget(i, 5, inherit);
        for (int j = 1; j < 5; j++)
            axes->cellWidget(i, j)->setEnabled(!selected || p.axis_override[i]);
        connect(sens, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, i](double v) {
            if (!loading) {
                draft.profiles[selected].axes[i].sensitivity = qRound(v * 1000);
                changed();
            }
        });
        connect(dead, qOverload<int>(&QSpinBox::valueChanged), this, [this, i](int v) {
            if (!loading) {
                draft.profiles[selected].axes[i].deadzone = v;
                changed();
            }
        });
        connect(inv, &QCheckBox::toggled, this, [this, i](bool v) {
            if (!loading) {
                draft.profiles[selected].axes[i].invert = v;
                changed();
            }
        });
        connect(mapping, qOverload<int>(&QSpinBox::valueChanged), this, [this, i](int v) {
            if (!loading) {
                draft.profiles[selected].axes[i].map = v;
                changed();
            }
        });
        connect(inherit, &QCheckBox::toggled, this, [this, i](bool v) {
            if (!loading) {
                draft.profiles[selected].axis_override[i] = !v;
                render();
                changed();
            }
        });
    }
    for (int i = 0; i < nbuttons; i++) {
        auto &b = p.buttons[i];
        auto *id = new QTableWidgetItem(QString::number(i + 1));
        id->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        id->setToolTip(tr("Hardware index %1").arg(i));
        buttons->setItem(i, 0, id);
        auto *action = new QComboBox;
        action->addItems({tr("Button"), tr("Keyboard shortcut"), tr("Sensitivity reset"),
                          tr("Sensitivity increase"), tr("Sensitivity decrease"),
                          tr("Disable rotation"), tr("Disable translation"), tr("Dominant axis")});
        action->setCurrentIndex(b.action ? b.action + 1 : b.count ? 1 : 0);
        buttons->setCellWidget(i, 1, action);
        if (action->currentIndex() == 0) {
            auto *map = new QSpinBox;
            map->setRange(1, 64);
            map->setValue(b.map + 1);
            buttons->setCellWidget(i, 2, map);
            connect(map, qOverload<int>(&QSpinBox::valueChanged), this, [this, i](int v) {
                if (!loading) {
                    draft.profiles[selected].buttons[i].map = v - 1;
                    changed();
                }
            });
        } else if (action->currentIndex() == 1) {
            auto *holder = new QWidget;
            auto *row = new QHBoxLayout(holder);
            row->setContentsMargins(0, 0, 0, 0);
            auto *rec = new QKeySequenceEdit(profileShortcutSequence(b));
            rec->setMaximumSequenceLength(1);
            row->addWidget(rec);
            auto *advanced = new QPushButton("…");
            advanced->setMaximumWidth(30);
            advanced->setToolTip(tr("Enter key names, including modifier-only bindings"));
            row->addWidget(advanced);
            QStringList names;
            for (int k = 0; k < b.count; k++) {
                const char *name = XKeysymToString(b.keys[k]);
                names << (name ? QString(name) : QString("0x%1").arg(b.keys[k], 0, 16));
            }
            holder->setToolTip(names.join('+'));
            buttons->setCellWidget(i, 2, holder);
            connect(rec, &QKeySequenceEdit::keySequenceChanged, this,
                    [this, i](const QKeySequence &seq) {
                        if (loading)
                            return;
                        unsigned int keys[8];
                        int n = profileShortcutKeys(seq, keys);
                        if (n < 0) {
                            state->setText(tr("Unsupported shortcut. Use a standard key with "
                                              "optional Ctrl, Shift, Alt or Meta."));
                            return;
                        }
                        auto &binding = draft.profiles[selected].buttons[i];
                        binding.count = n ? n : -1; /* An empty recorder is an incomplete draft. */
                        std::memcpy(binding.keys, keys, sizeof keys);
                        changed();
                    });
            connect(advanced, &QPushButton::clicked, this, [this, i, names] {
                bool ok;
                QString text = QInputDialog::getText(
                    this, tr("Key names"),
                    tr("One key or up to eight names joined by +, e.g. Control_L+s or Shift_L"),
                    QLineEdit::Normal, names.join('+'), &ok);
                if (!ok)
                    return;
                QStringList parts = text.split('+');
                unsigned int keys[8] = {};
                int count = 0;
                for (const QString &part : parts) {
                    unsigned long key = XStringToKeysym(part.trimmed().toLatin1().constData());
                    if (!key || count == 8) {
                        QMessageBox::warning(
                            this, tr("Invalid shortcut"),
                            tr("Use valid X key names and no more than eight keys."));
                        return;
                    }
                    keys[count++] = key;
                }
                auto &binding = draft.profiles[selected].buttons[i];
                binding.count = count;
                std::memcpy(binding.keys, keys, sizeof keys);
                render();
                changed();
            });
        } else
            buttons->setCellWidget(i, 2, new QLabel(tr("Built-in action")));
        auto *label = new QTableWidgetItem(QString::fromUtf8(b.label));
        buttons->setItem(i, 3, label);
        auto *inherit = new QCheckBox;
        inherit->setChecked(selected && !p.button_override[i]);
        inherit->setEnabled(selected);
        buttons->setCellWidget(i, 4, inherit);
        buttons->cellWidget(i, 1)->setEnabled(!selected || p.button_override[i]);
        buttons->cellWidget(i, 2)->setEnabled(!selected || p.button_override[i]);
        connect(inherit, &QCheckBox::toggled, this, [this, i](bool v) {
            if (!loading) {
                draft.profiles[selected].button_override[i] = !v;
                render();
                changed();
            }
        });
        connect(action, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, i](int v) {
            if (loading)
                return;
            auto &b = draft.profiles[selected].buttons[i];
            b.action = v > 1 ? v - 1 : 0;
            b.count = 0;
            std::memset(b.keys, 0, sizeof b.keys);
            if (v == 1) {
                b.count = -1; /* Require a recorded shortcut before applying. */
            }
            render();
            changed();
        });
    }
    loading = false;
}
void ProfileEditor::selectButton(int i)
{
    if (i >= 0 && i < nbuttons) {
        buttons->selectRow(i);
        buttons->scrollToItem(buttons->item(i, 0));
        tabs->setCurrentIndex(1);
    }
}
void ProfileEditor::createProfile(bool duplicate)
{
    if (draft.count >= 17) {
        QMessageBox::information(this, tr("Profiles"),
                                 tr("The daemon supports up to 16 application profiles."));
        return;
    }
    bool ok;
    QString name =
        QInputDialog::getText(
            this, tr("New profile"), tr("Profile name"), QLineEdit::Normal,
            duplicate ? QString::fromUtf8(draft.profiles[selected].name) + tr(" copy") : QString(),
            &ok)
            .trimmed();
    if (!ok || name.isEmpty())
        return;
    if (name.toUtf8().size() >= 64 || name.contains('"')) {
        QMessageBox::warning(this, tr("Name"),
                             tr("Use fewer than 64 UTF-8 bytes and no quotation marks."));
        return;
    }
    for (int i = 0; i < draft.count; i++)
        if (name == QString::fromUtf8(draft.profiles[i].name)) {
            QMessageBox::warning(this, tr("Name"), tr("That name already exists."));
            return;
        }
    auto &p = draft.profiles[draft.count];
    p = draft.profiles[duplicate ? selected : 0];
    copyText(p.name, 64, name);
    p.match[0] = 0;
    if (!duplicate || !selected) {
        p.overrides = 0;
        std::memset(p.axis_override, 0, sizeof p.axis_override);
        std::memset(p.button_override, 0, sizeof p.button_override);
    }
    selected = draft.count++;
    render();
    changed();
    match->setFocus();
}
void ProfileEditor::chooseApplication()
{
    if (!selected)
        return;
    QMap<QString, QString> apps;
    for (const QString &base :
         QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        QDir root(base + "/applications");
        QDirIterator it(root.absolutePath(), {"*.desktop"}, QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString path = it.next();
            QSettings s(path, QSettings::IniFormat);
            s.beginGroup("Desktop Entry");
            if (s.value("Hidden").toBool() || s.value("NoDisplay").toBool() ||
                s.value("Type").toString() != "Application")
                continue;
            QString id = root.relativeFilePath(path).replace('/', '-');
            QString title = s.value("Name").toString() + " (" + id + ")";
            if (!apps.contains(title))
                apps[title] = id;
        }
    }
    bool ok;
    QString title = QInputDialog::getItem(this, tr("Choose application"),
                                          tr("Installed application"), apps.keys(), 0, false, &ok);
    if (ok) {
        copyText(draft.profiles[selected].match, 256, "=" + apps[title]);
        render();
        changed();
    }
}
void ProfileEditor::detectApplication()
{
    if (!selected)
        return;
    const QString target = QString::fromUtf8(draft.profiles[selected].name);
    state->setText(
        tr("Switch to the application now. Its identity will be captured in 5 seconds."));
    QTimer::singleShot(5000, this, [this, target] {
        char id[256] = {};
        if (target != QString::fromUtf8(draft.profiles[selected].name) ||
            spnav_profile_focus(id, sizeof id) < 0 || !id[0] || strlen(id) > 254) {
            state->setText(tr("No application detected. Keep its window focused and try again."));
            return;
        }
        copyText(draft.profiles[selected].match, 256, "=" + QString::fromUtf8(id));
        render();
        changed();
    });
}
bool ProfileEditor::apply()
{
    if (!dirty())
        return true;
    refreshInherited();
    for(int p=0;p<draft.count;p++)for(int b=0;b<64;b++)if(draft.profiles[p].buttons[b].count<0){
        QMessageBox::warning(this,tr("Incomplete shortcut"),tr("Record a shortcut for button %1 in %2 before applying.").arg(b+1).arg(QString::fromUtf8(draft.profiles[p].name)));return false;
    }
    int res = spnav_profiles_apply(&draft);
    if (res < 0) {
        QMessageBox::warning(this, tr("Could not apply profiles"),
                             res == -3 ? tr("A shortcut uses an unsupported key. Use standard keys, F1–F24 or numpad keys.") : res == -2
                                 ? tr("The configuration changed elsewhere. Your draft is intact. "
                                      "Revert to reload before editing again.")
                                 : tr("The daemon rejected the draft. Check unique names, "
                                      "application matches, shortcuts and connection status."));
        return false;
    }
    for (int i = 0; i < draft.count; i++)
        draft.profiles[i].source_index = i;
    baseline = draft;
    unsaved = true;
    changed();
    setActiveProfile(spnav_profile_active());
    return true;
}
bool ProfileEditor::save()
{
    if (!apply())
        return false;
    if (spnav_cfg_save() < 0) {
        QMessageBox::warning(this, tr("Save failed"),
                             tr("The changes are active for this session, but could not be saved. "
                                "Check daemon permissions and retry."));
        return false;
    }
    unsaved = false;
    changed();
    return true;
}
bool ProfileEditor::confirmClose()
{
    if (!pending())
        return true;
    auto answer =
        QMessageBox::question(this, tr("Unsaved profiles"),
                              tr("Save your profile changes before closing? Discard leaves "
                                 "already-applied session changes running."),
                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    return answer == QMessageBox::Save ? save() : answer == QMessageBox::Discard;
}

void ProfileEditor::axisValue(int index,int value)
{
 if(index<0 || index>=naxes)return;
 if(auto *bar=qobject_cast<QProgressBar*>(axes->cellWidget(index,6))){int extent=std::max(bar->maximum(),std::abs(value));bar->setRange(-extent,extent);bar->setValue(value);}
}
