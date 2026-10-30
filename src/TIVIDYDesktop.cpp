/*#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QBrush>
#include <QColor>
#include <QTreeWidgetItem>
#include <QListWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QMessageBox>
#include <QDateTime>
#include <QHeaderView>
#include <QFont>
#include <QMap>
#include <QStringList>
#include <exception>
#include <string>
#include <vector>

#include "WorkItem.h"
#include "Task.h"
#include "SequentialStage.h"
#include "ParallelStage.h"
#include "Participant.h"

class TIVIDYDesktopWindow : public QMainWindow {
public:
    TIVIDYDesktopWindow() : root(0), participant("demo-nurse", "Demo Nurse") {
        setWindowTitle("TIVIDY | Hospital Workflow Manager");
        resize(1280, 800);
        setMinimumSize(980, 640);
        setStyleSheet(
            "QMainWindow,QWidget{background:#f3f6fb;color:#243147;font-family:'Segoe UI';font-size:13px;}"
            "QFrame#panel{background:white;border:1px solid #dbe3ef;border-radius:10px;}"
            "QPushButton{background:white;border:1px solid #cbd5e3;border-radius:7px;padding:8px 12px;}"
            "QPushButton:hover{border-color:#1685e8;color:#0876d1;}"
            "QPushButton:disabled{color:#9aa5b5;background:#f2f4f8;}"
            "QTreeWidget,QListWidget{background:white;border:1px solid #e0e6ef;border-radius:7px;padding:5px;}"
            "QHeaderView::section{background:#f7f9fc;border:0;border-bottom:1px solid #e0e6ef;padding:7px;font-weight:700;}"
        );

        QWidget *central = new QWidget;
        setCentralWidget(central);
        QVBoxLayout *page = new QVBoxLayout(central);
        page->setContentsMargins(18, 15, 18, 16);
        page->setSpacing(12);

        QHBoxLayout *header = new QHBoxLayout;
        QLabel *brand = new QLabel("TIVIDY");
        brand->setStyleSheet("font-size:25px;font-weight:800;letter-spacing:3px;color:#1674c4;");
        QLabel *sub = new QLabel("HOSPITAL WORKFLOW MANAGEMENT");
        sub->setStyleSheet("font-size:11px;font-weight:700;letter-spacing:1px;color:#718097;");
        header->addWidget(brand);
        header->addSpacing(8);
        header->addWidget(sub);
        header->addStretch();
        QLabel *tag = new QLabel("C++ WORKFLOW ENGINE");
        tag->setStyleSheet("background:#e3f1ff;color:#176fc1;padding:7px 10px;border-radius:7px;font-size:10px;font-weight:800;");
        header->addWidget(tag);
        page->addLayout(header);

        QGridLayout *metrics = new QGridLayout;
        metrics->setSpacing(10);
        activeMetric = makeMetric(metrics, 0, "0", "AVAILABLE / ACTIVE");
        doneMetric = makeMetric(metrics, 1, "0", "COMPLETED");
        waitingMetric = makeMetric(metrics, 2, "0", "NOT STARTED");
        failedMetric = makeMetric(metrics, 3, "0", "CANCELLED / REJECTED");
        page->addLayout(metrics);

        QSplitter *split = new QSplitter(Qt::Horizontal);
        QFrame *workflowPanel = new QFrame;
        workflowPanel->setObjectName("panel");
        QVBoxLayout *left = new QVBoxLayout(workflowPanel);
        left->setContentsMargins(14, 13, 14, 14);
        QLabel *workflowTitle = new QLabel("Workflow structure");
        workflowTitle->setStyleSheet("font-size:16px;font-weight:750;");
        QLabel *workflowHint = new QLabel("Select a task to inspect it, then use the lifecycle actions below.");
        workflowHint->setStyleSheet("color:#718097;");
        left->addWidget(workflowTitle);
        left->addWidget(workflowHint);
        tree = new QTreeWidget;
        tree->setColumnCount(3);
        tree->setHeaderLabels(QStringList() << "Work item" << "ID" << "Status");
        tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        tree->setAlternatingRowColors(true);
        left->addWidget(tree, 1);
        split->addWidget(workflowPanel);

        QFrame *rightPanel = new QFrame;
        rightPanel->setObjectName("panel");
        rightPanel->setMinimumWidth(310);
        rightPanel->setMaximumWidth(410);
        QVBoxLayout *right = new QVBoxLayout(rightPanel);
        right->setContentsMargins(14, 13, 14, 14);
        right->setSpacing(9);
        QLabel *detailsTitle = new QLabel("Work item details");
        detailsTitle->setStyleSheet("font-size:16px;font-weight:750;");
        right->addWidget(detailsTitle);
        details = new QLabel("Select an item in the workflow tree.");
        details->setWordWrap(true);
        details->setMinimumHeight(85);
        details->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        details->setStyleSheet("background:#f7f9fc;border:1px solid #e4eaf2;border-radius:7px;padding:10px;color:#5f6e84;");
        right->addWidget(details);
        QLabel *actionsTitle = new QLabel("Task actions");
        actionsTitle->setStyleSheet("font-size:14px;font-weight:750;margin-top:5px;");
        right->addWidget(actionsTitle);
        QGridLayout *actions = new QGridLayout;
        actions->setSpacing(7);
        QPushButton *available = new QPushButton("Make available");
        QPushButton *assign = new QPushButton("Assign to demo nurse");
        QPushButton *start = new QPushButton("Start task");
        QPushButton *complete = new QPushButton("Complete task");
        QPushButton *reject = new QPushButton("Reject task");
        QPushButton *cancel = new QPushButton("Cancel item");
        actions->addWidget(available, 0, 0);
        actions->addWidget(assign, 0, 1);
        actions->addWidget(start, 1, 0);
        actions->addWidget(complete, 1, 1);
        actions->addWidget(reject, 2, 0);
        actions->addWidget(cancel, 2, 1);
        right->addLayout(actions);
        QLabel *logTitle = new QLabel("Activity log");
        logTitle->setStyleSheet("font-size:14px;font-weight:750;margin-top:6px;");
        right->addWidget(logTitle);
        log = new QListWidget;
        right->addWidget(log, 1);
        split->addWidget(rightPanel);
        split->setStretchFactor(0, 3);
        split->setStretchFactor(1, 2);
        split->setSizes(QList<int>() << 740 << 390);
        page->addWidget(split, 1);

        QPushButton *refresh = new QPushButton("Refresh workflow");
        QPushButton *reset = new QPushButton("Reset demo workflow");
        QHBoxLayout *footer = new QHBoxLayout;
        QLabel *footnote = new QLabel("Powered by TIVIDY's WorkItem / Composite workflow core");
        footnote->setStyleSheet("color:#718097;font-size:11px;");
        footer->addWidget(footnote);
        footer->addStretch();
        footer->addWidget(refresh);
        footer->addWidget(reset);
        page->addLayout(footer);

        connect(tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
            showDetails(current);
        });
        connect(available, &QPushButton::clicked, this, [this] { perform("make available"); });
        connect(assign, &QPushButton::clicked, this, [this] { perform("assign"); });
        connect(start, &QPushButton::clicked, this, [this] { perform("start"); });
        connect(complete, &QPushButton::clicked, this, [this] { perform("complete"); });
        connect(reject, &QPushButton::clicked, this, [this] { perform("reject"); });
        connect(cancel, &QPushButton::clicked, this, [this] { perform("cancel"); });
        connect(refresh, &QPushButton::clicked, this, [this] { refreshTree(); addLog("Workflow view refreshed."); });
        connect(reset, &QPushButton::clicked, this, [this] { buildDemoWorkflow(); addLog("Demo workflow reset."); });

        buildDemoWorkflow();
    }

    ~TIVIDYDesktopWindow() override { delete root; }

private:
    QLabel *makeMetric(QGridLayout *layout, int column, const QString &number, const QString &caption) {
        QFrame *card = new QFrame;
        card->setObjectName("panel");
        QVBoxLayout *box = new QVBoxLayout(card);
        box->setContentsMargins(12, 9, 12, 9);
        QLabel *value = new QLabel(number);
        value->setStyleSheet("font-size:23px;font-weight:800;color:#176fc1;border:0;");
        QLabel *label = new QLabel(caption);
        label->setStyleSheet("font-size:10px;font-weight:750;color:#718097;border:0;");
        box->addWidget(value);
        box->addWidget(label);
        layout->addWidget(card, 0, column);
        return value;
    }

    void buildDemoWorkflow() {
        delete root;
        root = new SequentialStage("hospital-root", "Hospital admission workflow");
        SequentialStage *arrival = new SequentialStage("arrival-stage", "Arrival and triage");
        arrival->add(new Task("register", "Register patient", 0, 0));
        arrival->add(new Task("triage", "Nurse triage and priority", 0, 0));
        ParallelStage *parallelPrep = new ParallelStage("parallel-prep", "Prepare admission in parallel");
        parallelPrep->add(new Task("records", "Retrieve patient records", 0, 0));
        parallelPrep->add(new Task("bay", "Prepare treatment bay", 0, 0));
        arrival->add(parallelPrep);
        root->add(arrival);
        SequentialStage *admission = new SequentialStage("admission-stage", "Admission and treatment");
        admission->add(new Task("approval", "Request admission approval", 0, 0));
        admission->add(new Task("doctor", "Doctor accepts case", 0, 0));
        admission->add(new Task("treatment", "Assessment and treatment", 0, 0));
        admission->add(new Task("room", "Allocate room / ICU", 0, 0));
        root->add(admission);
        SequentialStage *closeout = new SequentialStage("closeout-stage", "Discharge and closeout");
        closeout->add(new Task("cleaning", "Clean room / treatment area", 0, 0));
        closeout->add(new Task("archive", "Archive patient history", 0, 0));
        root->add(closeout);
        root->makeAvailable();
        log->clear();
        addLog("Hospital admission workflow created.");
        refreshTree();
    }

    QTreeWidgetItem *appendItem(WorkItem *item, QTreeWidgetItem *parent) {
        QTreeWidgetItem *row = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree);
        row->setText(0, QString::fromStdString(item->getName()));
        row->setText(1, QString::fromStdString(item->getItemID()));
        row->setText(2, QString::fromStdString(item->getStateName()));
        row->setData(0, Qt::UserRole, QString::fromStdString(item->getItemID()));
        row->setToolTip(0, QString("Type: %1\nID: %2").arg(item->getChildren().empty() ? "Task" : "Stage", QString::fromStdString(item->getItemID())));
        const std::vector<WorkItem*> children = item->getChildren();
        for (std::size_t i = 0; i < children.size(); ++i) appendItem(children[i], row);
        return row;
    }

    void refreshTree() {
        QString selectedId;
        if (tree->currentItem()) selectedId = tree->currentItem()->data(0, Qt::UserRole).toString();
        tree->clear();
        itemById.clear();
        if (!root) return;
        appendItem(root, 0);
        QTreeWidgetItemIterator it(tree);
        while (*it) {
            itemById[(*it)->data(0, Qt::UserRole).toString()] = *it;
            (*it)->setExpanded(true);
            QString state = (*it)->text(2);
            if (state.contains("Completed") || state == "Finished") (*it)->setForeground(2, QBrush(QColor("#159957")));
            else if (state.contains("Cancelled") || state.contains("Rejected")) (*it)->setForeground(2, QBrush(QColor("#d94353")));
            else if (state.contains("Available") || state.contains("Started") || state == "InProgress") (*it)->setForeground(2, QBrush(QColor("#1674c4")));
            ++it;
        }
        if (!selectedId.isEmpty() && itemById.contains(selectedId)) tree->setCurrentItem(itemById[selectedId]);
        updateMetrics();
        showDetails(tree->currentItem());
    }

    WorkItem *selectedWorkItem() const {
        if (!tree->currentItem()) return 0;
        const QString id = tree->currentItem()->data(0, Qt::UserRole).toString();
        return workItemById(root, id.toStdString());
    }

    WorkItem *workItemById(WorkItem *item, const std::string &id) const {
        if (!item) return 0;
        if (item->getItemID() == id) return item;
        const std::vector<WorkItem*> children = item->getChildren();
        for (std::size_t i = 0; i < children.size(); ++i) {
            WorkItem *found = workItemById(children[i], id);
            if (found) return found;
        }
        return 0;
    }

    void showDetails(QTreeWidgetItem *row) {
        if (!row) { details->setText("Select an item in the workflow tree."); return; }
        WorkItem *item = selectedWorkItem();
        if (!item) return;
        const bool stage = !item->getChildren().empty();
        details->setText(QString("<b>%1</b><br>ID: %2<br>Type: %3<br>Current state: %4<br>Finished: %5<br>Available: %6")
            .arg(QString::fromStdString(item->getName()), QString::fromStdString(item->getItemID()),
                 stage ? "Composite stage" : "Task", QString::fromStdString(item->getStateName()),
                 item->isFinished() ? "Yes" : "No", item->isAvailable() ? "Yes" : "No"));
    }

    void perform(const QString &action) {
        WorkItem *item = selectedWorkItem();
        if (!item) {
            QMessageBox::information(this, "Select a work item", "Select a task or stage in the workflow tree first.");
            return;
        }
        try {
            if (action == "make available") item->makeAvailable();
            else if (action == "assign") item->assign(&participant);
            else if (action == "start") item->start();
            else if (action == "complete") item->complete();
            else if (action == "reject") item->reject("Rejected from TIVIDY desktop interface");
            else if (action == "cancel") item->cancel();
            root->activateChildren();
            addLog(QString("%1 — %2: %3").arg(QString::fromStdString(item->getItemID()), action,
                                               QString::fromStdString(item->getStateName())));
            refreshTree();
        } catch (const std::exception &e) {
            QMessageBox::warning(this, "Action not allowed", QString::fromUtf8(e.what()));
            addLog(QString("Action blocked — %1: %2").arg(action, QString::fromUtf8(e.what())));
            refreshTree();
        }
    }

    void updateMetrics() {
        int available = 0, completed = 0, pending = 0, cancelled = 0;
        QTreeWidgetItemIterator it(tree);
        while (*it) {
            const QString id = (*it)->data(0, Qt::UserRole).toString();
            WorkItem *item = workItemById(root, id.toStdString());
            if (item && item->getChildren().empty()) {
                const std::string state = item->getStateName();
                if (state == "AvailableState" || state == "AssignedState" || state == "StartedState") ++available;
                else if (state == "CompletedState") ++completed;
                else if (state == "CreatedState") ++pending;
                else if (state == "CancelledState" || state == "RejectedState") ++cancelled;
            }
            ++it;
        }
        activeMetric->setText(QString::number(available));
        doneMetric->setText(QString::number(completed));
        waitingMetric->setText(QString::number(pending));
        failedMetric->setText(QString::number(cancelled));
    }

    void addLog(const QString &message) {
        log->insertItem(0, QDateTime::currentDateTime().toString("HH:mm:ss") + "  " + message);
        while (log->count() > 100) delete log->takeItem(log->count() - 1);
    }

    WorkItem *root;
    Participant participant;
    QTreeWidget *tree{};
    QListWidget *log{};
    QLabel *details{}, *activeMetric{}, *doneMetric{}, *waitingMetric{}, *failedMetric{};
    QMap<QString, QTreeWidgetItem*> itemById;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TIVIDYDesktopWindow window;
    window.show();
    return app.exec();
}
*/