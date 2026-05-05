#include "widget.h"
#include "firstfloor.h"
#include "secondfloor.h"
#include "thirdfloor.h"
#include "fourthfloor.h"
#include "introanimation.h"
#include "outroanimation.h"
#include <QPainter>
#include <QFont>

// ============================================================
// 构造函数：初始化所有公共资源，创建各楼层实例
// 出生点在第四层（椅子旁边），先播放开场动画
// ============================================================
Widget::Widget(QWidget *parent)
    : QWidget(parent),
    m_gameState(STATE_INTRO),
    m_bagBtnHovered(false)
{
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    setFixedSize(800, 600);

    // --- 时钟初始值：23:30（图书馆闭馆时间）---
    m_clockHour   = 23;
    m_clockMinute = 30;

    // --- 背包按钮区域（左上角，时钟右侧）---
    // 时钟占 [10,10]~[125,50]，背包按钮紧跟
    m_bagBtnRect = QRect(134, 10, 40, 40);

    // --------------------------------------------------------
    // 创建背包弹窗（全局唯一，各楼层共用）
    // --------------------------------------------------------
    m_bagWin = new BagWindow(nullptr);

    // --------------------------------------------------------
    // 创建各楼层实例并连接信号
    // --------------------------------------------------------
    m_floor1 = new FirstFloor(this);
    m_floor2 = new SecondFloor(this);
    m_floor3 = new ThirdFloor(this);
    m_floor4 = new FourthFloor(this);

    connectFloor(m_floor1);
    connectFloor(m_floor2);
    connectFloor(m_floor3);
    connectFloor(m_floor4);


    connect(m_floor2, &SecondFloor::clearanceAchieved,
            this, &Widget::startOutro);     // 再连接
    // 连接结束游戏信号（由三层/四层机关触发游戏结局）
    connect(m_floor3, &ThirdFloor::gameEnd,  this, &Widget::startOutro);
    connect(m_floor4, &FourthFloor::gameEnd, this, &Widget::startOutro);

    // --------------------------------------------------------
    // 创建开场/结束动画
    // --------------------------------------------------------
    m_intro = new IntroAnimation(this);
    connect(m_intro, &IntroAnimation::finished, this, &Widget::onIntroFinished);

    m_outro = new OutroAnimation(this);
    connect(m_outro, &OutroAnimation::finished, this, &Widget::onOutroFinished);

    // --------------------------------------------------------
    // 初始楼层指针设为 nullptr，游戏状态先播放开场动画
    // --------------------------------------------------------
    m_currentFloor = nullptr;

    // --- 游戏主定时器 ---
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Widget::onGameTick);
    timer->start(20);

    // 播放开场动画
    m_intro->start();
}

Widget::~Widget()
{
    // BagWindow parent 是 nullptr，需要手动释放
    delete m_bagWin;
    delete m_intro;
    delete m_outro;
}

// ============================================================
// connectFloor：将楼层信号连接到 Widget 的处理槽
// ============================================================
void Widget::connectFloor(BaseFloor *floor)
{
    connect(floor, &BaseFloor::requestFloorChange,
            this,  &Widget::onFloorChangeRequested);
    connect(floor, &BaseFloor::puzzleSolved,
            this,  &Widget::onPuzzleSolved);
    // 连接spawnPlayer信号，设置玩家位置
    connect(floor, &BaseFloor::spawnPlayer,
            this,  [this](int x, int y) {
                m_player.x = x;
                m_player.y = y;
            });
}

// ============================================================
// switchFloor：执行楼层切换
// ============================================================
void Widget::switchFloor(int floorIndex)
{
    // 退出当前层
    if (m_currentFloor)
        m_currentFloor->onExit();

    // 切换指针，注入背包与时间
    switch (floorIndex) {
    case 1:
        m_floor1->setBagWindow(m_bagWin);
        m_currentFloor = m_floor1;
        break;
    case 2:
        m_floor2->setBagWindow(m_bagWin);
        m_floor2->setBaseTime(m_clockHour, m_clockMinute);
        m_currentFloor = m_floor2;
        break;
    case 3:
        m_floor3->setBagWindow(m_bagWin);
        m_currentFloor = m_floor3;
        break;
    case 4:
        m_floor4->setBagWindow(m_bagWin);
        m_currentFloor = m_floor4;
        break;
    default:
        return;
    }

    m_currentFloor->onEnter(width(), height());


    update();
}

