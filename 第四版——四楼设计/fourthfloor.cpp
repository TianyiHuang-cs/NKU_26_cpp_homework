#include "fourthfloor.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QFont>
#include <QVBoxLayout>

// ============================================================
// AIPuzzleDialog 实现（第一页：问AI / 开始做题）
// ============================================================
AIPuzzleDialog::AIPuzzleDialog(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
    m_dragging(false)
{
    setFixedSize(400, 260);
    setAttribute(Qt::WA_TranslucentBackground);

    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setGeometry(370, 8, 22, 22);
    m_closeBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#8B7355;"
        "  border:none; font-size:14px; font-weight:bold; }"
        "QPushButton:hover { color:#cc4444; }"
        );
    connect(m_closeBtn, &QPushButton::clicked, this, [this](){
        hide(); emit dialogClosed();
    });

    m_aiBtn = new QPushButton("问问 AI", this);
    m_aiBtn->setGeometry(100, 120, 200, 48);
    m_aiBtn->setStyleSheet(
        "QPushButton { background:#2a1a08; color:#c8a050;"
        "  border:2px solid #6a5020; font-size:14px; font-weight:bold; }"
        "QPushButton:hover { background:#4a3010; color:#f0c060; }"
        );
    connect(m_aiBtn, &QPushButton::clicked, this, [this](){
        hide(); emit requestAI();
    });

    m_quizBtn = new QPushButton("开始做题", this);
    m_quizBtn->setGeometry(100, 185, 200, 48);
    m_quizBtn->setStyleSheet(
        "QPushButton { background:#2a1a08; color:#c8a050;"
        "  border:2px solid #6a5020; font-size:14px; font-weight:bold; }"
        "QPushButton:hover { background:#4a3010; color:#f0c060; }"
        );
    connect(m_quizBtn, &QPushButton::clicked, this, [this](){
        hide(); emit requestQuiz();
    });
}

void AIPuzzleDialog::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                                   const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

void AIPuzzleDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    int W = width(), H = height();
    p.fillRect(4, 4, W-8, H-8, QColor(235, 220, 180, 248));

    QLinearGradient el(0,0,16,0); el.setColorAt(0,QColor(160,130,70,140)); el.setColorAt(1,Qt::transparent);
    p.fillRect(0,0,16,H,el);
    QLinearGradient er(W-16,0,W,0); er.setColorAt(0,Qt::transparent); er.setColorAt(1,QColor(160,130,70,140));
    p.fillRect(W-16,0,16,H,er);

    p.setPen(QPen(QColor(100,75,35), 2)); p.drawRect(3,3,W-7,H-7);
    p.setPen(QPen(QColor(145,110,50), 1)); p.drawRect(6,6,W-13,H-13);
    p.fillRect(8,8,W-16,50, QColor(180,148,80,120));
    p.setPen(QPen(QColor(120,88,38),1)); p.drawLine(8,58,W-9,58);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont tf("Georgia",11,QFont::Bold); tf.setItalic(true);
    p.setFont(tf); p.setPen(QColor(55,35,10));
    p.drawText(0,8,W-28,50, Qt::AlignCenter, "— 电 脑 —");

    QFont cf("Courier New",10); p.setFont(cf); p.setPen(QColor(60,38,12));
    p.drawText(20,68,W-40,44, Qt::AlignCenter|Qt::TextWordWrap, "现在该做什么呢？");
    p.setRenderHint(QPainter::Antialiasing, false);
}

void AIPuzzleDialog::mousePressEvent(QMouseEvent *e) {
    if (e->button()==Qt::LeftButton && e->pos().y()<60) {
        m_dragging=true; m_dragOffset=e->globalPos()-frameGeometry().topLeft();
    }
}
void AIPuzzleDialog::mouseMoveEvent(QMouseEvent *e) { if(m_dragging) move(e->globalPos()-m_dragOffset); }
void AIPuzzleDialog::mouseReleaseEvent(QMouseEvent *) { m_dragging=false; }

