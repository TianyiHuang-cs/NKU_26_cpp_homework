#include "widget.h"
#include <QPainter>
#include <QMessageBox>
#include <QPushButton>
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QFont>
#include <QFontMetrics>
#include <algorithm>

// ============================================================
// 构造函数：初始化窗口、所有状态变量、定时器
// ============================================================
Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    // 键盘焦点
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    // --- 按键状态初始化 ---
    keyLeft       = false;
    keyRight      = false;
    keySpace      = false;
    spaceConsumed = false;

    // --- 跳跃系统初始化 ---
    isJumping    = false;
    jumpVelocity = 0.0f;
    groundY      = 0;       // 将在 initScene 中赋值

    // --- 游戏阶段 ---
    currentPhase = PHASE_START;

    // --- 左门机关初始化 ---
    leftTrapActivated = false;
    leftMargin        = 0;
    rightMargin       = 0;
    originalWidth     = 800;

    // --- 右门机关初始化 ---
    rightTrapActivated = false;
    topMargin          = 0;
    bottomMargin       = 0;
    originalHeight     = 600;

    // --- 深渊状态 ---
    abyssTriggered = false;
    fallSpeed      = 4;

    // --- 成功消息 ---
    showSuccessMsg = false;
    successAlpha   = 0.0f;

    // --- 全局帧计数 ---
    globalTick = 0;

    // --- 深渊下坠定时器 ---
    fallTimer = new QTimer(this);
    fallTimer->setInterval(20);
    connect(fallTimer, &QTimer::timeout, this, &Widget::onFallTick);

    // --- 成功消息定时器（5秒后触发hideSuccessMessage，预留下一关接口） ---
    successMsgTimer = new QTimer(this);
    successMsgTimer->setSingleShot(true);
    connect(successMsgTimer, &QTimer::timeout, this, &Widget::hideSuccessMessage);

    // --- 窗口初始大小 ---
    resize(800, 600);

    // --- 初始化雨滴 ---
    initRain();

    // --- 初始化场景元素 ---
    initScene();

    // --- 游戏主定时器：20ms刷新 ---
    QTimer *gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &Widget::updateGame);
    gameTimer->start(20);
}

// ============================================================
// initScene：根据窗口尺寸，计算场景各元素位置与出生点
// 每次重置时调用，确保所有位置与当前窗口大小同步
// ============================================================
void Widget::initScene()
{
    int W = width();
    int H = height();

    // 地面Y：距离窗口底部 110px，角色脚底站在此线上
    groundY = H - 110;

    // 深渊：宽度设置为 80px（比角色稍宽，可以跳过）
    // 居中放置，顶部与地面齐平
    int abyssW = 80;
    int abyssX = (W - abyssW) / 2;
    abyssRect  = QRect(abyssX, groundY, abyssW, H - groundY);

    // 左门：靠近左侧，底部与地面对齐
    leftDoorRect  = QRect(50, groundY - 140, 72, 140);

    // 右门：靠近右侧，底部与地面对齐
    rightDoorRect = QRect(W - 122, groundY - 140, 72, 140);

    // 角色出生点：深渊左侧，站在地面上
    // 角色Y = groundY（脚底），但绘制时以中心点为准，
    // 所以 player.y = groundY（脚底位置，绘制时向上偏移半身高）
    player.x = abyssRect.left() - 100;
    player.y = groundY;
}

// ============================================================
// initRain：初始化背景雨滴（用于窗外下雨场景）
// 随机分布在窗口范围内，形成连续降雨效果
// ============================================================
void Widget::initRain()
{
    rainDrops.clear();
    for (int i = 0; i < 80; i++) {
        RainDrop drop;
        drop.x      = QRandomGenerator::global()->bounded(800);
        drop.y      = QRandomGenerator::global()->bounded(600);
        drop.speed  = QRandomGenerator::global()->bounded(4, 9);
        drop.length = QRandomGenerator::global()->bounded(8, 18);
        drop.alpha  = QRandomGenerator::global()->bounded(80, 180); // 【改色】提高雨滴透明度，更柔和
        rainDrops.append(drop);
    }
}

// ============================================================
// updateRain：每帧更新雨滴位置
// 雨滴向右下方斜落，超出边界则从顶部重置
// ============================================================
void Widget::updateRain()
{
    // 雨滴绘制区域为背景窗户内（左侧固定区域）
    // 但为了视觉连续性，在整个窗口范围内运动
    for (auto &drop : rainDrops) {
        drop.y += drop.speed;
        drop.x += drop.speed * 0.25f; // 微微向右倾斜
        if (drop.y > height() || drop.x > width()) {
            drop.y = QRandomGenerator::global()->bounded(-20, 0);
            drop.x = QRandomGenerator::global()->bounded(width());
        }
    }
}

