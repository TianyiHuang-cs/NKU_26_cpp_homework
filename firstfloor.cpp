#include "firstfloor.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QFont>
#include <QWidget>

// ============================================================
// 构造函数
// ★ 留声机正确答案：三张便签纸，ID = 102, 103, 104
//    （对应 fourthfloor 桌面物品：出去玩玩/按时吃饭/早点睡觉）
// ============================================================
FirstFloor::FirstFloor(QObject *parent)
    : BaseFloor(parent),
    m_groundY(0),
    m_nearDoor(false), m_nearGramophone(false),
    m_isJumping(false), m_jumpVelocity(0.0f),
    m_keyLeft(false), m_keyRight(false),
    m_keyUp(false), m_keySpace(false),
    m_tick(0),
    m_gramophoneSolved(false),   // ★ 机关初始未破解
    m_bagWin(nullptr),
    m_transitioning(false), m_transitionAlpha(0.0f), m_transitionDir(0)
{
    m_bgPixmap = QPixmap(":/images/first_floor.png");

    // --------------------------------------------------------
    // 创建留声机弹窗
    // --------------------------------------------------------
    m_gramWin = new GramophoneWindow(nullptr);

    connect(m_gramWin, &GramophoneWindow::requestOpenBag,
            this,      &FirstFloor::onRequestOpenBag);

    // ★ 答对信号连接到新槽（而不直接 emit puzzleSolved）
    connect(m_gramWin, &GramophoneWindow::puzzleSolved,
            this,      &FirstFloor::onGramophonePuzzleSolved);

    connect(m_gramWin, &GramophoneWindow::windowClosed,
            this,      &FirstFloor::onGramophoneWindowClosed);

    connect(m_gramWin, &GramophoneWindow::returnItemToBag,
            this, [this](int /*slot*/, int itemId, const QPixmap &icon, const QString &itemName){
                if (m_bagWin) m_bagWin->addItem(itemId, itemName, icon);
            });


    // ★ 正确答案：便签纸 ID 102, 103, 104（顺序无关）
    //   与 fourthfloor DeskItemsDialog 中定义的 id 对应
    m_gramWin->setCorrectAnswer({102, 103, 104});

    // --------------------------------------------------------
    // 时间倒流提示弹窗
    // --------------------------------------------------------
    m_rewindNoteDlg = new NoteDialog(nullptr);
    m_rewindNoteDlg->setTitle("— 时光倒流 —");
    m_rewindNoteDlg->setContent(
        "时间倒流 <b>10 分钟</b>。<br><br>"
        "那些被浪费的时间，<br>也许还可以找回来……",
        ""   // 第二页留空（翻页按钮会隐藏）
        );
    connect(m_rewindNoteDlg, &NoteDialog::dialogClosed, this, [this](){
        if (QWidget *w = qobject_cast<QWidget*>(this->parent())) w->setFocus();
    });
}

void FirstFloor::setBagWindow(BagWindow *bag)
{
    m_bagWin = bag;
    connect(m_bagWin, &BagWindow::itemSelected,
            this, &FirstFloor::onItemSelected,
            Qt::UniqueConnection);
}

FirstFloor::~FirstFloor()
{
    delete m_gramWin;
    delete m_rewindNoteDlg;
    // m_bagWin 由 Widget 持有，不在此 delete
}

// ============================================================
// onEnter
// ============================================================
void FirstFloor::onEnter(int windowW, int windowH)
{
    m_groundY = static_cast<int>(windowH * 0.75f);
    //-------------------------------------改出生点
    emit spawnPlayer(static_cast<int>(windowW * 0.38f), m_groundY);

    int doorW = 48, doorH = 100;
    m_doorRect = QRect(windowW - 52, m_groundY - doorH, doorW, doorH);

    m_gramophoneRect = QRect(windowW / 2 - 40, static_cast<int>(windowH * 0.60f),
                             80, 80);

    m_isJumping    = false;
    m_jumpVelocity = 0.0f;
    m_tick         = 0;
    m_nearDoor         = false;
    m_nearGramophone   = false;
    m_transitioning    = false;
    m_transitionAlpha  = 0.0f;
    m_transitionDir    = 0;
}

