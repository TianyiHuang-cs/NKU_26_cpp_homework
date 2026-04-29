#include "player.h"

// ============================================================
// 构造函数：初始化玩家数据，加载所有动画帧
// ============================================================
Player::Player()
{
    // 初始位置（将由Widget::initScene覆盖）
    x = 400;
    y = 300;

    // 移动速度
    speed = 4;

    // 动画初始状态
    isMoving     = false;
    currentFrame = 0;
    frameTimer   = 0;
    isFacingLeft = false;

    // ----------------------------------------
    // 加载5帧静止动画 idle（缩放至90x90）
    // 文件命名：man_idle000.png ~ man_idle004.png
    // ----------------------------------------
    for (int i = 0; i < 5; i++) {
        QString fileName = QString(":/images/man_idle%1.png").arg(i, 3, 10, QChar('0'));
        idleFrames.append(QPixmap(fileName).scaled(90, 90, Qt::KeepAspectRatio));
    }

    // ----------------------------------------
    // 加载5帧左跑动画 man_run_left
    // 文件命名：man_run_left000.png ~ man_run_left004.png
    // ----------------------------------------
    for (int i = 0; i < 5; i++) {
        QString fileName = QString(":/images/man_run_left%1.png").arg(i, 3, 10, QChar('0'));
        runLeftFrames.append(QPixmap(fileName).scaled(90, 90, Qt::KeepAspectRatio));
    }

    // ----------------------------------------
    // 加载5帧右跑动画 man_run_right
    // 文件命名：man_run_right000.png ~ man_run_right004.png
    // ----------------------------------------
    for (int i = 0; i < 5; i++) {
        QString fileName = QString(":/images/man_run_right%1.png").arg(i, 3, 10, QChar('0'));
        runRightFrames.append(QPixmap(fileName).scaled(90, 90, Qt::KeepAspectRatio));
    }
}

// ============================================================
// 向左移动：减少X坐标，标记朝左
// ============================================================
void Player::moveLeft()
{
    x -= speed;
    isMoving     = true;
    isFacingLeft = true;
}

// ============================================================
// 向右移动：增加X坐标，标记朝右
// ============================================================
void Player::moveRight()
{
    x += speed;
    isMoving     = true;
    isFacingLeft = false;
}

// ============================================================
// 动画帧更新：每6个游戏帧切换一张图片，5帧循环
// ============================================================
void Player::updateAnimation()
{
    frameTimer++;
    if (frameTimer >= 6) {
        frameTimer   = 0;
        currentFrame = (currentFrame + 1) % 5;
    }
}

// ============================================================
// X轴边界限制：防止角色跑出窗口左右边缘
// Y轴由Widget的跳跃系统单独管理，此处不处理
// ============================================================
void Player::checkBoundsX(int windowWidth)
{
    QPixmap sprite = getCurrentSprite();
    int hw = sprite.width() / 2;

    if (x < hw)              x = hw;
    if (x > windowWidth - hw) x = windowWidth - hw;
}

// ============================================================
// 获取当前帧图片：静止→idle，移动→按朝向选左跑/右跑
// ============================================================
QPixmap Player::getCurrentSprite()
{
    if (!isMoving) {
        return idleFrames[currentFrame];
    }
    return isFacingLeft ? runLeftFrames[currentFrame] : runRightFrames[currentFrame];
}
