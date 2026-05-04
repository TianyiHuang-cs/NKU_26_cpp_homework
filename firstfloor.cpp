#include "firstfloor.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QFont>
#include <QWidget>

// ============================================================
// 构造函数：创建子窗口，连接信号
// ============================================================
FirstFloor::FirstFloor(QObject *parent)
    : BaseFloor(parent),
    m_groundY(0),
    m_nearDoor(false), m_nearGramophone(false),
    m_isJumping(false), m_jumpVelocity(0.0f),
    m_keyLeft(false), m_keyRight(false),
    m_keyUp(false), m_keySpace(false),
    m_tick(0)
{
    // 加载背景图
    // 资源路径：:/images/first_floor.png
    // 请在 .qrc 中添加：
    //   <file alias="images/first_floor.png">your/path/first_floor.png</file>
    m_bgPixmap = QPixmap(":/images/first_floor.png");

    // --------------------------------------------------------
    // 创建留声机弹窗（独立顶层窗口，不设 parent 避免被裁剪）
    // --------------------------------------------------------
    m_gramWin = new GramophoneWindow(nullptr);
    connect(m_gramWin, &GramophoneWindow::requestOpenBag,
            this,      &FirstFloor::onRequestOpenBag);
    connect(m_gramWin, &GramophoneWindow::puzzleSolved,
            this,      [this]() { emit puzzleSolved(10); }); // 倒流10分钟
    connect(m_gramWin, &GramophoneWindow::windowClosed,
            this,      &FirstFloor::onGramophoneWindowClosed);
    connect(m_gramWin, &GramophoneWindow::returnItemToBag,
            this, [this](int /*slot*/, int itemId, const QPixmap &icon){
                // 物品退回背包
                m_bagWin->addItem(itemId, "退回物品", icon);
            });

    // --------------------------------------------------------
    // 【接口】设置留声机谜题正确答案（物品 ID 集合，顺序无关）
    // 示例：m_gramWin->setCorrectAnswer({1, 2, 3});
    // 当前留空（始终判错），后续填入真实答案
    // --------------------------------------------------------
    m_gramWin->setCorrectAnswer({});


    // --------------------------------------------------------
    // 创建本层专用背包弹窗
    // --------------------------------------------------------
    m_bagWin = new BagWindow(nullptr);
    connect(m_bagWin, &BagWindow::itemSelected,
            this,     &FirstFloor::onItemSelected);

    // --------------------------------------------------------
    // 【接口】向背包添加本层物品
    // 调用：m_bagWin->addItem(ID, "名称", ":/images/图标.png");
    // 图标建议 64×64 PNG，加入 .qrc 后可用
    //
    // 示例：
    //   m_bagWin->addItem(1, "心跳声", ":/images/item_heartbeat.png");
    //   m_bagWin->addItem(2, "雨声",   ":/images/item_rain.png");
    //   m_bagWin->addItem(3, "翻书声", ":/images/item_book.png");
    // --------------------------------------------------------
}

FirstFloor::~FirstFloor()
{
    // Qt 对象树会自动删除子对象，
    // 但 m_gramWin/m_bagWin 的 parent 是 nullptr，需手动释放
    delete m_gramWin;
    delete m_bagWin;
}

// ============================================================
// onEnter：进入第一层，初始化场景元素与玩家出生点
// ============================================================
void FirstFloor::onEnter(int windowW, int windowH)
{
    // 地面 Y：背景图石板地面顶部约在窗口高度的 75%
    m_groundY = static_cast<int>(windowH * 0.75f);

    // 右侧小黑门（比角色精灵高10px，宽48px，底部对齐地面）
    int doorW = 48, doorH = 100;
    m_doorRect = QRect(windowW - 52, m_groundY - doorH, doorW, doorH);

    // 留声机交互区（原图约在水平中央偏右，垂直60%附近）
    m_gramophoneRect = QRect(windowW / 2 - 40, static_cast<int>(windowH * 0.60f),
                             80, 80);

    // 重置跳跃状态（楼层切换时角色重新落地）
    m_isJumping    = false;
    m_jumpVelocity = 0.0f;
    m_tick         = 0;
    m_nearDoor         = false;
    m_nearGramophone   = false;

    //楼层切换状态初始化
    m_transitioning   = false;
    m_transitionAlpha = 0.0f;
    m_transitionDir   = 0;
}

