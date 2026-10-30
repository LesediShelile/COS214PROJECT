#pragma once
// TestTheatre.h - a tiny, header-only Qt window that shows a test run as a
// UML-style activity diagram and highlights the step that is executing.
//
//   (●) START ──▶ [ step ] ──▶ [ step ] ──▶ ... ──▶ (◉) END
//
// Sections become dashed "swimlane" groups, edges turn green/red as steps
// pass/fail, the running step glows, and a token travels along the arrow to
// the next step.
//
// The steps are NOT hard-coded here: main() registers them with step(), so
// whatever you add, remove or reorder in main shows up in the diagram
// automatically. The public API is unchanged. No Q_OBJECT / moc needed
// (lambdas only), same as before.

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QFrame>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QCursor>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string &m) : std::runtime_error(m) {}
};

enum TheatreState { TS_Pending = 0, TS_Running, TS_Passed, TS_Failed };

struct TheatreStep {
    QString section, title, detail, message;
    std::function<void()> body;
    int state = TS_Pending;
    // filled in by the canvas layout
    QRectF rect;
    double titleH = 0, detailH = 0, msgH = 0;
};

// ---------------------------------------------------------------------------
// The diagram itself: a custom-painted widget (no Q_OBJECT needed).
// ---------------------------------------------------------------------------
class TheatreCanvas : public QWidget {
public:
    explicit TheatreCanvas(std::vector<TheatreStep> *steps, QWidget *parent = 0)
        : QWidget(parent), steps_(steps) {
        setMouseTracking(true);
        dots_ = QPixmap(24, 24);
        dots_.fill(Qt::transparent);
        {
            QPainter tp(&dots_);
            tp.setRenderHint(QPainter::Antialiasing);
            tp.setPen(Qt::NoPen);
            tp.setBrush(QColor(196, 206, 226, 170));
            tp.drawEllipse(QPointF(12, 12), 1.3, 1.3);
        }
        clock_.start();
        QObject::connect(&tick_, &QTimer::timeout, this, [this] {
            if (running_ >= 0 || travelTo_ >= 0) update();
        });
        tick_.start(30);
    }

    void setRunning(int i) { running_ = i; update(); }
    void startTravel(int toIndex, int ms) {
        travelTo_ = toIndex;
        travelMs_ = std::max(1, ms);
        travelClock_.restart();
        update();
    }
    void stopTravel() { travelTo_ = -1; update(); }
    void relayout() { layoutNodes(); update(); }
    QRectF nodeRect(int i) const { return (*steps_)[static_cast<std::size_t>(i)].rect; }

protected:
    void resizeEvent(QResizeEvent *) override { layoutNodes(); }

    void mouseMoveEvent(QMouseEvent *) override {
        const int h = hitTest(mapFromGlobal(QCursor::pos()));
        if (h != hover_) { hover_ = h; update(); }
    }
    void leaveEvent(QEvent *) override {
        if (hover_ != -1) { hover_ = -1; update(); }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);

        QLinearGradient bg(0, 0, 0, height());
        bg.setColorAt(0, QColor("#f8faff"));
        bg.setColorAt(1, QColor("#eaf0fa"));
        p.fillRect(rect(), bg);
        p.fillRect(rect(), QBrush(dots_));

        const double t = clock_.elapsed() / 1000.0;
        const int n = static_cast<int>(steps_->size());
        drawGroups(p);
        drawEdges(p, t);
        drawTerminals(p);
        for (int i = 0; i < n; ++i) drawNode(p, i, t);
    }