// ============================================================
// AIChatDialog 实现
// ============================================================
AIChatDialog::AIChatDialog(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
    m_dragging(false), m_sent(false),
    m_showFirstLine(false), m_showSecondLine(false)
{
    setFixedSize(440, 380);
    setAttribute(Qt::WA_TranslucentBackground);

    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setGeometry(410, 8, 22, 22);
    m_closeBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#8B7355;"
        "  border:none; font-size:14px; font-weight:bold; }"
        "QPushButton:hover { color:#cc4444; }"
        );
    connect(m_closeBtn, &QPushButton::clicked, this, [this](){
        m_thinkTimer->stop(); hide(); emit dialogClosed();
    });

    // 发送按钮（下方）
    m_sendBtn = new QPushButton("图书馆闭馆了，我被困了，怎么办？", this);
    m_sendBtn->setGeometry(20, 318, 400, 42);
    m_sendBtn->setStyleSheet(
        "QPushButton { background:#2a1a08; color:#c8a050;"
        "  border:2px solid #6a5020; font-size:11px; font-weight:bold; }"
        "QPushButton:hover { background:#4a3010; }"
        "QPushButton:disabled { background:#1a0e04; color:#6a5020; }"
        );
    connect(m_sendBtn, &QPushButton::clicked, this, &AIChatDialog::onSendMessage);

    // 回复标签
    m_replyLabel = new QLabel(this);
    m_replyLabel->setGeometry(20, 180, 400, 130);
    m_replyLabel->setWordWrap(true);
    m_replyLabel->setStyleSheet(
        "QLabel { background:rgba(20,12,5,180); color:#c8a050;"
        "  border:1px solid #6a5020; font-size:11px; padding:8px; }"
        );
    m_replyLabel->setText("");

    // 定时器（3秒后显示第二句）
    m_thinkTimer = new QTimer(this);
    m_thinkTimer->setSingleShot(true);
    connect(m_thinkTimer, &QTimer::timeout, this, &AIChatDialog::onThinkingTimer);
}

void AIChatDialog::onSendMessage()
{
    if (m_sent) return;
    m_sent = true;
    m_sendBtn->setEnabled(false);
    m_showFirstLine = true;
    m_replyLabel->setText("思考中……用户被困，需要先提供情绪价值……");
    // 3秒后显示第二句
    m_thinkTimer->start(3000);
    update();
}

void AIChatDialog::onThinkingTimer()
{
    m_showSecondLine = true;
    m_replyLabel->setText(
        "思考中……用户被困，需要先提供情绪价值……\n\n"
        "太棒啦！刚好可以借此机会探索一下图书馆！"
        );
    update();
}

void AIChatDialog::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                                 const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w-1, h-1);
}

void AIChatDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    int W = width(), H = height();
    p.fillRect(4,4,W-8,H-8, QColor(20,12,5,240));

    p.setPen(QPen(QColor(100,75,35),2)); p.drawRect(3,3,W-7,H-7);
    p.setPen(QPen(QColor(145,110,50),1)); p.drawRect(6,6,W-13,H-13);
    p.fillRect(8,8,W-16,46, QColor(35,20,7));
    p.setPen(QPen(QColor(90,58,20),1)); p.drawLine(8,54,W-9,54);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont tf("Courier New",12,QFont::Bold); p.setFont(tf);
    p.setPen(QColor(195,150,55));
    p.drawText(0,8,W,46, Qt::AlignCenter, "— AI 对 话 —");

    // 模拟对话框（三个矩形代替气泡）
    QColor bubbleBg(35,20,7,200);
    QColor bubbleBorder(80,55,20);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(20, 62, W-40, 30, QColor(45,28,8,160));
    p.setPen(bubbleBorder); p.drawRect(20,62,W-41,29);
    p.fillRect(20, 100, W-40, 30, QColor(45,28,8,160));
    p.setPen(bubbleBorder); p.drawRect(20,100,W-41,29);
    p.fillRect(20, 138, W-40, 30, QColor(45,28,8,160));
    p.setPen(bubbleBorder); p.drawRect(20,138,W-41,29);

    QFont sf("Courier New",9); p.setFont(sf);
    p.setPen(QColor(100,80,35));
    p.drawText(28,62,W-56,30, Qt::AlignVCenter, "（等待用户输入…）");
    p.setRenderHint(QPainter::Antialiasing, false);
}

void AIChatDialog::mousePressEvent(QMouseEvent *e) {
    if (e->button()==Qt::LeftButton && e->pos().y()<58) {
        m_dragging=true; m_dragOffset=e->globalPos()-frameGeometry().topLeft();
    }
}
void AIChatDialog::mouseMoveEvent(QMouseEvent *e) { if(m_dragging) move(e->globalPos()-m_dragOffset); }
void AIChatDialog::mouseReleaseEvent(QMouseEvent *) { m_dragging=false; }

