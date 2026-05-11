#include "thirdfloor.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QFont>
#include <cmath>

// ============================================================
// 世界坐标 → 屏幕坐标
// ============================================================
int ThirdFloor::worldToScreen(int worldX) const
{
    return worldX - static_cast<int>(m_cameraX);
}

// ============================================================
// 接近检测（考虑循环边界）
// ============================================================
bool ThirdFloor::nearWorld(int targetWorldX, int range) const
{
    int diff = std::abs(m_worldX - targetWorldX);
    if (diff > BG_SCALE_W / 2) diff = BG_SCALE_W - diff;
    return diff < range;
}

// ============================================================
// 构造函数
// ============================================================
ThirdFloor::ThirdFloor(QObject *parent)
    : BaseFloor(parent),
    m_bgW(BG_SCALE_W), m_bgH(BG_SCALE_H),
    m_groundY(0),
    m_cameraX(0), m_targetCameraX(0),
    m_worldX(200),
    m_isJumping(false), m_jumpVelocity(0.0f),
    m_onLamp(false), m_lampWorldX(0), m_lampTopY(0),
    m_keyLeft(false), m_keyRight(false),
    m_keySpace(false), m_keyDown(false),
    m_tick(0),
    m_leftNoteOpen(false), m_rightNoteOpen(false),
    m_centerTriggered(false), m_puzzleSolved(false),
    m_nearLeftDesk(false), m_nearRightDesk(false),
    m_nearCenterDesk(false), m_nearDoor(false),
    m_transitioning(false), m_transitionAlpha(0.0f),
    m_transitionTarget(2),
    m_bagWin(nullptr)
{
    m_bgPixmap = QPixmap(":/images/corridor.png");

    // *** 桌子世界X（微调：修改比例系数）***
    m_leftDeskWorldX   = static_cast<int>(BG_SCALE_W * 0.18f);  // ≈432
    m_centerDeskWorldX = static_cast<int>(BG_SCALE_W * 0.50f);  // =1200
    m_rightDeskWorldX  = static_cast<int>(BG_SCALE_W * 0.82f);  // ≈1968
    m_doorWorldX       = BG_SCALE_W - 60;

    // *** 台灯世界X（中桌右侧约 +20px，微调改此值）***
    m_lampWorldX = m_centerDeskWorldX + 20;

    // --------------------------------------------------------
    // 左桌便签弹窗（可重复触发）
    // --------------------------------------------------------
    m_leftNoteWin = new NoteDialog(nullptr);
    m_leftNoteWin->setTitle("— 走廊的便签 —");
    m_leftNoteWin->setContent(
        "唉，每天都在赶着<b>DDL</b>，<br>生活<b>重复</b>又单调。。。",
        "我现在就在<b>重复</b>的走廊中，<br><b>追赶</b>消失的D区……"
        );
    connect(m_leftNoteWin, &NoteDialog::dialogClosed,
            this, [this](){ m_leftNoteOpen = false; });

    // --------------------------------------------------------
    // 右桌便签弹窗：标题"随手写下的观后感"（可重复触发）
    // --------------------------------------------------------
    m_rightNoteWin = new NoteDialog(nullptr);
    m_rightNoteWin->setTitle("— 随手写下的观后感 —");
    m_rightNoteWin->setContent(
        "死亡诗社 <b>D</b>ead Poets Society<br>"
        "——O Captain! My Captain!<br><br>"
        "学生们站上课桌，看到了不同的世界",
        "也许<b>站上课桌</b>，<br>我就能看到<b>重复生活</b>的另一面！"
        );
    connect(m_rightNoteWin, &NoteDialog::dialogClosed,
            this, [this](){ m_rightNoteOpen = false; });

    // --------------------------------------------------------
    // ★ 破解后弹窗（仅文字，无上下行按钮页）
    // --------------------------------------------------------
    m_solvedNoteWin = new NoteDialog(nullptr);
    m_solvedNoteWin->setTitle("— 发现了D区！ —");
    m_solvedNoteWin->setContent(
        "B 被遮住了一半……<br>"
        "<b>B</b> 的另一半就是 <b>D</b> ——<br>"
        "你找到 D 区了！",
        "如果你也为追赶 DDL 而焦虑，<br>"
        "<b>O Captain! My Captain!</b><br><br>"
        "站上书桌，看见重复枯燥生活的另一面"
        );
    // 弹窗关闭后焦点归还，不触发楼层切换（上下楼通过门按钮操作）
    connect(m_solvedNoteWin, &NoteDialog::dialogClosed, this, [this](){
        // 强制让人物从台灯上跳下来
        m_onLamp = false;       // 离开台灯
        m_isJumping = true;     // 开始下落
        m_jumpVelocity = 0.0f;  // 从静止开始下落
        if (QWidget *w = qobject_cast<QWidget*>(this->parent())) w->setFocus();
    });
}

