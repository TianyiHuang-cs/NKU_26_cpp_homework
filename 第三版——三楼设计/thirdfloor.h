#ifndef THIRDFLOOR_H
#define THIRDFLOOR_H

#include "basefloor.h"
#include "notedialog.h"
#include "bagwindow.h"
#include <QPixmap>
#include <QRect>

// ============================================================
// ThirdFloor：第三层——循环走廊
//
// 核心机制：
//   1. 场景宽 2400px，摄像机跟随角色
//   2. 走廊无限循环（puzzleSolved 后循环停止）
//   3. 左桌(A区)、右桌(C区)：便签机关，可多次重复触发
//   4. 中桌(B区)：台灯跳跃机关（无SPACE提示，玩家自行探索）
//      ★ 跳跃后头顶遮住B下半部分 → 触发通关
//   5. 通关后：
//      ★ 时间倒流10分钟（emit puzzleSolved(10)）
//      ★ 走廊循环停止
//      ★ 最右侧出现明显的彩色高亮大门
//      ★ 靠近门：显示 Up(4F) / Down(2F) 按钮
//      ★ 显示"找到D区"弹窗（仅文字，无上下行按钮页）
//   6. 机关成功后永久失效；失败可重复触发
// ============================================================
class ThirdFloor : public BaseFloor
{
    Q_OBJECT
public:
    explicit ThirdFloor(QObject *parent = nullptr);
    ~ThirdFloor() override;

    void onEnter(int windowW, int windowH) override;
    void onExit()  override;
    void update(Player &player, int windowW, int windowH) override;
    void render(QPainter &painter, const Player &player)  override;
    void onKeyPress(QKeyEvent *event,   Player &player) override;
    void onKeyRelease(QKeyEvent *event, Player &player) override;
    void onMousePress(QMouseEvent *event) override;
    void onMouseMove(QMouseEvent *event)  override;

    void setBagWindow(BagWindow *bag) { m_bagWin = bag; }
    int getGroundY() const { return m_groundY; }

private:
    // ===== 背景与场景 =====
    QPixmap m_bgPixmap;
    int     m_bgW, m_bgH;
    int     m_groundY;
    // *** 地面Y = windowH * 0.78，微调改此比例 ***

    // ===== 摄像机 =====
    float   m_cameraX;
    float   m_targetCameraX;

    static const int BG_SCALE_W = 2400;
    static const int BG_SCALE_H = 600;

    // ===== 角色世界坐标 =====
    int     m_worldX;

    // ===== 跳跃 =====
    bool    m_isJumping;
    float   m_jumpVelocity;
    static constexpr float GRAVITY = 0.55f;

    // ===== 站上台灯 =====
    bool    m_onLamp;
    int     m_lampWorldX;
    int     m_lampTopY;
    // *** 台灯顶部 = groundY - 130，微调改此偏移 ***

    // ===== 按键状态 =====
    bool    m_keyLeft, m_keyRight, m_keySpace, m_keyDown;

    // ===== 全局帧计数 =====
    int     m_tick;

    // ===== 机关状态 =====
    bool    m_leftNoteOpen;      // 当前左便签是否打开（关闭后重置，可重触）
    bool    m_rightNoteOpen;     // 当前右便签是否打开
    bool    m_centerTriggered;   // ★ 跳跃机关已触发（永久失效）
    bool    m_puzzleSolved;      // ★ 是否已破解（控制门/循环/按钮）

    // 接近检测
    bool    m_nearLeftDesk;
    bool    m_nearRightDesk;
    bool    m_nearCenterDesk;
    bool    m_nearDoor;

    // ===== 桌子世界X =====
    // *** 微调位置：修改下面的比例系数 ***
    int     m_leftDeskWorldX;    // BG_SCALE_W * 0.18 ≈ 432
    int     m_centerDeskWorldX;  // BG_SCALE_W * 0.50 = 1200
    int     m_rightDeskWorldX;   // BG_SCALE_W * 0.82 ≈ 1968
    int     m_doorWorldX;        // BG_SCALE_W - 60（最右侧）

    // ===== 过渡动画 =====
    bool    m_transitioning;
    float   m_transitionAlpha;
    int     m_transitionTarget;

    // ===== 弹窗 =====
    NoteDialog *m_leftNoteWin;
    NoteDialog *m_rightNoteWin;
    NoteDialog *m_solvedNoteWin;  // ★ 破解弹窗（仅文字，无上下行按钮页）

    // ===== 背包 =====
    BagWindow *m_bagWin;

    // ===== 绘制 =====
    void drawScene(QPainter &p);
    void drawDoor(QPainter &p);
    void drawHints(QPainter &p);
    void drawElevatorButtons(QPainter &p);
    void drawTransition(QPainter &p);
    void drawPlayerSprite(QPainter &p, const Player &player);
    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);

    int  worldToScreen(int worldX) const;
    bool nearWorld(int worldX, int range) const;
    void updateCamera(int windowW);
    void triggerSolved();   // 触发通关逻辑
    //---------------------------------------------------------------------------
signals:
    void gameEnd();
};

#endif // THIRDFLOOR_H