// ============================================================
// QuizDialog 实现（手写排序算法题）
// ============================================================
QuizDialog::QuizDialog(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
    m_clickCount(0), m_submitted(false), m_dragging(false)
{
    setFixedSize(440, 420);
    setAttribute(Qt::WA_TranslucentBackground);

    for (int i=0; i<3; i++) m_clickOrder[i] = 0;

    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setGeometry(410, 8, 22, 22);
    m_closeBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#8B7355;"
        "  border:none; font-size:14px; }"
        "QPushButton:hover { color:#cc4444; }"
        );
    connect(m_closeBtn, &QPushButton::clicked, this, [this](){ hide(); emit dialogClosed(); });

    // 三个选项按钮（竖向排列，左侧有方框显示序号）
    const char *labels[3] = {
        "  跑创高",
        "  完成C++大作业",
        "  新芽计划每周一晚上打卡"
    };
    int optY[3] = {195, 248, 301};
    for (int i=0; i<3; i++) {
        m_opts[i] = new QPushButton(labels[i], this);
        m_opts[i]->setGeometry(56, optY[i], 360, 40);
        m_opts[i]->setStyleSheet(
            "QPushButton { background:#2a1a08; color:#c8a050;"
            "  border:2px solid #6a5020; font-size:12px; text-align:left; padding-left:8px; }"
            "QPushButton:hover { background:#4a3010; }"
            );
        int idx = i;
        connect(m_opts[i], &QPushButton::clicked, this, [this,idx](){ onOptionClicked(idx); });
    }

    // 提交按钮
    m_submitBtn = new QPushButton("提 交", this);
    m_submitBtn->setGeometry(160, 360, 120, 38);
    m_submitBtn->setStyleSheet(
        "QPushButton { background:#3a2a10; color:#c8a050;"
        "  border:2px solid #6a5020; font-size:13px; font-weight:bold; }"
        "QPushButton:hover { background:#5a4018; }"
        );
    connect(m_submitBtn, &QPushButton::clicked, this, &QuizDialog::onSubmit);

    // 结果标签
    m_resultLabel = new QLabel("", this);
    m_resultLabel->setGeometry(20, 360, 400, 38);
    m_resultLabel->setAlignment(Qt::AlignCenter);
    m_resultLabel->setStyleSheet(
        "QLabel { color:#f0c060; font-size:12px; font-weight:bold; background:transparent; }"
        );
    m_resultLabel->hide();
}

void QuizDialog::onOptionClicked(int idx)
{
    if (m_submitted) return;
    if (m_clickOrder[idx] != 0) {
        // 取消选择：重排序号
        int removed = m_clickOrder[idx];
        m_clickOrder[idx] = 0;
        for (int i=0; i<3; i++) {
            if (m_clickOrder[i] > removed) m_clickOrder[i]--;
        }
        m_clickCount--;
    } else {
        m_clickCount++;
        m_clickOrder[idx] = m_clickCount;
    }
    update();
}

void QuizDialog::onSubmit()
{
    if (m_submitted) return;
    //m_submitted = true;
    //m_submitBtn->hide();
    m_resultLabel->show();

    // 答案校验：
    // 正确答案1：第三个按钮(idx=2)→第一个(idx=0)→第二个(idx=1)
    // 正确答案2：第三个(idx=2)→第一个(idx=0)，不选第二个
    bool correct = false;
    // 情况1：顺序 2,0,1（全选）
    if (m_clickOrder[2]==1 && m_clickOrder[0]==2 && m_clickOrder[1]==3) {
        correct = true;
    }
    // 情况2：顺序 0,2，不选1（只选两个）
    if (m_clickOrder[0]==2 && m_clickOrder[2]==1 && m_clickOrder[1]==0) {
        correct = true;
    }

    if (correct) {
        m_resultLabel->setText("bingo~因为我的大作业已经写完啦~（^-^）~");
        m_submitted = true;
        m_submitBtn->hide();
        emit quizCorrect(10); // 时间倒流10分钟
    } else {
        int labelY = static_cast<int>(height() * 0.35);
        int labelH = 60;

        m_resultLabel->setAlignment(Qt::AlignCenter);
        m_resultLabel->setGeometry(0, labelY, width(), labelH);

        m_resultLabel->setStyleSheet(
            "QLabel { color:#ff5555; font-size:16px; font-weight:bold; background:transparent; }"
            );

        m_resultLabel->setText("不要分心哦。只关注未完成的事情。");
        m_resultLabel->show();

        QTimer::singleShot(3000, this, [this]() {
            m_resultLabel->hide();
        });

        emit quizWrong();
    }
}

void QuizDialog::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                               const QColor &fill, const QColor &border)
{
    p.fillRect(x,y,w,h,fill); p.setPen(border); p.drawRect(x,y,w-1,h-1);
}

void QuizDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    int W=width(), H=height();
    p.fillRect(4,4,W-8,H-8, QColor(235,220,180,248));
    QLinearGradient el(0,0,16,0); el.setColorAt(0,QColor(160,130,70,140)); el.setColorAt(1,Qt::transparent);
    p.fillRect(0,0,16,H,el);
    p.setPen(QPen(QColor(100,75,35),2)); p.drawRect(3,3,W-7,H-7);
    p.setPen(QPen(QColor(145,110,50),1)); p.drawRect(6,6,W-13,H-13);
    p.fillRect(8,8,W-16,46, QColor(180,148,80,120));
    p.setPen(QPen(QColor(120,88,38),1)); p.drawLine(8,54,W-9,54);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont tf("Georgia",11,QFont::Bold); tf.setItalic(true);
    p.setFont(tf); p.setPen(QColor(55,35,10));
    p.drawText(0,8,W-28,46, Qt::AlignCenter, "— 手写排序算法题 —");

    QFont cf("Courier New",10); p.setFont(cf); p.setPen(QColor(55,35,12));
    p.drawText(16,62,W-32,130, Qt::AlignLeft|Qt::TextWordWrap,
               "题干：小明有3个ddl，他为此深感焦虑，而浪费了很多时间，"
               "请你手写排序，帮他妥善安排，节省时间。");

    // 绘制各选项前方的序号方框
    int optY[3] = {195, 248, 301};
    for (int i=0; i<3; i++) {
        // 方框
        p.setPen(QPen(QColor(100,75,35),2));
        p.fillRect(20, optY[i]+8, 28, 28, QColor(220,200,150,200));
        p.drawRect(20, optY[i]+8, 28, 28);
        // 序号
        if (m_clickOrder[i] > 0) {
            p.setFont(QFont("Courier New",12,QFont::Bold));
            p.setPen(QColor(80,45,10));
            p.drawText(20, optY[i]+8, 28, 28, Qt::AlignCenter,
                       QString::number(m_clickOrder[i]));
        }
    }
    p.setRenderHint(QPainter::Antialiasing, false);
}

void QuizDialog::mousePressEvent(QMouseEvent *e) {
    if (e->button()==Qt::LeftButton && e->pos().y()<58) {
        m_dragging=true; m_dragOffset=e->globalPos()-frameGeometry().topLeft();
    }
}
void QuizDialog::mouseMoveEvent(QMouseEvent *e) { if(m_dragging) move(e->globalPos()-m_dragOffset); }
void QuizDialog::mouseReleaseEvent(QMouseEvent *) { m_dragging=false; }

// ============================================================
// DeskItemsDialog 实现（6个可拾取物品）
// ============================================================
DeskItemsDialog::DeskItemsDialog(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
    m_dragging(false)
{
    setFixedSize(460, 340);
    setAttribute(Qt::WA_TranslucentBackground);

    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setGeometry(430, 8, 22, 22);
    m_closeBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#8B7355;"
        "  border:none; font-size:14px; }"
        "QPushButton:hover { color:#cc4444; }"
        );
    connect(m_closeBtn, &QPushButton::clicked, this, [this](){ hide(); emit dialogClosed(); });

    // 6个物品，2行3列排列
    // *** 物品数据（图片路径需在 .qrc 中配置）***
    struct ItemData { int id; const char *name; const char *path; } data[6] = {
                 {101, "美味的猫条",       ":/images/catfood.png"},
                 {102, "‘出去玩玩’的便签",   ":/images/note1.png"},
                 {103, "‘按时吃饭’的便签",   ":/images/note2.png"},
                 {104, "‘早点睡觉’的便签",   ":/images/note3.png"},
                 {105, "静音的键盘",       ":/images/keyboard.png"},
                 {106, "东野圭吾小说",":/images/book.png"},
                 };

    // 布局：3列，起始 x=30，间距 140；2行，起始 y=80，间距 120
    int cols = 3;
    for (int i=0; i<6; i++) {
        m_items[i].id       = data[i].id;
        m_items[i].name     = data[i].name;
        m_items[i].imagePath= data[i].path;
        m_items[i].pickedUp = false;
        int col = i % cols;
        int row = i / cols;
        // *** 物品格子位置：微调请修改起始坐标和间距 ***
        m_items[i].rect = QRect(30 + col * 132, 80 + row * 120, 110, 100);
    }
}

void DeskItemsDialog::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                                    const QColor &fill, const QColor &border)
{
    p.fillRect(x,y,w,h,fill); p.setPen(border); p.drawRect(x,y,w-1,h-1);
}

void DeskItemsDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    int W=width(), H=height();
    p.fillRect(4,4,W-8,H-8, QColor(20,12,5,245));
    p.setPen(QPen(QColor(100,75,35),2)); p.drawRect(3,3,W-7,H-7);
    p.setPen(QPen(QColor(145,110,50),1)); p.drawRect(6,6,W-13,H-13);
    p.fillRect(8,8,W-16,46, QColor(35,20,7));
    p.setPen(QPen(QColor(90,58,20),1)); p.drawLine(8,54,W-9,54);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont tf("Courier New",12,QFont::Bold); p.setFont(tf);
    p.setPen(QColor(195,150,55));
    p.drawText(0,8,W,46, Qt::AlignCenter, "— 凌 乱 的 桌 面 —");

    QFont nf("Courier New",8); p.setFont(nf);
    p.setRenderHint(QPainter::Antialiasing, false);

    for (int i=0; i<6; i++) {
        const DeskItem &it = m_items[i];
        if (it.pickedUp) {
            // 已拾取：显示空格
            drawPixelRect(p, it.rect.x(), it.rect.y(), it.rect.width(), it.rect.height(),
                          QColor(15,8,3,180), QColor(55,35,12));
            p.setPen(QColor(60,40,12));
            p.drawText(it.rect, Qt::AlignCenter, "（已拾取）");
        } else {
            // 未拾取：显示物品
            drawPixelRect(p, it.rect.x(), it.rect.y(), it.rect.width(), it.rect.height(),
                          QColor(30,18,6,200), QColor(90,62,22));
            // 尝试加载图片
            QPixmap icon(it.imagePath);
            if (!icon.isNull()) {
                QPixmap scaled = icon.scaled(64,64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                p.drawPixmap(
                    it.rect.x() + (it.rect.width()  - scaled.width())  / 2,
                    it.rect.y() + 4,
                    scaled);
            } else {
                // 图片不存在时显示名字占位
                p.setPen(QColor(160,120,50));
                p.drawText(it.rect.x(), it.rect.y(), it.rect.width(), it.rect.height()-20,
                           Qt::AlignCenter, it.name);
            }
            // 物品名称
            p.setPen(QColor(160,130,60));
            p.drawText(it.rect.x(), it.rect.y()+it.rect.height()-18,
                       it.rect.width(), 18, Qt::AlignCenter, it.name);

            // 点击提示
            p.setPen(QColor(120,90,35,160));
            QFont hf("Courier New",7); p.setFont(hf);
            p.drawText(it.rect.x(), it.rect.y()+it.rect.height()-6,
                       it.rect.width(), 10, Qt::AlignCenter, "点击放入背包");
            p.setFont(nf);
        }
    }
}

void DeskItemsDialog::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        if (e->pos().y() < 58) {
            m_dragging=true; m_dragOffset=e->globalPos()-frameGeometry().topLeft();
            return;
        }
        // 检测点击物品
        for (int i=0; i<6; i++) {
            if (!m_items[i].pickedUp && m_items[i].rect.contains(e->pos())) {
                m_items[i].pickedUp = true;
                QPixmap icon(m_items[i].imagePath);
                if (icon.isNull()) {
                    // 没有图片时生成色块占位
                    icon = QPixmap(64,64);
                    icon.fill(QColor(160,120,50));
                }
                emit itemPickedUp(m_items[i].id, m_items[i].name, icon);
                update();
                break;
            }
        }
    }
}
void DeskItemsDialog::mouseMoveEvent(QMouseEvent *e) { if(m_dragging) move(e->globalPos()-m_dragOffset); }
void DeskItemsDialog::mouseReleaseEvent(QMouseEvent *) { m_dragging=false; }

// ============================================================
// FourthFloor 实现    m_groundY = static_cast<int>(windowH * 0.70f);