ThirdFloor::~ThirdFloor()
{
    delete m_leftNoteWin;
    delete m_rightNoteWin;
    delete m_solvedNoteWin;
}

// ============================================================
// onEnter
// ============================================================
void ThirdFloor::onEnter(int windowW, int windowH)
{
    // *** 地面Y：约在窗口高度78%，微调改此比例 ***
    m_groundY = static_cast<int>(windowH * 0.82f);
    //-----------------------------------------------------改初始位置，出生点
    emit spawnPlayer(static_cast<int>(windowW * 0.38f), m_groundY);
    // *** 台灯顶部Y：地面以上130px，微调改此偏移 ***
    m_lampTopY = m_groundY - 140;

    m_isJumping    = false;
    m_jumpVelocity = 0.0f;
    m_onLamp       = false;
    m_tick         = 0;
    m_transitioning      = false;
    m_transitionAlpha    = 0.0f;
    m_keyLeft = m_keyRight = m_keySpace = m_keyDown = false;

    m_cameraX       = static_cast<float>(m_worldX - windowW / 2);
    m_targetCameraX = m_cameraX;
}

// ============================================================
// onExit
// ============================================================
void ThirdFloor::onExit()
{
    if (m_leftNoteWin  && m_leftNoteWin->isVisible())    m_leftNoteWin->hide();
    if (m_rightNoteWin && m_rightNoteWin->isVisible())   m_rightNoteWin->hide();
    if (m_solvedNoteWin && m_solvedNoteWin->isVisible()) m_solvedNoteWin->hide();
    m_keyLeft = m_keyRight = m_keySpace = m_keyDown = false;
}

// ============================================================
// updateCamera
// ============================================================
void ThirdFloor::updateCamera(int windowW)
{
    float target = static_cast<float>(m_worldX) - windowW / 2.0f;
    target = std::max(0.0f, std::min(target, static_cast<float>(BG_SCALE_W - windowW)));
    m_targetCameraX = target;
    m_cameraX += (m_targetCameraX - m_cameraX) * 0.12f;
}

// ============================================================
// triggerSolved：触发通关
// ★ 时间倒流 + 走廊停止循环 + 弹窗
// ============================================================
void ThirdFloor::triggerSolved()
{
    if (m_centerTriggered) return;  // 防止重复触发

    m_centerTriggered = true;
    m_puzzleSolved    = true;

    // ★ 时间倒流 10 分钟
    emit puzzleSolved(10);

    // ★ 显示破解弹窗（仅文字页，不含上下行按钮）
    QScreen *scr = QApplication::primaryScreen();
    QRect sg = scr->availableGeometry();
    m_solvedNoteWin->move(sg.center().x() - m_solvedNoteWin->width()  / 2,
                          sg.center().y() - m_solvedNoteWin->height() / 2);
    m_solvedNoteWin->show();
    m_solvedNoteWin->raise();
}