// ============================================================
// onExit：离开第一层，关闭所有弹窗
// ============================================================
void FirstFloor::onExit()
{
    if (m_gramWin->isVisible()) m_gramWin->hide();
    if (m_bagWin->isVisible())  m_bagWin->hide();
    m_keyLeft = m_keyRight = m_keyUp = m_keySpace = false;
}

// ============================================================
// update：每帧逻辑更新
// ============================================================
void FirstFloor::update(Player &player, int windowW, int windowH)
{
    m_tick++;

    // --- 水平移动 ---
    player.isMoving = false;
    if (m_keyLeft)  player.moveLeft();
    if (m_keyRight) player.moveRight();

    // --- 跳跃物理 ---
    if (m_isJumping) {
        m_jumpVelocity += GRAVITY;
        player.y       += static_cast<int>(m_jumpVelocity);
        if (player.y >= m_groundY) {
            player.y       = m_groundY;
            m_isJumping    = false;
            m_jumpVelocity = 0.0f;
        }
    }

    // --- 动画推进 ---
    player.updateAnimation();

    // --- X 轴边界 ---
    player.checkBoundsX(windowW);

    // --- 接近门检测（站在地面上，X 距离 < 80px） ---
    m_nearDoor = (!m_isJumping) &&
                 (player.x > m_doorRect.left() - 80) &&
                 (player.x < m_doorRect.right() + 30);

    // --- 接近留声机检测 ---
    m_nearGramophone = (player.x > m_gramophoneRect.left() - 80) &&
                       (player.x < m_gramophoneRect.right() + 80);

    // 场景切换动画推进
    if (m_transitioning) {
        m_transitionAlpha += 0.04f; // 控制速度（越小越慢）
        if (m_transitionAlpha >= 1.0f) {
            m_transitioning   = false;
            m_transitionAlpha = 0.0f;
            emit requestFloorChange(2);
        }
    }
}

// ============================================================
// render：绘制第一层完整画面
// ============================================================
void FirstFloor::render(QPainter &painter, const Player &player)
{
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    drawBackground(painter);
    drawDoor(painter);
    drawElevatorButton(painter);
    drawGramophoneHint(painter);
    drawPlayerSprite(painter, player);

    // 由下往上的黑色遮罩过渡动画
    if (m_transitioning) {
        // 遮罩从底部向上覆盖（clipH 从0增长到窗口高度）
        int totalH = 600;
        int clipH  = static_cast<int>(m_transitionAlpha * totalH * 2);
        clipH = qMin(clipH, totalH);
        painter.fillRect(0, totalH - clipH, 800, clipH, QColor(0, 0, 0));
    }
}

// ============================================================
// drawPixelRect：像素风矩形辅助
// ============================================================
void FirstFloor::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                               const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

// ============================================================
// drawBackground：背景图铺满窗口
// ============================================================
void FirstFloor::drawBackground(QPainter &p)
{
    if (!m_bgPixmap.isNull())
        p.drawPixmap(0, 0, 800, 600, m_bgPixmap);
    else
        p.fillRect(0, 0, 800, 600, QColor(20, 12, 6));
}

// ============================================================
// drawDoor：右侧小黑门（像素风木框）
// ============================================================
void FirstFloor::drawDoor(QPainter &p)
{
    int dx = m_doorRect.x(), dy = m_doorRect.y();
    int dw = m_doorRect.width(), dh = m_doorRect.height();

    // 门主体（纯黑）
    p.fillRect(dx, dy, dw, dh, QColor(5, 3, 2));

    // 外框（深棕木框，两层）
    p.setPen(QPen(QColor(35, 20, 6), 3));
    p.drawRect(dx - 3, dy - 2, dw + 6, dh + 2);
    p.setPen(QPen(QColor(65, 42, 15), 1));
    p.drawRect(dx - 1, dy, dw + 1, dh);

    // 门顶拱形装饰
    p.setPen(QPen(QColor(50, 32, 10), 2));
    p.drawArc(dx - 2, dy - 8, dw + 4, 20, 0, 180 * 16);

    // 门把手
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(110, 80, 28));
    p.drawEllipse(dx + dw - 12, dy + dh / 2, 7, 7);
    p.setBrush(QColor(170, 130, 50));
    p.drawEllipse(dx + dw - 10, dy + dh / 2 + 1, 4, 4);

    // 门缝竖线
    p.setPen(QPen(QColor(0, 0, 0), 1));
    p.drawLine(dx + dw / 2, dy + 8, dx + dw / 2, dy + dh - 4);
}