// ============================================================
// onExit
// ============================================================
void FirstFloor::onExit()
{
    if (m_gramWin && m_gramWin->isVisible())       m_gramWin->hide();
    if (m_rewindNoteDlg && m_rewindNoteDlg->isVisible()) m_rewindNoteDlg->hide();
    if (m_bagWin && m_bagWin->isVisible())          m_bagWin->hide();
    m_keyLeft = m_keyRight = m_keyUp = m_keySpace = false;
}

// ============================================================
// update
// ============================================================
void FirstFloor::update(Player &player, int windowW, int windowH)
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

    m_nearDoor = (!m_isJumping) &&
                 (player.x > m_doorRect.left() - 80) &&
                 (player.x < m_doorRect.right() + 30);

    // ★ 已破解则不再检测接近
    m_nearGramophone = !m_gramophoneSolved &&
                       (player.x > m_gramophoneRect.left() - 80) &&
                       (player.x < m_gramophoneRect.right() + 80);

    if (m_transitioning) {
        m_transitionAlpha += 0.04f;
        if (m_transitionAlpha >= 1.0f) {
            m_transitioning   = false;
            m_transitionAlpha = 0.0f;
            emit requestFloorChange(2);
        }
    }

    Q_UNUSED(windowH)
}

// ============================================================
// render
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

    if (m_transitioning) {
        int totalH = 600;
        int clipH  = static_cast<int>(m_transitionAlpha * totalH * 2);
        clipH = qMin(clipH, totalH);
        painter.fillRect(0, totalH - clipH, 800, clipH, QColor(0, 0, 0));
    }
}

// ============================================================
// drawPixelRect
// ============================================================
void FirstFloor::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                               const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

// ============================================================
// drawBackground
// ============================================================
void FirstFloor::drawBackground(QPainter &p)
{
    if (!m_bgPixmap.isNull())
        p.drawPixmap(0, 0, 800, 600, m_bgPixmap);
    else
        p.fillRect(0, 0, 800, 600, QColor(20, 12, 6));
}

// ============================================================
// drawDoor
// ============================================================
void FirstFloor::drawDoor(QPainter &p)
{
    int dx = m_doorRect.x(), dy = m_doorRect.y();
    int dw = m_doorRect.width(), dh = m_doorRect.height();

    p.fillRect(dx, dy, dw, dh, QColor(5, 3, 2));
    p.setPen(QPen(QColor(35, 20, 6), 3));
    p.drawRect(dx - 3, dy - 2, dw + 6, dh + 2);
    p.setPen(QPen(QColor(65, 42, 15), 1));
    p.drawRect(dx - 1, dy, dw + 1, dh);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(50, 32, 10), 2));
    p.drawArc(dx - 2, dy - 8, dw + 4, 20, 0, 180 * 16);
    p.setRenderHint(QPainter::Antialiasing, false);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(110, 80, 28));
    p.drawEllipse(dx + dw - 12, dy + dh / 2, 7, 7);
    p.setBrush(QColor(170, 130, 50));
    p.drawEllipse(dx + dw - 10, dy + dh / 2 + 1, 4, 4);
    p.setPen(QPen(QColor(0, 0, 0), 1));
    p.drawLine(dx + dw / 2, dy + 8, dx + dw / 2, dy + dh - 4);
}

