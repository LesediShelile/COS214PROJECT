#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QListWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QMessageBox>
#include <QDateTime>
#include <QHeaderView>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QTimer>
#include <QSlider>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QMap>
#include <QSet>
#include <QMouseEvent>
#include <QFont>
#include <QRegularExpression>
#include <QStringList>
#include <exception>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#include "WorkItem.h"
#include "Task.h"
#include "SequentialStage.h"
#include "ParallelStage.h"
#include "Participant.h"
#include "Stage.h"

namespace {
QString qstr(const std::string &value) { return QString::fromStdString(value); }

QString statusColour(const QString &state) {
    if (state.contains("Completed", Qt::CaseInsensitive) || state == "Finished") return "#159957";
    if (state.contains("Rejected", Qt::CaseInsensitive) || state.contains("Cancelled", Qt::CaseInsensitive)) return "#d94353";
    if (state.contains("Started", Qt::CaseInsensitive) || state.contains("Assigned", Qt::CaseInsensitive) || state.contains("Available", Qt::CaseInsensitive) || state == "InProgress") return "#1685e8";
    return "#8793a5";
}

struct GraphNode {
    WorkItem *item;
    int parent;
    int depth;
    QPointF center;
    QRectF rect;
};
}

/** A native Qt workflow graph. Nodes are drawn from the live Composite tree. */
class WorkflowCanvas : public QWidget {
public:
    explicit WorkflowCanvas(QWidget *parent = 0) : QWidget(parent), root_(0), selectedId_(), highlightedId_() {
        setMinimumSize(850, 500);
        setMouseTracking(true);
        setStyleSheet("background:#f7f9fc;");
    }

    void setWorkflow(WorkItem *root) { root_ = root; rebuild(); }
    void setSelectedId(const QString &id) { selectedId_ = id; update(); }
    void setHighlightedId(const QString &id) { highlightedId_ = id; update(); }

    QSize sizeHint() const override { return QSize(canvasWidth_, canvasHeight_); }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), QColor("#f7f9fc"));
        if (nodes_.empty()) {
            p.setPen(QColor("#718097"));
            p.drawText(rect(), Qt::AlignCenter, "No workflow loaded");
            return;
        }

        // Subtle swimlanes make the hierarchy easier to follow, like the sample theatre.
        for (int d = 0; d <= maxDepth_; ++d) {
            QRectF lane(0, d * 150.0 + 15.0, canvasWidth_, 132.0);
            p.fillRect(lane, d % 2 == 0 ? QColor("#f1f5fb") : QColor("#f8faff"));
            p.setPen(QColor("#aab6c8"));
            p.setFont(QFont("Segoe UI", 9, QFont::DemiBold));
            p.drawText(QRectF(14, d * 150.0 + 20, 180, 18), Qt::AlignLeft, depthLabel(d));
        }

        // Draw connectors before nodes.
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            const GraphNode &n = nodes_[i];
            if (n.parent < 0) continue;
            const GraphNode &parent = nodes_[static_cast<std::size_t>(n.parent)];
            QPointF start(parent.center.x(), parent.rect.bottom());
            QPointF end(n.center.x(), n.rect.top());
            QPainterPath path(start);
            const qreal midY = (start.y() + end.y()) / 2.0;
            path.cubicTo(start.x(), midY, end.x(), midY, end.x(), end.y());
            QPen pen(QColor("#b9c5d5"), 2.0);
            if (n.item && qstr(n.item->getStateName()).contains("Completed")) pen.setColor(QColor("#6cc7a1"));
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
        }

        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            const GraphNode &n = nodes_[i];
            const QString id = qstr(n.item->getItemID());
            const QString state = qstr(n.item->getStateName());
            const bool stage = !n.item->getChildren().empty();
            const bool selected = id == selectedId_;
            const bool highlighted = id == highlightedId_;
            QColor accent(statusColour(state));
            QRectF r = n.rect;
            QColor fill = QColor("white");
            if (state.contains("Completed", Qt::CaseInsensitive) || state == "Finished") fill = QColor("#e9f8f0");
            else if (state.contains("Rejected", Qt::CaseInsensitive) || state.contains("Cancelled", Qt::CaseInsensitive)) fill = QColor("#fff0f1");
            else if (state.contains("Available", Qt::CaseInsensitive) || state.contains("Started", Qt::CaseInsensitive) || state.contains("Assigned", Qt::CaseInsensitive) || state == "InProgress") fill = QColor("#eaf4ff");
            if (highlighted) fill = QColor("#fff4d9");

            p.setPen(QPen(selected ? QColor("#0a84ff") : accent, selected ? 3.0 : 1.6));
            p.setBrush(fill);
            if (stage) p.drawRoundedRect(r, 11, 11); else p.drawRoundedRect(r, 8, 8);

            QRectF labelRect(r.x() + 8, r.y() + 7, r.width() - 16, 30);
            p.setPen(QColor("#1b2333"));
            p.setFont(QFont("Segoe UI", 9, QFont::DemiBold));
            p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, qstr(n.item->getName()));
            p.setPen(accent);
            p.setFont(QFont("Segoe UI", 8));
            p.drawText(QRectF(r.x() + 6, r.bottom() - 18, r.width() - 12, 13), Qt::AlignCenter, stateLabel(state));
            p.setPen(QColor("#7c8798"));
            p.setFont(QFont("Consolas", 7));
            p.drawText(QRectF(r.x() + 4, r.bottom() + 2, r.width() - 8, 13), Qt::AlignCenter, id);
        }
    }

    void mousePressEvent(QMouseEvent *event) override {
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            if (nodes_[i].rect.contains(event->pos())) {
                selectedId_ = qstr(nodes_[i].item->getItemID());
                emitSelected(selectedId_);
                update();
                return;
            }
        }
        QWidget::mousePressEvent(event);
    }