// ============================================================
// drawElevatorButton：闪动的上行按钮（靠近门时显示）
// ============================================================
void FirstFloor::drawElevatorButton(QPainter &p)
{
    if (!m_nearDoor) return;

    // 每 25 帧切换亮/暗
    bool bright = (m_tick / 25) % 2 == 0;

    int btnW = 64, btnH = 34;
    int btnX = m_doorRect.left() + (m_doorRect.width() - btnW) / 2;
    int btnY = m_doorRect.top() - btnH - 14;

    QColor bgColor  = bright ? QColor(200, 140, 30) : QColor(60, 40, 10);
    QColor rimColor = bright ? QColor(240, 190, 80) : QColor(90, 60, 18);
    drawPixelRect(p, btnX, btnY, btnW, btnH, bgColor, rimColor);
    p.setPen(bright ? QColor(255, 220, 100) : QColor(70, 48, 14));
    p.drawRect(btnX + 2, btnY + 2, btnW - 5, btnH - 5);

    // 向上箭头
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(bright ? QColor(255, 255, 200) : QColor(100, 70, 20));
    QPolygon arr;
    int ax = btnX + btnW / 2, ay = btnY + 8;
    arr << QPoint(ax, ay) << QPoint(ax - 10, ay + 12) << QPoint(ax + 10, ay + 12);
    p.drawPolygon(arr);
    p.setRenderHint(QPainter::Antialiasing, false);

    // 箭头指向线
    p.setPen(QPen(bright ? QColor(240, 190, 80) : QColor(70, 48, 14), 1));
    p.drawLine(btnX + btnW / 2, btnY + btnH,
               btnX + btnW / 2, m_doorRect.top() - 2);

    // "upstairs" 文字
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f("Courier New", 9, QFont::Bold);
    p.setFont(f);
    p.setPen(QColor(20, 12, 2));
    p.drawText(btnX - 10, btnY + btnH + 2, btnW + 20, 16, Qt::AlignCenter, "upstairs");
    p.setPen(bright ? QColor(240, 200, 80) : QColor(140, 100, 30));
    p.drawText(btnX - 10, btnY + btnH + 1, btnW + 20, 16, Qt::AlignCenter, "upstairs");
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawGramophoneHint：留声机上方闪动的 SPACE 提示
// ============================================================
void FirstFloor::drawGramophoneHint(QPainter &p)
{
    if (!m_nearGramophone) return;
    if (m_gramWin->isVisible()) return;

    // 每 20 帧闪烁一次
    if ((m_tick / 20) % 2 != 0) return;

    int hintW = 80, hintH = 22;
    int hintX = m_gramophoneRect.left() + (m_gramophoneRect.width() - hintW) / 2;
    int hintY = m_gramophoneRect.top() - hintH - 10;

    drawPixelRect(p, hintX, hintY, hintW, hintH,
                  QColor(12, 8, 4, 220), QColor(160, 110, 40));

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f("Courier New", 9, QFont::Bold);
    p.setFont(f);
    p.setPen(QColor(230, 175, 55));
    p.drawText(hintX, hintY, hintW, hintH, Qt::AlignCenter, "SPACE");
    p.setRenderHint(QPainter::Antialiasing, false);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(160, 110, 40));
    QPolygon arr;
    int ax = hintX + hintW / 2;
    arr << QPoint(ax - 5, hintY + hintH)
        << QPoint(ax + 5, hintY + hintH)
        << QPoint(ax,     hintY + hintH + 6);
    p.drawPolygon(arr);
}