// ============================================================
FourthFloor::FourthFloor(QObject *parent)
    : BaseFloor(parent),
    m_nearComputer(false), m_nearLamp(false), m_nearDoor(false),
    m_keyLeft(false), m_keyRight(false),
    m_keyDown(false), m_keySpace(false),
    m_isJumping(false), m_jumpVelocity(0.0f),
    m_tick(0),
    m_lampItemsTriggered(false),
    m_quizSolved(false),
    m_transitioning(false), m_transitionAlpha(0.0f),
    m_bagWin(nullptr)
{
    // *** 背景图路径，请在 .qrc 中配置 ***
    m_bgPixmap = QPixmap(":/images/fourth_floor.png");

    // 电脑处机关弹窗（第一页）
    m_aiPuzzleDlg = new AIPuzzleDialog(nullptr);
    connect(m_aiPuzzleDlg, &AIPuzzleDialog::requestAI, this, [this](){
        showAIPuzzleDialog(); // 隐藏自己后已在 slot 里显示 AI 对话
        QScreen *scr = QApplication::primaryScreen();
        QRect sg = scr->availableGeometry();
        m_aiChatDlg->move(sg.center().x()-m_aiChatDlg->width()/2,
                          sg.center().y()-m_aiChatDlg->height()/2);
        m_aiChatDlg->show(); m_aiChatDlg->raise();
    });
    connect(m_aiPuzzleDlg, &AIPuzzleDialog::requestQuiz, this, [this](){
        QScreen *scr = QApplication::primaryScreen();
        QRect sg = scr->availableGeometry();
        m_quizDlg->move(sg.center().x()-m_quizDlg->width()/2,
                        sg.center().y()-m_quizDlg->height()/2);
        m_quizDlg->show(); m_quizDlg->raise();
    });
    connect(m_aiPuzzleDlg, &AIPuzzleDialog::dialogClosed, this, [this](){
        if (QWidget *w = qobject_cast<QWidget*>(this->parent())) w->setFocus();
    });

    // AI 对话弹窗
    m_aiChatDlg = new AIChatDialog(nullptr);
    connect(m_aiChatDlg, &AIChatDialog::dialogClosed, this, [this](){
        if (QWidget *w = qobject_cast<QWidget*>(this->parent())) w->setFocus();
    });

    // 做题弹窗
    m_quizDlg = new QuizDialog(nullptr);
    connect(m_quizDlg, &QuizDialog::quizCorrect, this, [this](int min){
        // ★ 答对：机关永久失效
        m_quizSolved = true;
        emit puzzleSolved(min);          // 时间倒流 10 分钟

        // ★ 显示时间倒流提示弹窗
        m_timeRewindDlg->setTitle("— 时光倒流 —");
        m_timeRewindDlg->setContent(
            "时间倒流 <b>10 分钟</b>。<br><br>"
            "bingo~ 因为我的大作业已经写完啦~（^-^）~<br><br>",
            ""
            );
        QScreen *scr = QApplication::primaryScreen();
        QRect sg = scr->availableGeometry();
        m_timeRewindDlg->move(sg.center().x()-m_timeRewindDlg->width()/2,
                              sg.center().y()-m_timeRewindDlg->height()/2);
        m_timeRewindDlg->show(); m_timeRewindDlg->raise();
    });
    connect(m_quizDlg, &QuizDialog::quizWrong, this, [this](){
        if (QWidget *w = qobject_cast<QWidget*>(this->parent())) w->setFocus();
    });
    connect(m_quizDlg, &QuizDialog::dialogClosed, this, [this](){
        if (QWidget *w = qobject_cast<QWidget*>(this->parent())) w->setFocus();
    });

    // 桌面物品弹窗
    m_deskDlg = new DeskItemsDialog(nullptr);
    connect(m_deskDlg, &DeskItemsDialog::itemPickedUp,
            this, [this](int id, const QString &name, const QPixmap &icon){
                if (m_bagWin) m_bagWin->addItem(id, name, icon);
            });
    connect(m_deskDlg, &DeskItemsDialog::dialogClosed, this, [this](){
        if (QWidget *w = qobject_cast<QWidget*>(this->parent())) w->setFocus();
    });

    // 时间倒流提示弹窗
    m_timeRewindDlg = new NoteDialog(nullptr);
    connect(m_timeRewindDlg, &NoteDialog::dialogClosed, this, [this](){
        if (QWidget *w = qobject_cast<QWidget*>(this->parent())) w->setFocus();
    });
}

FourthFloor::~FourthFloor()
{
    delete m_aiPuzzleDlg;
    delete m_aiChatDlg;
    delete m_quizDlg;
    delete m_deskDlg;
    delete m_timeRewindDlg;
}

// ============================================================
// onEnter
// ============================================================
void FourthFloor::onEnter(int windowW, int windowH)
{
    // *** 地面Y：约在窗口高度78%，微调请改此比例 ***
    m_groundY = static_cast<int>(windowH * 0.74f);

    // *** 出生点：椅子旁边，约在背景图左中区域
    //     x = windowW * 0.38 ≈ 304，修改此值调整出生位置 ***
    // （player.x/y 由 switchFloor 调用 onEnter 后在此设置，
    //   但 player 由 Widget 持有，这里通过信号或直接 onEnter 参数无法访问。
    //   出生点通过 BaseFloor::onEnter 注入，见下方 emit）

    // *** 各机关区域（微调请修改比例系数）***
    // 电脑处：背景图中央偏左，约 45%~60%
    m_computerRect = QRect(static_cast<int>(windowW * 0.44f), m_groundY - 120,
                           static_cast<int>(windowW * 0.16f), 100);

    // 右侧台灯：约 68%~78%
    m_lampRect = QRect(static_cast<int>(windowW * 0.68f), m_groundY - 100,
                       static_cast<int>(windowW * 0.10f), 80);

    // 最右侧门：最右边
    m_doorRect = QRect(windowW - 58, m_groundY - 110, 48, 110);

    // 重置
    m_isJumping    = false;
    m_jumpVelocity = 0.0f;
    m_tick         = 0;
    m_transitioning      = false;
    m_transitionAlpha    = 0.0f;
    m_nearComputer = m_nearLamp = m_nearDoor = false;
    m_keyLeft = m_keyRight = m_keyDown = m_keySpace = false;
    // ★ 注意：m_quizSolved 不在此重置——机关一旦破解永久失效

    // 设置出生点（通知 Widget 更新 player 位置）
    emit spawnPlayer(static_cast<int>(windowW * 0.38f), m_groundY);
}