private:
    struct Group {
        QString name;
        int first = 0, last = 0, color = 0;
        QRectF rect;
    };

    static QFont mk(int px, bool bold = false, double spacing = 0) {
        QFont f("Segoe UI");
        f.setPixelSize(px);
        f.setBold(bold);
        if (spacing != 0) f.setLetterSpacing(QFont::AbsoluteSpacing, spacing);
        return f;
    }

    static QColor accent(int i) {
        static const char *pal[] = {"#0a84ff", "#7a5af8", "#0e9f8e", "#f08c00", "#d6336c"};
        return QColor(pal[i % 5]);
    }

    int hitTest(const QPoint &pt) const {
        for (std::size_t i = 0; i < steps_->size(); ++i)
            if ((*steps_)[i].rect.contains(QPointF(pt))) return static_cast<int>(i);
        return -1;
    }

    // ---- layout -----------------------------------------------------------
    void layoutNodes() {
        groups_.clear();
        const double W = width();
        cx_ = W / 2.0;
        nodeW_ = qBound(300.0, W - 120.0, 460.0);
        const double titleW = nodeW_ - 58 - 14 - 76;
        const double bodyW = nodeW_ - 58 - 14;
        QFontMetricsF tm(mk(14, true)), dm(mk(11));

        double y = 24;
        startY_ = y + 11;
        y += 22 + 28;

        const int n = static_cast<int>(steps_->size());
        int i = 0;
        while (i < n) {
            int j = i;
            while (j + 1 < n && (*steps_)[static_cast<std::size_t>(j + 1)].section ==
                                    (*steps_)[static_cast<std::size_t>(i)].section)
                ++j;
            const bool boxed = !(*steps_)[static_cast<std::size_t>(i)].section.isEmpty();
            Group g;
            g.name = (*steps_)[static_cast<std::size_t>(i)].section;
            g.first = i;
            g.last = j;
            g.color = static_cast<int>(groups_.size());
            const double top = y;
            if (boxed) y += 38;
            for (int k = i; k <= j; ++k) {
                TheatreStep &s = (*steps_)[static_cast<std::size_t>(k)];
                s.titleH = tm.boundingRect(QRectF(0, 0, titleW, 10000), Qt::TextWordWrap, s.title).height();
                s.detailH = s.detail.isEmpty() ? 0
                    : dm.boundingRect(QRectF(0, 0, bodyW, 10000), Qt::TextWordWrap, s.detail).height();
                s.msgH = s.message.isEmpty() ? 0
                    : dm.boundingRect(QRectF(0, 0, bodyW, 10000), Qt::TextWordWrap, s.message).height();
                double content = s.titleH;
                if (s.detailH > 0) content += 3 + s.detailH;
                if (s.msgH > 0) content += 5 + s.msgH;
                const double h = std::max(56.0, 28.0 + content);
                s.rect = QRectF(cx_ - nodeW_ / 2, y, nodeW_, h);
                y += h;
                if (k < j) y += 30;
            }
            if (boxed) {
                y += 16;
                g.rect = QRectF(cx_ - nodeW_ / 2 - 22, top, nodeW_ + 44, y - top);
            }
            groups_.push_back(g);
            y += 28;
            i = j + 1;
        }
        endY_ = y + 13;
        const int total = static_cast<int>(endY_ + 13 + 30);
        if (minimumHeight() != total) setMinimumHeight(total);
    }

    // ---- drawing ----------------------------------------------------------
    void drawGroups(QPainter &p) {
        for (std::size_t gi = 0; gi < groups_.size(); ++gi) {
            const Group &g = groups_[gi];
            if (g.name.isEmpty()) continue;
            const QColor acc = accent(g.color);
            QColor edge = acc;
            edge.setAlpha(110);
            p.setPen(QPen(edge, 1.4, Qt::DashLine));
            p.setBrush(QColor(255, 255, 255, 150));
            p.drawRoundedRect(g.rect, 20, 20);

            p.setPen(Qt::NoPen);
            p.setBrush(acc);
            p.drawRoundedRect(QRectF(g.rect.left() + 16, g.rect.top() + 12, 4, 16), 2, 2);

            p.setFont(mk(10, true, 1.2));
            p.setPen(acc.darker(130));
            p.drawText(QRectF(g.rect.left() + 28, g.rect.top() + 10, g.rect.width() - 120, 20),
                       Qt::AlignVCenter | Qt::AlignLeft, g.name.toUpper());

            int done = 0, bad = 0;
            const int total = g.last - g.first + 1;
            for (int k = g.first; k <= g.last; ++k) {
                const int st = (*steps_)[static_cast<std::size_t>(k)].state;
                if (st == TS_Passed) ++done;
                if (st == TS_Failed) { ++done; ++bad; }
            }
            QColor cc("#69758f");
            if (bad) cc = QColor("#d94353");
            else if (done == total) cc = QColor("#159957");
            p.setPen(cc);
            p.drawText(QRectF(g.rect.right() - 90, g.rect.top() + 10, 74, 20),
                       Qt::AlignVCenter | Qt::AlignRight, QString("%1 / %2").arg(done).arg(total));
        }
    }

    void drawEdges(QPainter &p, double t) {
        const int n = static_cast<int>(steps_->size());
        const QColor green("#159957"), red("#d94353"), grey("#b7c2d6"), blue("#0a84ff");
        for (int k = 0; k <= n; ++k) {
            const QPointF a = (k == 0)
                ? QPointF(cx_, startY_ + 11)
                : QPointF((*steps_)[static_cast<std::size_t>(k - 1)].rect.center().x(),
                          (*steps_)[static_cast<std::size_t>(k - 1)].rect.bottom());
            const QPointF b = (k == n)
                ? QPointF(cx_, endY_ - 14)
                : QPointF((*steps_)[static_cast<std::size_t>(k)].rect.center().x(),
                          (*steps_)[static_cast<std::size_t>(k)].rect.top());

            QColor col = grey;
            if (k == 0) {
                if (n > 0 && (*steps_)[0].state != TS_Pending) col = green;
            } else {
                const int ps = (*steps_)[static_cast<std::size_t>(k - 1)].state;
                if (ps == TS_Passed) col = green;
                else if (ps == TS_Failed) col = red;
            }
            const bool run = (k < n && running_ == k);
            const bool trav = (travelTo_ == k);
            const QPointF lineEnd = b - QPointF(0, 6);

            p.setPen(QPen(col, 2.5, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(a, lineEnd);

            if (run) {
                QPen dp(blue, 3.5, Qt::SolidLine, Qt::FlatCap);
                dp.setDashPattern(QVector<qreal>() << 1.2 << 2.2);
                dp.setDashOffset(-t * 14);
                p.setPen(dp);
                p.drawLine(a, lineEnd);
            }
            if (trav) {
                double prog = std::min(1.0, travelClock_.elapsed() / double(travelMs_));
                prog = prog * prog * (3 - 2 * prog); // smoothstep
                const QPointF pos = a + (lineEnd - a) * prog;
                p.setPen(QPen(blue, 3, Qt::SolidLine, Qt::RoundCap));
                p.drawLine(a, pos);
                QRadialGradient glow(pos, 14);
                glow.setColorAt(0, QColor(10, 132, 255, 150));
                glow.setColorAt(1, QColor(10, 132, 255, 0));
                p.setPen(Qt::NoPen);
                p.setBrush(glow);
                p.drawEllipse(pos, 14, 14);
                p.setBrush(Qt::white);
                p.drawEllipse(pos, 6.5, 6.5);
                p.setBrush(blue);
                p.drawEllipse(pos, 4.5, 4.5);
            }

            QPolygonF ah;
            ah << b << b + QPointF(-6.5, -10) << b + QPointF(6.5, -10);
            p.setPen(Qt::NoPen);
            p.setBrush((run || trav) ? blue : col);
            p.drawPolygon(ah);
        }
    }

    void drawTerminals(QPainter &p) {
        const QColor ink("#1b2333");
        // start node: filled circle
        const QPointF s(cx_, startY_);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(30, 50, 90, 28));
        p.drawEllipse(s + QPointF(0, 2), 13, 13);
        p.setBrush(ink);
        p.drawEllipse(s, 11, 11);
        p.setFont(mk(10, true, 1.2));
        p.setPen(QColor("#69758f"));
        p.drawText(QRectF(cx_ + 22, startY_ - 10, 260, 20), Qt::AlignVCenter | Qt::AlignLeft, "START");

        // end node: bullseye, coloured by the outcome
        int done = 0, bad = 0;
        const int n = static_cast<int>(steps_->size());
        for (int i = 0; i < n; ++i) {
            const int st = (*steps_)[static_cast<std::size_t>(i)].state;
            if (st == TS_Passed) ++done;
            if (st == TS_Failed) { ++done; ++bad; }
        }
        const bool finished = (done == n && n > 0);
        QColor c("#8793a5");
        QString label = "END";
        if (finished) {
            c = bad ? QColor("#d94353") : QColor("#159957");
            label = bad ? QString("FINISHED · %1 FAILED").arg(bad) : QString("ALL PASSED");
        }
        const QPointF e(cx_, endY_);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(30, 50, 90, 28));
        p.drawEllipse(e + QPointF(0, 2), 15, 15);
        p.setBrush(QColor("#f8faff"));
        p.setPen(QPen(c, 2.5));
        p.drawEllipse(e, 13, 13);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(e, 7.5, 7.5);
        p.setFont(mk(10, true, 1.2));
        p.setPen(c);
        p.drawText(QRectF(cx_ + 24, endY_ - 10, 260, 20), Qt::AlignVCenter | Qt::AlignLeft, label);
    }

    void drawNode(QPainter &p, int i, double t) {
        const TheatreStep &s = (*steps_)[static_cast<std::size_t>(i)];
        const QRectF r = s.rect;
        const bool hovered = (hover_ == i);

        QColor border("#cfd6e6"), fill("#ffffff"), badge("#8793a5");
        QString word = "WAITING";
        double bw = 1.5;
        switch (s.state) {
        case TS_Running: border = QColor("#0a84ff"); fill = QColor("#eef6ff"); badge = border; word = "RUNNING"; bw = 2.5; break;
        case TS_Passed:  border = QColor("#159957"); fill = QColor("#f0faf5"); badge = border; word = "PASSED"; break;
        case TS_Failed:  border = QColor("#d94353"); fill = QColor("#fff3f4"); badge = border; word = "FAILED"; break;
        default: if (hovered) border = QColor("#8fb8ee"); break;
        }

        // soft drop shadow
        p.setPen(Qt::NoPen);
        for (int k = 7; k >= 1; --k) {
            p.setBrush(QColor(30, 50, 90, hovered ? 11 : 7));
            p.drawRoundedRect(r.adjusted(-k, -k + 4, k, k + 4), 14 + k, 14 + k);
        }

        // pulsing glow for the running node
        if (s.state == TS_Running) {
            const double pulse = 0.5 + 0.5 * std::sin(t * 6.0);
            for (int k = 1; k <= 4; ++k) {
                p.setPen(QPen(QColor(10, 132, 255, int((70 - k * 14) * (0.4 + 0.6 * pulse))), 2));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(r.adjusted(-k * 2.2, -k * 2.2, k * 2.2, k * 2.2), 14 + k * 2.2, 14 + k * 2.2);
            }
        }

        // body
        p.setPen(QPen(border, bw));
        p.setBrush(fill);
        p.drawRoundedRect(r, 14, 14);

        // badge
        const QPointF c(r.left() + 30, r.top() + std::min(r.height() / 2, 28.0));
        p.setPen(Qt::NoPen);
        if (s.state == TS_Running) {
            p.setPen(QPen(QColor(10, 132, 255, 55), 3));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(c, 19, 19);
            QPen sp(QColor("#0a84ff"), 3, Qt::SolidLine, Qt::RoundCap);
            p.setPen(sp);
            p.drawArc(QRectF(c.x() - 19, c.y() - 19, 38, 38), int(-t * 540 * 16), 100 * 16);
            p.setPen(Qt::NoPen);
        }
        p.setBrush(badge);
        p.drawEllipse(c, 15, 15);
        if (s.state == TS_Passed || s.state == TS_Failed) {
            QPen wp(Qt::white, 2.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(wp);
            p.setBrush(Qt::NoBrush);
            if (s.state == TS_Passed) {
                QPainterPath ck;
                ck.moveTo(c.x() - 5.5, c.y() + 0.5);
                ck.lineTo(c.x() - 1.5, c.y() + 4.5);
                ck.lineTo(c.x() + 6, c.y() - 4.5);
                p.drawPath(ck);
            } else {
                p.drawLine(QPointF(c.x() - 4.5, c.y() - 4.5), QPointF(c.x() + 4.5, c.y() + 4.5));
                p.drawLine(QPointF(c.x() - 4.5, c.y() + 4.5), QPointF(c.x() + 4.5, c.y() - 4.5));
            }
        } else {
            p.setFont(mk(13, true));
            p.setPen(Qt::white);
            p.drawText(QRectF(c.x() - 15, c.y() - 15, 30, 30), Qt::AlignCenter, QString::number(i + 1));
        }

        // text column
        const double textX = r.left() + 58;
        const double titleW = r.width() - 58 - 14 - 76;
        const double bodyW = r.width() - 58 - 14;
        double content = s.titleH;
        if (s.detailH > 0) content += 3 + s.detailH;
        if (s.msgH > 0) content += 5 + s.msgH;
        double y = r.top() + (r.height() - content) / 2.0;

        p.setFont(mk(14, true));
        p.setPen(QColor("#1b2333"));
        p.drawText(QRectF(textX, y, titleW, s.titleH), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, s.title);

        // status pill text (top right of the node)
        p.setFont(mk(10, true, 0.8));
        p.setPen(s.state == TS_Pending ? QColor("#a3adc0") : badge);
        p.drawText(QRectF(r.right() - 14 - 74, y + 1, 74, 16), Qt::AlignRight | Qt::AlignVCenter, word);

        y += s.titleH;
        if (s.detailH > 0) {
            y += 3;
            p.setFont(mk(11));
            p.setPen(QColor("#69758f"));
            p.drawText(QRectF(textX, y, bodyW, s.detailH), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, s.detail);
            y += s.detailH;
        }
        if (s.msgH > 0) {
            y += 5;
            p.setFont(mk(11));
            p.setPen(QColor("#d94353"));
            p.drawText(QRectF(textX, y, bodyW, s.msgH), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, s.message);
        }
    }

    std::vector<TheatreStep> *steps_;
    std::vector<Group> groups_;
    QTimer tick_;
    QElapsedTimer clock_, travelClock_;
    QPixmap dots_;
    double cx_ = 0, nodeW_ = 400, startY_ = 0, endY_ = 0;
    int running_ = -1, travelTo_ = -1, travelMs_ = 1, hover_ = -1;
};

