#pragma once
// Playground.h - an interactive workflow sandbox.
//
//   ┌ header: title · status · Undo · Reset ───────────────────────────────┐
//   │ add bar:  [ name... ] [+ Task] [+ Sequential] [+ Parallel] [Templates]│
//   ├──────────────── workflow tree ───────┬──────── inspector ─────────────┤
//   │ stats chips + progress               │ selected item, state, owner    │
//   │ ▸ Workflow                           │ actions enabled by state       │
//   │    • Task …            ● Started     │ [Assign as: Alice ▾]           │
//   ├──────────────────────────────────────┴────────────────────────────────┤
//   │ terminal: coloured activity log                                       │
//   └───────────────────────────────────────────────────────────────────────┘
//
// Ways to play:
//   * build a workflow with the add bar (items go inside the selected stage)
//   * pick a template (Order fulfilment, Hiring pipeline)
//   * select any item and use the inspector buttons, or right-click it
//   * double-click an item to push it forward one state
//   * Step / Auto-play runs the workflow one transition at a time so you can
//     watch the sequential and parallel rules open tasks
//   * Undo walks back assign / start / complete (Command + Memento)
//
// Shortcuts: Enter add task · Ctrl+1/2/3 add task/sequential/parallel
//            F6 step · F5 auto-play · Ctrl+Z undo
// No Q_OBJECT / moc needed.

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QTextEdit>
#include <QScrollBar>
#include <QLineEdit>
#include <QProgressBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QShortcut>
#include <QKeySequence>
#include <QTime>
#include <QVariant>
#include <QColor>
#include <QBrush>
#include <QFont>
#include <QStringList>

#include <exception>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "WorkItem.h"
#include "Task.h"
#include "SequentialStage.h"
#include "ParallelStage.h"
#include "Participant.h"
#include "Stage.h"
#include "CommandInvoker.h"
#include "AssignCommand.h"
#include "StartCommand.h"
#include "CompleteCommand.h"

class Playground : public QWidget {
public:
    Playground()
        : QWidget(nullptr), root_(nullptr), invoker_(nullptr), nextId_(0)
    {
        people_.push_back(new Participant("p1", "You", 0));
        people_.push_back(new Participant("p2", "Alice", 0));
        people_.push_back(new Participant("p3", "Bob", 0));
        people_.push_back(new Participant("p4", "Carol", 0));
        build();
        resetWorkflow(true);
    }

    ~Playground() {
        if (timer_) timer_->stop();
        delete invoker_;
        delete root_;
        for (std::size_t i = 0; i < people_.size(); ++i) delete people_[i];
    }

private:
    enum LogKind { Info, Ok, Err, Step };

    // ======================================================================
    // UI construction
    // ======================================================================
    void build() {
        setObjectName("playground");
        setMinimumSize(900, 700);

        QVBoxLayout *page = new QVBoxLayout(this);
        page->setContentsMargins(20, 18, 20, 16);
        page->setSpacing(12);

        // ---- header ----------------------------------------------------
        QHBoxLayout *header = new QHBoxLayout;
        QVBoxLayout *heading = new QVBoxLayout;
        heading->setSpacing(0);
        QLabel *title = new QLabel("Workflow Playground");
        title->setObjectName("pageTitle");
        QLabel *subtitle = new QLabel("Build a workflow, then watch it run one transition at a time.");
        subtitle->setObjectName("mutedText");
        heading->addWidget(title);
        heading->addWidget(subtitle);

        status_ = new QLabel("Ready");
        status_->setObjectName("statusBadge");

        undoBtn_ = button("↶ Undo", "ghostButton", [this] { undo(); });
        undoBtn_->setToolTip("Undo the last assign / start / complete  (Ctrl+Z)");
        QPushButton *reset = button("⟲ Reset", "ghostButton", [this] { resetWorkflow(false); });

        header->addLayout(heading, 1);
        header->addWidget(status_);
        header->addWidget(undoBtn_);
        header->addWidget(reset);
        page->addLayout(header);

        // ---- add bar ---------------------------------------------------
        QFrame *addBar = new QFrame;
        addBar->setObjectName("card");
        QVBoxLayout *addCol = new QVBoxLayout(addBar);
        addCol->setContentsMargins(14, 12, 14, 12);
        addCol->setSpacing(8);

        QHBoxLayout *addRow = new QHBoxLayout;
        addRow->setSpacing(8);
        nameEdit_ = new QLineEdit;
        nameEdit_->setPlaceholderText("Name a new item, then press Enter to add a task…");
        nameEdit_->setClearButtonEnabled(true);
        connect(nameEdit_, &QLineEdit::returnPressed, this, [this] { addFromBar(2); });

        addRow->addWidget(nameEdit_, 1);
        addRow->addWidget(button("+ Task", "primaryButton", [this] { addFromBar(2); }));
        addRow->addWidget(button("⇣ Sequential", "softButton", [this] { addFromBar(0); }));
        addRow->addWidget(button("⇉ Parallel", "softButton", [this] { addFromBar(1); }));

        QPushButton *templates = new QPushButton("✦ Templates ▾");
        templates->setObjectName("softButton");
        QMenu *tm = new QMenu(templates);
        QAction *a1 = tm->addAction("Order fulfilment");
        QAction *a2 = tm->addAction("Hiring pipeline");
        QAction *a3 = tm->addAction("Release train");
        connect(a1, &QAction::triggered, this, [this] { loadTemplate(0); });
        connect(a2, &QAction::triggered, this, [this] { loadTemplate(1); });
        connect(a3, &QAction::triggered, this, [this] { loadTemplate(2); });
        templates->setMenu(tm);
        addRow->addWidget(templates);
        addCol->addLayout(addRow);

        target_ = new QLabel;
        target_->setObjectName("mutedText");
        addCol->addWidget(target_);
        page->addWidget(addBar);

        // ---- middle: tree + inspector ----------------------------------
        QSplitter *mid = new QSplitter(Qt::Horizontal);
        mid->setChildrenCollapsible(false);
        mid->setHandleWidth(8);
        mid->setStyleSheet("QSplitter::handle{background:transparent;}");
        mid->addWidget(buildTreeCard());
        mid->addWidget(buildInspector());
        mid->setStretchFactor(0, 3);
        mid->setStretchFactor(1, 2);
        mid->setSizes(QList<int>() << 560 << 320);

        // ---- bottom: terminal -------------------------------------------
        QWidget *terminal = buildTerminal();

        QSplitter *vert = new QSplitter(Qt::Vertical);
        vert->setChildrenCollapsible(false);
        vert->setHandleWidth(8);
        vert->setStyleSheet("QSplitter::handle{background:transparent;}");
        vert->addWidget(mid);
        vert->addWidget(terminal);
        vert->setStretchFactor(0, 4);
        vert->setStretchFactor(1, 2);
        vert->setSizes(QList<int>() << 430 << 190);
        page->addWidget(vert, 1);

        applyStyle();

        // ---- timer for auto-play ---------------------------------------
        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, [this] {
            if (!advanceOnce()) stopPlay(true);
        });

