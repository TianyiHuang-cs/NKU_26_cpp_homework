#ifndef PLAYER_H
#define PLAYER_H

#include <QPixmap>
#include <QVector>

// ============================================================
// Player：玩家数据与动画管理类
// 负责：坐标、速度、动画帧切换、边界检测
// ============================================================
class Player
{
public:
    Player();

    // 玩家坐标（中心点）
    int x;
    int y;

    // 移动速度
    int speed;

    // --- 动画控制 ---
    bool isMoving;          // 是否正在移动（决定播idle还是run）
    int  currentFrame;      // 当前动画第几帧
    int  frameTimer;        // 动画时钟（控制动画播放速度）
    bool isFacingLeft;      // 记录角色面朝左还是右

    // --- 动画帧集合 ---
    QVector<QPixmap> idleFrames;        // 静止5帧
    QVector<QPixmap> runLeftFrames;     // 左跑5帧
    QVector<QPixmap> runRightFrames;    // 右跑5帧

    // --- 移动函数 ---
    void moveLeft();
    void moveRight();

    // 动画更新（每帧调用，纯播放素材帧）
    void updateAnimation();

    // 边界限制（只限制X轴，Y轴由跳跃系统控制）
    void checkBoundsX(int windowWidth);

    // 获取当前应显示的图片
    QPixmap getCurrentSprite();
};

#endif // PLAYER_H