// ============================================================
// onGameTick：主循环，每 20ms 驱动更新
// ============================================================
void Widget::onGameTick()
{
    switch (m_gameState) {
    case STATE_INTRO:
        m_intro->update();
        break;
    case STATE_PLAYING:
        if (m_currentFloor)
            m_currentFloor->update(m_player, width(), height());
        break;
    case STATE_OUTRO:
        m_outro->update();
        break;
    }
    update();
}

// ============================================================
// paintEvent：绘制
// ============================================================
void Widget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    switch (m_gameState) {
    case STATE_INTRO:
        m_intro->render(p);
        return; // 开场动画不显示全局UI

    case STATE_PLAYING:
        // 当前楼层绘制
        if (m_currentFloor)
            m_currentFloor->render(p, m_player);
        // 全局 UI 覆盖在最顶层
        drawClock(p);
        drawBagButton(p);
        break;

    case STATE_OUTRO:
        m_outro->render(p);
        return; // 结束动画不显示全局UI
    }
}

// ============================================================
// 输入事件：转发给当前楼层处理
// ============================================================
void Widget::keyPressEvent(QKeyEvent *event)
{
    if (m_gameState == STATE_INTRO) {
        m_intro->onAnyKey();
        return;
    }
    if (m_gameState == STATE_OUTRO) {
        m_outro->onAnyKey(event);
        return;
    }
    if (m_currentFloor)
        m_currentFloor->onKeyPress(event, m_player);
}

void Widget::keyReleaseEvent(QKeyEvent *event)
{
    if (m_gameState != STATE_PLAYING) return;
    if (m_currentFloor)
        m_currentFloor->onKeyRelease(event, m_player);
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    if (m_gameState == STATE_OUTRO) {
        m_outro->onMousePress(event);
        return;
    }
    if (m_gameState != STATE_PLAYING) return;

    // 优先检测全局 UI 点击（背包按钮）
    if (event->button() == Qt::LeftButton &&
        m_bagBtnRect.contains(event->pos()))
    {
        m_bagWin->setTargetSlot(-1);
        QPoint gp = mapToGlobal(QPoint(0, 0));
        m_bagWin->move(gp.x() + 10, gp.y() + 60);
        m_bagWin->show();
        m_bagWin->raise();
        return;
    }

    // 再转发给当前楼层
    if (m_currentFloor)
        m_currentFloor->onMousePress(event);
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_gameState != STATE_PLAYING) return;

    // 更新背包按钮悬停状态
    bool hov = m_bagBtnRect.contains(event->pos());
    if (hov != m_bagBtnHovered) {
        m_bagBtnHovered = hov;
        update();
    }

    if (m_currentFloor)
        m_currentFloor->onMouseMove(event);
}

// ============================================================
// 槽：楼层切换请求
// ============================================================
void Widget::onFloorChangeRequested(int floorIndex)
{
    switchFloor(floorIndex);
}

// ============================================================
// 槽：谜题破解，时钟倒流 minutes 分钟
// ============================================================
void Widget::onPuzzleSolved(int minutes)
{
    int total = m_clockHour * 60 + m_clockMinute - minutes;
    if (total < 0) total += 24 * 60;
    m_clockHour   = total / 60;
    m_clockMinute = total % 60;
    // 同步更新第二层基准时间（若当前在二层）
    m_floor2->setBaseTime(m_clockHour, m_clockMinute);
    update();
}

