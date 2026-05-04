#include "widget.h"
#include "firstfloor.h"
#include "secondfloor.h"
#include "thirdfloor.h"
#include "fourthfloor.h"
#include <QPainter>
#include <QFont>

// ============================================================
// 构造函数：初始化所有公共资源，创建各楼层实例，进入第一层
// ============================================================
Widget::Widget(QWidget *parent)
    : QWidget(parent),
    m_bagBtnHovered(false)
{
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    setFixedSize(800, 600);

    // --- 时钟初始值 ---
    m_clockHour   = 23;
    m_clockMinute = 30;

    // --- 背包按钮区域（左上角，时钟右侧） ---
    // 时钟占 [10,10]~[125,50]，背包按钮紧跟
    m_bagBtnRect = QRect(134, 10, 40, 40);

    // --------------------------------------------------------
    // 创建背包弹窗（全局唯一，各楼层共用）
    // --------------------------------------------------------
    m_bagWin = new BagWindow(nullptr);
    // 背包物品在各楼层 onEnter 时按需填充，或在此统一添加：
    // m_bagWin->addItem(1, "心跳声", ":/images/item_heartbeat.png");

    // --------------------------------------------------------
    // 创建各楼层实例并连接信号
    // --------------------------------------------------------
    m_floor1 = new FirstFloor(this);

    m_floor2 = new SecondFloor(this);
    /*
    m_floor3 = new ThirdFloor(this);
    m_floor4 = new FourthFloor(this);*/

    connectFloor(m_floor1);
    connectFloor(m_floor2);
    /*
    connectFloor(m_floor3);
    connectFloor(m_floor4);*/

    // 初始进入第一层
    m_currentFloor = nullptr;
    switchFloor(1);

    // --- 游戏主定时器 ---
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Widget::onGameTick);
    timer->start(20);
}

Widget::~Widget() {}

// ============================================================
// connectFloor：将楼层信号连接到 Widget 的处理槽
// 每个楼层实例只需连接一次
// ============================================================
void Widget::connectFloor(BaseFloor *floor)
{
    connect(floor, &BaseFloor::requestFloorChange,
            this,  &Widget::onFloorChangeRequested);
    connect(floor, &BaseFloor::puzzleSolved,
            this,  &Widget::onPuzzleSolved);
}

// ============================================================
// switchFloor：执行楼层切换
// 1. 通知当前层退出
// 2. 切换指针
// 3. 通知新层进入（传入窗口尺寸，新层设置玩家出生点）
// ============================================================
void Widget::switchFloor(int floorIndex)
{
    // 退出当前层
    if (m_currentFloor)
        m_currentFloor->onExit();

    // 切换指针
    switch (floorIndex) {
    case 1: m_currentFloor = m_floor1; break;

    case 2: m_floor2->setBagWindow(m_bagWin);
            m_floor2->setBaseTime(m_clockHour, m_clockMinute);
            m_currentFloor = m_floor2; break;
    /*
    case 3: m_currentFloor = m_floor3; break;
    case 4: m_currentFloor = m_floor4; break;*/
    default: return;
    }

    // 进入新层（传入当前窗口尺寸）
    m_currentFloor->onEnter(width(), height());
    update();
}

// ============================================================
// onGameTick：主循环，每 20ms 驱动当前楼层更新
// ============================================================
void Widget::onGameTick()
{
    if (m_currentFloor)
        m_currentFloor->update(m_player, width(), height());
    update();
}

// ============================================================
// paintEvent：绘制当前楼层 + 全局 UI
// ============================================================
void Widget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    // 1. 当前楼层负责绘制自己的一切（背景、元素、角色）
    if (m_currentFloor)
        m_currentFloor->render(p, m_player);

    // 2. 全局 UI 始终覆盖在最顶层
    drawClock(p);
    drawBagButton(p);
}

// ============================================================
// 输入事件：转发给当前楼层处理
// ============================================================
void Widget::keyPressEvent(QKeyEvent *event)
{
    if (m_currentFloor)
        m_currentFloor->onKeyPress(event, m_player);
}

void Widget::keyReleaseEvent(QKeyEvent *event)
{
    if (m_currentFloor)
        m_currentFloor->onKeyRelease(event, m_player);
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    // 优先检测全局 UI 点击（背包按钮）
    if (event->button() == Qt::LeftButton &&
        m_bagBtnRect.contains(event->pos()))
    {
        // 打开背包（浏览模式，不指定目标槽位）
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
    update();
}

// ============================================================
// drawPixelRect：像素风矩形（无抗锯齿）
// ============================================================
void Widget::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                           const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

// ============================================================
// drawClock：左上角时钟（⏰ 23:00）
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
// drawBagButton：左上角背包按钮（悬停高亮）
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