private:
    QString depthLabel(int depth) const {
        if (depth == 0) return "WORKFLOW";
        if (depth == 1) return "STAGES";
        if (depth == 2) return "SUB-STAGES";
        return "TASKS";
    }
    QString stateLabel(const QString &state) const {
        if (state == "CreatedState") return "NOT STARTED";
        if (state == "AvailableState") return "AVAILABLE";
        if (state == "AssignedState") return "ASSIGNED";
        if (state == "StartedState") return "IN PROGRESS";
        if (state == "CompletedState") return "COMPLETED";
        if (state == "CancelledState") return "CANCELLED";
        if (state == "RejectedState") return "REJECTED";
        return state.toUpper();
    }
    void emitSelected(const QString &id) { if (onSelect) onSelect(id); }

public:
    // Callback kept as a plain function object to avoid requiring Q_OBJECT/moc.
    std::function<void(const QString &)> onSelect;

private:
    int addNode(WorkItem *item, int parentIndex, int depth, int &leafCursor) {
        GraphNode node;
        node.item = item;
        node.parent = parentIndex;
        node.depth = depth;
        node.center = QPointF(0, depth * 150.0 + 86.0);
        node.rect = QRectF(0, node.center.y() - 30.0, 180.0, 60.0);
        const int thisIndex = static_cast<int>(nodes_.size());
        nodes_.push_back(node);
        maxDepth_ = std::max(maxDepth_, depth);
        const std::vector<WorkItem*> children = item->getChildren();
        if (children.empty()) {
            nodes_[static_cast<std::size_t>(thisIndex)].center.setX(140.0 + leafCursor * 220.0);
            ++leafCursor;
        } else {
            std::vector<int> childIndices;
            for (std::size_t i = 0; i < children.size(); ++i) childIndices.push_back(addNode(children[i], thisIndex, depth + 1, leafCursor));
            qreal sum = 0;
            for (std::size_t i = 0; i < childIndices.size(); ++i) sum += nodes_[static_cast<std::size_t>(childIndices[i])].center.x();
            nodes_[static_cast<std::size_t>(thisIndex)].center.setX(childIndices.empty() ? 140.0 : sum / childIndices.size());
        }
        GraphNode &stored = nodes_[static_cast<std::size_t>(thisIndex)];
        stored.rect = QRectF(stored.center.x() - 90.0, stored.center.y() - 30.0, 180.0, 60.0);
        return thisIndex;
    }
    void rebuild() {
        nodes_.clear();
        maxDepth_ = 0;
        int leafCursor = 0;
        if (root_) addNode(root_, -1, 0, leafCursor);
        canvasWidth_ = std::max(1000, 280 + leafCursor * 220);
        canvasHeight_ = std::max(600, (maxDepth_ + 1) * 150 + 90);
        setMinimumSize(canvasWidth_, canvasHeight_);
        resize(canvasWidth_, canvasHeight_);
        updateGeometry();
        update();
    }

  WorkItem *root_;
    QString selectedId_;
    QString highlightedId_;
    std::vector<GraphNode> nodes_;
    int maxDepth_ = 0;
    int canvasWidth_ = 1000;
    int canvasHeight_ = 600;
};

