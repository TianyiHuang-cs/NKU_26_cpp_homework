#include "secondfloor.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QFont>
#include <QWidget>
#include <cmath>

// ============================================================
// 构造函数
// ============================================================
SecondFloor::SecondFloor(QObject *parent)
    : BaseFloor(parent),
    m_groundY(0),
    m_nearCat(false), m_nearPiano(false),
    m_nearDoor(false), m_nearStair(false), m_inMoonZone(false),
    m_keyLeft(false), m_keyRight(false),
    m_keyUp(false), m_keyDown(false), m_keySpace(false),
    m_isJumping(false), m_jumpVelocity(0.0f),
    m_tick(0),
    m_baseHour(11), m_baseMinute(0),
    m_pianoTriggered(false),
    m_doorMessageShown(false), m_showDoorMsg(false), m_doorMsgTimer(0),
    m_moonStillTick(0), m_moonCleared(false), m_clearAlpha(0.0f),
    m_transitioning(false), m_transitionAlpha(0.0f), m_transitionTarget(1),
    m_bagWin(nullptr)
{
    m_bgPixmap = QPixmap(":/images/second_floor.png");

    // --------------------------------------------------------
    // 创建猫猫机关弹窗
    // --------------------------------------------------------
    m_catWin = new CatPuzzleWindow(nullptr);
    connect(m_catWin, &CatPuzzleWindow::requestOpenBag,
            this,     &SecondFloor::onRequestOpenBagFromCat);
    connect(m_catWin, &CatPuzzleWindow::windowClosed,
            this,     &SecondFloor::onCatWindowClosed);

    // --------------------------------------------------------
    // 创建琴谱弹窗
    // --------------------------------------------------------
    m_sheetWin = new SheetMusicWindow(nullptr);
    connect(m_sheetWin, &SheetMusicWindow::addToBag,
            this,       &SecondFloor::onSheetAddToBag);
    connect(m_sheetWin, &SheetMusicWindow::leaveInPlace,
            this,       &SecondFloor::onSheetLeaveInPlace);

    connect(m_catWin, &CatPuzzleWindow::returnItemToBag,
            this, [this](int itemId, const QPixmap &icon, const QString &itemName){
                if (m_bagWin) {
                    m_bagWin->addItem(itemId, itemName, icon);
                }
            });

    m_moonShowDialog = false;
    m_moonEndingType = 0;
}

SecondFloor::~SecondFloor()
{
    delete m_catWin;
    delete m_sheetWin;
}

// ============================================================
// onEnter：进入第二层，初始化所有元素位置与状态
// ============================================================
void SecondFloor::onEnter(int windowW, int windowH)
{
    m_groundY = static_cast<int>(windowH * 0.78f);
    emit spawnPlayer(static_cast<int>(windowW * 0.38f), m_groundY);

    int W = windowW;

    m_catRect        = QRect(static_cast<int>(W * 0.44f), m_groundY - 60, 60, 60);
    m_pianoRect      = QRect(static_cast<int>(W * 0.20f), m_groundY - 80, 80, 80);
    m_leftShelfRect  = QRect(0,                           0,               static_cast<int>(W * 0.60f), windowH);
    m_rightShelfRect = QRect(static_cast<int>(W * 0.60f), 0,               static_cast<int>(W * 0.20f), windowH);
    m_doorRect       = QRect(static_cast<int>(W * 0.82f), m_groundY - 180, static_cast<int>(W * 0.10f), 130);
    m_stairRect      = QRect(0,                           0,               static_cast<int>(W * 0.10f), windowH);
    m_moonZoneRect   = QRect(static_cast<int>(W * 0.58f), m_groundY - 120, static_cast<int>(W * 0.06f), 120);

    m_isJumping          = false;
    m_jumpVelocity       = 0.0f;
    m_tick               = 0;
    m_moonStillTick      = 0;
    m_moonCleared        = false;
    m_clearAlpha         = 0.0f;
    m_showDoorMsg        = false;
    m_doorMsgTimer       = 0;
    m_doorMessageShown   = false;
    m_transitioning      = false;
    m_transitionAlpha    = 0.0f;
    m_keyLeft = m_keyRight = m_keyUp = m_keyDown = m_keySpace = false;
}

// ============================================================
// onExit：离开第二层
// ============================================================
void SecondFloor::onExit()
{
    if (m_catWin->isVisible())   m_catWin->hide();
    if (m_sheetWin->isVisible()) m_sheetWin->hide();
    m_keyLeft = m_keyRight = m_keyUp = m_keyDown = m_keySpace = false;
}