// ============================================================
// update：每帧逻辑
// ============================================================
void ThirdFloor::update(Player &player, int windowW, int windowH)
{
    m_tick++;

    // --- 水平移动 ---
    player.isMoving = false;
    int moveSpeed = player.speed;
    if (m_keyLeft) {
        m_worldX -= moveSpeed;
        player.isMoving     = true;
        player.isFacingLeft = true;
    }
    if (m_keyRight) {
        m_worldX += moveSpeed;
        player.isMoving     = true;
        player.isFacingLeft = false;
    }

    // ★ 走廊循环：puzzleSolved 后停止循环（线性场景）
    if (!m_puzzleSolved) {
        if (m_worldX < 0)           m_worldX += BG_SCALE_W;
        if (m_worldX >= BG_SCALE_W) m_worldX -= BG_SCALE_W;
    } else {
        // 破解后限制在 [0, BG_SCALE_W - 1]
        m_worldX = qBound(0, m_worldX, BG_SCALE_W - 1);
    }

    // --- 跳跃物理 ---
    if (m_isJumping) {
        m_jumpVelocity += GRAVITY;
        player.y       += static_cast<int>(m_jumpVelocity);

        if (m_onLamp) {
            if (player.y >= m_groundY) {
                player.y       = m_groundY;
                m_isJumping    = false;
                m_jumpVelocity = 0.0f;
                m_onLamp       = false;
            }
        } else {
            if (player.y >= m_groundY) {
                player.y       = m_groundY;
                m_isJumping    = false;
                m_jumpVelocity = 0.0f;
            }
        }
    }

    player.updateAnimation();
    updateCamera(windowW);

    // --- 屏幕X ---
    player.x = worldToScreen(m_worldX);

    // --- 接近检测 ---
    m_nearLeftDesk   = nearWorld(m_leftDeskWorldX,   90);
    m_nearRightDesk  = nearWorld(m_rightDeskWorldX,  90);
    m_nearCenterDesk = nearWorld(m_centerDeskWorldX, 90);
    m_nearDoor       = m_puzzleSolved && nearWorld(m_doorWorldX, 100);

    // ★ 跳跃最高点检测：头顶遮住B字下半部分
    // *** B字下半顶端约 = windowH * 0.26 ≈ 155，微调改此比例 ***
    // *** 角色精灵高度约 = 80px，微调改此值 ***
    if (m_isJumping && m_onLamp && !m_centerTriggered &&
        nearWorld(m_centerDeskWorldX, 100))
    {
        int spriteH      = 80;
        int headY        = player.y - spriteH;
        int bLowerHalfY  = static_cast<int>(windowH * 0.22f);  // ≈155

        if (headY <= bLowerHalfY) {
            // 停住当前位置（悬停效果）
            m_isJumping    = false;
            m_jumpVelocity = 0.0f;
            triggerSolved();
        }
    }

    // --- 过渡动画 ---
    if (m_transitioning) {
        m_transitionAlpha += 0.04f;
        if (m_transitionAlpha >= 1.0f) {
            m_transitioning   = false;
            m_transitionAlpha = 0.0f;
            emit requestFloorChange(m_transitionTarget);
        }
    }

    Q_UNUSED(windowH)
}

// ============================================================
// render
// ============================================================
void ThirdFloor::render(QPainter &painter, const Player &player)
{
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    drawScene(painter);
    drawDoor(painter);
    drawHints(painter);
    if (m_puzzleSolved) drawElevatorButtons(painter);
    drawPlayerSprite(painter, player);
    drawTransition(painter);
}

// ============================================================
// drawPixelRect
// ============================================================
void ThirdFloor::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                               const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

// ============================================================
// drawScene：绘制背景（循环处理）
// ============================================================
void ThirdFloor::drawScene(QPainter &p)
{
    if (m_bgPixmap.isNull()) {
        p.fillRect(0, 0, 800, 600, QColor(10, 8, 15));
        return;
    }

    int camX = static_cast<int>(m_cameraX);

    p.drawPixmap(-camX, 0, BG_SCALE_W, BG_SCALE_H,
                 m_bgPixmap,
                 0, 0, m_bgPixmap.width(), m_bgPixmap.height());

    if (camX + 800 > BG_SCALE_W) {
        p.drawPixmap(BG_SCALE_W - camX, 0, BG_SCALE_W, BG_SCALE_H,
                     m_bgPixmap, 0, 0, m_bgPixmap.width(), m_bgPixmap.height());
    }
    if (camX < 0) {
        p.drawPixmap(-BG_SCALE_W - camX, 0, BG_SCALE_W, BG_SCALE_H,
                     m_bgPixmap, 0, 0, m_bgPixmap.width(), m_bgPixmap.height());
    }
}

