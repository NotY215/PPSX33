/* PPSX33 Modify — Qt6 per-game launcher (prebuilt next to host, shipped with games). */
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTextEdit>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QMessageBox>
#include <QFileInfo>
#include <QMap>

static const QStringList kDefaultKeys = {
    "LeftStickLeft","LeftStickDown","LeftStickRight","LeftStickUp",
    "RightStickLeft","RightStickDown","RightStickRight","RightStickUp",
    "Start","Select","PsButton",
    "Square","Cross","Circle","Triangle",
    "DpadLeft","DpadDown","DpadRight","DpadUp",
    "R1","R2","R3","L1","L2","L3"
};
static const QMap<QString,QString> kDefaultMap = {
    {"LeftStickLeft","Left"},{"LeftStickDown","Down"},{"LeftStickRight","Right"},{"LeftStickUp","Up"},
    {"RightStickLeft","H"},{"RightStickDown","J"},{"RightStickRight","K"},{"RightStickUp","U"},
    {"Start","V"},{"Select","Space"},{"PsButton","Backspace"},
    {"Square","A"},{"Cross","Z"},{"Circle","X"},{"Triangle","S"},
    {"DpadLeft","NumPad4"},{"DpadDown","NumPad5"},{"DpadRight","NumPad6"},{"DpadUp","NumPad8"},
    {"R1","R"},{"R2","RControlKey"},{"R3","LControlKey"},
    {"L1","L"},{"L2","RShiftKey"},{"L3","LShiftKey"}
};

class ModifyWindow : public QMainWindow {
public:
    ModifyWindow() {
        setWindowTitle(QStringLiteral("PPSX33 Modify"));
        resize(560, 640);
        m_dir = QCoreApplication::applicationDirPath();
        discoverGame();
        buildUi();
        loadKeymap();
        applyTheme();
    }
private:
    QString m_dir, m_gameExe;
    QComboBox *m_api = nullptr, *m_res = nullptr;
    QMap<QString, QLineEdit*> m_edits;
    QTextEdit* m_log = nullptr;

    void log(const QString& s) { if (m_log) m_log->append(s); }

    void discoverGame() {
        // Prefer ppsx33_tag.txt + scan EXEs for PPSX33_TAG_
        QString expectTag;
        QFile tf(m_dir + "/ppsx33_tag.txt");
        if (tf.open(QIODevice::ReadOnly))
            expectTag = QString::fromUtf8(tf.readAll()).trimmed();

        const QDir d(m_dir);
        for (const QFileInfo& fi : d.entryInfoList({"*.exe"}, QDir::Files)) {
            if (fi.fileName().compare("Modify.exe", Qt::CaseInsensitive) == 0) continue;
            QFile f(fi.absoluteFilePath());
            if (!f.open(QIODevice::ReadOnly)) continue;
            const QByteArray data = f.read(fi.size() > 8*1024*1024 ? 8*1024*1024 : fi.size());
            if (!data.contains("PPSX33_TAG_")) continue;
            if (!expectTag.isEmpty() && !data.contains(expectTag.toUtf8())) continue;
            m_gameExe = fi.fileName();
            break;
        }
        if (m_gameExe.isEmpty()) {
            for (const QFileInfo& fi : d.entryInfoList({"*.exe"}, QDir::Files)) {
                if (fi.fileName().compare("Modify.exe", Qt::CaseInsensitive) == 0) continue;
                m_gameExe = fi.fileName();
                break;
            }
        }
    }