// ---------------------------------------------------------------------------
// The window: toolbar, progress bar, scrolling diagram and log.
// ---------------------------------------------------------------------------
class TestTheatre : public QWidget {
public:
    typedef std::function<void()> Body;
    enum Status { Pending = TS_Pending, Running = TS_Running, Passed = TS_Passed, Failed = TS_Failed };

    explicit TestTheatre(const QString &title = "Workflow tests") : QWidget(0), title_(title) {
        setWindowTitle(title);
        resize(640, 900);
        setStyleSheet(
            "QWidget{background:#f3f5fa;color:#1b2333;font-family:'Segoe UI';font-size:13px;}"
            "QPushButton,QComboBox{background:white;border:1px solid #cfd6e6;border-radius:7px;padding:6px 12px;}"
            "QPushButton:hover{border-color:#0a84ff;color:#0a84ff;}"
            "QPlainTextEdit{background:white;border:1px solid #dce3ef;border-radius:7px;}");
    }

    // ---- API used by main() -------------------------------------------------
    /** Start a new group; every following step() belongs to it. */
    void section(const QString &name) { section_ = name; }

    /** Register a step. The body runs when the step becomes the highlighted one. */
    void step(const QString &title, const QString &detail, Body body) {
        TheatreStep s;
        s.section = section_;
        s.title = title;
        s.detail = detail;
        s.body = body;
        steps_.push_back(s);
    }