// ============================================================
// drawElevatorButton
// ============================================================
void FirstFloor::drawElevatorButton(QPainter &p)
{
    if (!m_nearDoor) return;

    bool bright = (m_tick / 25) % 2 == 0;
    int btnW = 64, btnH = 34;
    int btnX = m_doorRect.left() + (m_doorRect.width() - btnW) / 2;
    int btnY = m_doorRect.top() - btnH - 14;

    QColor bgColor  = bright ? QColor(200, 140, 30) : QColor(60, 40, 10);
    QColor rimColor = bright ? QColor(240, 190, 80) : QColor(90, 60, 18);
    drawPixelRect(p, btnX, btnY, btnW, btnH, bgColor, rimColor);
    p.setPen(bright ? QColor(255, 220, 100) : QColor(70, 48, 14));
    p.drawRect(btnX + 2, btnY + 2, btnW - 5, btnH - 5);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(bright ? QColor(255, 255, 200) : QColor(100, 70, 20));
    QPolygon arr;
    int ax = btnX + btnW / 2, ay = btnY + 8;
    arr << QPoint(ax, ay) << QPoint(ax - 10, ay + 12) << QPoint(ax + 10, ay + 12);
    p.drawPolygon(arr);
    p.setRenderHint(QPainter::Antialiasing, false);

    p.setPen(QPen(bright ? QColor(240, 190, 80) : QColor(70, 48, 14), 1));
    p.drawLine(btnX + btnW / 2, btnY + btnH,
               btnX + btnW / 2, m_doorRect.top() - 2);

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
// drawGramophoneHint
// ★ 机关已破解（m_gramophoneSolved）则不再显示提示
// ============================================================
void FirstFloor::drawGramophoneHint(QPainter &p)
{
    if (!m_nearGramophone) return;          // 已破解时 m_nearGramophone 恒为 false
    if (m_gramWin->isVisible()) return;
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
// drawPlayerSprite
// ============================================================
void FirstFloor::drawPlayerSprite(QPainter &p, const Player &player)
{
    QPixmap sprite = player.getCurrentSprite();
    p.drawPixmap(player.x - sprite.width()  / 2,
                 player.y - sprite.height(),
                 sprite);
}

// ============================================================
// onKeyPress
// ============================================================
void FirstFloor::onKeyPress(QKeyEvent *event, Player &player)
{
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
            // ★ 已破解则不再触发留声机
            if (!m_gramophoneSolved &&
                m_nearGramophone && !m_gramWin->isVisible())
            {
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
            if (m_nearDoor && !m_transitioning) {
                m_transitioning   = true;
                m_transitionAlpha = 0.0f;
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
// onMousePress
// ============================================================
void FirstFloor::onMousePress(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_nearDoor) return;

    int btnW = 64, btnH = 34;
    int btnX = m_doorRect.left() + (m_doorRect.width() - btnW) / 2;
    int btnY = m_doorRect.top() - btnH - 14;
    QRect btnRect(btnX, btnY, btnW, btnH);

    if (btnRect.contains(event->pos()) && !m_transitioning) {
        m_transitioning   = true;
        m_transitionAlpha = 0.0f;
    }
}

void FirstFloor::onMouseMove(QMouseEvent *) {}

// ============================================================
// onRequestOpenBag
// ============================================================
void FirstFloor::onRequestOpenBag(int slotIndex)
{
    if (!m_bagWin) return;
    m_bagWin->setTargetSlot(slotIndex);
    QScreen *scr = QApplication::primaryScreen();
    QRect sg = scr->availableGeometry();
    m_bagWin->move(sg.x() + 20, sg.y() + (sg.height() - m_bagWin->height()) / 2);
    m_bagWin->show();
    m_bagWin->raise();
}

// ============================================================
// onItemSelected
// ============================================================
void FirstFloor::onItemSelected(int slotIndex, int itemId, const QPixmap &icon)
{
    static const QMap<int,QString> nameMap = {
                                               {100,"意味深长的琴谱"},{101,"美味的猫条"},
                                               {102,"出去玩玩"},{103,"按时吃饭"},{104,"早点睡觉"},
                                               {105,"静音的键盘"},{106,"东野圭吾小说"},
                                               };
    m_gramWin->placeItemIntoSlot(slotIndex, itemId, icon, nameMap.value(itemId,"未知物品"));
}

// ============================================================
// onGramophoneWindowClosed
// ============================================================
void FirstFloor::onGramophoneWindowClosed()
{
    if (QWidget *w = qobject_cast<QWidget*>(parent()))
        w->setFocus();
}

// ============================================================
// ★ onGramophonePuzzleSolved：留声机答对处理
//   1. 标记机关永久失效
//   2. 通知 Widget 时间倒流 10 分钟
//   3. 显示时间倒流提示弹窗
// ============================================================
void FirstFloor::onGramophonePuzzleSolved()
{
    if (m_gramophoneSolved) return; // 防止重复触发

    m_gramophoneSolved = true;      // ★ 机关永久失效

    emit puzzleSolved(10);          // ★ 通知 Widget 时间倒流 10 分钟

    // 显示倒流提示弹窗
    QScreen *scr = QApplication::primaryScreen();
    QRect sg = scr->availableGeometry();
    m_rewindNoteDlg->move(sg.center().x() - m_rewindNoteDlg->width()  / 2,
                          sg.center().y() - m_rewindNoteDlg->height() / 2);
    m_rewindNoteDlg->show();
    m_rewindNoteDlg->raise();
}