// ============================================================
// drawPlayerSprite：绘制角色（脚底对齐 player.y）
// ============================================================
void FirstFloor::drawPlayerSprite(QPainter &p, const Player &player)
{
    QPixmap sprite = player.getCurrentSprite();
    p.drawPixmap(player.x - sprite.width()  / 2,
                 player.y - sprite.height(),
                 sprite);
}

// ============================================================
// onKeyPress：按键处理
// ============================================================
void FirstFloor::onKeyPress(QKeyEvent *event, Player &player)
{
    switch (event->key()) {
    case Qt::Key_A: m_keyLeft  = true; break;
    case Qt::Key_D: m_keyRight = true; break;

    case Qt::Key_Space:
        if (!m_keySpace) {
            m_keySpace = true;
            // 起跳（站地面时有效）
            if (!m_isJumping) {
                m_isJumping    = true;
                m_jumpVelocity = -13.0f;
            }
            // 靠近留声机 → 打开弹窗
            if (m_nearGramophone && !m_gramWin->isVisible()) {
                // 弹窗居中于屏幕
                QScreen *scr = QApplication::primaryScreen();
                QRect sg = scr->availableGeometry();
                m_gramWin->move(
                    sg.x() + (sg.width()  - m_gramWin->width())  / 2,
                    sg.y() + (sg.height() - m_gramWin->height()) / 2
                    );
                m_gramWin->show();
                m_gramWin->raise();
            }
        }
        break;

    case Qt::Key_Up:
        if (!m_keyUp) {
            m_keyUp = true;
            // 靠近门 → 请求切换到第二层
            if (m_nearDoor)
                if (!m_transitioning) {
                    m_transitioning   = true;
                    m_transitionAlpha = 0.0f;
                }
        }
        break;

    default: break;
    }
}

// ============================================================
// onKeyRelease：按键释放
// ============================================================
void FirstFloor::onKeyRelease(QKeyEvent *event, Player &)
{
    switch (event->key()) {
    case Qt::Key_A:     m_keyLeft  = false; break;
    case Qt::Key_D:     m_keyRight = false; break;
    case Qt::Key_Space: m_keySpace = false; break;
    case Qt::Key_Up:    m_keyUp    = false; break;
    default: break;
    }
}

// ============================================================
// onMousePress：鼠标点击（电梯按钮区域）
// ============================================================
void FirstFloor::onMousePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_nearDoor) return;

    int btnW = 64, btnH = 34;
    int btnX = m_doorRect.left() + (m_doorRect.width() - btnW) / 2;
    int btnY = m_doorRect.top() - btnH - 14;
    QRect btnRect(btnX, btnY, btnW, btnH);

    if (btnRect.contains(event->pos()))
        if (!m_transitioning) {
            m_transitioning   = true;
            m_transitionAlpha = 0.0f;
        }
}

// ============================================================
// onMouseMove：鼠标移动（当前层无需处理）
// ============================================================
void FirstFloor::onMouseMove(QMouseEvent *) {}

// ============================================================
// onRequestOpenBag：留声机弹窗请求打开背包
// ============================================================
void FirstFloor::onRequestOpenBag(int slotIndex)
{
    m_bagWin->setTargetSlot(slotIndex);
    QScreen *scr = QApplication::primaryScreen();
    QRect sg = scr->availableGeometry();
    m_bagWin->move(sg.x() + 20, sg.y() + (sg.height() - m_bagWin->height()) / 2);
    m_bagWin->show();
    m_bagWin->raise();
}

// ============================================================
// onItemSelected：背包选中物品，转发给留声机弹窗槽位
// ============================================================
void FirstFloor::onItemSelected(int slotIndex, int itemId, const QPixmap &icon)
{
    m_gramWin->placeItemIntoSlot(slotIndex, itemId, icon);
}

// ============================================================
// onGramophoneWindowClosed：弹窗关闭，焦点交还给父级窗口
// ============================================================
void FirstFloor::onGramophoneWindowClosed()
{
    // 找到父级 QWidget 并重新设置焦点
    if (QWidget *w = qobject_cast<QWidget*>(parent()))
        w->setFocus();
}