    void buildUi() {
        auto* central = new QWidget(this);
        setCentralWidget(central);
        auto* root = new QVBoxLayout(central);
        root->setContentsMargins(16, 16, 16, 16);
        root->setSpacing(12);

        auto* title = new QLabel(QStringLiteral("PPSX33  ·  MODIFY"), central);
        title->setObjectName("title");
        root->addWidget(title);
        auto* sub = new QLabel(
            m_gameExe.isEmpty() ? QStringLiteral("No tagged game EXE found in this folder")
                                : QStringLiteral("Bound game: %1").arg(m_gameExe), central);
        sub->setObjectName("sub");
        root->addWidget(sub);

        auto* tabs = new QTabWidget(central);
        // Graphics
        auto* gpage = new QWidget;
        auto* gl = new QFormLayout(gpage);
        m_api = new QComboBox(gpage);
        m_api->addItem("Vulkan 1.1", "vulkan");
        m_api->addItem("OpenGL 4.5", "opengl");
        gl->addRow("Present API", m_api);
        tabs->addTab(gpage, "Graphics");

        // Quality
        auto* qpage = new QWidget;
        auto* ql = new QFormLayout(qpage);
        m_res = new QComboBox(qpage);
        m_res->addItem("560p", 560);
        m_res->addItem("720p", 720);
        m_res->addItem("480p", 480);
        m_res->setCurrentIndex(0);
        m_res->addItem("480p", 480);
        ql->addRow("Height (width 720)", m_res);
        tabs->addTab(qpage, "Quality");

        // Controller
        auto* cpage = new QWidget;
        auto* cv = new QVBoxLayout(cpage);
        auto* scroll = new QScrollArea(cpage);
        scroll->setWidgetResizable(true);
        auto* formHost = new QWidget;
        auto* form = new QFormLayout(formHost);
        for (const QString& k : kDefaultKeys) {
            auto* ed = new QLineEdit(formHost);
            ed->setPlaceholderText(kDefaultMap.value(k));
            m_edits[k] = ed;
            form->addRow(k, ed);
        }
        scroll->setWidget(formHost);
        cv->addWidget(scroll);
        auto* row = new QHBoxLayout;
        auto* save = new QPushButton("Save keymap", cpage);
        auto* reset = new QPushButton("Reset to default", cpage);
        connect(save, &QPushButton::clicked, this, &ModifyWindow::saveKeymap);
        connect(reset, &QPushButton::clicked, this, &ModifyWindow::resetKeymap);
        row->addWidget(save);
        row->addWidget(reset);
        cv->addLayout(row);
        tabs->addTab(cpage, "Controller");

        root->addWidget(tabs, 1);

        auto* play = new QPushButton(QStringLiteral("PLAY"), central);
        play->setObjectName("play");
        play->setMinimumHeight(44);
        connect(play, &QPushButton::clicked, this, &ModifyWindow::play);
        root->addWidget(play);

        m_log = new QTextEdit(central);
        m_log->setReadOnly(true);
        m_log->setMaximumHeight(120);
        root->addWidget(m_log);
        if (!m_gameExe.isEmpty()) log("Ready · " + m_gameExe);
    }

    void applyTheme() {
        setStyleSheet(R"(
            QMainWindow, QWidget { background: #0c101c; color: #c8e6f0; font-family: 'Segoe UI'; font-size: 13px; }
            QLabel#title { font-size: 18px; font-weight: 600; color: #7ef0ff; }
            QLabel#sub { color: #7a9aaa; }
            QTabWidget::pane { border: 1px solid #1e2a3a; background: #121826; }
            QTabBar::tab { background: #121826; color: #9ab; padding: 8px 14px; margin-right: 2px; }
            QTabBar::tab:selected { background: #1a2838; color: #7ef0ff; border-bottom: 2px solid #00c8dc; }
            QLineEdit, QComboBox { background: #0a0e18; border: 1px solid #2a3a4a; padding: 6px; border-radius: 6px; color: #e0f4ff; }
            QPushButton { background: #1a2838; border: 1px solid #2a4a5a; border-radius: 8px; padding: 8px 14px; color: #c8e6f0; }
            QPushButton:hover { border-color: #00c8dc; }
            QPushButton#play { background: #00c8dc; color: #041018; font-weight: 700; font-size: 15px; border: none; }
            QTextEdit { background: #0a0e18; border: 1px solid #1e2a3a; border-radius: 6px; }
        )");
    }

    QString keymapPath() const { return m_dir + "/keymap.json"; }

    void loadKeymap() {
        QJsonObject o;
        QFile f(keymapPath());
        if (f.open(QIODevice::ReadOnly))
            o = QJsonDocument::fromJson(f.readAll()).object();
        for (const QString& k : kDefaultKeys) {
            QString v = o.value(k).toString();
            if (v.isEmpty()) v = kDefaultMap.value(k);
            if (m_edits.contains(k)) m_edits[k]->setText(v);
        }
    }

    void saveKeymap() {
        QJsonObject o;
        for (const QString& k : kDefaultKeys) {
            QString v = m_edits[k]->text().trimmed();
            if (v.isEmpty()) v = kDefaultMap.value(k);
            o.insert(k, v);
        }
        QFile f(keymapPath());
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
        log("Saved keymap.json");
    }

    void resetKeymap() {
        for (const QString& k : kDefaultKeys) {
            if (m_edits.contains(k)) m_edits[k]->setText(kDefaultMap.value(k));
        }
        saveKeymap();
        log("Controller reset to default");
    }

    void play() {
        if (m_gameExe.isEmpty()) {
            QMessageBox::warning(this, "PPSX33 Modify", "No game EXE with PPSX33 tag found.");
            return;
        }
        saveKeymap();
        QStringList args;
        args << "-nogui";
        args << (m_api->currentData().toString() == "opengl" ? "-api=opengl" : "-api=vulkan");
        args << QString("-h=%1").arg(m_res->currentData().toInt());
        const QString exe = m_dir + "/" + m_gameExe;
        log("Launch " + exe + " " + args.join(' '));
        if (!QProcess::startDetached(exe, args, m_dir))
            log("Launch failed");
        else
            log("Launched");
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("PPSX33 Modify");
    ModifyWindow w;
    w.show();
    return app.exec();
}