    /** Write a line to the log panel (callable from inside a step body). */
    void say(const QString &msg) { log(msg); }

    /** Fail the current step if the condition is false. */
    static void check(bool ok, const QString &what) {
        if (!ok) throw TestFailure(what.toStdString());
    }

    /** Fail the current step unless f() throws. */
    static void expectThrows(const Body &f, const QString &what) {
        bool threw = false;
        try { f(); } catch (...) { threw = true; }
        if (!threw) throw TestFailure(("Expected an exception: " + what).toStdString());
    }

    /** Build the window from the registered steps and show it. */
    void run() {
        build();
        show();
    }

private:
    void build() {
        QVBoxLayout *page = new QVBoxLayout(this);
        page->setContentsMargins(14, 12, 14, 12);
        page->setSpacing(10);

        QHBoxLayout *head = new QHBoxLayout;
        QLabel *t = new QLabel(title_);
        t->setStyleSheet("font-size:18px;font-weight:800;color:#0878d1;");
        summary_ = new QLabel;
        head->addWidget(t);
        head->addStretch();
        head->addWidget(summary_);
        page->addLayout(head);

        progress_ = new QProgressBar;
        progress_->setTextVisible(false);
        progress_->setFixedHeight(6);
        page->addWidget(progress_);

        QHBoxLayout *bar = new QHBoxLayout;
        runBtn_ = new QPushButton("▶ Run all");
        stepBtn_ = new QPushButton("Step ▶");
        restartBtn_ = new QPushButton("⏮ Restart");
        speed_ = new QComboBox;
        speed_->addItem("Slow", 1600);
        speed_->addItem("Normal", 800);
        speed_->addItem("Fast", 300);
        speed_->setCurrentIndex(1);
        bar->addWidget(runBtn_);
        bar->addWidget(stepBtn_);
        bar->addWidget(restartBtn_);
        bar->addStretch();
        bar->addWidget(new QLabel("Speed"));
        bar->addWidget(speed_);
        page->addLayout(bar);

        QLabel *legend = new QLabel(
            "<span style='color:#8793a5'>●</span> waiting &nbsp;&nbsp;"
            "<span style='color:#0a84ff'>●</span> running &nbsp;&nbsp;"
            "<span style='color:#159957'>●</span> passed &nbsp;&nbsp;"
            "<span style='color:#d94353'>●</span> failed");
        legend->setStyleSheet("color:#69758f;font-size:11px;");
        page->addWidget(legend);

        canvas_ = new TheatreCanvas(&steps_);
        scroll_ = new QScrollArea;
        scroll_->setWidgetResizable(true);
        scroll_->setFrameShape(QFrame::NoFrame);
        scroll_->setStyleSheet(
            "QScrollArea{border:1px solid #dce3ef;background:#f8faff;}"
            "QScrollBar:vertical{background:transparent;width:10px;margin:2px;}"
            "QScrollBar::handle:vertical{background:#c3cde0;border-radius:4px;min-height:30px;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}");
        scroll_->setWidget(canvas_);
        page->addWidget(scroll_, 1);

        log_ = new QPlainTextEdit;
        log_->setReadOnly(true);
        log_->setMaximumHeight(110);
        page->addWidget(log_);

        connect(runBtn_, &QPushButton::clicked, this, [this] {
            if (auto_) {
                auto_ = false;
                runBtn_->setText("▶ Run all");
                if (!busy_) canvas_->stopTravel();
                return;
            }
            if (index_ >= static_cast<int>(steps_.size())) resetAll();
            auto_ = true;
            runBtn_->setText("Ⅱ Pause");
            if (!busy_) startStep();
        });
        connect(stepBtn_, &QPushButton::clicked, this, [this] {
            if (busy_) return;
            auto_ = false;
            runBtn_->setText("▶ Run all");
            if (index_ >= static_cast<int>(steps_.size())) resetAll();
            startStep();
        });
        connect(restartBtn_, &QPushButton::clicked, this, [this] { resetAll(); });

        resetAll();
    }