// ============================================================
// 槽：开场动画结束 → 进入游戏，出生在第四层
// ============================================================
void Widget::onIntroFinished()
{
    m_gameState = STATE_PLAYING;
    // *** 出生点在第四层（椅子旁边）***
    switchFloor(4);
}

// ============================================================
// 槽：结束动画结束
// ============================================================
void Widget::onOutroFinished()
{
    // Game Over，关闭窗口
    close();
}

// ============================================================
// 开始结束动画
// ============================================================
void Widget::startOutro()
{
    if (m_currentFloor) m_currentFloor->onExit();
    m_currentFloor = nullptr;
    m_gameState = STATE_OUTRO;
    m_outro->start();
}

// ============================================================
// drawPixelRect
// ============================================================
void Widget::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                           const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

// ============================================================
// drawClock：左上角时钟（全局，所有楼层共用）
// ============================================================
void Widget::drawClock(QPainter &p)
{
    p.setRenderHint(QPainter::Antialiasing, false);
    int cx = 10, cy = 10, cw = 115, ch = 40;

    drawPixelRect(p, cx, cy, cw, ch,
                  QColor(12, 8, 4, 210), QColor(80, 55, 18));
    p.setPen(QPen(QColor(55, 36, 10), 1));
    p.drawRect(cx + 2, cy + 2, cw - 5, ch - 5);

    p.setRenderHint(QPainter::Antialiasing, true);

    // 表盘
    int ox = cx + 12, oy = cy + ch / 2;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(180, 140, 50));
    p.drawEllipse(ox - 9, oy - 9, 18, 18);
    p.setBrush(QColor(20, 12, 5));
    p.drawEllipse(ox - 7, oy - 7, 14, 14);
    p.setPen(QPen(QColor(200, 160, 55), 2));
    p.drawLine(ox, oy, ox - 3, oy - 5);
    p.setPen(QPen(QColor(220, 185, 80), 1));
    p.drawLine(ox, oy, ox + 4, oy - 3);

    // 时间数字
    QString ts = QString("%1:%2")
                     .arg(m_clockHour,   2, 10, QChar('0'))
                     .arg(m_clockMinute, 2, 10, QChar('0'));
    QFont f("Courier New", 14, QFont::Bold);
    p.setFont(f);
    p.setPen(QColor(10, 5, 2));
    p.drawText(cx + 26, cy + 1, cw - 30, ch, Qt::AlignVCenter, ts);
    p.setPen(QColor(210, 165, 55));
    p.drawText(cx + 25, cy,     cw - 30, ch, Qt::AlignVCenter, ts);

    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawBagButton：左上角背包按钮（全局，所有楼层共用）
// ============================================================
void Widget::drawBagButton(QPainter &p)
{
    p.setRenderHint(QPainter::Antialiasing, false);
    int bx = m_bagBtnRect.x(), by = m_bagBtnRect.y();
    int bw = m_bagBtnRect.width(), bh = m_bagBtnRect.height();

    QColor bg  = m_bagBtnHovered ? QColor(80, 55, 18) : QColor(35, 22, 7);
    QColor rim = m_bagBtnHovered ? QColor(180, 130, 45) : QColor(90, 60, 18);
    drawPixelRect(p, bx, by, bw, bh, bg, rim);

    // 背包图标
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(160, 115, 38));
    p.drawRect(bx + 8,  by + 14, bw - 16, bh - 20);
    p.drawRect(bx + 13, by + 8,  bw - 26, 8);
    p.setBrush(QColor(200, 160, 60));
    p.drawEllipse(bx + bw / 2 - 3, by + 6, 6, 6);

    if (m_bagBtnHovered) {
        p.setRenderHint(QPainter::Antialiasing, true);
        QFont tf("Courier New", 8);
        p.setFont(tf);
        p.setPen(QColor(200, 160, 55));
        p.drawText(bx - 10, by + bh + 2, bw + 20, 14,
                   Qt::AlignCenter, "背包");
        p.setRenderHint(QPainter::Antialiasing, false);
    }
}
