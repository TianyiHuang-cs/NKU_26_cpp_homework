#ifndef FIRSTFLOOR_H
#define FIRSTFLOOR_H

#include "basefloor.h"
#include "gramophonewindow.h"
#include "bagwindow.h"
#include "notedialog.h"
#include <QPixmap>
#include <QRect>

// ============================================================
// FirstFloor：第一层图书馆场景
//
// 场景元素：
//   - 背景图（暗黑像素风图书馆）
//   - 留声机机关（SPACE 触发弹窗谜题）
//     ★ 正确答案：背包中三张便签纸（ID: 102, 103, 104）
//     ★ 答对后：时间倒流10分钟 + 机关永久失效
//     ★ 答错后：可重复触发
//   - 右侧小黑门 + 电梯上行按钮（切换至第二层）
// ============================================================
class FirstFloor : public BaseFloor
{
    Q_OBJECT

public:
    explicit FirstFloor(QObject *parent = nullptr);
    ~FirstFloor() override;

    // --- BaseFloor 接口实现 ---
    void onEnter(int windowW, int windowH) override;
    void onExit()  override;
    void update(Player &player, int windowW, int windowH) override;
    void render(QPainter &painter, const Player &player)  override;
    void onKeyPress(QKeyEvent *event,   Player &player) override;
    void onKeyRelease(QKeyEvent *event, Player &player) override;
    void onMousePress(QMouseEvent *event) override;
    void onMouseMove(QMouseEvent *event)  override;
    void setBagWindow(BagWindow *bag);

private:
    // ===== 场景资源 =====
    QPixmap m_bgPixmap;

    // ===== 场景布局 =====
    int   m_groundY;
    QRect m_doorRect;
    QRect m_gramophoneRect;

    // ===== 接近检测 =====
    bool m_nearDoor;
    bool m_nearGramophone;

    // ===== 跳跃系统 =====
    bool  m_isJumping;
    float m_jumpVelocity;
    static constexpr float GRAVITY = 0.6f;

    // ===== 按键状态 =====
    bool m_keyLeft, m_keyRight;
    bool m_keyUp;
    bool m_keySpace;

    // ===== 全局帧计数 =====
    int m_tick;

    // ===== 机关状态 =====
    // ★ 答对后设为 true，此后留声机提示和触发均关闭
    bool m_gramophoneSolved;

    // ===== 子窗口 =====
    GramophoneWindow *m_gramWin;
    BagWindow        *m_bagWin;

    // ★ 时间倒流提示弹窗（答对后显示，风格与 NoteDialog 一致）
    NoteDialog       *m_rewindNoteDlg;

    // ===== 场景切换动画 =====
    bool  m_transitioning;
    float m_transitionAlpha;
    int   m_transitionDir;

    // ===== 绘制私有函数 =====
    void drawBackground(QPainter &p);
    void drawDoor(QPainter &p);
    void drawElevatorButton(QPainter &p);
    void drawGramophoneHint(QPainter &p);
    void drawPlayerSprite(QPainter &p, const Player &player);
    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);

private slots:
    void onRequestOpenBag(int slotIndex);
    void onItemSelected(int slotIndex, int itemId, const QPixmap &icon);
    void onGramophoneWindowClosed();
    void onGramophonePuzzleSolved();   // ★ 新增：答对处理
};

#endif // FIRSTFLOOR_H