        // ---- shortcuts ---------------------------------------------------
        addShortcut("Ctrl+1", [this] { addFromBar(2); });
        addShortcut("Ctrl+2", [this] { addFromBar(0); });
        addShortcut("Ctrl+3", [this] { addFromBar(1); });
        addShortcut("Ctrl+Z", [this] { undo(); });
        addShortcut("F6", [this] { stopPlay(false); stepOnce(); });
        addShortcut("F5", [this] { togglePlay(); });
    }

    QWidget *buildTreeCard() {
        QFrame *card = new QFrame;
        card->setObjectName("card");
        QVBoxLayout *col = new QVBoxLayout(card);
        col->setContentsMargins(14, 12, 14, 12);
        col->setSpacing(10);

        QHBoxLayout *head = new QHBoxLayout;
        QLabel *t = new QLabel("WORKFLOW");
        t->setObjectName("sectionLabel");
        QLabel *hint = new QLabel("double-click an item to advance it");
        hint->setObjectName("mutedText");
        head->addWidget(t);
        head->addStretch();
        head->addWidget(hint);
        col->addLayout(head);

        // stats chips
        QHBoxLayout *chips = new QHBoxLayout;
        chips->setSpacing(6);
        chipTotal_ = chip("#55617a", "#eef1f7");
        chipPending_ = chip("#69758f", "#eef1f7");
        chipActive_ = chip("#9a5b00", "#fff2dd");
        chipDone_ = chip("#14774f", "#e4f6ec");
        chipStopped_ = chip("#b3283a", "#fde8ea");
        chips->addWidget(chipTotal_);
        chips->addWidget(chipPending_);
        chips->addWidget(chipActive_);
        chips->addWidget(chipDone_);
        chips->addWidget(chipStopped_);
        chips->addStretch();
        col->addLayout(chips);

        progress_ = new QProgressBar;
        progress_->setTextVisible(false);
        progress_->setFixedHeight(7);
        col->addWidget(progress_);

        tree_ = new QTreeWidget;
        tree_->setColumnCount(4);
        tree_->setHeaderLabels(QStringList() << "Name" << "Type" << "State" << "Owner");
        tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        tree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        tree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        tree_->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        tree_->setUniformRowHeights(true);
        tree_->setIndentation(22);
        tree_->setAnimated(true);
        tree_->setSelectionMode(QAbstractItemView::SingleSelection);
        tree_->setContextMenuPolicy(Qt::CustomContextMenu);
        col->addWidget(tree_, 1);

        connect(tree_, &QTreeWidget::itemSelectionChanged, this, [this] { onSelectionChanged(); });
        connect(tree_, &QTreeWidget::itemDoubleClicked, this,
                [this](QTreeWidgetItem *, int) { advanceSelected(); });
        connect(tree_, &QTreeWidget::customContextMenuRequested, this,
                [this](const QPoint &pt) { showContextMenu(pt); });
        return card;
    }

    QWidget *buildInspector() {
        QFrame *card = new QFrame;
        card->setObjectName("card");
        QVBoxLayout *col = new QVBoxLayout(card);
        col->setContentsMargins(16, 14, 16, 14);
        col->setSpacing(8);

        QLabel *t = new QLabel("INSPECTOR");
        t->setObjectName("sectionLabel");
        col->addWidget(t);

        insName_ = new QLabel("Nothing selected");
        insName_->setObjectName("insName");
        insName_->setWordWrap(true);
        col->addWidget(insName_);

        QHBoxLayout *pills = new QHBoxLayout;
        pills->setSpacing(6);
        insType_ = chip("#55617a", "#eef1f7");
        insState_ = chip("#55617a", "#eef1f7");
        pills->addWidget(insType_);
        pills->addWidget(insState_);
        pills->addStretch();
        col->addLayout(pills);

        insInfo_ = new QLabel;
        insInfo_->setObjectName("mutedText");
        insInfo_->setWordWrap(true);
        col->addWidget(insInfo_);

        insHint_ = new QLabel;
        insHint_->setObjectName("hintBox");
        insHint_->setWordWrap(true);
        col->addWidget(insHint_);

        // who performs assignments
        QHBoxLayout *who = new QHBoxLayout;
        QLabel *wl = new QLabel("Assign as");
        wl->setObjectName("mutedText");
        person_ = new QComboBox;
        for (std::size_t i = 0; i < people_.size(); ++i)
            person_->addItem(QString::fromStdString(people_[i]->getName()));
        who->addWidget(wl);
        who->addWidget(person_, 1);
        col->addLayout(who);

        // action buttons
        QGridLayout *g = new QGridLayout;
        g->setSpacing(7);
        bOpen_     = button("▶ Open stage",     "actionButton", [this] { actOpen(); });
        bAvail_    = button("◌ Make available", "actionButton", [this] { actMakeAvailable(); });
        bAssign_   = button("➜ Assign",         "actionButton", [this] { actAssign(); });
        bStart_    = button("▷ Start",          "actionButton", [this] { actStart(); });
        bComplete_ = button("✔ Complete",       "goodButton",   [this] { actComplete(); });
        bReject_   = button("↩ Reject",         "warnButton",   [this] { actReject(); });
        bCancel_   = button("✘ Cancel",         "badButton",    [this] { actCancel(); });
        bDrive_    = button("⏭ Drive to end",   "actionButton", [this] { actDrive(); });
        g->addWidget(bOpen_, 0, 0);
        g->addWidget(bAvail_, 0, 1);
        g->addWidget(bAssign_, 1, 0);
        g->addWidget(bStart_, 1, 1);
        g->addWidget(bComplete_, 2, 0);
        g->addWidget(bReject_, 2, 1);
        g->addWidget(bCancel_, 3, 0);
        g->addWidget(bDrive_, 3, 1);
        col->addLayout(g);

        col->addSpacing(4);
        QLabel *rt = new QLabel("RUN THE WHOLE WORKFLOW");
        rt->setObjectName("sectionLabel");
        col->addWidget(rt);

        QHBoxLayout *run = new QHBoxLayout;
        run->setSpacing(7);
        stepBtn_ = button("Step ▶", "softButton", [this] { stopPlay(false); stepOnce(); });
        stepBtn_->setToolTip("Do exactly one transition  (F6)");
        playBtn_ = button("▶ Auto-play", "primaryButton", [this] { togglePlay(); });
        playBtn_->setToolTip("Run continuously  (F5)");
        speed_ = new QComboBox;
        speed_->addItem("Slow", 1000);
        speed_->addItem("Normal", 500);
        speed_->addItem("Fast", 150);
        speed_->setCurrentIndex(1);
        connect(speed_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            if (timer_->isActive()) timer_->start(speed_->currentData().toInt());
        });
        run->addWidget(stepBtn_);
        run->addWidget(playBtn_, 1);
        run->addWidget(speed_);
        col->addLayout(run);
        col->addStretch();
        return card;
    }

    QWidget *buildTerminal() {
        QFrame *panel = new QFrame;
        panel->setObjectName("terminalPanel");
        QVBoxLayout *v = new QVBoxLayout(panel);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);

        QFrame *head = new QFrame;
        head->setObjectName("terminalHead");
        QHBoxLayout *h = new QHBoxLayout(head);
        h->setContentsMargins(14, 6, 8, 6);
        QLabel *tab = new QLabel("ACTIVITY");
        tab->setObjectName("terminalTitle");
        QLabel *hint = new QLabel("workflow output");
        hint->setObjectName("terminalHint");
        QPushButton *clear = button("⌫ Clear", "terminalClear", [this] {
            log_->clear();
            appendLog(Info, "Activity log cleared.");
        });
        h->addWidget(tab);
        h->addWidget(hint);
        h->addStretch();
        h->addWidget(clear);
        v->addWidget(head);

        log_ = new QTextEdit;
        log_->setObjectName("activityLog");
        log_->setReadOnly(true);
        log_->setFrameShape(QFrame::NoFrame);
        QFont f("Consolas");
        f.setStyleHint(QFont::Monospace);
        f.setPixelSize(12);
        log_->setFont(f);
        v->addWidget(log_, 1);
        return panel;
    }

    // ---- small UI helpers -------------------------------------------------
    QPushButton *button(const QString &text, const char *objectName, std::function<void()> fn) {
        QPushButton *b = new QPushButton(text);
        b->setObjectName(objectName);
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [fn] { fn(); });
        return b;
    }

    QLabel *chip(const QString &fg, const QString &bg) {
        QLabel *l = new QLabel;
        l->setStyleSheet(QString("background:%1;color:%2;border-radius:9px;padding:3px 10px;"
                                 "font-size:11px;font-weight:700;").arg(bg, fg));
        return l;
    }

    void setChip(QLabel *l, const QString &text, const QString &fg, const QString &bg) {
        l->setText(text);
        l->setStyleSheet(QString("background:%1;color:%2;border-radius:9px;padding:3px 10px;"
                                 "font-size:11px;font-weight:700;").arg(bg, fg));
    }

    void addShortcut(const char *keys, std::function<void()> fn) {
        QShortcut *s = new QShortcut(QKeySequence(keys), this);
        connect(s, &QShortcut::activated, this, [fn] { fn(); });
    }

    // ======================================================================
    // Model helpers
    // ======================================================================
    static QString stateOf(WorkItem *w) { return QString::fromStdString(w->getStateName()); }
    static bool isStage(WorkItem *w) { return dynamic_cast<Stage*>(w) != nullptr; }
    static bool isTask(WorkItem *w) { return dynamic_cast<Task*>(w) != nullptr; }
    static bool releasedItem(WorkItem *w) { return w->isAvailable() || w->getStateName() != "Created"; }

    static QString typeOf(WorkItem *w) {
        if (dynamic_cast<ParallelStage*>(w)) return "⇉ Parallel";
        if (dynamic_cast<SequentialStage*>(w)) return "⇣ Sequential";
        if (dynamic_cast<Task*>(w)) return "• Task";
        return "Item";
    }

    static QColor stateColor(const QString &s) {
        if (s == "Completed") return QColor("#14774f");
        if (s == "Started")   return QColor("#1f74c4");
        if (s == "Assigned")  return QColor("#7657c8");
        if (s == "Available") return QColor("#b8730a");
        if (s.contains("Rejected") || s == "Cancelled") return QColor("#c43d4b");
        return QColor("#8995a6");
    }

    static QColor stateBg(const QString &s) {
        if (s == "Completed") return QColor("#e4f6ec");
        if (s == "Started")   return QColor("#e3f0fd");
        if (s == "Assigned")  return QColor("#efe9fb");
        if (s == "Available") return QColor("#fff2dd");
        if (s.contains("Rejected") || s == "Cancelled") return QColor("#fde8ea");
        return QColor("#eef1f7");
    }

    QString displayName(WorkItem *item) const {
        std::map<WorkItem*, QString>::const_iterator it = names_.find(item);
        if (it != names_.end()) return it->second;
        return QString::fromStdString(item->getItemID());
    }

    Participant *currentPerson() {
        const int i = person_ ? person_->currentIndex() : 0;
        return people_[static_cast<std::size_t>(i < 0 ? 0 : i)];
    }

    void collectTasks(WorkItem *item, std::vector<WorkItem*> &out) {
        if (isTask(item)) out.push_back(item);
        const std::vector<WorkItem*> kids = item->getChildren();
        for (std::size_t i = 0; i < kids.size(); ++i) collectTasks(kids[i], out);
    }

    WorkItem *selected() {
        QTreeWidgetItem *n = tree_->currentItem();
        if (!n) return nullptr;
        const QVariant v = n->data(0, Qt::UserRole);
        if (!v.isValid()) return nullptr;
        return reinterpret_cast<WorkItem*>(static_cast<quintptr>(v.toULongLong()));
    }

    // Where a new item goes: inside the selected stage, next to a selected
    // task (inside its parent), otherwise under the root.
    Stage *targetStage() {
        WorkItem *sel = selected();
        if (sel) {
            if (Stage *s = dynamic_cast<Stage*>(sel)) return s;
            std::map<WorkItem*, Stage*>::iterator it = parents_.find(sel);
            if (it != parents_.end()) return it->second;
        }
        return dynamic_cast<Stage*>(root_);
    }

    // Create an item of kind 0 = sequential, 1 = parallel, 2 = task, no UI.
    WorkItem *makeItem(int kind, const QString &name, Stage *parent) {
        ++nextId_;
        const std::string id = std::string(kind == 0 ? "seq-" : kind == 1 ? "par-" : "task-")
                               + std::to_string(nextId_);
        WorkItem *item = nullptr;
        if (kind == 0) item = new SequentialStage(id, name.toStdString());
        else if (kind == 1) item = new ParallelStage(id, name.toStdString());
        else item = new Task(id, name.toStdString(), 0, 0);
        parent->add(item);
        names_[item] = name;
        parents_[item] = parent;
        return item;
    }

    // ======================================================================
    // Actions (all go through guard so errors land in the log, not a crash)
    // ======================================================================
    void guard(const QString &message, const std::function<void()> &action) {
        try {
            action();
            setStatus(message, true);
            appendLog(Ok, message);
        } catch (const std::exception &e) {
            const QString t = QString::fromUtf8(e.what());
            setStatus(t, false);
            appendLog(Err, message + " failed: " + t);
        } catch (...) {
            setStatus("Unknown exception.", false);
            appendLog(Err, message + " failed: unknown exception");
        }
        releaseNext();
        refresh();
    }

    // After any transition, let the stages open whatever just became eligible.
    void releaseNext() {
        try { root_->activateChildren(); } catch (...) {}
    }

    template <class C>
    void runCommand(C *cmd) { invoker_->executeCommand(cmd); }

    void actOpen() {
        WorkItem *w = selected();
        if (!w) return;
        const QString n = displayName(w);
        guard("Opened " + n, [w] {
            if (w->getStateName() == "Created") w->makeAvailable();
            w->activateChildren();
        });
    }

    void actMakeAvailable() {
        WorkItem *w = selected();
        if (!w) return;
        const QString n = displayName(w);
        guard(n + ": made available", [w] { w->makeAvailable(); });
    }

    void actAssign() {
        WorkItem *w = selected();
        if (!w) return;
        Participant *p = currentPerson();
        const QString n = displayName(w), who = QString::fromStdString(p->getName());
        guard(n + ": assigned to " + who, [this, w, p] { runCommand(new AssignCommand(w, p)); });
    }

    void actStart() {
        WorkItem *w = selected();
        if (!w) return;
        const QString n = displayName(w);
        guard(n + ": started", [this, w] { runCommand(new StartCommand(w)); });
    }

    void actComplete() {
        WorkItem *w = selected();
        if (!w) return;
        const QString n = displayName(w);
        guard(n + ": completed", [this, w] { runCommand(new CompleteCommand(w)); });
    }

    void actReject() {
        WorkItem *w = selected();
        if (!w) return;
        bool ok = false;
        const QString reason = QInputDialog::getText(this, "Reject task", "Reason:",
                                                     QLineEdit::Normal, "Needs rework", &ok).trimmed();
        if (!ok) return;
        const QString n = displayName(w);
        guard(n + ": rejected (" + reason + ")", [w, reason] { w->reject(reason.toStdString()); });
    }

    void actCancel() {
        WorkItem *w = selected();
        if (!w) return;
        const QString n = displayName(w);
        guard(n + ": cancelled", [w] { w->cancel(); });
    }

    void actDrive() {
        WorkItem *w = selected();
        if (!w) return;
        const QString n = displayName(w);
        guard(n + ": driven to completion", [this, w] { driveItem(w); });
    }

    // Push a task straight through its lifecycle.
    void driveTask(WorkItem *t) {
        QString s = stateOf(t);
        if (s == "Created" || s == "Rejected") { t->makeAvailable(); s = stateOf(t); }
        if (s == "Available") { runCommand(new AssignCommand(t, currentPerson())); s = stateOf(t); }
        if (s == "Assigned")  { runCommand(new StartCommand(t)); s = stateOf(t); }
        if (s == "Started")   { runCommand(new CompleteCommand(t)); }
    }

    void driveItem(WorkItem *w) {
        if (isTask(w)) { driveTask(w); return; }
        if (stateOf(w) == "Created") w->makeAvailable();
        std::vector<WorkItem*> tasks;
        collectTasks(w, tasks);
        for (int pass = 0; pass < 60; ++pass) {
            root_->activateChildren();
            bool any = false;
            for (std::size_t i = 0; i < tasks.size(); ++i)
                if (!tasks[i]->isFinished() && releasedItem(tasks[i])) { driveTask(tasks[i]); any = true; }
            if (!any) break;
        }
    }

    void undo() {
        if (!invoker_ || !invoker_->canUndo()) {
            appendLog(Info, "Nothing to undo.");
            return;
        }
        guard("Undid the last action", [this] {
            if (!invoker_->undoLast()) throw std::runtime_error("undo was refused");
        });
    }

    // ---- one transition at a time (Step / Auto-play / double-click) ---------
    // Moves one task a single state forward. Returns a description or an empty
    // string if nothing could be done.
    QString transitionOnce(WorkItem *t) {
        const QString before = stateOf(t);
        const QString n = displayName(t);
        if (before == "Created" || before == "Rejected") t->makeAvailable();
        else if (before == "Available") runCommand(new AssignCommand(t, currentPerson()));
        else if (before == "Assigned")  runCommand(new StartCommand(t));
        else if (before == "Started")   runCommand(new CompleteCommand(t));
        else return QString();
        return QString("%1:  %2 → %3").arg(n, before, stateOf(t));
    }

    // Find the first task the workflow has released and advance it one step.
    bool advanceOnce() {
        try {
            if (stateOf(root_) == "Created") {
                root_->makeAvailable();
                root_->activateChildren();
                appendLog(Step, "Workflow opened: first tasks released");
                refresh();
                return true;
            }
            root_->activateChildren();
            std::vector<WorkItem*> tasks;
            collectTasks(root_, tasks);
            for (std::size_t i = 0; i < tasks.size(); ++i) {
                WorkItem *t = tasks[i];
                if (t->isFinished() || !releasedItem(t)) continue;
                const QString what = transitionOnce(t);
                if (what.isEmpty()) continue;
                root_->activateChildren();
                appendLog(Step, what);
                setStatus(what, true);
                refresh();
                return true;
            }
        } catch (const std::exception &e) {
            appendLog(Err, QString("Step failed: ") + QString::fromUtf8(e.what()));
            setStatus(QString::fromUtf8(e.what()), false);
            refresh();
            return false;
        }
        return false;
    }

    void stepOnce() {
        if (!advanceOnce()) reportIdle();
    }

    void reportIdle() {
        std::vector<WorkItem*> tasks;
        collectTasks(root_, tasks);
        if (tasks.empty()) {
            appendLog(Info, "The workflow is empty. Add tasks or load a template.");
            return;
        }
        bool all = true;
        for (std::size_t i = 0; i < tasks.size(); ++i) if (!tasks[i]->isFinished()) all = false;
        if (all) {
            setStatus("Workflow finished", true);
            appendLog(Ok, "Workflow finished: every task is done or cancelled.");
        } else {
            setStatus("Stuck: nothing can move", false);
            appendLog(Err, "Nothing can move. Reset a rejected task with Make available, or cancel it.");
        }
    }

    void togglePlay() {
        if (timer_->isActive()) { stopPlay(false); return; }
        playBtn_->setText("Ⅱ Pause");
        timer_->start(speed_->currentData().toInt());
        appendLog(Info, "Auto-play started.");
    }

    void stopPlay(bool reportIfIdle) {
        const bool was = timer_->isActive();
        timer_->stop();
        playBtn_->setText("▶ Auto-play");
        if (was && reportIfIdle) reportIdle();
        else if (was) appendLog(Info, "Auto-play paused.");
    }

    void advanceSelected() {
        WorkItem *w = selected();
        if (!w) return;
        if (isStage(w)) { actOpen(); return; }
        const QString n = displayName(w);
        QString what;
        guard("Advanced " + n, [this, w, &what] {
            what = transitionOnce(w);
            if (what.isEmpty()) throw std::runtime_error("this item is finished and cannot move");
        });
        if (!what.isEmpty()) appendLog(Step, what);
    }

    // ---- adding / renaming -------------------------------------------------
    void addFromBar(int kind) {
        QString name = nameEdit_->text().trimmed();
        if (name.isEmpty()) {
            name = kind == 0 ? "Sequential stage" : kind == 1 ? "Parallel stage" : "Task";
            name += QString(" %1").arg(nextId_ + 1);
        }
        Stage *parent = targetStage();
        if (!parent) return;
        WorkItem *created = nullptr;
        guard(QString("Added %1 \"%2\" to %3")
                  .arg(kind == 2 ? "task" : kind == 0 ? "sequential stage" : "parallel stage")
                  .arg(name, displayName(parent)),
              [this, kind, name, parent, &created] { created = makeItem(kind, name, parent); });
        nameEdit_->clear();
        nameEdit_->setFocus();
        if (created) select(created);
    }

    void renameSelected() {
        WorkItem *w = selected();
        if (!w) return;
        bool ok = false;
        const QString name = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal,
                                                   displayName(w), &ok).trimmed();
        if (!ok || name.isEmpty()) return;
        names_[w] = name;
        appendLog(Info, "Renamed to " + name);
        refresh();
    }

    void loadTemplate(int which) {
        if (!parents_.empty()) {
            if (QMessageBox::question(this, "Load template",
                                      "This replaces the current workflow. Continue?") != QMessageBox::Yes)
                return;
        }
        resetWorkflow(true);
        Stage *r = dynamic_cast<Stage*>(root_);
        QString label;
        if (which == 0) {
            label = "Order fulfilment";
            names_[root_] = label;
            Stage *prep = dynamic_cast<Stage*>(makeItem(1, "Prepare order", r));
            makeItem(2, "Check stock", prep);
            makeItem(2, "Verify payment", prep);
            Stage *ful = dynamic_cast<Stage*>(makeItem(0, "Fulfil order", r));
            makeItem(2, "Pick items", ful);
            makeItem(2, "Pack parcel", ful);
            makeItem(2, "Ship parcel", ful);
            makeItem(2, "Send receipt", r);
        } else if (which == 1) {
            label = "Hiring pipeline";
            names_[root_] = label;
            makeItem(2, "Screen CVs", r);
            Stage *iv = dynamic_cast<Stage*>(makeItem(1, "Interviews", r));
            makeItem(2, "Technical interview", iv);
            makeItem(2, "Culture interview", iv);
            makeItem(2, "Reference checks", r);
            makeItem(2, "Make offer", r);
        } else {
            label = "Release train";
            names_[root_] = label;
            Stage *qa = dynamic_cast<Stage*>(makeItem(1, "Quality gates", r));
            makeItem(2, "Unit tests", qa);
            makeItem(2, "Integration tests", qa);
            makeItem(2, "Security scan", qa);
            makeItem(2, "Approve release", r);
            Stage *dep = dynamic_cast<Stage*>(makeItem(0, "Deploy", r));
            makeItem(2, "Deploy to staging", dep);
            makeItem(2, "Smoke test", dep);
            makeItem(2, "Deploy to production", dep);
        }
        appendLog(Ok, "Loaded template: " + label);
        setStatus("Loaded " + label, true);
        refresh();
    }

    void resetWorkflow(bool quiet) {
        stopPlay(false);
        delete invoker_;
        invoker_ = nullptr;
        delete root_;
        root_ = nullptr;
        names_.clear();
        parents_.clear();
        nextId_ = 0;

        root_ = new SequentialStage("root", "Workflow");
        names_[root_] = "Workflow";
        invoker_ = new CommandInvoker();

        if (!quiet || log_->document()->isEmpty()) {
            log_->clear();
            appendLog(Info, "Workflow reset. Build something, load a template, or press Step.");
        }
        setStatus("Ready", true);
        refresh();
    }

    // ======================================================================
    // View updates
    // ======================================================================
    void select(WorkItem *w) {
        pendingSelect_ = w;
        refresh();
    }

    void onSelectionChanged() {
        updateInspector();
        updateTarget();
    }

    void refresh() {
        WorkItem *keep = pendingSelect_ ? pendingSelect_ : selected();
        pendingSelect_ = nullptr;

        tree_->blockSignals(true);
        tree_->clear();
        QTreeWidgetItem *hit = nullptr;
        fillTree(nullptr, root_, keep, hit);
        tree_->expandAll();
        if (root_->getChildren().empty()) {
            QTreeWidgetItem *hint = new QTreeWidgetItem(tree_->topLevelItem(0));
            hint->setText(0, "Empty. Add a task above or pick a template.");
            hint->setForeground(0, QBrush(QColor("#9aa6b8")));
            hint->setFlags(Qt::ItemIsEnabled);
            tree_->expandAll();
        }
        if (hit) tree_->setCurrentItem(hit);
        else tree_->setCurrentItem(tree_->topLevelItem(0));
        tree_->blockSignals(false);

        updateStats();
        updateInspector();
        updateTarget();
        undoBtn_->setEnabled(invoker_ && invoker_->canUndo());
    }

    void fillTree(QTreeWidgetItem *parent, WorkItem *item, WorkItem *keep, QTreeWidgetItem *&hit) {
        if (!item) return;
        QTreeWidgetItem *node = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
        const QString state = stateOf(item);

        node->setData(0, Qt::UserRole,
                      QVariant(static_cast<qulonglong>(reinterpret_cast<quintptr>(item))));
        node->setText(0, displayName(item));
        node->setText(1, typeOf(item));
        node->setText(2, "● " + state);
        node->setForeground(2, QBrush(stateColor(state)));
        node->setForeground(1, QBrush(QColor("#7b879b")));

        if (Task *t = dynamic_cast<Task*>(item)) {
            Participant *p = t->getAssignee();
            if (p) node->setText(3, QString::fromStdString(p->getName()));
        }
        if (isStage(item)) {
            QFont f = node->font(0);
            f.setBold(true);
            node->setFont(0, f);
        }
        if (item == keep) hit = node;

        const std::vector<WorkItem*> kids = item->getChildren();
        for (std::size_t i = 0; i < kids.size(); ++i) fillTree(node, kids[i], keep, hit);
    }

    void updateStats() {
        std::vector<WorkItem*> tasks;
        collectTasks(root_, tasks);
        int pending = 0, active = 0, done = 0, stopped = 0;
        for (std::size_t i = 0; i < tasks.size(); ++i) {
            const QString s = stateOf(tasks[i]);
            if (s == "Created") ++pending;
            else if (s == "Available" || s == "Assigned" || s == "Started") ++active;
            else if (s == "Completed") ++done;
            else ++stopped;
        }
        const int total = static_cast<int>(tasks.size());
        setChip(chipTotal_, QString("%1 tasks").arg(total), "#55617a", "#eef1f7");
        setChip(chipPending_, QString("%1 waiting").arg(pending), "#69758f", "#eef1f7");
        setChip(chipActive_, QString("%1 active").arg(active), "#9a5b00", "#fff2dd");
        setChip(chipDone_, QString("%1 done").arg(done), "#14774f", "#e4f6ec");
        setChip(chipStopped_, QString("%1 stopped").arg(stopped), "#b3283a", "#fde8ea");

        progress_->setRange(0, total > 0 ? total : 1);
        progress_->setValue(done);
    }

    void updateTarget() {
        Stage *t = targetStage();
        target_->setText(t ? "New items will be added inside:  " + displayName(t)
                           : "New items will be added to the workflow");
    }

    static QString hintFor(WorkItem *w) {
        const QString s = QString::fromStdString(w->getStateName());
        if (dynamic_cast<Stage*>(w)) {
            if (dynamic_cast<ParallelStage*>(w))
                return "Parallel stage: every child is released at once, and it finishes when the slowest child does.";
            return "Sequential stage: each child waits until the one before it has finished.";
        }
        if (s == "Created")   return "Waiting. A task is released by its stage once the earlier work is done.";
        if (s == "Available") return "Released and waiting for someone. Assign it to a person.";
        if (s == "Assigned")  return "Has an owner. Start it, or reject it to send it back.";
        if (s == "Started")   return "In progress. Complete it, or reject it if it must be redone.";
        if (s == "Completed") return "Done. Finishing this may have released the next task.";
        if (s == "Rejected")  return "Sent back. Make it available again to reroute it.";
        if (s == "Cancelled") return "Cancelled. It counts as finished and will not run.";
        return QString();
    }

    void updateInspector() {
        WorkItem *w = selected();
        const bool has = w != nullptr;

        bOpen_->setVisible(has && isStage(w));
        bAvail_->setVisible(!has || !isStage(w));
        if (!has) {
            insName_->setText("Nothing selected");
            setChip(insType_, "—", "#55617a", "#eef1f7");
            setChip(insState_, "—", "#55617a", "#eef1f7");
            insInfo_->setText("Click an item in the tree to inspect it.");
            insHint_->setText("Tip: press Step to run the workflow one transition at a time.");
            const std::vector<QPushButton*> all = actionButtons();
            for (std::size_t i = 0; i < all.size(); ++i) all[i]->setEnabled(false);
            return;
        }

        const QString s = stateOf(w);
        insName_->setText(displayName(w));
        setChip(insType_, typeOf(w), "#55617a", "#eef1f7");
        const QColor fg = stateColor(s), bg = stateBg(s);
        setChip(insState_, "● " + s, fg.name(), bg.name());

        QString info = "id: " + QString::fromStdString(w->getItemID());
        const int kids = static_cast<int>(w->getChildren().size());
        if (isStage(w)) info += QString("   ·   %1 item%2").arg(kids).arg(kids == 1 ? "" : "s");
        if (Task *t = dynamic_cast<Task*>(w)) {
            Participant *p = t->getAssignee();
            info += "   ·   owner: " + (p ? QString::fromStdString(p->getName()) : QString("nobody"));
        }
        const QString reason = QString::fromStdString(w->getData("rejectionReason"));
        if (!reason.isEmpty() && s == "Rejected") info += "\nrejected: " + reason;
        insInfo_->setText(info);
        insHint_->setText(hintFor(w));

        const bool task = isTask(w), fin = w->isFinished();
        bOpen_->setEnabled(isStage(w) && !fin);
        bAvail_->setEnabled(task && (s == "Created" || s == "Rejected"));
        bAssign_->setEnabled(task && s == "Available");
        bStart_->setEnabled(task && s == "Assigned");
        bComplete_->setEnabled(task && s == "Started");
        bReject_->setEnabled(task && (s == "Assigned" || s == "Started"));
        bCancel_->setEnabled(!fin);
        bDrive_->setEnabled(!fin);
    }

    std::vector<QPushButton*> actionButtons() {
        std::vector<QPushButton*> v;
        v.push_back(bOpen_);   v.push_back(bAvail_);  v.push_back(bAssign_);
        v.push_back(bStart_);  v.push_back(bComplete_); v.push_back(bReject_);
        v.push_back(bCancel_); v.push_back(bDrive_);
        return v;
    }

    void showContextMenu(const QPoint &pt) {
        QTreeWidgetItem *n = tree_->itemAt(pt);
        if (!n || !n->data(0, Qt::UserRole).isValid()) return;
        tree_->setCurrentItem(n);
        WorkItem *w = selected();
        if (!w) return;

        QMenu menu(this);
        QAction *adv = menu.addAction(isStage(w) ? "Open stage" : "Advance one step");
        QAction *drv = menu.addAction("Drive to completion");
        menu.addSeparator();
        QAction *ren = menu.addAction("Rename…");
        QAction *rej = menu.addAction("Reject…");
        QAction *can = menu.addAction("Cancel");

        const QString s = stateOf(w);
        const bool task = isTask(w), fin = w->isFinished();
        adv->setEnabled(!fin);
        drv->setEnabled(!fin);
        rej->setEnabled(task && (s == "Assigned" || s == "Started"));
        can->setEnabled(!fin);

        QAction *picked = menu.exec(tree_->viewport()->mapToGlobal(pt));
        if (picked == adv) advanceSelected();
        else if (picked == drv) actDrive();
        else if (picked == ren) renameSelected();
        else if (picked == rej) actReject();
        else if (picked == can) actCancel();
    }

    // ---- status badge + terminal log ---------------------------------------
    void setStatus(const QString &message, bool success) {
        QString m = message;
        if (m.size() > 70) m = m.left(67) + "…";
        status_->setText(success ? "●  " + m : "●  " + m);
        status_->setStyleSheet(success
            ? "background:#e4f6ec;color:#14774f;border-radius:9px;padding:7px 13px;font-size:11px;font-weight:700;"
            : "background:#fde8ea;color:#b3283a;border-radius:9px;padding:7px 13px;font-size:11px;font-weight:700;");
    }

    void appendLog(LogKind kind, const QString &message) {
        const char *glyph = "›", *color = "#9cdcfe";
        if (kind == Ok)   { glyph = "✔"; color = "#23d18b"; }
        if (kind == Err)  { glyph = "✘"; color = "#f14c4c"; }
        if (kind == Step) { glyph = "▶"; color = "#e5c07b"; }
        log_->append(QString("<span style='color:#6a6a6a'>%1</span>&nbsp;&nbsp;"
                             "<span style='color:%2'>%3</span>&nbsp;"
                             "<span style='color:#d4d4d4'>%4</span>")
                         .arg(QTime::currentTime().toString("HH:mm:ss"), color, glyph,
                              message.toHtmlEscaped()));
        log_->verticalScrollBar()->setValue(log_->verticalScrollBar()->maximum());
    }

    void applyStyle() {
        setStyleSheet(
            "QWidget#playground{background:#f3f5fa;color:#1b2333;}"
            "#playground QLabel{background:transparent;}"
            "#playground QLabel#pageTitle{font-size:24px;font-weight:800;color:#12294d;}"
            "#playground QLabel#mutedText{color:#7a869b;font-size:11px;}"
            "#playground QLabel#sectionLabel{color:#66758b;font-size:10px;font-weight:800;letter-spacing:1px;}"
            "#playground QLabel#insName{font-size:18px;font-weight:800;color:#12294d;}"
            "#playground QLabel#hintBox{background:#f3f7fd;border:1px solid #dbe7f6;border-radius:9px;"
            "color:#3d5372;padding:9px 11px;font-size:12px;}"
            "#playground QFrame#card{background:white;border:1px solid #e0e7f1;border-radius:14px;}"
            "#playground QLineEdit{background:#f7f9fc;border:1px solid #d8e0ec;border-radius:9px;"
            "padding:8px 12px;font-size:13px;}"
            "#playground QLineEdit:focus{border:1px solid #0a84ff;background:white;}"
            "#playground QComboBox{background:#f7f9fc;border:1px solid #d8e0ec;border-radius:8px;padding:6px 10px;}"
            "#playground QPushButton{border:0;border-radius:9px;padding:8px 14px;font-weight:700;min-height:20px;}"
            "#playground QPushButton:disabled{background:#f0f3f8;color:#b6c0d0;}"
            "#playground QPushButton#primaryButton{background:#0a84ff;color:white;}"
            "#playground QPushButton#primaryButton:hover{background:#0a6fd8;}"
            "#playground QPushButton#softButton{background:#e8f2ff;color:#0a6fd1;}"
            "#playground QPushButton#softButton:hover{background:#d6e9ff;}"
            "#playground QPushButton#softButton::menu-indicator{image:none;}"
            "#playground QPushButton#ghostButton{background:white;color:#34445a;border:1px solid #d8e0ec;}"
            "#playground QPushButton#ghostButton:hover{border-color:#0a84ff;color:#0a84ff;}"
            "#playground QPushButton#actionButton{background:#eef3fa;color:#26384f;}"
            "#playground QPushButton#actionButton:hover:enabled{background:#dde9f8;}"
            "#playground QPushButton#goodButton{background:#e4f6ec;color:#14774f;}"
            "#playground QPushButton#goodButton:hover:enabled{background:#cdeedc;}"
            "#playground QPushButton#warnButton{background:#fff2dd;color:#9a5b00;}"
            "#playground QPushButton#warnButton:hover:enabled{background:#ffe6bd;}"
            "#playground QPushButton#badButton{background:#fde8ea;color:#b3283a;}"
            "#playground QPushButton#badButton:hover:enabled{background:#facdd2;}"
            "#playground QTreeWidget{background:white;border:1px solid #edf1f6;border-radius:10px;"
            "outline:0;font-size:13px;}"
            "#playground QTreeWidget::item{padding:5px 2px;}"
            "#playground QTreeWidget::item:hover{background:#f3f8ff;}"
            "#playground QTreeWidget::item:selected{background:#e3f0fd;color:#0b4a8a;}"
            "#playground QHeaderView::section{background:#f7f9fc;color:#7a869b;border:0;"
            "padding:7px;font-size:10px;font-weight:800;}"
            "#playground QProgressBar{background:#e3e9f4;border:0;border-radius:3px;}"
            "#playground QProgressBar::chunk{border-radius:3px;"
            "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0a84ff,stop:1 #35c9a0);}"
            "#playground QFrame#terminalPanel{background:#1e1e1e;border-radius:12px;}"
            "#playground QFrame#terminalHead{background:#252526;border-top-left-radius:12px;"
            "border-top-right-radius:12px;}"
            "#playground QLabel#terminalTitle{color:#e7e7e7;font-size:10px;font-weight:800;letter-spacing:1px;}"
            "#playground QLabel#terminalHint{color:#8b8b8b;font-size:10px;margin-left:8px;}"
            "#playground QPushButton#terminalClear{background:transparent;color:#cccccc;padding:3px 9px;"
            "min-height:16px;}"
            "#playground QPushButton#terminalClear:hover{background:#3c3c3c;color:white;}"
            "#playground QTextEdit#activityLog{background:#1e1e1e;color:#d4d4d4;border:0;padding:8px 12px;"
            "selection-background-color:#264f78;border-bottom-left-radius:12px;"
            "border-bottom-right-radius:12px;}"
        );
    }

    // ======================================================================
    // State
    // ======================================================================
    std::vector<Participant*> people_;
    Stage *root_;
    CommandInvoker *invoker_;
    std::map<WorkItem*, QString> names_;
    std::map<WorkItem*, Stage*> parents_;
    WorkItem *pendingSelect_ = nullptr;
    int nextId_;

    QTimer *timer_ = nullptr;
    QLabel *status_ = nullptr, *target_ = nullptr;
    QLineEdit *nameEdit_ = nullptr;
    QTreeWidget *tree_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QLabel *chipTotal_ = nullptr, *chipPending_ = nullptr, *chipActive_ = nullptr,
           *chipDone_ = nullptr, *chipStopped_ = nullptr;
    QLabel *insName_ = nullptr, *insType_ = nullptr, *insState_ = nullptr,
           *insInfo_ = nullptr, *insHint_ = nullptr;
    QComboBox *person_ = nullptr, *speed_ = nullptr;
    QPushButton *undoBtn_ = nullptr, *stepBtn_ = nullptr, *playBtn_ = nullptr;
    QPushButton *bOpen_ = nullptr, *bAvail_ = nullptr, *bAssign_ = nullptr, *bStart_ = nullptr,
                *bComplete_ = nullptr, *bReject_ = nullptr, *bCancel_ = nullptr, *bDrive_ = nullptr;
    QTextEdit *log_ = nullptr;
};