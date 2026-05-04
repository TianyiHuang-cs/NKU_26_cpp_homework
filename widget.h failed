#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QVector>
#include <QRect>
#include "player.h"

// ============================================================
// Widget：主窗口类，负责游戏主循环、渲染、输入处理
// 当前实现：开始界面场景（像素风图书馆 → 深渊入梦）
// ============================================================
class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void updateGame();          // 游戏主循环（每20ms触发）
    void onFallTick();          // 深渊下坠每帧回调（窗口+角色同步下降）
    void hideSuccessMessage();  // 隐藏"welcome to dream"文字

private:
    // --- 玩家 ---
    Player player;

    // --- 按键状态 ---
    bool keyLeft, keyRight, keySpace;
    bool spaceConsumed;         // 本次Space是否已消费（防止长按重复触发）

    // --- 跳跃系统 ---
    bool  isJumping;            // 当前是否处于跳跃状态
    float jumpVelocity;         // 跳跃纵向速度（负值=向上）
    const float GRAVITY = 0.55f; // 重力加速度（每帧叠加）

    // --- 地面与平台 ---
    int groundY;                // 地面Y坐标（角色可站立的最低平面）

    // --- 游戏阶段枚举 ---
    enum GamePhase {
        PHASE_START,            // 正常开始界面
        PHASE_LEFT_SHRINK,      // 左门机关：左右边界向中心压缩
        PHASE_RIGHT_SHRINK,     // 右门机关：上下边界向中心压缩
        PHASE_FALLING,          // 角色跳入深渊，窗口+角色同步下坠
        PHASE_DREAM_SUCCESS,    // 显示"welcome to dream"
        PHASE_GAME_OVER         // 显示"wake up"弹窗（被边界挤压）
    };
    GamePhase currentPhase;

    // --- 场景元素（initScene中初始化） ---
    QRect leftDoorRect;         // 左门区域（像素风木门）
    QRect rightDoorRect;        // 右门区域（像素风木门）
    QRect abyssRect;            // 深渊区域（窄缝，可跳过）

    // --- 左门机关：左右边界向中心压缩 ---
    bool  leftTrapActivated;    // 左门机关是否已激活
    int   leftMargin;           // 当前左侧压缩边界X（从0向右移动）
    int   rightMargin;          // 当前右侧压缩边界X（从width()向左移动）
    int   originalWidth;        // 机关激活时记录的原始窗口宽度

    // --- 右门机关：上下边界向中心压缩 ---
    bool  rightTrapActivated;   // 右门机关是否已激活
    int   topMargin;            // 当前上侧压缩边界Y（从0向下移动）
    int   bottomMargin;         // 当前下侧压缩边界Y（从height()向上移动）
    int   originalHeight;       // 机关激活时记录的原始窗口高度

    // --- 深渊下坠 ---
    bool   abyssTriggered;      // 深渊是否已触发
    QTimer *fallTimer;          // 窗口+角色下坠定时器（每20ms）
    int    fallSpeed;           // 当前下坠速度（逐帧加速）

    // --- "welcome to dream"显示控制 ---
    bool   showSuccessMsg;      // 是否显示成功文字
    float  successAlpha;        // 文字透明度（用于淡入）
    QTimer *successMsgTimer;    // 成功文字显示计时器（5秒后可切换）

    // --- 雨动画 ---
    struct RainDrop {
        float x, y;             // 雨滴当前位置
        float speed;            // 下落速度
        float length;           // 雨丝长度
        float alpha;            // 透明度
    };
    QVector<RainDrop> rainDrops;  // 背景雨滴集合
    void initRain();              // 初始化雨滴
    void updateRain();            // 每帧更新雨滴位置

    // --- 像素风动画帧计数 ---
    int  globalTick;            // 全局帧计数（用于像素风闪烁/动画）

    // --- 初始化 ---
    void initScene();           // 初始化所有场景元素位置与状态

    // --- 各机关逻辑 ---
    void updateJump();                  // 更新跳跃物理
    void updateLeftTrap();              // 更新左门压缩机关
    void updateRightTrap();             // 更新右门压缩机关
    void checkAbyssEntry();             // 检测角色是否进入深渊
    void activateLeftTrap();            // 激活左门机关
    void activateRightTrap();           // 激活右门机关
    void triggerAbyss();                // 触发深渊（开始下坠）
    void checkBoundaryCollision();      // 检测角色是否碰到压缩边界
    void showWakeUpDialog();            // 弹出"wake up suddenly"对话框
    void resetToStart();                // 重置到出生点

    // --- 绘制各层（像素风分层渲染） ---
    void drawBackground(QPainter &p);       // 绘制背景（窗外下雨+图书馆内景）
    void drawBookshelf(QPainter &p);        // 绘制背景书架
    void drawRainWindow(QPainter &p);       // 绘制窗外下雨场景
    void drawFloorAndAbyss(QPainter &p);    // 绘制地板与深渊
    void drawDoors(QPainter &p);            // 绘制左右两扇像素风木门
    void drawTitle(QPainter &p);            // 绘制标题"Fall Asleep in Lib"
    void drawPlayer(QPainter &p);           // 绘制角色
    void drawDoorSpaceHint(QPainter &p);    // 绘制靠近门时的SPACE提示
    void drawCompressionBounds(QPainter &p); // 绘制压缩边界（压迫感墙壁）
    void drawSuccessMessage(QPainter &p);   // 绘制"welcome to dream"覆盖文字

    // --- 像素风辅助绘制 ---
    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border); // 绘制像素风矩形（无抗锯齿）
};

#endif // WIDGET_H