// ============================================================
// drawDoor：破解后绘制明显的彩色高亮大门
// ★ 门比较明显：加发光效果+鲜艳颜色
// ============================================================
void ThirdFloor::drawDoor(QPainter &p)
{
    if (!m_puzzleSolved) return;

    int screenX = worldToScreen(m_doorWorldX);
    int doorW = 64, doorH = 130;
    int doorY = m_groundY - doorH;

    // --- 门外发光光晕（橙色）---
    p.setRenderHint(QPainter::Antialiasing, true);
    for (int i = 12; i >= 1; i--) {
        int alpha = 12 * i;
        p.setBrush(QColor(220, 150, 30, alpha));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(screenX - i*2, doorY - i*2,
                          doorW + i*4, doorH + i*4, 6, 6);
    }
    p.setRenderHint(QPainter::Antialiasing, false);

    // --- 门主体（深木色）---
    p.fillRect(screenX, doorY, doorW, doorH, QColor(40, 22, 8));

    // --- 门板纹理（两块竖向木板）---
    p.fillRect(screenX + 4,          doorY + 6, doorW/2 - 6, doorH - 12, QColor(55, 30, 10));
    p.fillRect(screenX + doorW/2 + 2, doorY + 6, doorW/2 - 6, doorH - 12, QColor(55, 30, 10));

    // --- 外框（三层，亮金色）---
    p.setPen(QPen(QColor(200, 150, 40), 3));
    p.drawRect(screenX - 3, doorY - 3, doorW + 6, doorH + 3);
    p.setPen(QPen(QColor(240, 190, 80), 1));
    p.drawRect(screenX - 1, doorY - 1, doorW + 2, doorH + 1);
    p.setPen(QPen(QColor(140, 100, 25), 2));
    p.drawRect(screenX + 2, doorY + 2, doorW - 4, doorH - 4);

    // --- 门顶拱形（鲜明）---
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(220, 170, 50), 3));
    p.setBrush(Qt::NoBrush);
    p.drawArc(screenX - 3, doorY - 14, doorW + 6, 28, 0, 180 * 16);
    p.setRenderHint(QPainter::Antialiasing, false);

    // --- 门把手（黄铜球形）---
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    // 把手底座
    p.setBrush(QColor(140, 100, 28));
    p.drawEllipse(screenX + doorW - 16, doorY + doorH / 2 - 6, 14, 14);
    // 把手高光
    p.setBrush(QColor(220, 175, 60));
    p.drawEllipse(screenX + doorW - 14, doorY + doorH / 2 - 4, 9, 9);
    p.setBrush(QColor(255, 235, 130));
    p.drawEllipse(screenX + doorW - 13, doorY + doorH / 2 - 3, 4, 4);
    p.setRenderHint(QPainter::Antialiasing, false);

    // --- 门缝 ---
    p.setPen(QPen(QColor(15, 8, 2), 2));
    p.drawLine(screenX + doorW/2, doorY + 10,
               screenX + doorW/2, doorY + doorH - 6);

    // --- 门上方悬挂标牌 "D区" ---
    p.setRenderHint(QPainter::Antialiasing, true);
    int signW = 48, signH = 22;
    int signX = screenX + (doorW - signW) / 2;
    int signY = doorY - 36;
    p.fillRect(signX, signY, signW, signH, QColor(180, 140, 40, 230));
    p.setPen(QPen(QColor(100, 70, 15), 2));
    p.drawRect(signX, signY, signW - 1, signH - 1);
    QFont sf("Courier New", 11, QFont::Bold);
    p.setFont(sf);
    p.setPen(QColor(30, 15, 3));
    p.drawText(signX, signY, signW, signH, Qt::AlignCenter, "D区");
    p.setRenderHint(QPainter::Antialiasing, false);

    // --- 门口地面发光条 ---
    p.fillRect(screenX - 4, m_groundY - 4, doorW + 8, 4, QColor(200, 160, 50, 180));
}