// ============================================================
// updateGame：每20ms调用一次，处理所有游戏逻辑
// ============================================================
void Widget::updateGame()
{
    // 成功消息阶段或游戏结束阶段：暂停所有游戏逻辑
    if (currentPhase == PHASE_DREAM_SUCCESS || currentPhase == PHASE_GAME_OVER)
        return;

    // 深渊下坠阶段：只更新动画，不处理输入（下坠由fallTimer驱动）
    if (currentPhase == PHASE_FALLING) {
        player.updateAnimation();
        updateRain();
        globalTick++;
        update();
        return;
    }

    // --- 全局帧计数自增（用于像素风闪烁效果） ---
    globalTick++;

    // --- 水平移动 ---
    player.isMoving = false;
    if (keyLeft)  player.moveLeft();
    if (keyRight) player.moveRight();

    // --- 跳跃物理更新 ---
    updateJump();

    // --- 动画更新 ---
    player.updateAnimation();

    // --- X轴边界限制 ---
    player.checkBoundsX(width());

    // --- 各机关逻辑更新 ---
    if (currentPhase == PHASE_LEFT_SHRINK)
        updateLeftTrap();

    if (currentPhase == PHASE_RIGHT_SHRINK)
        updateRightTrap();

    // --- 碰到压缩边界检测 ---
    if (currentPhase == PHASE_LEFT_SHRINK || currentPhase == PHASE_RIGHT_SHRINK)
        checkBoundaryCollision();

    // --- 检测是否落入深渊（任何阶段均可触发） ---
    checkAbyssEntry();

    // --- 雨滴更新 ---
    updateRain();

    // --- 成功消息淡入（若正在显示） ---
    if (showSuccessMsg && successAlpha < 255.0f)
        successAlpha = qMin(255.0f, successAlpha + 3.0f);

    update(); // 触发重绘
}

// ============================================================
// updateJump：处理跳跃的上升与下落物理
// 角色脚底对准 groundY，按Space起跳，受重力影响落回地面
// ============================================================
void Widget::updateJump()
{
    if (isJumping) {
        // 每帧施加重力
        jumpVelocity += GRAVITY;
        player.y += static_cast<int>(jumpVelocity);

        // 落回地面：停止跳跃，对齐地面
        if (player.y >= groundY) {
            player.y     = groundY;
            isJumping    = false;
            jumpVelocity = 0.0f;
        }
    }
}

// ============================================================
// updateLeftTrap：左门机关——每帧将左右边界向中心各压缩2px
// leftMargin 从 0 向右扩展，rightMargin 从 width() 向左收缩
// 两者合拢时停止（理论上会被checkBoundaryCollision先触发）
// ============================================================
void Widget::updateLeftTrap()
{
    int W = width();
    int maxShrink = W / 2 - 60; // 最多压缩到剩60px宽才停（保险）

    if (leftMargin < maxShrink) {
        leftMargin  += 2;
        rightMargin -= 2;
        if (rightMargin < leftMargin) {
            leftMargin  = maxShrink;
            rightMargin = W - maxShrink;
        }
    }
}

// ============================================================
// updateRightTrap：右门机关——每帧将上下边界向中心各压缩2px
// topMargin 从 0 向下扩展，bottomMargin 从 height() 向上收缩
// ============================================================
void Widget::updateRightTrap()
{
    int H = height();
    int maxShrink = H / 2 - 60;

    if (topMargin < maxShrink) {
        topMargin    += 2;
        bottomMargin -= 2;
        if (bottomMargin < topMargin) {
            topMargin    = maxShrink;
            bottomMargin = H - maxShrink;
        }
    }
}

// ============================================================
// checkBoundaryCollision：检测角色是否被压缩边界触碰
// 左门机关：角色X超出 leftMargin 或 rightMargin 则触发
// 右门机关：角色Y超出 topMargin 或 bottomMargin 则触发
// 触发后：恢复窗口，弹出wakeup对话框
// ============================================================
void Widget::checkBoundaryCollision()
{
    QPixmap sprite = player.getCurrentSprite();
    int hw = sprite.width()  / 2;
    int hh = sprite.height() / 2;

    bool hit = false;

    if (currentPhase == PHASE_LEFT_SHRINK) {
        // 角色左边缘碰到左侧压缩墙，或右边缘碰到右侧压缩墙
        if (player.x - hw <= leftMargin || player.x + hw >= rightMargin)
            hit = true;
    }

    if (currentPhase == PHASE_RIGHT_SHRINK) {
        // 角色上边缘碰到顶部压缩墙，或下边缘碰到底部压缩墙
        if (player.y - hh <= topMargin || player.y + hh >= bottomMargin)
            hit = true;
    }

    if (hit) {
        showWakeUpDialog();
    }
}

// ============================================================
// checkAbyssEntry：检测角色是否走入/跳入深渊
// 条件：角色中心X在深渊范围内，且Y开始超过地面（开始坠落）
// 深渊随时可进入，无需触发门机关
// ============================================================
void Widget::checkAbyssEntry()
{
    if (abyssTriggered) return;

    int px = player.x;
    int py = player.y;

    // 角色中心X在深渊横向范围内
    bool inAbyssX = (px > abyssRect.left() + 10 && px < abyssRect.right() - 10);
    // 角色脚底超过地面（已经落入深渊空间）
    bool fallingIn = (py >= groundY && isJumping == false && inAbyssX);
    // 或者跳跃中跌入（在深渊上方跳跃并在深渊内降落）
    bool jumpingIn = (isJumping && inAbyssX && py > groundY - 10);

    if (fallingIn || jumpingIn) {
        // 站在深渊上方且未跳跃：开始下落
        if (fallingIn) {
            isJumping    = true;
            jumpVelocity = 2.0f; // 初始向下速度（重力继续加速）
        }
        triggerAbyss();
    }
}

