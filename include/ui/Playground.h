

#pragma once



#include <QWidget>

#include <QLabel>

#include <QPushButton>

#include <QVBoxLayout>

#include <QHBoxLayout>

#include <QGridLayout>

#include <QFrame>

#include <QTreeWidget>

#include <QTreeWidgetItem>

#include <QHeaderView>

#include <QPlainTextEdit>

#include <QInputDialog>

#include <QMessageBox>

#include <QComboBox>

#include <QShortcut>

#include <QKeySequence>

#include <QTime>

#include <QVariant>

#include <QColor>

#include <QBrush>

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



class Playground : public QWidget {

public:

    Playground()

        : QWidget(nullptr),

          who_("you", "You", 0),

          root_(nullptr),

          nextId_(0)

    {

        build();

        resetWorkflow();

    }



    ~Playground() {

        delete root_;

    }



private:

    void build() {

        setObjectName("playground");

        setMinimumSize(900, 680);



        QVBoxLayout *page = new QVBoxLayout(this);

        page->setContentsMargins(22, 20, 22, 18);

        page->setSpacing(14);



    // Header

        QHBoxLayout *header = new QHBoxLayout;



        QVBoxLayout *heading = new QVBoxLayout;



        QLabel *title = new QLabel("TIVIDY");

        title->setObjectName("pageTitle");



        QLabel *subtitle = new QLabel("Workflow Manager");

        subtitle->setObjectName("mutedText");



        heading->addWidget(title);

        heading->addWidget(subtitle);



        status_ = new QLabel("Ready");

        status_->setObjectName("statusBadge");



        QPushButton *reset = new QPushButton("Reset");

        reset->setObjectName("secondaryButton");



        header->addLayout(heading, 1);

        header->addWidget(status_);

        header->addWidget(reset);



        page->addLayout(header);



    // Main menu

        QFrame *menuPanel = new QFrame;

        menuPanel->setObjectName("surface");



        QVBoxLayout *menuLayout = new QVBoxLayout(menuPanel);

        menuLayout->setContentsMargins(18, 16, 18, 18);

        menuLayout->setSpacing(10);



        QLabel *menuTitle = new QLabel("MAIN MENU");

        menuTitle->setObjectName("sectionLabel");



        QLabel *menuHint = new QLabel(

            "Choose an action by clicking an option or pressing its number."

        );

        menuHint->setObjectName("mutedText");



        menuLayout->addWidget(menuTitle);

        menuLayout->addWidget(menuHint);



        QGridLayout *menuGrid = new QGridLayout;

        menuGrid->setSpacing(9);



        addMenuOption(menuGrid, 1, "Create a new task",

                      "Add a task to your workflow.", 0, 0);



        addMenuOption(menuGrid, 2, "Create a sequential stage",

                      "Execute tasks one after another.", 0, 1);



        addMenuOption(menuGrid, 3, "Create a parallel stage",

                      "Run tasks independently.", 1, 0);



        addMenuOption(menuGrid, 4, "View workflow",

                      "Inspect stages, tasks and states.", 1, 1);



        addMenuOption(menuGrid, 5, "Manage a task",

                      "Assign, start, complete, reject or cancel.", 2, 0);



        addMenuOption(menuGrid, 6, "Start workflow",

                      "Begin executing the workflow.", 2, 1);



        addMenuOption(menuGrid, 7, "View activity log",

                      "Review workflow events and errors.", 3, 0);



        addMenuOption(menuGrid, 0, "Reset workflow",

                      "Clear the current workflow.", 3, 1);



        menuLayout->addLayout(menuGrid);

        page->addWidget(menuPanel);



    // Workflow overview

        QFrame *overview = new QFrame;

        overview->setObjectName("surface");



        QVBoxLayout *overviewLayout = new QVBoxLayout(overview);

        overviewLayout->setContentsMargins(14, 12, 14, 14);



        QHBoxLayout *overviewHeader = new QHBoxLayout;



        QLabel *overviewTitle = new QLabel("WORKFLOW OVERVIEW");

        overviewTitle->setObjectName("sectionLabel");



        QLabel *overviewHint = new QLabel("Live structure");

        overviewHint->setObjectName("mutedText");



        overviewHeader->addWidget(overviewTitle);

        overviewHeader->addStretch();

        overviewHeader->addWidget(overviewHint);



        overviewLayout->addLayout(overviewHeader);



        tree_ = new QTreeWidget;

        tree_->setColumnCount(3);

        tree_->setHeaderLabels(

            QStringList() << "Name" << "Type" << "State"

        );



        tree_->header()->setSectionResizeMode(

            0, QHeaderView::Stretch

        );

        tree_->header()->setSectionResizeMode(

            1, QHeaderView::ResizeToContents

        );

        tree_->header()->setSectionResizeMode(

            2, QHeaderView::ResizeToContents

        );



        tree_->setAlternatingRowColors(true);

        tree_->setUniformRowHeights(true);

        tree_->setIndentation(20);



        overviewLayout->addWidget(tree_);

        page->addWidget(overview, 1);



    // Activity panel

        QFrame *activity = new QFrame;

        activity->setObjectName("terminalPanel");



        QVBoxLayout *activityLayout = new QVBoxLayout(activity);

        activityLayout->setContentsMargins(13, 10, 13, 12);



        QHBoxLayout *activityHeader = new QHBoxLayout;



        QLabel *activityTitle = new QLabel("ACTIVITY");

        activityTitle->setObjectName("terminalTitle");



        QLabel *activityHint = new QLabel("Workflow output");

        activityHint->setObjectName("terminalHint");



        QPushButton *clear = new QPushButton("Clear");

        clear->setObjectName("terminalClear");



        activityHeader->addWidget(activityTitle);

        activityHeader->addWidget(activityHint);

        activityHeader->addStretch();

        activityHeader->addWidget(clear);



        activityLayout->addLayout(activityHeader);



        log_ = new QPlainTextEdit;

        log_->setReadOnly(true);

        log_->setMaximumBlockCount(300);

        log_->setMinimumHeight(95);

        log_->setObjectName("activityLog");



        activityLayout->addWidget(log_);

        page->addWidget(activity);



        applyStyle();



        connect(reset, &QPushButton::clicked,

                this, [this] { resetWorkflow(); });



        connect(clear, &QPushButton::clicked,

                this, [this] {

            log_->clear();

            appendLog("Activity log cleared.");

        });



    // Keyboard menu shortcuts

        for (int i = 0; i <= 7; ++i) {

            QShortcut *shortcut = new QShortcut(

                QKeySequence(QString::number(i)), this

            );



            connect(shortcut, &QShortcut::activated,

                    this, [this, i] { executeMenu(i); });

        }

    }



