#include "player.h"

// ============================================================
// 构造函数：初始化所有成员，加载动画帧资源
// ============================================================
Player::Player()
{
    // 出生坐标（将在 Widget::initScene 中覆盖）
    x = 200;
    y = 450;

    // 水平移动速度
    speed = 4;

    // 动画初始状态
    isMoving     = false;
    currentFrame = 0;
    frameTimer   = 0;
    isFacingLeft = false;

    // ----------------------------------------
    // 加载静止动画 idle，5帧
    // 资源路径：:/images/man_idle000.png ~ man_idle004.png
    // 缩放到 90×90（保持宽高比）
    // ----------------------------------------
    for (int i = 0; i < 5; i++) {
        QString path = QString(":/images/man_idle%1.png").arg(i, 3, 10, QChar('0'));
        idleFrames.append(QPixmap(path).scaled(90, 90, Qt::KeepAspectRatio));
    }

    // ----------------------------------------
    // 加载左跑动画，5帧
    // 资源路径：:/images/man_run_left000.png ~ man_run_left004.png
    // ----------------------------------------
    for (int i = 0; i < 5; i++) {
        QString path = QString(":/images/man_run_left%1.png").arg(i, 3, 10, QChar('0'));
        runLeftFrames.append(QPixmap(path).scaled(90, 90, Qt::KeepAspectRatio));
    }

    // ----------------------------------------
    // 加载右跑动画，5帧
    // 资源路径：:/images/man_run_right000.png ~ man_run_right004.png
    // ----------------------------------------
    for (int i = 0; i < 5; i++) {
        QString path = QString(":/images/man_run_right%1.png").arg(i, 3, 10, QChar('0'));
        runRightFrames.append(QPixmap(path).scaled(90, 90, Qt::KeepAspectRatio));
    }
}

// ============================================================
// 向左移动：x 减小，标记朝左，标记正在移动
// ============================================================
void Player::moveLeft()
{
    x -= speed;
    isMoving     = true;
    isFacingLeft = true;
}

// ============================================================
// 向右移动：x 增大，标记朝右，标记正在移动
// ============================================================
void Player::moveRight()
{
    x += speed;
    isMoving     = true;
    isFacingLeft = false;
}

// ============================================================
// 动画推进：每 6 个游戏帧切换一张图，5 帧循环
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
// X 轴边界夹紧：角色中心点不超出窗口左右边缘
// ============================================================
void Player::checkBoundsX(int windowWidth)
{
    QPixmap sprite = getCurrentSprite();
    int hw = sprite.width() / 2; // 精灵半宽
    if (x < hw)               x = hw;
    if (x > windowWidth - hw) x = windowWidth - hw;
}

// ============================================================
// 返回当前帧精灵：
//   静止 → idleFrames[currentFrame]
//   移动 → 按朝向选 runLeft / runRight
// ============================================================
QPixmap Player::getCurrentSprite() const
{
    if (!isMoving)
        return idleFrames[currentFrame];
    return isFacingLeft ? runLeftFrames[currentFrame] : runRightFrames[currentFrame];
}