// ============================================================
// triggerAbyss：触发深渊机关，启动窗口+角色同步下坠
// ============================================================
void Widget::triggerAbyss()
{
    abyssTriggered = true;
    currentPhase   = PHASE_FALLING;
    fallSpeed      = 4;     // 初始下坠速度
    fallTimer->start();     // 启动下坠定时器
}

// ============================================================
// onFallTick：窗口+角色下坠每帧回调（每20ms）
// 窗口和角色同步向下移动，角色在画面中持续下落
// 窗口底部触碰屏幕底部边界时停止，瞬移到屏幕中央
// ============================================================
void Widget::onFallTick()
{
    // 逐帧加速下坠（模拟自由落体感）
    fallSpeed = qMin(fallSpeed + 1, 28);

    // 角色在画面中持续向下运动（体现下坠感）
    player.y += fallSpeed / 2 + 2;

    // 窗口同步向下移动
    this->move(this->x(), this->y() + fallSpeed);

    // 获取屏幕可用区域
    QScreen *screen     = QApplication::primaryScreen();
    QRect    screenRect = screen->availableGeometry();

    // 判断窗口底部是否触碰到屏幕底部边界
    if (this->y() + this->height() >= screenRect.bottom()) {
        fallTimer->stop();

        // 瞬移到屏幕正中央
        int cx = screenRect.x() + (screenRect.width()  - this->width())  / 2;
        int cy = screenRect.y() + (screenRect.height() - this->height()) / 2;
        this->move(cx, cy);

        // 角色位置重置到画面中央（入梦后的展示位置）
        player.x = width()  / 2;
        player.y = height() / 2;

        // 进入成功阶段，显示文字至少5秒
        showSuccessMsg  = true;
        successAlpha    = 0.0f;
        currentPhase    = PHASE_DREAM_SUCCESS;
        successMsgTimer->start(5000);

        update();
    }

    update();
}

// ============================================================
// hideSuccessMessage：5秒后隐藏成功消息
// 预留接口：后续在此处切换到第一关场景
// ============================================================
void Widget::hideSuccessMessage()
{
    showSuccessMsg = false;
    // TODO：切换至第一关（暗黑恐怖主题场景）
}

// ============================================================
// activateLeftTrap：激活左门机关（左右边界压缩）
// 记录当前窗口宽度，初始化压缩边界
// ============================================================
void Widget::activateLeftTrap()
{
    if (leftTrapActivated) return;
    leftTrapActivated = true;
    currentPhase      = PHASE_LEFT_SHRINK;
    originalWidth     = width();
    leftMargin        = 0;
    rightMargin       = width();
}

// ============================================================
// activateRightTrap：激活右门机关（上下边界压缩）
// 记录当前窗口高度，初始化压缩边界
// ============================================================
void Widget::activateRightTrap()
{
    if (rightTrapActivated) return;
    rightTrapActivated = true;
    currentPhase       = PHASE_RIGHT_SHRINK;
    originalHeight     = height();
    topMargin          = 0;
    bottomMargin       = height();
}

// ============================================================
// showWakeUpDialog：被边界挤压后弹出"wake up"对话框
// 使用明艳色调，象征从梦中猛然惊醒
// 左按钮：关闭游戏；右按钮：回到出生点
// ============================================================
void Widget::showWakeUpDialog()
{
    currentPhase = PHASE_GAME_OVER;

    // 恢复窗口原始大小（压缩边界消失）
    if (leftTrapActivated)  resize(originalWidth,  height());
    if (rightTrapActivated) resize(width(), originalHeight);

    QMessageBox *box = new QMessageBox(this);
    box->setWindowTitle(" ");
    // 明艳色调：黄色背景 + 橙红警告文字，象征猛然惊醒
    box->setText(
        "<span style='color:#FF4500; font-size:18px; font-weight:bold;'>"
        "! wake up suddenly !"
        "</span>"
        );
    box->setStyleSheet(
        // 对话框整体：亮白背景，高对比色调
        "QMessageBox {"
        "  background-color: #F5F5F7;"  // 【改色】浅灰高级底色
        "  border: 2px solid #B09878;"   // 【改色】柔和棕边框
        "}"
        "QLabel {"
        "  color: #444;"
        "  font-size: 13px;"
        "  padding: 8px;"
        "}"
        // 按钮：莫兰迪暖灰，高级不刺眼
        "QPushButton {"
        "  background-color: #8A7967;"   // 【改色】莫兰迪棕灰
        "  color: white;"
        "  border: none;"
        "  padding: 8px 24px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #A08C75;"   // 【改色】hover提亮一档
        "}"
        );

    // 左：ok（直接关闭游戏）
    QPushButton *btnOk    = box->addButton("ok",         QMessageBox::RejectRole);
    // 右：sleep more（回到每关出生点）
    QPushButton *btnSleep = box->addButton("sleep more", QMessageBox::AcceptRole);

    box->exec();

    if (box->clickedButton() == btnOk) {
        QApplication::quit();
    } else if (box->clickedButton() == btnSleep) {
        resetToStart();
    }
}