    void addMenuOption(

        QGridLayout *grid,

        int number,

        const QString &title,

        const QString &description,

        int row,

        int column)

    {

        QPushButton *button = new QPushButton;

        button->setObjectName(

            number == 6 ? "primaryButton" : "menuButton"

        );



        button->setMinimumHeight(66);

        button->setText(

            QString("%1.  %2\n%3")

                .arg(number)

                .arg(title)

                .arg(description)

        );



        grid->addWidget(button, row, column);



        connect(button, &QPushButton::clicked,

                this, [this, number] {

            executeMenu(number);

        });

    }



    void executeMenu(int option) {

        switch (option) {

        case 1:

            createItem(2);

            break;

        case 2:

            createItem(0);

            break;

        case 3:

            createItem(1);

            break;

        case 4:

            viewWorkflow();

            break;

        case 5:

            manageTask();

            break;

        case 6:

            startWorkflow();

            break;

        case 7:

            viewActivity();

            break;

        case 0:

            resetWorkflow();

            break;

        default:

            break;

        }

    }



    void createItem(int kind) {

        bool ok = false;



        QString name = QInputDialog::getText(

            this,

            "Create workflow item",

            "Enter a name:",

            QLineEdit::Normal,

            QString(),

            &ok

        ).trimmed();



        if (!ok)

            return;



        if (name.isEmpty()) {

            name = kind == 0 ? "Sequential stage"

                 : kind == 1 ? "Parallel stage"

                 : "Task";



            name += QString(" %1").arg(nextId_ + 1);

        }



    // Select a parent stage

        std::vector<Stage*> stages;

        collectStages(root_, stages);



        QStringList stageNames;

        for (Stage *stage : stages) {

            stageNames << displayName(stage);

        }



        QString parentName = displayName(root_);



        if (stages.size() > 1) {

            parentName = QInputDialog::getItem(

                this,

                "Choose parent stage",

                "Add this item to:",

                stageNames,

                0,

                false,

                &ok

            );



            if (!ok)

                return;

        }



        Stage *parent = root_;



        for (Stage *stage : stages) {

            if (displayName(stage) == parentName) {

                parent = stage;

                break;

            }

        }



        ++nextId_;



        std::string id =

            std::string(

                kind == 0 ? "seq-"

                : kind == 1 ? "par-"

                : "task-"

            ) + std::to_string(nextId_);



        WorkItem *item = nullptr;



        if (kind == 0) {

            item = new SequentialStage(

                id, name.toStdString()

            );

        }

        else if (kind == 1) {

            item = new ParallelStage(

                id, name.toStdString()

            );

        }

        else {

            item = new Task(

                id, name.toStdString(), 0, 0

            );

        }



        names_[item] = name;



        guard("Created " + name, [parent, item] {

            parent->add(item);

        });

    }



