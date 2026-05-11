#ifndef PLAYER_H
#define PLAYER_H

#include <QPixmap>
#include <QVector>

// ============================================================
// Player：玩家数据与动画管理类
// 负责：坐标、速度、动画帧切换、X轴边界检测
// Y轴（跳跃/重力）由 Widget 统一管理
// ============================================================
class Player
{
public:
    Player();

    // 玩家坐标
    // x：水平中心点
    // y：脚底位置（绘制时向上偏移精灵高度，使角色站在地面上）
    int x;
    int y;

    // 水平移动速度（像素/帧）
    int speed;

    // --- 动画控制 ---
    bool isMoving;       // 是否正在移动（决定播 idle 还是 run）
    int  currentFrame;   // 当前动画第几帧（0~4）
    int  frameTimer;     // 动画时钟（每 frameInterval 帧切换一张）
    bool isFacingLeft;   // 角色朝向：true=左，false=右

    // --- 动画帧集合 ---
    QVector<QPixmap> idleFrames;      // 静止动画，5帧
    QVector<QPixmap> runLeftFrames;   // 左跑动画，5帧
    QVector<QPixmap> runRightFrames;  // 右跑动画，5帧

    // --- 移动函数 ---
    void moveLeft();    // 向左移动一帧
    void moveRight();   // 向右移动一帧

    // 动画帧推进（每游戏帧调用一次）
    void updateAnimation();

    // X 轴边界夹紧（防止角色跑出窗口左右边缘）
    // windowWidth：当前窗口像素宽度
    void checkBoundsX(int windowWidth);

    // 返回当前应显示的精灵帧
    QPixmap getCurrentSprite() const;
};

#endif // PLAYER_H