// ============================================================
// resetToStart：将游戏完全重置到初始出生状态
// 恢复窗口大小、位置，清空所有机关状态
// ============================================================
void Widget::resetToStart()
{
    // 恢复窗口到800x600并居中
    resize(800, 600);
    QScreen *screen     = QApplication::primaryScreen();
    QRect    screenRect = screen->availableGeometry();
    int cx = screenRect.x() + (screenRect.width()  - 800) / 2;
    int cy = screenRect.y() + (screenRect.height() - 600) / 2;
    this->move(cx, cy);

    // 重置所有状态
    currentPhase       = PHASE_START;
    leftTrapActivated  = false;
    rightTrapActivated = false;
    abyssTriggered     = false;
    isJumping          = false;
    jumpVelocity       = 0.0f;
    showSuccessMsg     = false;
    successAlpha       = 0.0f;
    leftMargin         = 0;
    rightMargin        = 0;
    topMargin          = 0;
    bottomMargin       = 0;
    spaceConsumed      = false;

    // 重置场景位置
    initScene();
}

// ============================================================
// paintEvent：主绘制函数，按层次渲染场景
// 像素风渲染：关闭抗锯齿（文字与图标除外）
// ============================================================
void Widget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    // 像素风：整体关闭抗锯齿，保持像素边缘清晰
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    drawBackground(p);          // 1. 背景（图书馆内景 + 窗外雨景）
    drawFloorAndAbyss(p);       // 2. 地板与深渊
    drawDoors(p);               // 3. 左右像素木门
    drawTitle(p);               // 4. 标题
    drawPlayer(p);              // 5. 角色
    drawDoorSpaceHint(p);       // 6. 靠近门时的SPACE提示
    drawCompressionBounds(p);   // 7. 压缩边界墙体（机关激活时显示）
    drawSuccessMessage(p);      // 8. 入梦成功文字（全屏覆盖）
}

// ============================================================
// drawPixelRect：绘制像素风矩形（硬边缘，无抗锯齿）
// fill：填充色，border：边框色（像素风通常用高对比边框）
// ============================================================
void Widget::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                           const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1); // drawRect坐标包含端点，-1对齐像素
}

// ============================================================
// drawBackground：绘制图书馆内景背景
// 分层：墙壁 → 窗户（雨景） → 书架
// ============================================================
void Widget::drawBackground(QPainter &p)
{
    int W = width();
    int H = height();

    // --- 墙壁基底：【改色】提亮高级灰紫调，不再死黑 ---
    p.fillRect(0, 0, W, groundY, QColor(60, 58, 70));
    // 墙体像素砖纹【改色】浅一号砖缝，层次更柔和
    p.setPen(QColor(75, 72, 85));
    for (int row = 0; row < groundY / 16; row++) {
        int offsetX = (row % 2) * 16; // 交错砖缝
        for (int col = -16; col < W + 32; col += 32) {
            p.drawLine(0, row * 16, W, row * 16);
            p.drawLine(col + offsetX, row * 16, col + offsetX, (row + 1) * 16);
        }
    }

    // --- 绘制窗户与雨景 ---
    drawRainWindow(p);

    // --- 绘制书架 ---
    drawBookshelf(p);

    // --- 顶部暗晕【改色】降低暗晕浓度，不压暗画面 ---
    QLinearGradient topVig(0, 0, 0, 80);
    topVig.setColorAt(0.0, QColor(0, 0, 0, 100));
    topVig.setColorAt(1.0, Qt::transparent);
    p.fillRect(0, 0, W, 80, topVig);
}

// ============================================================
// drawRainWindow：绘制背景中的窗户（含窗外下雨场景）
// 像素风窗框 + 雨滴斜线 + 窗外夜色
// ============================================================
void Widget::drawRainWindow(QPainter &p)
{
    int W = width();

    // 窗户尺寸与位置（居中，靠上方墙壁）
    int winW  = 180;
    int winH  = 130;
    int winX  = (W - winW) / 2;
    int winY  = 30;

    // --- 窗外夜景【改色】提亮夜空，深灰蓝不发黑 ---
    QLinearGradient nightGrad(winX, winY, winX, winY + winH);
    nightGrad.setColorAt(0.0, QColor(35, 40, 60));
    nightGrad.setColorAt(1.0, QColor(50, 55, 75));
    p.fillRect(winX, winY, winW, winH, nightGrad);

    // --- 窗外远处隐约的屋顶/树剪影【改色】浅灰剪影，不死黑 ---
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(25, 30, 45));
    // 左侧屋顶轮廓
    QPolygon roofL;
    roofL << QPoint(winX, winY + winH)
          << QPoint(winX, winY + 80)
          << QPoint(winX + 30, winY + 55)
          << QPoint(winX + 60, winY + 80)
          << QPoint(winX + 60, winY + winH);
    p.drawPolygon(roofL);
    // 右侧屋顶轮廓
    QPolygon roofR;
    roofR << QPoint(winX + winW, winY + winH)
          << QPoint(winX + winW, winY + 80)
          << QPoint(winX + winW - 30, winY + 55)
          << QPoint(winX + winW - 60, winY + 80)
          << QPoint(winX + winW - 60, winY + winH);
    p.drawPolygon(roofR);

    // --- 雨滴【改色】浅青灰雨线，高级柔和 ---
    p.setClipRect(winX + 2, winY + 2, winW - 4, winH - 4);
    p.setPen(QPen(QColor(120, 160, 220, 0), 1));
    for (const auto &drop : rainDrops) {
        float relX = drop.x / width()  * winW + winX;
        float relY = drop.y / height() * winH + winY;
        QColor rainColor(160, 185, 220, static_cast<int>(drop.alpha)); // 【改色】柔和雨色
        p.setPen(QPen(rainColor, 1));
        p.drawLine(
            static_cast<int>(relX),
            static_cast<int>(relY),
            static_cast<int>(relX + drop.length * 0.2f),
            static_cast<int>(relY + drop.length)
            );
    }
    p.setClipping(false);

    // --- 窗框【改色】原木灰棕，高级质感 ---
    drawPixelRect(p, winX - 6, winY - 6, winW + 12, winH + 12,
                  QColor(70, 60, 50), QColor(50, 42, 32)); // 【改色】窗框深浅棕灰
    // 内框高光
    p.setPen(QColor(95, 82, 68)); // 【改色】提亮高光
    p.drawRect(winX - 4, winY - 4, winW + 7, winH + 7);

    // 窗格十字【改色】浅棕分格线
    p.setPen(QPen(QColor(65, 55, 45), 3));
    p.drawLine(winX + winW / 2, winY, winX + winW / 2, winY + winH);
    p.drawLine(winX, winY + winH / 2, winX + winW, winY + winH / 2);
    // 窗格高光
    p.setPen(QPen(QColor(100, 85, 70), 1));
    p.drawLine(winX + winW / 2 + 2, winY, winX + winW / 2 + 2, winY + winH);

    // 玻璃反光【改色】降低反光浓度，更内敛
    p.setPen(QPen(QColor(200, 210, 240, 40), 1));
    p.drawLine(winX + 10, winY + 5,  winX + 30,  winY + 35);
    p.drawLine(winX + 15, winY + 5,  winX + 35,  winY + 35);
    p.drawLine(winX + winW/2 + 10, winY + 5, winX + winW/2 + 30, winY + 35);
}