    void collectStages(

        WorkItem *item,

        std::vector<Stage*> &result)

    {

        Stage *stage = dynamic_cast<Stage*>(item);



        if (stage)

            result.push_back(stage);



        for (WorkItem *child : item->getChildren())

            collectStages(child, result);

    }



    QString displayName(WorkItem *item) const {

        if (names_.count(item))

            return names_.at(item);



        return QString::fromStdString(item->getItemID());

    }



    void manageTask() {

        std::vector<Task*> tasks;

        collectTasks(root_, tasks);



        if (tasks.empty()) {

            QMessageBox::information(

                this,

                "Manage task",

                "There are no tasks in this workflow yet."

            );

            return;

        }



        QStringList taskNames;



        for (Task *task : tasks) {

            taskNames << displayName(task);

        }



        bool ok = false;



        QString chosen = QInputDialog::getItem(

            this,

            "Manage task",

            "Select a task:",

            taskNames,

            0,

            false,

            &ok

        );



        if (!ok)

            return;



        Task *task = nullptr;



        for (Task *candidate : tasks) {

            if (displayName(candidate) == chosen) {

                task = candidate;

                break;

            }

        }



        if (!task)

            return;



        QStringList actions;

        actions << "Make available"

                << "Assign"

                << "Start"

                << "Complete"

                << "Reject"

                << "Cancel"

                << "Drive to completion";



        QString action = QInputDialog::getItem(

            this,

            chosen,

            "Choose an action:",

            actions,

            0,

            false,

            &ok

        );



        if (!ok)

            return;



        guard(chosen + ": " + action, [this, task, action] {

            if (action == "Make available")

                task->makeAvailable();

            else if (action == "Assign")

                task->assign(&who_);

            else if (action == "Start")

                task->start();

            else if (action == "Complete")

                task->complete();

            else if (action == "Reject")

                task->reject("Rejected from the GUI");

            else if (action == "Cancel")

                task->cancel();

            else if (action == "Drive to completion")

                driveTask(task);

            // Release newly eligible children after a task transition.
            root_->activateChildren();
        });

    }



    void collectTasks(

        WorkItem *item,

        std::vector<Task*> &result)

    {

        Task *task = dynamic_cast<Task*>(item);



        if (task)

            result.push_back(task);



        for (WorkItem *child : item->getChildren())

            collectTasks(child, result);

    }



    void driveTask(Task *task) {

        QString state =

            QString::fromStdString(task->getStateName());



        if (state == "Created") {

            task->makeAvailable();

            state = QString::fromStdString(task->getStateName());

        }



        if (state == "Available") {

            task->assign(&who_);

            state = QString::fromStdString(task->getStateName());

        }



        if (state == "Assigned") {

            task->start();

            state = QString::fromStdString(task->getStateName());

        }



        if (state == "Started")

            task->complete();

    }



    void startWorkflow() {

        guard("Workflow started", [this] {

            root_->makeAvailable();

            root_->activateChildren();

        });

    }



    void viewWorkflow() {

        refresh();



        QMessageBox::information(

            this,

            "View workflow",

            "The complete workflow structure and current states "

            "are shown in the overview panel."

        );

    }



    void viewActivity() {

        QMessageBox box(this);

        box.setWindowTitle("Activity log");

        box.setText("Recent workflow activity:");



        QPlainTextEdit *text = new QPlainTextEdit;

        text->setReadOnly(true);

        text->setPlainText(log_->toPlainText());

        text->setMinimumSize(600, 350);



        box.layout()->addWidget(text);

        box.exec();

    }



    void guard(

        const QString &message,

        const std::function<void()> &action)

    {

        try {

            action();



            setStatus(message, true);

            appendLog("[OK] " + message);

        }

        catch (const std::exception &error) {

            QString errorText =

                QString::fromUtf8(error.what());



            setStatus(errorText, false);

            appendLog("[ERROR] " + errorText);

        }

        catch (...) {

            setStatus("Unknown exception.", false);

            appendLog("[ERROR] Unknown exception.");

        }



        refresh();

    }



    void resetWorkflow() {

        delete root_;



        root_ = nullptr;

        names_.clear();

        nextId_ = 0;



        root_ = new SequentialStage(

            "root", "Workflow"

        );



        names_[root_] = "Workflow";



        refresh();



        log_->clear();



        setStatus("Ready", true);

        appendLog("Workflow reset. Ready for input.");

    }