    void setStatus(std::size_t i, TheatreState st, const QString &msg) {
        steps_[i].state = st;
        steps_[i].message = msg;
        canvas_->relayout();
    }

    int delay() const { return speed_->currentData().toInt(); }

    void startStep() {
        if (index_ >= static_cast<int>(steps_.size())) { finish(); return; }
        busy_ = true;
        const std::size_t i = static_cast<std::size_t>(index_);
        canvas_->stopTravel();
        setStatus(i, TS_Running, QString());
        canvas_->setRunning(index_);
        const QRectF r = canvas_->nodeRect(index_);
        scroll_->ensureVisible(static_cast<int>(r.center().x()), static_cast<int>(r.center().y()), 0,
                               static_cast<int>(r.height() / 2 + 110));
        log("▶ " + steps_[i].title);
        const int g = gen_;
        QTimer::singleShot(delay() / 2, this, [this, g] { if (g == gen_) finishStep(); });
    }

    void finishStep() {
        const std::size_t i = static_cast<std::size_t>(index_);
        bool ok = true;
        QString err;
        try {
            steps_[i].body();
        } catch (const std::exception &e) {
            ok = false;
            err = QString::fromUtf8(e.what());
        } catch (...) {
            ok = false;
            err = "Unknown exception";
        }
        canvas_->setRunning(-1);
        setStatus(i, ok ? TS_Passed : TS_Failed, err);
        log(ok ? "   ✔ passed" : "   ✘ failed: " + err);
        ++index_;
        busy_ = false;
        updateSummary();
        if (index_ >= static_cast<int>(steps_.size())) { finish(); return; }
        if (auto_) {
            const int g = gen_;
            const int ms = delay() / 2;
            canvas_->startTravel(index_, ms);
            QTimer::singleShot(ms, this, [this, g] { if (g == gen_ && auto_) startStep(); });
        }
    }