// ============================================================
// drawBookshelf：绘制背景书架（图书馆氛围）
// 像素风书架：竖向隔板 + 不同色块书脊
// ============================================================
void Widget::drawBookshelf(QPainter &p)
{
    int W = width();
    int shelfY   = 165;    // 书架第一层顶部Y
    int shelfH   = 14;     // 每层隔板厚度
    int bookH    = 52;     // 每层书的高度
    int shelfGap = bookH + shelfH; // 两层之间间距

    // 绘制左侧书架（两层，靠左墙）
    auto drawShelfUnit = [&](int sx, int sw) {
        for (int layer = 0; layer < 2; layer++) {
            int sy = shelfY + layer * shelfGap;

            // 隔板【改色】灰棕木质，和墙壁区分开
            drawPixelRect(p, sx, sy + bookH, sw, shelfH,
                          QColor(65, 55, 45), QColor(45, 38, 28));

            // 书脊【改色】莫兰迪低饱和色系，不暗沉
            QRandomGenerator rng(sx * 7 + layer * 13); // 固定种子
            int bx = sx + 2;
            while (bx < sx + sw - 4) {
                int bw   = rng.bounded(8, 22);
                int bh   = rng.bounded(bookH - 10, bookH - 2);
                int by   = sy + (bookH - bh);
                // 【改色】低饱和灰棕、灰蓝、灰绿，高级不杂乱
                int r    = rng.bounded(50, 90);
                int g    = rng.bounded(45, 80);
                int b2   = rng.bounded(55, 95);
                QColor bookColor(r, g, b2);
                drawPixelRect(p, bx, by, bw, bh, bookColor, bookColor.darker(120));
                // 书脊标题亮线
                p.setPen(bookColor.lighter(160));
                p.drawLine(bx + bw / 2, by + 3, bx + bw / 2, by + bh - 3);
                bx += bw + 1;
            }
        }
    };

    // 左侧书架（紧靠左墙）
    drawShelfUnit(0, 44);

    // 右侧书架（紧靠右墙）
    drawShelfUnit(W - 44, 44);
}

// ============================================================
// drawFloorAndAbyss：绘制地板与中央深渊
// 像素风石板地面 + 深邃黑色裂缝
// ============================================================
void Widget::drawFloorAndAbyss(QPainter &p)
{
    int W = width();
    int H = height();

    // --- 地板【改色】暖灰石板，和墙面明显区分 ---
    p.fillRect(0, groundY, W, H - groundY, QColor(55, 48, 42));

    // 石板格子纹【改色】浅一号石缝
    p.setPen(QColor(70, 62, 55));
    for (int x = 0; x < W; x += 32) {
        p.drawLine(x, groundY, x, H);
    }
    for (int y = groundY; y < H; y += 16) {
        p.drawLine(0, y, W, y);
    }

    // 地面顶部分隔线【改色】深灰缝
    p.setPen(QPen(QColor(35, 30, 25), 2));
    p.drawLine(0, groundY, abyssRect.left(), groundY);
    p.drawLine(abyssRect.right(), groundY, W, groundY);

    // --- 深渊 ---
    p.fillRect(abyssRect, QColor(0, 0, 0));

    // 深渊两侧边缘【改色】暗紫灰线条，不刺眼
    p.setPen(QColor(80, 50, 90));
    p.drawLine(abyssRect.left(),  groundY, abyssRect.left(),  H);
    p.drawLine(abyssRect.right(), groundY, abyssRect.right(), H);

    // 深渊顶部光晕【改色】淡紫光晕，更柔和
    QLinearGradient abyssGlow(abyssRect.left(), groundY - 8,
                              abyssRect.left(), groundY + 16);
    abyssGlow.setColorAt(0.0, Qt::transparent);
    abyssGlow.setColorAt(1.0, QColor(70, 20, 100, 60));
    p.fillRect(abyssRect.left(), groundY - 8, abyssRect.width(), 24, abyssGlow);

    // 深渊内部竖向渐变【改色】深灰紫渐变，不死黑
    QLinearGradient depthGrad(0, groundY, 0, H);
    depthGrad.setColorAt(0.0, QColor(25, 10, 35, 150));
    depthGrad.setColorAt(1.0, QColor(0, 0, 0, 255));
    p.fillRect(abyssRect, depthGrad);
}