class TIVIDYDesktopWindow : public QMainWindow {
public:
    TIVIDYDesktopWindow() : root_(0), participant_("demo-nurse", "Demo Nurse", 0), playing_(false), playbackIndex_(0) {
        setWindowTitle("TIVIDY | Workflow Theatre");
        resize(1500, 920);
        setMinimumSize(1100, 720);
        setStyleSheet(
            "QMainWindow,QWidget{background:#f3f5fa;color:#1b2333;font-family:'Segoe UI';font-size:13px;}"
            "QFrame#panel{background:white;border:1px solid #cfd6e6;border-radius:10px;}"
            "QPushButton,QComboBox{background:white;border:1px solid #cfd6e6;border-radius:7px;padding:7px 10px;}"
            "QPushButton:hover{border-color:#0a84ff;color:#0a84ff;}"
            "QPushButton:disabled{color:#9aa5b5;background:#f2f4f8;}"
            "QTreeWidget,QListWidget,QPlainTextEdit{background:white;border:1px solid #dce3ef;border-radius:7px;padding:5px;}"
            "QHeaderView::section{background:#f7f9fc;border:0;border-bottom:1px solid #e0e6ef;padding:7px;font-weight:700;}"
            "QSlider::groove:horizontal{height:5px;background:#dbe3ef;border-radius:2px;}"
            "QSlider::handle:horizontal{background:#0a84ff;width:13px;margin:-5px 0;border-radius:6px;}"
        );
        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, [this] { advancePlayback(); });
        buildUi();
        buildHospitalWorkflow();
    }

    ~TIVIDYDesktopWindow() override { delete root_; }