// ============================================================
// calcDisplayTime：计算当前应显示的时间（基准 + 偏移）
// ============================================================
void SecondFloor::calcDisplayTime(int &outHour, int &outMinute) const
{
    int total = m_baseHour * 60 + m_baseMinute;
    total     = total % (24 * 60);
    outHour   = total / 60;
    outMinute = total % 60;
}

// ============================================================
// update：每帧逻辑
// ============================================================
void SecondFloor::update(Player &player, int windowW, int windowH)
{
    m_tick++;

    player.isMoving = false;
    if (m_keyLeft)  player.moveLeft();
    if (m_keyRight) player.moveRight();

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

    m_nearCat   = (std::abs(player.x - m_catRect.center().x())  < 80) && !m_isJumping;
    m_nearPiano = (std::abs(player.x - m_pianoRect.center().x()) < 80) && !m_isJumping;
    m_nearDoor  = (player.x > m_doorRect.left() - 80) && !m_isJumping;
    m_nearStair = (player.x < m_stairRect.right() + 60) && !m_isJumping;
    m_inMoonZone= m_moonZoneRect.contains(player.x, player.y - 10) && !m_isJumping;

    // --------------------------------------------------------
    // 月光驻留计时：在月光区域静止连续20秒（1000帧@20ms）
    // 触发条件：时间为 23:00（23*60=1380分钟）
    // --------------------------------------------------------
    if (m_inMoonZone && !player.isMoving && !m_moonCleared) {
        // ★ 改为检查时间是否已到达 23:00（1380分钟）
        bool tasksComplete = (m_baseHour * 60 + m_baseMinute == 1380);
        m_moonStillTick++;
        if (m_moonStillTick >= 1000) {
            if (tasksComplete) {
                m_moonCleared    = true;
                m_moonEndingType = 1;
            } else {
                m_moonShowDialog = true;
                m_moonStillTick  = 0;
            }
        }
    }
    else if (!m_inMoonZone) {
        m_moonStillTick = 0;
    }

    if (m_moonCleared && m_clearAlpha < 1.0f) {
        m_clearAlpha += 0.008f;
        if (m_clearAlpha >= 1.0f) {
            m_clearAlpha = 1.0f;
            emit clearanceAchieved();
        }
    }

    if (m_showDoorMsg) {
        m_doorMsgTimer++;
        if (m_doorMsgTimer > 200) {
            m_showDoorMsg  = false;
            m_doorMsgTimer = 0;
        }
    }

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
// render：绘制第二层
// ============================================================
void SecondFloor::render(QPainter &painter, const Player &player)
{
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    drawBackground(painter);
    drawCatHint(painter);
    drawPianoHint(painter);
    drawDoorHint(painter);
    drawStairHint(painter);
    drawMoonProgress(painter);
    drawDoorMessage(painter);
    drawPlayerSprite(painter, player);

    drawTransition(painter);

    if (m_moonShowDialog) {
        int mW = 340, mH = 70;
        int mX = (800 - mW) / 2, mY = 200;
        painter.fillRect(mX, mY, mW, mH, QColor(12, 8, 22, 230));
        painter.setPen(QPen(QColor(80, 60, 120), 2));
        painter.drawRect(mX, mY, mW - 1, mH - 1);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QFont f("Courier New", 11);
        painter.setFont(f);
        painter.setPen(QColor(170, 150, 210));
        painter.drawText(mX + 10, mY, mW - 20, mH,
                         Qt::AlignCenter | Qt::TextWordWrap,
                         "怎么让时间倒流呢，思考ing......");
        painter.setRenderHint(QPainter::Antialiasing, false);
    }
}

// ============================================================
// drawPixelRect：像素风矩形辅助
// ============================================================
void SecondFloor::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                                const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

// ============================================================
// drawBackground
// ============================================================
void SecondFloor::drawBackground(QPainter &p)
{
    if (!m_bgPixmap.isNull())
        p.drawPixmap(0, 0, 800, 600, m_bgPixmap);
    else
        p.fillRect(0, 0, 800, 600, QColor(10, 15, 30));
}

// ============================================================
// drawCatHint
// ============================================================
void SecondFloor::drawCatHint(QPainter &p)
{
    if (!m_nearCat) return;
    if (m_catWin->isVisible()) return;
    if ((m_tick / 20) % 2 != 0) return;

    int hintW = 80, hintH = 22;
    int hintX = m_catRect.center().x() - hintW / 2;
    int hintY = m_catRect.top() - hintH - 10;
    drawPixelRect(p, hintX, hintY, hintW, hintH,
                  QColor(12, 8, 25, 220), QColor(130, 100, 200));

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f("Courier New", 9, QFont::Bold);
    p.setFont(f);
    p.setPen(QColor(180, 155, 230));
    p.drawText(hintX, hintY, hintW, hintH, Qt::AlignCenter, "SPACE");
    p.setRenderHint(QPainter::Antialiasing, false);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(130, 100, 200));
    QPolygon arr;
    int ax = hintX + hintW / 2;
    arr << QPoint(ax - 5, hintY + hintH)
        << QPoint(ax + 5, hintY + hintH)
        << QPoint(ax,     hintY + hintH + 6);
    p.drawPolygon(arr);
}