    void setStatus(

        const QString &message,

        bool success)

    {

        status_->setText(

            success ? "Ready  |  " + message

                    : "Error  |  " + message

        );



        status_->setStyleSheet(

            success

            ? "background:#e8f5ef;color:#18764f;"

            : "background:#fff0f0;color:#b83243;"

        );

    }



    void appendLog(const QString &message) {

        log_->appendPlainText(

            "[" + QTime::currentTime().toString("HH:mm:ss")

            + "]  " + message

        );

    }



    void refresh() {

        tree_->clear();

        fillTree(nullptr, root_);

        tree_->expandAll();

    }



    void fillTree(

        QTreeWidgetItem *parent,

        WorkItem *item)

    {

        if (!item)

            return;



        QTreeWidgetItem *node =

            parent

            ? new QTreeWidgetItem(parent)

            : new QTreeWidgetItem(tree_);



        QString state =

            QString::fromStdString(item->getStateName());



        QString type =

            dynamic_cast<ParallelStage*>(item)

            ? "Parallel"

            : dynamic_cast<SequentialStage*>(item)

            ? "Sequential"

            : dynamic_cast<Task*>(item)

            ? "Task"

            : "Item";



        node->setText(0, displayName(item));

        node->setText(1, type);

        node->setText(2, state);



        QColor color("#8995a6");



        if (state == "Completed")

            color = QColor("#16865b");

        else if (state == "Started")

            color = QColor("#287dcc");

        else if (state == "Assigned")

            color = QColor("#7657c8");

        else if (state == "Available")

            color = QColor("#bd7917");

        else if (state.contains("Rejected") ||

                 state == "Cancelled")

            color = QColor("#c43d4b");



        node->setForeground(2, QBrush(color));



        for (WorkItem *child : item->getChildren())

            fillTree(node, child);

    }



    void applyStyle() {

        setStyleSheet(

            "QWidget#playground{background:#f3f6fb;color:#1f2937;}"

            "QLabel#pageTitle{font-size:25px;font-weight:800;color:#172b4d;}"

            "QLabel#mutedText{color:#78869a;font-size:11px;}"

            "QLabel#sectionLabel{color:#66758b;font-size:10px;font-weight:800;}"

            "QLabel#statusBadge{background:#e8eef6;color:#53657c;"

            "border-radius:7px;padding:8px 11px;font-size:11px;}"

            "QFrame#surface{background:white;border:1px solid #e1e8f0;"

            "border-radius:12px;}"

            "QPushButton{border:0;border-radius:8px;padding:10px;"

            "font-weight:650;min-height:22px;}"

            "QPushButton#menuButton{background:#f7f9fc;color:#243247;"

            "border:1px solid #e1e8f0;text-align:left;}"

            "QPushButton#menuButton:hover{background:#eaf3fc;"

            "border:1px solid #b8d7f2;}"

            "QPushButton#primaryButton{background:#287dcc;color:white;}"

            "QPushButton#primaryButton:hover{background:#176bb8;}"

            "QPushButton#secondaryButton{background:#edf2f8;color:#34445a;}"

            "QTreeWidget{background:white;alternate-background-color:#f8fafd;"

            "border:1px solid #edf1f6;border-radius:8px;padding:5px;}"

            "QTreeWidget::item{padding:6px;border-bottom:1px solid #f0f3f7;}"

            "QTreeWidget::item:selected{background:#e8f2fc;color:#174f83;}"

            "QHeaderView::section{background:#f7f9fc;color:#718096;"

            "border:0;padding:8px;font-size:10px;font-weight:800;}"

            "QFrame#terminalPanel{background:#172333;"

            "border:1px solid #26384d;border-radius:11px;}"

            "QLabel#terminalTitle{color:#dce8f5;font-size:10px;font-weight:800;}"

            "QLabel#terminalHint{color:#8fa2b8;font-size:10px;}"

            "QPushButton#terminalClear{color:#b9c8d8;background:#26384d;}"

            "QPlainTextEdit#activityLog{background:#111c2a;color:#c7d6e5;"

            "border:1px solid #26384d;border-radius:7px;padding:8px;"

            "font-family:monospace;font-size:11px;}"

        );

    }



    Participant who_;

    Stage *root_;

    std::map<WorkItem*, QString> names_;

    int nextId_;



    QTreeWidget *tree_ = nullptr;

    QLabel *status_ = nullptr;

    QPlainTextEdit *log_ = nullptr;

};
