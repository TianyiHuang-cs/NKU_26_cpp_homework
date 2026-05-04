#ifndef FIRSTFLOOR_H
#define FIRSTFLOOR_H

#include "basefloor.h"
#include "gramophonewindow.h"
#include "bagwindow.h"
#include <QPixmap>
#include <QRect>

// ============================================================
// FirstFloor：第一层图书馆场景
//
// 场景元素：
//   - 背景图（暗黑像素风图书馆）
//   - 留声机机关（SPACE 触发弹窗谜题）
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


private:
    // ===== 场景资源 =====
    QPixmap m_bgPixmap;         // 背景图

    // ===== 场景布局 =====
    int   m_groundY;            // 地面 Y（脚底站立线）
    QRect m_doorRect;           // 右侧小黑门区域
    QRect m_gramophoneRect;     // 留声机交互检测区域

    // ===== 接近检测 =====
    bool m_nearDoor;            // 角色是否靠近门
    bool m_nearGramophone;      // 角色是否靠近留声机

    // ===== 跳跃系统（FirstFloor 自管理） =====
    bool  m_isJumping;
    float m_jumpVelocity;
    static constexpr float GRAVITY = 0.6f;

    // ===== 按键状态 =====
    bool m_keyLeft, m_keyRight;
    bool m_keyUp;               // 上键：触发电梯
    bool m_keySpace;            // 空格：触发留声机

    // ===== 全局帧计数（闪烁动画） =====
    int m_tick;

    // ===== 子窗口 =====
    GramophoneWindow *m_gramWin;   // 留声机弹窗
    BagWindow        *m_bagWin;    // 背包弹窗（从父层获取或独立持有）

    // ===== 绘制私有函数 =====
    void drawBackground(QPainter &p);
    void drawDoor(QPainter &p);
    void drawElevatorButton(QPainter &p);
    void drawGramophoneHint(QPainter &p);
    void drawPlayerSprite(QPainter &p, const Player &player);
    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);
    // 场景切换动画（由下往上滑入第二层）
    bool  m_transitioning;     // 是否正在切换
    float m_transitionAlpha;   // 遮罩透明度 0→1→切换→1→0
    int   m_transitionDir;     // 1=向上切换到二层




private slots:
    void onRequestOpenBag(int slotIndex);
    void onItemSelected(int slotIndex, int itemId, const QPixmap &icon);
    void onGramophoneWindowClosed();
};

#endif // FIRSTFLOOR_H