// ============================================================
// drawHints：SPACE 提示光标
// ★ 中桌台灯区域无任何提示
// ============================================================
void ThirdFloor::drawHints(QPainter &p)
{
    auto drawSpaceHint = [&](int screenX, int hintY, bool visible) {
        if (!visible) return;
        if ((m_tick / 20) % 2 != 0) return;
        int hW = 80, hH = 22;
        int hX = screenX - hW / 2;
        drawPixelRect(p, hX, hintY, hW, hH,
                      QColor(12, 8, 4, 220), QColor(160, 110, 40));
        p.setRenderHint(QPainter::Antialiasing, true);
        QFont f("Courier New", 9, QFont::Bold); p.setFont(f);
        p.setPen(QColor(230, 175, 55));
        p.drawText(hX, hintY, hW, hH, Qt::AlignCenter, "SPACE");
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(160, 110, 40));
        QPolygon arr;
        arr << QPoint(hX + hW/2 - 5, hintY + hH)
            << QPoint(hX + hW/2 + 5, hintY + hH)
            << QPoint(hX + hW/2,     hintY + hH + 6);
        p.drawPolygon(arr);
    };

    // *** 左桌提示光标 Y = m_groundY - 180，微调改此偏移 ***
    int leftScreenX  = worldToScreen(m_leftDeskWorldX);
    drawSpaceHint(leftScreenX, m_groundY - 180,
                  m_nearLeftDesk && !m_leftNoteOpen);

    // *** 右桌提示光标 Y = m_groundY - 180 ***
    int rightScreenX = worldToScreen(m_rightDeskWorldX);
    drawSpaceHint(rightScreenX, m_groundY - 180,
                  m_nearRightDesk && !m_rightNoteOpen);

    // ★★★ 中桌台灯：无任何提示 ★★★
}