// ============================================================
// drawDoors：绘制左右两扇像素风木门
// 木门质感：竖纹板 + 门框 + 金属门把手 + 门缝像素细节
// ============================================================
void Widget::drawDoors(QPainter &p)
{
    auto drawPixelDoor = [&](const QRect &door, bool isLeft) {
        int dx = door.x(), dy = door.y();
        int dw = door.width(), dh = door.height();

        // 门框外层【改色】灰棕实木色
        drawPixelRect(p, dx - 4, dy - 4, dw + 8, dh + 4,
                      QColor(65, 52, 40), QColor(45, 35, 25));

        // 门板主体【改色】浅木棕，和墙壁、地板拉开层次
        drawPixelRect(p, dx, dy, dw, dh,
                      QColor(85, 68, 52), QColor(60, 45, 32));

        // 门板竖纹【改色】深浅木纹线
        for (int x = dx + 6; x < dx + dw - 6; x += 8) {
            p.setPen(QColor(70, 55, 42));
            p.drawLine(x, dy + 4, x, dy + dh - 4);
            p.setPen(QColor(100, 80, 60));
            p.drawLine(x + 1, dy + 4, x + 1, dy + dh - 4);
        }

        // 门板横向装饰线【改色】柔和木色线条
        p.setPen(QPen(QColor(50, 38, 28), 2));
        int midY = dy + dh * 2 / 5;
        p.drawLine(dx + 4, midY, dx + dw - 4, midY);
        p.setPen(QColor(95, 75, 58));
        p.drawLine(dx + 4, midY + 1, dx + dw - 4, midY + 1);

        // 门把手【改色】哑光金属灰，不艳
        int hx = isLeft ? dx + dw - 14 : dx + 10;
        int hy = dy + dh / 2 + 10;
        drawPixelRect(p, hx - 2, hy - 6, 8, 12,
                      QColor(100, 95, 85), QColor(70, 65, 55));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(130, 125, 115));
        p.drawEllipse(hx, hy - 4, 6, 8);
        p.setPen(QColor(180, 175, 165));
        p.drawPoint(hx + 2, hy - 2);

        // 门缝【改色】深灰细缝
        p.setPen(QColor(20, 15, 10));
        p.drawLine(dx, dy + dh - 1, dx + dw, dy + dh - 1);

        // 门顶装饰【改色】深浅木边
        p.setPen(QColor(40, 30, 20));
        p.drawLine(dx, dy, dx + dw - 1, dy);
        p.setPen(QColor(105, 85, 65));
        p.drawLine(dx + 1, dy + 1, dx + dw - 2, dy + 1);
    };

    drawPixelDoor(leftDoorRect,  true);
    drawPixelDoor(rightDoorRect, false);
}