// ============================================================
// drawPianoHint
// ============================================================
void SecondFloor::drawPianoHint(QPainter &p)
{
    if (!m_nearPiano) return;
    if (m_pianoTriggered) return;
    if (m_sheetWin->isVisible()) return;
    if ((m_tick / 20) % 2 != 0) return;

    int hintW = 80, hintH = 22;
    int hintX = m_pianoRect.center().x() - hintW / 2;
    int hintY = m_pianoRect.top() - hintH - 10;
    drawPixelRect(p, hintX, hintY, hintW, hintH,
                  QColor(10, 8, 18, 220), QColor(100, 80, 160));

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f("Courier New", 9, QFont::Bold);
    p.setFont(f);
    p.setPen(QColor(160, 140, 210));
    p.drawText(hintX, hintY, hintW, hintH, Qt::AlignCenter, "SPACE");
    p.setRenderHint(QPainter::Antialiasing, false);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(100, 80, 160));
    QPolygon arr;
    int ax = hintX + hintW / 2;
    arr << QPoint(ax - 5, hintY + hintH)
        << QPoint(ax + 5, hintY + hintH)
        << QPoint(ax,     hintY + hintH + 6);
    p.drawPolygon(arr);
}

// ============================================================
// drawDoorHint
// ============================================================
void SecondFloor::drawDoorHint(QPainter &p)
{
    if (!m_nearDoor) return;
    if (m_showDoorMsg) return;
    if ((m_tick / 20) % 2 != 0) return;

    int hintW = 80, hintH = 22;
    int hintX = m_doorRect.center().x() - hintW / 2;
    int hintY = m_doorRect.top() - hintH - 14;
    drawPixelRect(p, hintX, hintY, hintW, hintH,
                  QColor(18, 10, 6, 220), QColor(160, 110, 40));

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
// drawStairHint
// ============================================================
void SecondFloor::drawStairHint(QPainter &p)
{
    if (!m_nearStair) return;

    bool bright = (m_tick / 30) % 2 == 0;

    int upBtnX = 20, upBtnY = m_groundY - 180;
    QColor bgUp  = bright ? QColor(40, 60, 100)  : QColor(15, 20, 40);
    QColor rimUp = bright ? QColor(100, 140, 220) : QColor(40, 60, 100);
    drawPixelRect(p, upBtnX, upBtnY, 60, 32, bgUp, rimUp);
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f("Courier New", 9, QFont::Bold);
    p.setFont(f);
    p.setPen(bright ? QColor(160, 200, 255) : QColor(60, 90, 150));
    p.drawText(upBtnX, upBtnY, 60, 32, Qt::AlignCenter, "▲ up");

    int downBtnY = upBtnY + 40;
    QColor bgDn  = bright ? QColor(60, 40, 20)  : QColor(25, 15, 8);
    QColor rimDn = bright ? QColor(180, 130, 60) : QColor(80, 55, 20);
    drawPixelRect(p, upBtnX, downBtnY, 60, 32, bgDn, rimDn);
    p.setPen(bright ? QColor(230, 180, 80) : QColor(100, 70, 25));
    p.drawText(upBtnX, downBtnY, 60, 32, Qt::AlignCenter, "▼ dn");
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawMoonProgress：月光区域驻留进度条
// ============================================================
void SecondFloor::drawMoonProgress(QPainter &p)
{
    if (!m_inMoonZone || m_moonStillTick == 0 || m_moonCleared) return;

    float prog = static_cast<float>(m_moonStillTick) / 1000.0f;

    int barW = 80, barH = 8;
    int barX = m_moonZoneRect.center().x() - barW / 2;
    int barY = m_moonZoneRect.top() - 24;

    drawPixelRect(p, barX, barY, barW, barH,
                  QColor(15, 12, 28, 200), QColor(60, 50, 100));

    int fillW = static_cast<int>(prog * (barW - 4));
    if (fillW > 0) {
        int r = static_cast<int>(100 + prog * 155);
        int g = static_cast<int>(120 + prog * 135);
        int b = 220;
        p.fillRect(barX + 2, barY + 2, fillW, barH - 4, QColor(r, g, b));
    }

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f("Courier New", 7);
    p.setFont(f);
    p.setPen(QColor(160, 150, 220, 200));
    p.drawText(barX - 10, barY - 12, barW + 20, 12,
               Qt::AlignCenter, "沐浴月光...");
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawDoorMessage
// ============================================================
void SecondFloor::drawDoorMessage(QPainter &p)
{
    if (!m_showDoorMsg) return;

    int msgW = 320, msgH = 60;
    int msgX = (800 - msgW) / 2;
    int msgY = m_groundY - 150;
    drawPixelRect(p, msgX, msgY, msgW, msgH,
                  QColor(10, 8, 18, 220), QColor(100, 80, 140));
    p.setPen(QPen(QColor(60, 45, 90), 1));
    p.drawRect(msgX + 2, msgY + 2, msgW - 5, msgH - 5);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f("Courier New", 11);
    p.setFont(f);
    p.setPen(QColor(185, 165, 220));
    p.drawText(msgX + 10, msgY, msgW - 20, msgH,
               Qt::AlignCenter | Qt::TextWordWrap, m_doorMessage);
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawClearance：通关渐变遮罩
// ★ 23:00 沐浴月光后触发，文字淡入显示
// ============================================================
/*void SecondFloor::drawClearance(QPainter &p)
{
    if (!m_moonCleared && m_clearAlpha <= 0.0f) return;

    int alpha = static_cast<int>(m_clearAlpha * 255.0f);

    // 深蓝偏黑的夜色渐变（符合23:00夜晚氛围）
    QLinearGradient grad(0, 0, 0, 600);
    grad.setColorAt(0.0, QColor(10, 15, 40, alpha));
    grad.setColorAt(1.0, QColor(5, 8, 25, alpha));
    p.fillRect(0, 0, 800, 600, grad);

    if (alpha > 80) {
        p.setRenderHint(QPainter::Antialiasing, true);
        QFont f("Courier New", 18, QFont::Bold);
        f.setItalic(true);
        p.setFont(f);

        // alpha 从 80 开始淡入，逐渐清晰
        int textAlpha = qMin(255, (alpha - 80) * 2);
        p.setPen(QColor(180, 200, 240, textAlpha));
        p.drawText(0, 0, 800, 600, Qt::AlignCenter,
                   "既然出不去了，那就好好享受一下夜色吧");

        p.setRenderHint(QPainter::Antialiasing, false);
    }
}*/

// ============================================================
// drawTransition
// ============================================================
void SecondFloor::drawTransition(QPainter &p)
{
    if (!m_transitioning) return;
    int clipH = static_cast<int>(m_transitionAlpha * 1200.0f);
    clipH = qMin(clipH, 600);
    p.fillRect(0, 600 - clipH, 800, clipH, QColor(0, 0, 0));
}

// ============================================================
// drawPlayerSprite
// ============================================================
void SecondFloor::drawPlayerSprite(QPainter &p, const Player &player)
{
    QPixmap sprite = player.getCurrentSprite();
    p.drawPixmap(player.x - sprite.width()  / 2,
                 player.y - sprite.height() + 60,
                 sprite);
}

// ============================================================
// onKeyPress
// ============================================================
void SecondFloor::onKeyPress(QKeyEvent *event, Player &player)
{
    if (m_moonShowDialog) {
        m_moonShowDialog = false;
        return;
    }

    switch (event->key()) {
    case Qt::Key_A: m_keyLeft  = true; break;
    case Qt::Key_D: m_keyRight = true; break;

    case Qt::Key_Space:
        if (!m_keySpace) {
            m_keySpace = true;

            if (!m_isJumping) {
                m_isJumping    = true;
                m_jumpVelocity = -13.0f;
            }

            if (m_nearCat && !m_catWin->isVisible()) {
                QScreen *scr = QApplication::primaryScreen();
                QRect sg = scr->availableGeometry();
                m_catWin->move(sg.center().x() - m_catWin->width()  / 2,
                               sg.center().y() - m_catWin->height() / 2);
                m_catWin->show();
                m_catWin->raise();
            }

            if (m_nearPiano && !m_pianoTriggered && !m_sheetWin->isVisible()) {
                QScreen *scr = QApplication::primaryScreen();
                QRect sg = scr->availableGeometry();
                m_sheetWin->move(sg.center().x() - m_sheetWin->width()  / 2,
                                 sg.center().y() - m_sheetWin->height() / 2);
                m_sheetWin->show();
                m_sheetWin->raise();
            }

            if (m_nearDoor && !m_showDoorMsg) {
                int dh, dm;
                calcDisplayTime(dh, dm);
                int totalMin = dh * 60 + dm;
                if (totalMin == 23 * 60) {
                    m_doorMessage = "啊哦，刚好关门了";
                } else if (totalMin > 23 * 60) {
                    m_doorMessage = "大门紧闭着...图书馆11:00就闭馆了。";
                } else {
                    m_doorMessage = "1";
                }
                m_showDoorMsg      = true;
                m_doorMsgTimer     = 0;
                m_doorMessageShown = true;
            }
        }
        break;

    case Qt::Key_Up:
        if (!m_keyUp) {
            m_keyUp = true;
            if (m_nearStair && !m_transitioning) {
                m_transitioning    = true;
                m_transitionAlpha  = 0.0f;
                m_transitionTarget = 3;
            }
        }
        break;

    case Qt::Key_Down:
        if (!m_keyDown) {
            m_keyDown = true;
            if (m_nearStair && !m_transitioning) {
                m_transitioning    = true;
                m_transitionAlpha  = 0.0f;
                m_transitionTarget = 1;
            }
        }
        break;

    default: break;
    }

    Q_UNUSED(player)
}

// ============================================================
// onKeyRelease
// ============================================================
void SecondFloor::onKeyRelease(QKeyEvent *event, Player &)
{
    switch (event->key()) {
    case Qt::Key_A:     m_keyLeft  = false; break;
    case Qt::Key_D:     m_keyRight = false; break;
    case Qt::Key_Space: m_keySpace = false; break;
    case Qt::Key_Up:    m_keyUp    = false; break;
    case Qt::Key_Down:  m_keyDown  = false; break;
    default: break;
    }
}

// ============================================================
// onMousePress
// ============================================================
void SecondFloor::onMousePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_nearStair) return;

    int upBtnX = 20, upBtnY  = m_groundY - 180;
    int downBtnY = upBtnY + 40;

    QRect upRect(upBtnX, upBtnY,   60, 32);
    QRect downRect(upBtnX, downBtnY, 60, 32);

    if (upRect.contains(event->pos()) && !m_transitioning) {
        m_transitioning    = true;
        m_transitionAlpha  = 0.0f;
        m_transitionTarget = 3;
    }
    if (downRect.contains(event->pos()) && !m_transitioning) {
        m_transitioning    = true;
        m_transitionAlpha  = 0.0f;
        m_transitionTarget = 1;
    }
}

void SecondFloor::onMouseMove(QMouseEvent *) {}

// ============================================================
// 槽：猫猫窗口关闭
// ============================================================
void SecondFloor::onCatWindowClosed()
{
    if (QWidget *w = qobject_cast<QWidget*>(parent()))
        w->setFocus();
}

// ============================================================
// 槽：猫猫弹窗请求打开背包
// ============================================================
void SecondFloor::onRequestOpenBagFromCat()
{
    if (!m_bagWin) return;
    m_bagWin->setTargetSlot(-2);

    connect(m_bagWin, &BagWindow::itemSelected,
            this,     &SecondFloor::onCatItemSelected,
            Qt::UniqueConnection);

    QScreen *scr = QApplication::primaryScreen();
    QRect sg = scr->availableGeometry();
    m_bagWin->move(sg.x() + 20, sg.center().y() - m_bagWin->height() / 2);
    m_bagWin->show();
    m_bagWin->raise();
}

// ============================================================
// 槽：背包选中物品投喂给猫猫
// ============================================================
void SecondFloor::onCatItemSelected(int /*slotIndex*/, int itemId, const QPixmap &icon)
{
    static const QMap<int,QString> nameMap = {
                                               {100, "意味深长的琴谱"}, {101, "美味的猫条"},
                                               {102, "出去玩玩"},      {103, "按时吃饭"},
                                               {104, "早点睡觉"},      {105, "静音的键盘"},
                                               {106, "东野圭吾小说"},
                                               };
    QString name = nameMap.value(itemId, "未知物品");
    bool isCatSnack = (itemId == 101);
    m_catWin->placeItem(itemId, icon, name, isCatSnack);
}

// ============================================================
// 槽：琴谱加入背包
// ============================================================
void SecondFloor::onSheetAddToBag(int itemId, const QPixmap &icon)
{
    if (m_bagWin)
        m_bagWin->addItem(100, "意味深长的琴谱", QPixmap(":/images/sheetmusic.png"));
    m_pianoTriggered = true;
    Q_UNUSED(itemId)
    Q_UNUSED(icon)
}

// ============================================================
// 槽：琴谱放在原地
// ============================================================
void SecondFloor::onSheetLeaveInPlace()
{
    m_pianoTriggered = false;
}