// ============================================================
// drawElevatorButtons：破解后靠近门时显示 Up/Down 按钮
// ============================================================
void ThirdFloor::drawElevatorButtons(QPainter &p)
{
    if (!m_nearDoor) return;

    bool bright = (m_tick / 25) % 2 == 0;
    int screenX = worldToScreen(m_doorWorldX);
    int btnW = 88, btnH = 34;
    // *** 按钮X = 门中心，微调改此偏移 ***
    int btnX = screenX + 32 - btnW / 2;

    // *** 上行按钮Y = m_groundY - 230，微调改此偏移 ***
    int upY   = m_groundY - 230;
    // *** 下行按钮Y = m_groundY - 185，微调改此偏移 ***
    int downY = m_groundY - 185;

    // --- 上楼按钮（去四层）---
    QColor bgU  = bright ? QColor(50, 35, 8)   : QColor(20, 12, 4);
    QColor rimU = bright ? QColor(200, 160, 50) : QColor(80, 55, 18);
    drawPixelRect(p, btnX, upY, btnW, btnH, bgU, rimU);
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f("Courier New", 9, QFont::Bold); p.setFont(f);
    p.setPen(bright ? QColor(240, 195, 65) : QColor(90, 62, 18));
    p.drawText(btnX, upY, btnW, btnH, Qt::AlignCenter, "▲  Upstairs");

    // --- 下楼按钮（去二层）---
    QColor bgD  = bright ? QColor(50, 35, 8)   : QColor(20, 12, 4);
    QColor rimD = bright ? QColor(200, 160, 50) : QColor(80, 55, 18);
    drawPixelRect(p, btnX, downY, btnW, btnH, bgD, rimD);
    p.setPen(bright ? QColor(240, 195, 65) : QColor(90, 62, 18));
    p.drawText(btnX, downY, btnW, btnH, Qt::AlignCenter, "▼  Downstairs");

    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawPlayerSprite
// ============================================================
void ThirdFloor::drawPlayerSprite(QPainter &p, const Player &player)
{
    QPixmap sprite = player.getCurrentSprite();
    p.drawPixmap(player.x - sprite.width()  / 2,
                 player.y - sprite.height(),
                 sprite);
}

// ============================================================
// drawTransition
// ============================================================
void ThirdFloor::drawTransition(QPainter &p)
{
    if (!m_transitioning) return;
    int clipH = static_cast<int>(m_transitionAlpha * 1200.0f);
    clipH = qMin(clipH, 600);
    p.fillRect(0, 600 - clipH, 800, clipH, QColor(0, 0, 0));
}

// ============================================================
// onKeyPress
// ============================================================
void ThirdFloor::onKeyPress(QKeyEvent *event, Player &player)
{
    switch (event->key()) {
    case Qt::Key_A: m_keyLeft  = true; break;
    case Qt::Key_D: m_keyRight = true; break;

    case Qt::Key_Space:
        if (!m_keySpace) {
            m_keySpace = true;

            // ---- 优先：站在台灯上 → 跳跃 ----
            if (m_onLamp && !m_isJumping && !m_centerTriggered) {
                m_isJumping    = true;
                // *** 跳跃初速：负值越大跳越高
                //     需从 lampTopY(≈338) 上升到 headY(≈75)，约263px
                //     v = sqrt(2*0.55*263) ≈ 17  → 取 -17.0f 保证能触发
                //     若仍不够高，增大绝对值；太高则减小 ***
                m_jumpVelocity = -23.0f;
                break;
            }

            // ---- 靠近中桌 → 站上台灯（无提示，静默触发）----
            if (m_nearCenterDesk && !m_onLamp && !m_isJumping
                && !m_centerTriggered)
            {
                m_onLamp = true;
                player.y = m_lampTopY;
                break;
            }

            // ---- 普通跳跃 ----
            if (!m_isJumping && !m_onLamp) {
                m_isJumping    = true;
                m_jumpVelocity = -13.0f;
            }

            // ---- 靠近左桌 → 左便签（可重复）----
            if (m_nearLeftDesk && !m_leftNoteOpen
                && !m_leftNoteWin->isVisible())
            {
                m_leftNoteOpen = true;
                QScreen *scr = QApplication::primaryScreen();
                QRect sg = scr->availableGeometry();
                m_leftNoteWin->move(sg.center().x() - m_leftNoteWin->width()  / 2,
                                    sg.center().y() - m_leftNoteWin->height() / 2);
                m_leftNoteWin->show();
                m_leftNoteWin->raise();
            }

            // ---- 靠近右桌 → 右便签（可重复）----
            if (m_nearRightDesk && !m_rightNoteOpen
                && !m_rightNoteWin->isVisible())
            {
                m_rightNoteOpen = true;
                QScreen *scr = QApplication::primaryScreen();
                QRect sg = scr->availableGeometry();
                m_rightNoteWin->move(sg.center().x() - m_rightNoteWin->width()  / 2,
                                     sg.center().y() - m_rightNoteWin->height() / 2);
                m_rightNoteWin->show();
                m_rightNoteWin->raise();
            }
        }
        break;

    case Qt::Key_Up:
        // 靠近门：上楼（去四层）
        if (m_nearDoor && !m_transitioning) {
            m_transitioning    = true;
            m_transitionAlpha  = 0.0f;
            m_transitionTarget = 4;
        }
        break;

    case Qt::Key_Down:
        if (!m_keyDown) {
            m_keyDown = true;
            // 靠近门：下楼（去二层）
            if (m_nearDoor && !m_transitioning) {
                m_transitioning    = true;
                m_transitionAlpha  = 0.0f;
                m_transitionTarget = 2;
            }
        }
        break;

    default: break;
    }
}

// ============================================================
// onKeyRelease
// ============================================================
void ThirdFloor::onKeyRelease(QKeyEvent *event, Player &)
{
    switch (event->key()) {
    case Qt::Key_A:     m_keyLeft  = false; break;
    case Qt::Key_D:     m_keyRight = false; break;
    case Qt::Key_Space: m_keySpace = false; break;
    case Qt::Key_Down:  m_keyDown  = false; break;
    default: break;
    }
}

// ============================================================
// onMousePress：鼠标点击上/下按钮
// ============================================================
void ThirdFloor::onMousePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_nearDoor) return;

    int screenX = worldToScreen(m_doorWorldX);
    int btnW = 88, btnH = 34;
    int btnX = screenX + 32 - btnW / 2;
    int upY   = m_groundY - 230;
    int downY = m_groundY - 185;

    QRect upRect(btnX, upY, btnW, btnH);
    QRect downRect(btnX, downY, btnW, btnH);

    if (upRect.contains(event->pos()) && !m_transitioning) {
        m_transitioning    = true;
        m_transitionAlpha  = 0.0f;
        m_transitionTarget = 4;
    }
    if (downRect.contains(event->pos()) && !m_transitioning) {
        m_transitioning    = true;
        m_transitionAlpha  = 0.0f;
        m_transitionTarget = 2;
    }
}

void ThirdFloor::onMouseMove(QMouseEvent *) {}