private:
    QLabel *makeMetric(QGridLayout *layout, int column, const QString &value, const QString &caption, const QString &colour) {
        QFrame *card = new QFrame;
        card->setObjectName("panel");
        QVBoxLayout *box = new QVBoxLayout(card);
        box->setContentsMargins(13, 10, 13, 10);
        QLabel *number = new QLabel(value);
        number->setStyleSheet(QString("font-size:25px;font-weight:800;color:%1;border:0;").arg(colour));
        QLabel *label = new QLabel(caption);
        label->setStyleSheet("font-size:10px;font-weight:750;color:#718097;border:0;letter-spacing:.5px;");
        box->addWidget(number);
        box->addWidget(label);
        layout->addWidget(card, 0, column);
        return number;
    }

    void buildUi() {
        QWidget *central = new QWidget;
        setCentralWidget(central);
        QVBoxLayout *page = new QVBoxLayout(central);
        page->setContentsMargins(16, 12, 16, 14);
        page->setSpacing(10);

        QHBoxLayout *header = new QHBoxLayout;
        QLabel *brand = new QLabel("TIVIDY");
        brand->setStyleSheet("font-size:25px;font-weight:900;letter-spacing:3px;color:#0878d1;");
        QLabel *subtitle = new QLabel("WORKFLOW THEATRE  /  HOSPITAL WORKFLOW MANAGEMENT");
        subtitle->setStyleSheet("font-size:10px;font-weight:800;letter-spacing:1px;color:#69758f;");
        header->addWidget(brand);
        header->addSpacing(12);
        header->addWidget(subtitle);
        header->addStretch();
        QLabel *scenarioLabel = new QLabel("SCENARIO");
        scenarioLabel->setStyleSheet("font-size:10px;font-weight:800;color:#69758f;");
        header->addWidget(scenarioLabel);
        scenario_ = new QComboBox;
        scenario_->addItem("Hospital admission", "hospital");
        scenario_->addItem("Law firm intake", "law");
        scenario_->setMinimumWidth(180);
        header->addWidget(scenario_);
        page->addLayout(header);

        QGridLayout *metrics = new QGridLayout;
        metrics->setSpacing(9);
        activeMetric_ = makeMetric(metrics, 0, "0", "ACTIVE / AVAILABLE", "#0a84ff");
        doneMetric_ = makeMetric(metrics, 1, "0", "COMPLETED", "#12a150");
        waitingMetric_ = makeMetric(metrics, 2, "0", "WAITING", "#d98200");
        failedMetric_ = makeMetric(metrics, 3, "0", "FAILED / CANCELLED", "#e5484d");
        page->addLayout(metrics);

        QSplitter *split = new QSplitter(Qt::Horizontal);
        split->setChildrenCollapsible(false);
        QFrame *graphPanel = new QFrame;
        graphPanel->setObjectName("panel");
        QVBoxLayout *graphLayout = new QVBoxLayout(graphPanel);
        graphLayout->setContentsMargins(12, 11, 12, 12);
        QHBoxLayout *graphHeading = new QHBoxLayout;
        QLabel *graphTitle = new QLabel("Workflow map");
        graphTitle->setStyleSheet("font-size:16px;font-weight:800;");
        QLabel *graphHint = new QLabel("Select a node to inspect it. Colours show each item's live state.");
        graphHint->setStyleSheet("color:#69758f;font-size:11px;");
        graphHeading->addWidget(graphTitle);
        graphHeading->addSpacing(10);
        graphHeading->addWidget(graphHint);
        graphHeading->addStretch();
        graphLayout->addLayout(graphHeading);
        canvas_ = new WorkflowCanvas;
        canvas_->onSelect = [this](const QString &id) { selectById(id); };
        QScrollArea *scroll = new QScrollArea;
        scroll->setWidget(canvas_);
        scroll->setWidgetResizable(false);
        scroll->setFrameShape(QFrame::NoFrame);
        graphLayout->addWidget(scroll, 1);
        split->addWidget(graphPanel);

        QFrame *sidePanel = new QFrame;
        sidePanel->setObjectName("panel");
        sidePanel->setMinimumWidth(320);
        sidePanel->setMaximumWidth(410);
        QVBoxLayout *side = new QVBoxLayout(sidePanel);
        side->setContentsMargins(13, 12, 13, 13);
        side->setSpacing(8);
        QLabel *detailsTitle = new QLabel("Selected work item");
        detailsTitle->setStyleSheet("font-size:16px;font-weight:800;");
        side->addWidget(detailsTitle);
        details_ = new QLabel("Select a node in the workflow map.");
        details_->setWordWrap(true);
        details_->setMinimumHeight(105);
        details_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        details_->setStyleSheet("background:#f7f9fc;border:1px solid #e4eaf2;border-radius:7px;padding:10px;color:#526178;");
        side->addWidget(details_);
        QLabel *actionsTitle = new QLabel("Lifecycle actions");
        actionsTitle->setStyleSheet("font-size:14px;font-weight:800;margin-top:4px;");
        side->addWidget(actionsTitle);
        QGridLayout *actions = new QGridLayout;
        actions->setSpacing(6);
        QPushButton *available = new QPushButton("Make available");
        QPushButton *assign = new QPushButton("Assign");
        QPushButton *start = new QPushButton("Start");
        QPushButton *complete = new QPushButton("Complete");
        QPushButton *reject = new QPushButton("Reject");
        QPushButton *cancel = new QPushButton("Cancel");
        actions->addWidget(available, 0, 0);
        actions->addWidget(assign, 0, 1);
        actions->addWidget(start, 1, 0);
        actions->addWidget(complete, 1, 1);
        actions->addWidget(reject, 2, 0);
        actions->addWidget(cancel, 2, 1);
        side->addLayout(actions);
        QLabel *logTitle = new QLabel("Activity stream");
        logTitle->setStyleSheet("font-size:14px;font-weight:800;margin-top:4px;");
        side->addWidget(logTitle);
        log_ = new QListWidget;
        log_->setMinimumHeight(130);
        side->addWidget(log_, 1);
        split->addWidget(sidePanel);
        split->setStretchFactor(0, 4);
        split->setStretchFactor(1, 1);
        split->setSizes(QList<int>() << 1060 << 360);
        page->addWidget(split, 1);

        QFrame *controlPanel = new QFrame;
        controlPanel->setObjectName("panel");
        QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
        controlLayout->setContentsMargins(12, 8, 12, 9);
        QHBoxLayout *controls = new QHBoxLayout;
        restart_ = new QPushButton("⏮ Restart");
        previous_ = new QPushButton("◀ Previous");
        play_ = new QPushButton("▶ Play");
        next_ = new QPushButton("Next ▶");
        controls->addWidget(restart_);
        controls->addWidget(previous_);
        controls->addWidget(play_);
        controls->addWidget(next_);
        controls->addSpacing(8);
        controls->addWidget(new QLabel("Playback speed"));
        speed_ = new QComboBox;
        speed_->addItem("0.5×", 2000);
        speed_->addItem("1×", 1200);
        speed_->addItem("2×", 600);
        speed_->addItem("4×", 300);
        speed_->setCurrentIndex(1);
        controls->addWidget(speed_);
        controls->addStretch();
        QPushButton *refresh = new QPushButton("Refresh view");
        QPushButton *reset = new QPushButton("Reset workflow");
        controls->addWidget(refresh);
        controls->addWidget(reset);
        controlLayout->addLayout(controls);
        QHBoxLayout *progress = new QHBoxLayout;
        QLabel *progressLabel = new QLabel("Playback position");
        progressLabel->setStyleSheet("color:#69758f;font-size:11px;");
        progress->addWidget(progressLabel);
        slider_ = new QSlider(Qt::Horizontal);
        slider_->setRange(0, 0);
        progress->addWidget(slider_, 1);
        stepLabel_ = new QLabel("0 / 0");
        stepLabel_->setMinimumWidth(55);
        stepLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        stepLabel_->setStyleSheet("color:#69758f;font-size:11px;");
        progress->addWidget(stepLabel_);
        controlLayout->addLayout(progress);
        page->addWidget(controlPanel);

        QGroupBox *eventBox = new QGroupBox("Live event stream (JSONL) — paste events and replay them on the workflow");
        QHBoxLayout *eventLayout = new QHBoxLayout(eventBox);
        eventStream_ = new QPlainTextEdit;
        eventStream_->setMaximumHeight(78);
        eventStream_->setPlaceholderText("One JSON object per line, e.g. {\"type\":\"TASK_STARTED\",\"sourceID\":\"register\",\"details\":\"Registration started\"}");
        eventLayout->addWidget(eventStream_, 1);
        QPushButton *replay = new QPushButton("Replay stream");
        replay->setMinimumHeight(42);
        eventLayout->addWidget(replay);
        page->addWidget(eventBox);

        connect(scenario_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (index == 1) buildLawWorkflow(); else buildHospitalWorkflow();
        });
        connect(available, &QPushButton::clicked, this, [this] { perform("make available"); });
        connect(assign, &QPushButton::clicked, this, [this] { perform("assign"); });
        connect(start, &QPushButton::clicked, this, [this] { perform("start"); });
        connect(complete, &QPushButton::clicked, this, [this] { perform("complete"); });
        connect(reject, &QPushButton::clicked, this, [this] { perform("reject"); });
        connect(cancel, &QPushButton::clicked, this, [this] { perform("cancel"); });
        connect(refresh, &QPushButton::clicked, this, [this] { refreshView(); addLog("Workflow view refreshed."); });
        connect(reset, &QPushButton::clicked, this, [this] { resetCurrentScenario(); });
        connect(restart_, &QPushButton::clicked, this, [this] { restartPlayback(); });
        connect(previous_, &QPushButton::clicked, this, [this] { stepPlayback(-1); });
        connect(next_, &QPushButton::clicked, this, [this] { stepPlayback(1); });
        connect(play_, &QPushButton::clicked, this, [this] { togglePlayback(); });
        connect(speed_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (index >= 0) timer_->setInterval(speed_->itemData(index).toInt());
        });
        connect(slider_, &QSlider::valueChanged, this, [this](int value) {
            if (value >= 0 && value < playbackItems_.size()) {
                playbackIndex_ = value;
                updatePlaybackUi();
                canvas_->setHighlightedId(qstr(playbackItems_[static_cast<std::size_t>(value)]->getItemID()));
            }
        });
        connect(replay, &QPushButton::clicked, this, [this] { replayEventStream(); });
    }

    void buildHospitalWorkflow() {
        stopPlayback();
        delete root_;
        root_ = new SequentialStage("hospital-root", "Hospital admission workflow");
        SequentialStage *arrival = new SequentialStage("arrival-stage", "Arrival and triage");
        arrival->add(new Task("register", "Reception registration", 0, 0));
        arrival->add(new Task("triage", "Nurse triage and priority", 0, 0));
        ParallelStage *prep = new ParallelStage("prep-stage", "Prepare admission in parallel");
        prep->add(new Task("records", "Retrieve patient records", 0, 0));
        prep->add(new Task("bay", "Prepare treatment bay", 0, 0));
        arrival->add(prep);
        root_->add(arrival);
        SequentialStage *admission = new SequentialStage("admission-stage", "Admission and treatment");
        admission->add(new Task("approval", "Request admission approval", 0, 0));
        admission->add(new Task("doctor", "Doctor accepts case", 0, 0));
        admission->add(new Task("treatment", "Assessment and treatment", 0, 0));
        admission->add(new Task("room", "Allocate room / ICU", 0, 0));
        root_->add(admission);
        SequentialStage *closeout = new SequentialStage("closeout-stage", "Discharge and closeout");
        closeout->add(new Task("cleaning", "Clean treatment area", 0, 0));
        closeout->add(new Task("archive", "Archive patient history", 0, 0));
        root_->add(closeout);
        initialiseWorkflow("Hospital admission workflow loaded.");
    }

    void buildLawWorkflow() {
        stopPlayback();
        delete root_;
        root_ = new SequentialStage("law-root", "Law firm intake workflow");
        SequentialStage *intake = new SequentialStage("intake-stage", "Client intake");
        intake->add(new Task("inquiry", "Log client inquiry", 0, 0));
        intake->add(new Task("identity", "Verify client identity", 0, 0));
        ParallelStage *checks = new ParallelStage("checks-stage", "Compliance checks in parallel");
        checks->add(new Task("conflict", "Conflict-of-interest check", 0, 0));
        checks->add(new Task("documents", "Collect supporting documents", 0, 0));
        intake->add(checks);
        root_->add(intake);
        SequentialStage *review = new SequentialStage("review-stage", "Case review and approval");
        review->add(new Task("evaluate", "Associate evaluates case", 0, 0));
        review->add(new Task("partner", "Partner approves engagement", 0, 0));
        review->add(new Task("fees", "Prepare fee estimate", 0, 0));
        root_->add(review);
        SequentialStage *open = new SequentialStage("open-stage", "Open matter and closeout");
        open->add(new Task("sign", "Sign engagement letter", 0, 0));
        open->add(new Task("billing", "Confirm retainer payment", 0, 0));
        open->add(new Task("archive", "Open and archive matter", 0, 0));
        root_->add(open);
        initialiseWorkflow("Law firm intake workflow loaded.");
    }

    void initialiseWorkflow(const QString &message) {
        root_->makeAvailable();
        playbackIndex_ = 0;
        log_->clear();
        addLog(message);
        refreshView();
    }

    void resetCurrentScenario() {
        if (scenario_->currentData().toString() == "law") buildLawWorkflow();
        else buildHospitalWorkflow();
        addLog("Workflow reset to its initial state.");
    }

    void collectLeaves(WorkItem *item) {
        if (!item) return;
        const std::vector<WorkItem*> children = item->getChildren();
        if (children.empty()) playbackItems_.push_back(item);
        else for (std::size_t i = 0; i < children.size(); ++i) collectLeaves(children[i]);
    }

    WorkItem *findById(WorkItem *item, const std::string &id) const {
        if (!item) return 0;
        if (item->getItemID() == id) return item;
        const std::vector<WorkItem*> children = item->getChildren();
        for (std::size_t i = 0; i < children.size(); ++i) {
            WorkItem *found = findById(children[i], id);
            if (found) return found;
        }
        return 0;
    }

    WorkItem *selectedItem() const {
        if (selectedId_.isEmpty()) return 0;
        return findById(root_, selectedId_.toStdString());
    }

    void selectById(const QString &id) {
        selectedId_ = id;
        canvas_->setSelectedId(id);
        refreshDetails();
    }

    void refreshView() {
        playbackItems_.clear();
        collectLeaves(root_);
        canvas_->setWorkflow(root_);
        canvas_->setSelectedId(selectedId_);
        if (selectedId_.isEmpty() || !findById(root_, selectedId_.toStdString())) {
            selectedId_ = qstr(root_->getItemID());
            canvas_->setSelectedId(selectedId_);
        }
        updateMetrics();
        refreshDetails();
        slider_->blockSignals(true);
        slider_->setRange(0, std::max(0, static_cast<int>(playbackItems_.size()) - 1));
        slider_->setValue(std::min(playbackIndex_, std::max(0, static_cast<int>(playbackItems_.size()) - 1)));
        slider_->blockSignals(false);
        updatePlaybackUi();
    }

    void refreshDetails() {
        WorkItem *item = selectedItem();
        if (!item) { details_->setText("Select a node in the workflow map."); return; }
        const bool stage = !item->getChildren().empty();
        details_->setText(QString("<b style='font-size:15px;color:#1b2333'>%1</b><br><br>"
                                  "<b>ID</b> &nbsp; %2<br><b>Type</b> &nbsp; %3<br>"
                                  "<b>State</b> &nbsp; <span style='color:%4'>%5</span><br>"
                                  "<b>Finished</b> &nbsp; %6<br><b>Available</b> &nbsp; %7")
            .arg(qstr(item->getName()), qstr(item->getItemID()), stage ? "Composite stage" : "Task",
                 statusColour(qstr(item->getStateName())), qstr(item->getStateName()),
                 item->isFinished() ? "Yes" : "No", item->isAvailable() ? "Yes" : "No"));
    }

    void perform(const QString &action) {
        WorkItem *item = selectedItem();
        if (!item) {
            QMessageBox::information(this, "Select a work item", "Select a node in the workflow map first.");
            return;
        }
        try {
            if (action == "make available") item->makeAvailable();
            else if (action == "assign") item->assign(&participant_);
            else if (action == "start") item->start();
            else if (action == "complete") item->complete();
            else if (action == "reject") item->reject("Rejected from TIVIDY desktop interface");
            else if (action == "cancel") item->cancel();
            root_->activateChildren();
            addLog(QString("%1 — %2 → %3").arg(qstr(item->getItemID()), action, qstr(item->getStateName())));
            refreshView();
        } catch (const std::exception &e) {
            QMessageBox::warning(this, "Action not allowed", QString::fromUtf8(e.what()));
            addLog(QString("Action blocked — %1: %2").arg(action, QString::fromUtf8(e.what())));
            refreshView();
        }
    }

    void updateMetrics() {
        int active = 0, completed = 0, waiting = 0, failed = 0;
        for (std::size_t i = 0; i < playbackItems_.size(); ++i) {
            const QString state = qstr(playbackItems_[i]->getStateName());
            if (state == "AvailableState" || state == "AssignedState" || state == "StartedState") ++active;
            else if (state == "CompletedState") ++completed;
            else if (state == "CreatedState") ++waiting;
            else if (state == "CancelledState" || state == "RejectedState") ++failed;
        }
        activeMetric_->setText(QString::number(active));
        doneMetric_->setText(QString::number(completed));
        waitingMetric_->setText(QString::number(waiting));
        failedMetric_->setText(QString::number(failed));
    }

    void addLog(const QString &message) {
        log_->insertItem(0, QDateTime::currentDateTime().toString("HH:mm:ss") + "   " + message);
        while (log_->count() > 150) delete log_->takeItem(log_->count() - 1);
    }

    void togglePlayback() {
        if (playing_) stopPlayback();
        else {
            if (playbackItems_.empty()) return;
            playing_ = true;
            timer_->setInterval(speed_->currentData().toInt());
            timer_->start();
            play_->setText("Ⅱ Pause");
            addLog("Playback started.");
        }
    }

    void stopPlayback() {
        playing_ = false;
        if (timer_) timer_->stop();
        if (play_) play_->setText("▶ Play");
    }

    void restartPlayback() {
        stopPlayback();
        playbackIndex_ = 0;
        canvas_->setHighlightedId(playbackItems_.empty() ? QString() : qstr(playbackItems_[0]->getItemID()));
        slider_->setValue(0);
        updatePlaybackUi();
        addLog("Playback returned to the first task.");
    }

    void stepPlayback(int direction) {
        stopPlayback();
        if (playbackItems_.empty()) return;
        playbackIndex_ = std::max(0, std::min(static_cast<int>(playbackItems_.size()) - 1, playbackIndex_ + direction));
        slider_->setValue(playbackIndex_);
        canvas_->setHighlightedId(qstr(playbackItems_[static_cast<std::size_t>(playbackIndex_)]->getItemID()));
        updatePlaybackUi();
    }

    void advancePlayback() {
        if (playbackItems_.empty() || playbackIndex_ >= static_cast<int>(playbackItems_.size())) {
            stopPlayback();
            return;
        }
        root_->activateChildren();
        WorkItem *item = playbackItems_[static_cast<std::size_t>(playbackIndex_)];
        try {
            if (!item->isFinished()) {
                // Playback respects the Composite's activation rules instead of
                // forcing a future sequential task to become available early.
                if (qstr(item->getStateName()) == "CreatedState" && !item->isAvailable()) {
                    addLog(QString("Waiting for the workflow to release %1.").arg(qstr(item->getItemID())));
                    return;
                }
                if (qstr(item->getStateName()) == "CreatedState") item->makeAvailable();
                if (qstr(item->getStateName()) == "AvailableState") item->assign(&participant_);
                if (qstr(item->getStateName()) == "AssignedState") item->start();
                if (qstr(item->getStateName()) == "StartedState") item->complete();
                root_->activateChildren();
                addLog(QString("Playback — %1: %2").arg(qstr(item->getItemID()), qstr(item->getStateName())));
            }
        } catch (const std::exception &e) {
            addLog(QString("Playback skipped %1: %2").arg(qstr(item->getItemID()), QString::fromUtf8(e.what())));
        }
        canvas_->setHighlightedId(qstr(item->getItemID()));
        ++playbackIndex_;
        refreshView();
        if (playbackIndex_ >= static_cast<int>(playbackItems_.size())) {
            stopPlayback();
            addLog("Playback reached the final task.");
        }
    }

    void updatePlaybackUi() {
        stepLabel_->setText(QString("%1 / %2").arg(playbackItems_.empty() ? 0 : playbackIndex_ + 1).arg(playbackItems_.size()));
        previous_->setEnabled(!playbackItems_.empty() && playbackIndex_ > 0);
        next_->setEnabled(!playbackItems_.empty() && playbackIndex_ < static_cast<int>(playbackItems_.size()));
    }

    void replayEventStream() {
        const QStringList lines = eventStream_->toPlainText().split('\n', Qt::SkipEmptyParts);
        if (lines.isEmpty()) {
            QMessageBox::information(this, "No events", "Paste one JSON event per line, then choose Replay stream.");
            return;
        }
        int replayed = 0;
        for (int i = 0; i < lines.size(); ++i) {
            const QString line = lines[i].trimmed();
            const QRegularExpression sourcePattern("\\\"sourceID\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
            const QRegularExpression typePattern("\\\"type\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
            const QRegularExpression detailsPattern("\\\"details\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
            const QString source = sourcePattern.match(line).captured(1);
            const QString type = typePattern.match(line).captured(1);
            const QString detail = detailsPattern.match(line).captured(1);
            if (source.isEmpty() || type.isEmpty()) {
                addLog(QString("Ignored malformed event line %1.").arg(i + 1));
                continue;
            }
            WorkItem *item = findById(root_, source.toStdString());
            if (item) {
                selectedId_ = source;
                canvas_->setSelectedId(source);
                canvas_->setHighlightedId(source);
            }
            addLog(QString("%1  %2 — %3").arg(type, source, detail.isEmpty() ? "Event replayed" : detail));
            ++replayed;
        }
        refreshView();
        addLog(QString("Event stream replay complete: %1 event(s) read.").arg(replayed));
    }

    Stage *root_;
    Participant participant_;
    bool playing_;
    int playbackIndex_;
    QTimer *timer_ = 0;
    QComboBox *scenario_ = 0;
    QComboBox *speed_ = 0;
    QSlider *slider_ = 0;
    QLabel *activeMetric_ = 0, *doneMetric_ = 0, *waitingMetric_ = 0, *failedMetric_ = 0;
    QLabel *details_ = 0, *stepLabel_ = 0;
    QPushButton *restart_ = 0, *previous_ = 0, *play_ = 0, *next_ = 0;
    QListWidget *log_ = 0;
    WorkflowCanvas *canvas_ = 0;
    QPlainTextEdit *eventStream_ = 0;
    QString selectedId_;
    std::vector<WorkItem*> playbackItems_;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TIVIDYDesktopWindow window;
    window.show();
    return app.exec();
}