// ============================================================
// onExit
// ============================================================
void FourthFloor::onExit()
{
    if (m_aiPuzzleDlg->isVisible()) m_aiPuzzleDlg->hide();
    if (m_aiChatDlg->isVisible())   m_aiChatDlg->hide();
    if (m_quizDlg->isVisible())     m_quizDlg->hide();
    if (m_deskDlg->isVisible())     m_deskDlg->hide();
    if (m_timeRewindDlg->isVisible()) m_timeRewindDlg->hide();
    m_keyLeft = m_keyRight = m_keyDown = m_keySpace = false;
}

// ============================================================
// update
// ============================================================
void FourthFloor::update(Player &player, int windowW, int windowH)
{
    m_tick++;

    player.isMoving = false;
    if (m_keyLeft)  player.moveLeft();
    if (m_keyRight) player.moveRight();

    // 跳跃物理
    if (m_isJumping) {
        m_jumpVelocity += GRAVITY;
        player.y       += static_cast<int>(m_jumpVelocity);
        if (player.y >= m_groundY) {
            player.y       = m_groundY;
            m_isJumping    = false;
            m_jumpVelocity = 0.0f;
        }
    }

    player.updateAnimation();
    player.checkBoundsX(windowW);

    // 接近检测
    m_nearComputer = (player.x > m_computerRect.left() - 70) &&
                     (player.x < m_computerRect.right() + 70);
    m_nearLamp     = (player.x > m_lampRect.left() - 70) &&
                 (player.x < m_lampRect.right() + 70);
    m_nearDoor     = (player.x > m_doorRect.left() - 80) &&
                 (player.x < m_doorRect.right() + 30);

    // 过渡动画
    if (m_transitioning) {
        m_transitionAlpha += 0.04f;
        if (m_transitionAlpha >= 1.0f) {
            m_transitioning   = false;
            m_transitionAlpha = 0.0f;
            emit requestFloorChange(3); // 去三层
        }
    }

    Q_UNUSED(windowH)
}

// ============================================================
// render
// ============================================================
void FourthFloor::render(QPainter &painter, const Player &player)
{
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    drawBackground(painter);
    drawDoor(painter);
    drawHints(painter);
    drawPlayerSprite(painter, player);
    drawTransition(painter);
}

void FourthFloor::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                                const QColor &fill, const QColor &border)
{
    p.fillRect(x,y,w,h,fill); p.setPen(border); p.drawRect(x,y,w-1,h-1);
}

void FourthFloor::drawBackground(QPainter &p)
{
    if (!m_bgPixmap.isNull())
        p.drawPixmap(0, 0, 800, 600, m_bgPixmap);
    else
        p.fillRect(0, 0, 800, 600, QColor(15, 10, 5));
}

void FourthFloor::drawDoor(QPainter &p)
{
    int dx=m_doorRect.x(), dy=m_doorRect.y();
    int dw=m_doorRect.width(), dh=m_doorRect.height();
    p.fillRect(dx,dy,dw,dh, QColor(5,3,2));
    p.setPen(QPen(QColor(35,20,6),3)); p.drawRect(dx-3,dy-2,dw+6,dh+2);
    p.setPen(QPen(QColor(65,42,15),1)); p.drawRect(dx-1,dy,dw+1,dh);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(50,32,10),2));
    p.drawArc(dx-2,dy-8,dw+4,20, 0, 180*16);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::NoPen); p.setBrush(QColor(110,80,28));
    p.drawEllipse(dx+dw-12,dy+dh/2,7,7);
    p.setPen(QPen(QColor(0,0,0),1));
    p.drawLine(dx+dw/2,dy+8,dx+dw/2,dy+dh-4);
}