    void finish() {
        auto_ = false;
        busy_ = false;
        canvas_->setRunning(-1);
        canvas_->stopTravel();
        runBtn_->setText("▶ Run all");
        updateSummary();
        const QRectF r = canvas_->rect();
        scroll_->ensureVisible(static_cast<int>(r.width() / 2), canvas_->height(), 0, 40);
        log("Run complete.");
    }

    void resetAll() {
        ++gen_;
        busy_ = false;
        auto_ = false;
        index_ = 0;
        runBtn_->setText("▶ Run all");
        log_->clear();
        for (std::size_t i = 0; i < steps_.size(); ++i) {
            steps_[i].state = TS_Pending;
            steps_[i].message.clear();
        }
        canvas_->setRunning(-1);
        canvas_->stopTravel();
        canvas_->relayout();
        updateSummary();
        scroll_->verticalScrollBar()->setValue(0);
    }

    void updateSummary() {
        int pass = 0, fail = 0;
        for (std::size_t i = 0; i < steps_.size(); ++i) {
            if (steps_[i].state == TS_Passed) ++pass;
            else if (steps_[i].state == TS_Failed) ++fail;
        }
        summary_->setText(QString("%1 passed · %2 failed · %3 total").arg(pass).arg(fail).arg(steps_.size()));
        summary_->setStyleSheet(fail ? "color:#d94353;font-weight:700;" : "color:#69758f;font-weight:700;");

        progress_->setRange(0, static_cast<int>(std::max<std::size_t>(1, steps_.size())));
        progress_->setValue(pass + fail);
        progress_->setStyleSheet(QString(
            "QProgressBar{background:#dfe5f1;border:0;border-radius:3px;}"
            "QProgressBar::chunk{border-radius:3px;background:qlineargradient(x1:0,y1:0,x2:1,y2:0,%1);}")
            .arg(fail ? "stop:0 #ff8a5c,stop:1 #d94353" : "stop:0 #0a84ff,stop:1 #35c9a0"));
    }

    void log(const QString &msg) {
        if (log_) log_->appendPlainText(QDateTime::currentDateTime().toString("HH:mm:ss") + "  " + msg);
    }

    QString title_, section_;
    std::vector<TheatreStep> steps_;
    QLabel *summary_ = 0;
    QProgressBar *progress_ = 0;
    QPushButton *runBtn_ = 0, *stepBtn_ = 0, *restartBtn_ = 0;
    QComboBox *speed_ = 0;
    QScrollArea *scroll_ = 0;
    TheatreCanvas *canvas_ = 0;
    QPlainTextEdit *log_ = 0;
    int index_ = 0;
    int gen_ = 0;
    bool busy_ = false;
    bool auto_ = false;
};