// ============================================================
// drawTitle：绘制游戏标题 "Fall Asleep in Lib"
// 像素风点阵字体效果："Fall" 加粗暗红色，其余灰白色
// 副标题小字点缀，整体与背景融合
// ============================================================
void Widget::drawTitle(QPainter &p)
{
    // 标题区域：窗户下方，书架之间的墙面
    int titleY = 205;

    // --- 标题背景衬板【改色】高级灰石板底色 ---
    int plaqW = 400, plaqH = 40;
    int plaqX = (width() - plaqW) / 2;
    drawPixelRect(p, plaqX, titleY - 8, plaqW, plaqH,
                  QColor(45, 42, 55), QColor(80, 70, 90));
    // 牌匾内框
    p.setPen(QColor(95, 85, 105));
    p.drawRect(plaqX + 2, titleY - 6, plaqW - 5, plaqH - 5);

    // 开启抗锯齿（仅用于文字渲染，保持清晰）
    p.setRenderHint(QPainter::Antialiasing, true);

    // --- "Fall" 暗红【改色】莫兰迪酒红，不刺眼 ---
    QFont fallFont("Courier New", 22, QFont::Bold);
    p.setFont(fallFont);
    QFontMetrics fallFm(fallFont);
    int fallW = fallFm.horizontalAdvance("Fall");

    for (int g = 3; g >= 1; g--) {
        p.setPen(QPen(QColor(120, 40, 40, 35 * g), g));
        p.drawText(width() / 2 - 185 + g, titleY + 24 + g, "Fall");
    }
    p.setPen(QColor(180, 70, 70));
    p.drawText(width() / 2 - 185, titleY + 24, "Fall");

    // --- " Asleep in Lib" 【改色】浅灰紫，高级柔和 ---
    QFont restFont("Courier New", 20, QFont::Normal);
    p.setFont(restFont);
    p.setPen(QColor(190, 185, 200));
    p.drawText(width() / 2 - 185 + fallW + 3, titleY + 24, " Asleep in Lib");

    // --- 副标题【改色】浅灰低调文字 ---
    QFont subFont("Courier New", 8);
    p.setFont(subFont);
    p.setPen(QColor(100, 90, 110, 180));
    p.drawText(0, titleY + 34, width(), 16, Qt::AlignCenter,
               "- - - - - - - - - - - - -");

    // 关闭抗锯齿，恢复像素风模式
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawPlayer：绘制角色精灵图
// 角色Y坐标为脚底位置，绘制时向上偏移精灵高度，使脚踩在地面上
// ============================================================
void Widget::drawPlayer(QPainter &p)
{
    QPixmap sprite = player.getCurrentSprite();
    int sw = sprite.width();
    int sh = sprite.height();

    // 绘制位置：中心X = player.x，脚底Y = player.y
    // 所以绘制起点：左上角 = (player.x - sw/2, player.y - sh)
    p.drawPixmap(player.x - sw / 2, player.y - sh, sprite);
}

// ============================================================
// drawDoorSpaceHint：当角色靠近门时，在门上方显示像素风SPACE提示
// 使用全局帧计数globalTick驱动闪烁效果
// ============================================================
void Widget::drawDoorSpaceHint(QPainter &p)
{
    // 机关已激活则不再显示提示
    if (leftTrapActivated && rightTrapActivated) return;

    // 检测靠近范围（角色中心到门框边缘的距离）
    const int NEAR_DIST = 80;
    int px = player.x;

    bool nearLeft  = !leftTrapActivated  &&
                    (px > leftDoorRect.left()  - NEAR_DIST) &&
                    (px < leftDoorRect.right() + NEAR_DIST);
    bool nearRight = !rightTrapActivated &&
                     (px > rightDoorRect.left()  - NEAR_DIST) &&
                     (px < rightDoorRect.right() + NEAR_DIST);

    if (!nearLeft && !nearRight) return;

    // 开启抗锯齿用于文字
    p.setRenderHint(QPainter::Antialiasing, true);

    // 闪烁效果：每30帧切换一次可见性（全局帧计数驱动）
    bool visible = (globalTick / 20) % 2 == 0;
    if (!visible) {
        p.setRenderHint(QPainter::Antialiasing, false);
        return;
    }

    auto drawHint = [&](const QRect &door) {
        // 提示框位置：门正上方
        int hintW = 72;
        int hintH = 20;
        int hx    = door.left() + (door.width() - hintW) / 2;
        int hy    = door.top() - hintH - 6;

        // 像素风提示背景框【改色】半透灰紫
        drawPixelRect(p, hx, hy, hintW, hintH,
                      QColor(30, 25, 40, 200), QColor(140, 110, 180));

        // "SPACE" 文字【改色】浅紫白
        QFont hintFont("Courier New", 9, QFont::Bold);
        p.setFont(hintFont);
        p.setPen(QColor(220, 200, 240));
        p.drawText(hx, hy, hintW, hintH, Qt::AlignCenter, "[ SPACE ]");

        // 指向箭头【改色】淡紫
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(140, 110, 180));
        QPolygon arrow;
        int ax = hx + hintW / 2;
        arrow << QPoint(ax - 5, hy + hintH)
              << QPoint(ax + 5, hy + hintH)
              << QPoint(ax,     hy + hintH + 6);
        p.drawPolygon(arrow);
    };

    if (nearLeft)  drawHint(leftDoorRect);
    if (nearRight) drawHint(rightDoorRect);

    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawCompressionBounds：绘制机关激活时的压缩边界墙体
// 左门机关：左右两侧黑色像素墙向中心逼近
// 右门机关：上下两侧黑色像素墙向中心逼近
// 带有像素风锯齿边缘，增强压迫感
// ============================================================
void Widget::drawCompressionBounds(QPainter &p)
{
    int W = width();
    int H = height();

    if (currentPhase == PHASE_LEFT_SHRINK) {
        // 左侧压缩墙【改色】深灰紫，不是死黑
        if (leftMargin > 0) {
            p.fillRect(0, 0, leftMargin, H, QColor(25, 22, 35));
            // 锯齿边缘【改色】亮紫灰线条
            p.setPen(QColor(90, 50, 110));
            for (int y = 0; y < H; y += 8) {
                p.drawLine(leftMargin - 1, y, leftMargin + 2, y + 4);
                p.drawLine(leftMargin + 2, y + 4, leftMargin - 1, y + 8);
            }
            p.setPen(QPen(QColor(110, 60, 130), 1));
            p.drawLine(leftMargin, 0, leftMargin, H);
        }

        // 右侧压缩墙【改色】同左侧深灰紫
        if (rightMargin < W) {
            p.fillRect(rightMargin, 0, W - rightMargin, H, QColor(25, 22, 35));
            p.setPen(QColor(90, 50, 110));
            for (int y = 0; y < H; y += 8) {
                p.drawLine(rightMargin + 1, y, rightMargin - 2, y + 4);
                p.drawLine(rightMargin - 2, y + 4, rightMargin + 1, y + 8);
            }
            p.setPen(QPen(QColor(110, 60, 130), 1));
            p.drawLine(rightMargin, 0, rightMargin, H);
        }
    }

    if (currentPhase == PHASE_RIGHT_SHRINK) {
        // 顶部压缩墙【改色】深灰紫
        if (topMargin > 0) {
            p.fillRect(0, 0, W, topMargin, QColor(25, 22, 35));
            p.setPen(QColor(90, 50, 110));
            for (int x = 0; x < W; x += 8) {
                p.drawLine(x, topMargin - 1, x + 4, topMargin + 2);
                p.drawLine(x + 4, topMargin + 2, x + 8, topMargin - 1);
            }
            p.setPen(QPen(QColor(110, 60, 130), 1));
            p.drawLine(0, topMargin, W, topMargin);
        }

        // 底部压缩墙【改色】深灰紫
        if (bottomMargin < H) {
            p.fillRect(0, bottomMargin, W, H - bottomMargin, QColor(25, 22, 35));
            p.setPen(QColor(90, 50, 110));
            for (int x = 0; x < W; x += 8) {
                p.drawLine(x, bottomMargin + 1, x + 4, bottomMargin - 2);
                p.drawLine(x + 4, bottomMargin - 2, x + 8, bottomMargin + 1);
            }
            p.setPen(QPen(QColor(110, 60, 130), 1));
            p.drawLine(0, bottomMargin, W, bottomMargin);
        }
    }
}

// ============================================================
// drawSuccessMessage：绘制入梦成功的全屏覆盖文字
// "welcome to dream" — 白色渐变红色，"dream" 放大加粗突出
// ============================================================
void Widget::drawSuccessMessage(QPainter &p)
{
    if (!showSuccessMsg) return;

    int alpha = static_cast<int>(successAlpha);

    // 全屏黑色遮罩【改色】半透深灰，不压抑
    p.fillRect(rect(), QColor(0, 0, 0, qMin(alpha, 180)));

    p.setRenderHint(QPainter::Antialiasing, true);

    // --- "welcome to " 【改色】灰白渐变柔红 ---
    int rVal = qMin(255, 160 + alpha / 4);
    int gVal = qMax(0,   240 - alpha / 2);
    int bVal = qMax(0,   240 - alpha / 2);
    QColor welcomeColor(rVal, gVal, bVal, alpha);

    QFont welcomeFont("Courier New", 18, QFont::Normal);
    p.setFont(welcomeFont);
    p.setPen(welcomeColor);

    QFontMetrics wFm(welcomeFont);
    QString welcomeStr = "welcome to ";
    int welcomeW = wFm.horizontalAdvance(welcomeStr);

    QFont dreamFont("Courier New", 30, QFont::Bold);
    QFontMetrics dFm(dreamFont);
    int dreamW = dFm.horizontalAdvance("dream");
    int totalW = welcomeW + dreamW;

    int startX = (width()  - totalW) / 2;
    int baseY  = (height() / 2) + 14;

    p.drawText(startX, baseY, welcomeStr);

    // --- "dream" 【改色】莫兰迪暗红光晕 ---
    p.setFont(dreamFont);
    for (int g = 4; g >= 1; g--) {
        p.setPen(QPen(QColor(160, 20, 20, (alpha / 4) * g), g));
        p.drawText(startX + welcomeW + g, baseY + g, "dream");
    }
    p.setPen(QColor(200, 40, 40, alpha));
    p.drawText(startX + welcomeW, baseY, "dream");

    p.setRenderHint(QPainter::Antialiasing, false);
}
// ============================================================
// keyPressEvent：按键按下处理
// ============================================================
void Widget::keyPressEvent(QKeyEvent *event)
{
    if (currentPhase == PHASE_DREAM_SUCCESS) return;

    switch (event->key()) {
    case Qt::Key_A:
        keyLeft = true;
        break;

    case Qt::Key_D:
        keyRight = true;
        break;

    case Qt::Key_Space:
        if (!keySpace) { // 防止长按重复触发
            keySpace = true;

            // 跳跃（只有站在地面时才能起跳）
            if (!isJumping) {
                isJumping    = true;
                jumpVelocity = -12.0f; // 初始向上速度（负=向上，可调整跳跃高度）
            }

            // 检测是否靠近左门（激活左门机关）
            {
                bool nearLeft = !leftTrapActivated  &&
                                !rightTrapActivated &&
                                (player.x > leftDoorRect.left()  - 80) &&
                                (player.x < leftDoorRect.right() + 80) &&
                                (!isJumping || jumpVelocity == -12.0f); // 刚按下Space时
                if (nearLeft) activateLeftTrap();
            }

            // 检测是否靠近右门（激活右门机关）
            {
                bool nearRight = !rightTrapActivated &&
                                 !leftTrapActivated  &&
                                 (player.x > rightDoorRect.left()  - 80) &&
                                 (player.x < rightDoorRect.right() + 80) &&
                                 (!isJumping || jumpVelocity == -12.0f);
                if (nearRight) activateRightTrap();
            }
        }
        break;

    default:
        break;
    }
}

// ============================================================
// keyReleaseEvent：按键松开处理
// ============================================================
void Widget::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_A:     keyLeft  = false; break;
    case Qt::Key_D:     keyRight = false; break;
    case Qt::Key_Space: keySpace = false; break;
    default: break;
    }
}