// ============================================================
// drawHints：SPACE 提示光标
// *** 所有提示光标位置标注如下，微调修改对应偏移量 ***
// ============================================================
void FourthFloor::drawHints(QPainter &p)
{
    bool blink = (m_tick / 20) % 2 == 0;

    auto drawSpaceHint = [&](int cx, int y) {
        if (!blink) return;
        int hW = 80, hH = 22;
        int hX = cx - hW/2;
        drawPixelRect(p, hX, y, hW, hH, QColor(12,8,4,220), QColor(160,110,40));
        p.setRenderHint(QPainter::Antialiasing, true);
        QFont f("Courier New",9,QFont::Bold); p.setFont(f);
        p.setPen(QColor(230,175,55));
        p.drawText(hX, y, hW, hH, Qt::AlignCenter, "SPACE");
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(Qt::NoPen); p.setBrush(QColor(160,110,40));
        QPolygon arr;
        arr << QPoint(hX+hW/2-5,y+hH) << QPoint(hX+hW/2+5,y+hH) << QPoint(hX+hW/2,y+hH+6);
        p.drawPolygon(arr);
    };

    // 电脑处 SPACE 提示：★ 已答对则不显示（机关失效）
    // *** 电脑提示光标 X = m_computerRect 中心，Y = m_groundY - 150 ***
    if (m_nearComputer && !m_aiPuzzleDlg->isVisible() && !m_quizSolved)
        drawSpaceHint(m_computerRect.center().x()-10, m_groundY - 160);

    // 台灯处 SPACE 提示
    // *** 台灯提示光标 X = m_lampRect 中心，Y = m_groundY - 130 ***
    if (m_nearLamp && !m_deskDlg->isVisible())
        drawSpaceHint(m_lampRect.center().x()-60, m_groundY - 130);

    // 门处：只显示向下箭头提示（无 SPACE 文字）
    // *** 下键提示 X = m_doorRect 中心，Y = m_doorRect.top() - 30 ***
    if (m_nearDoor) {
        bool db = (m_tick / 25) % 2 == 0;
        if (db) {
            int cx = m_doorRect.center().x();
            int y  = m_doorRect.top() - 35;
            int hW = 64, hH = 24;
            int hX = cx - hW/2;
            drawPixelRect(p, hX, y, hW, hH, QColor(12,8,4,220), QColor(160,110,40));
            p.setRenderHint(QPainter::Antialiasing, true);
            QFont f("Courier New",9,QFont::Bold); p.setFont(f);
            p.setPen(QColor(230,175,55));
            p.drawText(hX, y, hW, hH, Qt::AlignCenter, "▼");
            p.setRenderHint(QPainter::Antialiasing, false);
        }
    }
}

void FourthFloor::drawTransition(QPainter &p)
{
    if (!m_transitioning) return;
    int clipH = static_cast<int>(m_transitionAlpha * 1200.0f);
    clipH = qMin(clipH, 600);
    p.fillRect(0, 600-clipH, 800, clipH, QColor(0,0,0));
}

void FourthFloor::drawPlayerSprite(QPainter &p, const Player &player)
{
    QPixmap sprite = player.getCurrentSprite();
    p.drawPixmap(player.x - sprite.width()/2, player.y - sprite.height(), sprite);
}

void FourthFloor::showAIPuzzleDialog()
{
    QScreen *scr = QApplication::primaryScreen();
    QRect sg = scr->availableGeometry();
    m_aiPuzzleDlg->move(sg.center().x()-m_aiPuzzleDlg->width()/2,
                        sg.center().y()-m_aiPuzzleDlg->height()/2);
    m_aiPuzzleDlg->show(); m_aiPuzzleDlg->raise();
}

void FourthFloor::showDeskItemsDialog()
{
    QScreen *scr = QApplication::primaryScreen();
    QRect sg = scr->availableGeometry();
    m_deskDlg->move(sg.center().x()-m_deskDlg->width()/2,
                    sg.center().y()-m_deskDlg->height()/2);
    m_deskDlg->show(); m_deskDlg->raise();
}

// ============================================================
// onKeyPress
// ============================================================
void FourthFloor::onKeyPress(QKeyEvent *event, Player &player)
{
    switch (event->key()) {
    case Qt::Key_A: m_keyLeft  = true; break;
    case Qt::Key_D: m_keyRight = true; break;

    case Qt::Key_Space:
        if (!m_keySpace) {
            m_keySpace = true;
            // 普通跳跃
            if (!m_isJumping) {
                m_isJumping    = true;
                m_jumpVelocity = -13.0f;
            }
            // 电脑处机关
            // 电脑处机关：★ 做题已答对(m_quizSolved)则不可重触；弹窗第一页(选择)可多次打开
            if (m_nearComputer && !m_aiPuzzleDlg->isVisible() && !m_quizSolved)
                showAIPuzzleDialog();
            // ★ 若已答对，电脑处机关完全失效（提示光标也在 drawHints 中隐藏）
            // 台灯处机关
            if (m_nearLamp && !m_deskDlg->isVisible())
                showDeskItemsDialog();
        }
        break;

    case Qt::Key_Down:
        if (m_nearDoor && !m_transitioning) {
            m_transitioning   = true;
            m_transitionAlpha = 0.0f;
        }
        break;

    default: break;
    }

    Q_UNUSED(player)
}

void FourthFloor::onKeyRelease(QKeyEvent *event, Player &)
{
    switch (event->key()) {
    case Qt::Key_A:     m_keyLeft  = false; break;
    case Qt::Key_D:     m_keyRight = false; break;
    case Qt::Key_Space: m_keySpace = false; break;
    default: break;
    }
}

void FourthFloor::onMousePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    // 门区域点击（向下键同等效果）
    if (m_nearDoor && m_doorRect.contains(event->pos()) && !m_transitioning) {
        m_transitioning   = true;
        m_transitionAlpha = 0.0f;
    }
}

void FourthFloor::onMouseMove(QMouseEvent *) {}

