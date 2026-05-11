#ifndef SECONDFLOOR_H
#define SECONDFLOOR_H

#include "basefloor.h"
#include "catpuzzlewindow.h"
#include "sheetmusicwindow.h"
#include "bagwindow.h"
#include <QPixmap>
#include <QRect>
#include <QTimer>

// ============================================================
// SecondFloor：第二层图书馆场景
//
// 场景元素（对应背景图）：
//   - 背景：月夜图书馆大厅（钢琴、猫猫、书架、大门、楼梯）
//   - 猫猫机关：靠近触发投喂谜题
//   - 钢琴机关：靠近琴凳触发琴谱弹窗
//   - 书架时间机关：走动时根据X位置动态改变显示时间偏移
//   - 大门机关：时间判断，11:00能进但显示提示
//   - 楼梯机关：上/下键或鼠标切换楼层
//   - 月光驻留通关：在两书架中间静止20秒触发通关
// ============================================================
class SecondFloor : public BaseFloor
{
    Q_OBJECT
public:
    explicit SecondFloor(QObject *parent = nullptr);
    ~SecondFloor() override;

    void onEnter(int windowW, int windowH) override;
    void onExit()  override;
    void update(Player &player, int windowW, int windowH) override;
    void render(QPainter &painter, const Player &player)  override;
    void onKeyPress(QKeyEvent *event,   Player &player) override;
    void onKeyRelease(QKeyEvent *event, Player &player) override;
    void onMousePress(QMouseEvent *event) override;
    void onMouseMove(QMouseEvent *event)  override;

    // Widget 调用：注入背包窗口（全局共享）
    void setBagWindow(BagWindow *bag) { m_bagWin = bag; }

    // Widget 调用：注入当前全局时间（分钟数），用于基准时间
    // 每次进入本层时调用一次
    void setBaseTime(int hour, int minute) {
        m_baseHour   = hour;
        m_baseMinute = minute;
    }



signals:
    // 通关动画完成后的信号（预留，触发后续过场）
    void clearanceAchieved();

private:
    // ===== 背景图 =====
    QPixmap m_bgPixmap;

    // ===== 场景布局 =====
    int   m_groundY;            // 地面Y（脚底线）

    // 各区域检测矩形
    QRect m_catRect;            // 猫猫位置（中央地面）
    QRect m_pianoRect;          // 钢琴凳位置（靠近触发）
    QRect m_leftShelfRect;      // 左侧书架区域右边界
    QRect m_rightShelfRect;     // 右侧书架区域
    QRect m_doorRect;           // 右侧大门
    QRect m_stairRect;          // 左侧楼梯区域
    QRect m_moonZoneRect;       // 月光驻留区域（两书架中间）

    // ===== 接近检测标志 =====
    bool m_nearCat;
    bool m_nearPiano;
    bool m_nearDoor;
    bool m_nearStair;
    bool m_inMoonZone;          // 是否在月光区域内

    // ===== 按键状态 =====
    bool m_keyLeft, m_keyRight;
    bool m_keyUp, m_keyDown;
    bool m_keySpace;

    // ===== 跳跃系统 =====
    bool  m_isJumping;
    float m_jumpVelocity;
    static constexpr float GRAVITY = 0.6f;

    // ===== 帧计数 =====
    int m_tick;

    // ===== 书架时间偏移机关 =====
    // 基准时间（进入本层时由Widget注入）
    int m_baseHour;
    int m_baseMinute;

    // ===== 猫猫机关 =====
    CatPuzzleWindow *m_catWin;
    // 是否为猫条的判断（物品ID=99 表示猫条，后续在背包添加）
    static const int CAT_SNACK_ID = 99;

    // ===== 钢琴机关 =====
    SheetMusicWindow *m_sheetWin;
    bool m_pianoTriggered;      // 已加入背包则失效

    // ===== 大门机关 =====
    bool m_doorMessageShown;    // 防止重复弹出
    QString m_doorMessage;      // 大门提示文字
    bool m_showDoorMsg;         // 是否显示大门文字
    int  m_doorMsgTimer;        // 显示计时（帧数）

    // ===== 月光驻留通关 =====
    int  m_moonStillTick;       // 在月光区域静止帧数（20秒=1000帧@20ms）
    bool m_moonCleared;         // 是否已通关
    float m_clearAlpha;         // 通关遮罩透明度

    // ===== 过渡动画（楼层切换） =====
    bool  m_transitioning;
    float m_transitionAlpha;
    int   m_transitionTarget;   // 目标楼层

    // ===== 子窗口 =====
    BagWindow *m_bagWin;        // 由Widget注入

    // ===== 绘制函数 =====
    void drawBackground(QPainter &p);
    void drawCatHint(QPainter &p);
    void drawPianoHint(QPainter &p);
    void drawDoorHint(QPainter &p);
    void drawStairHint(QPainter &p);
    void drawMoonProgress(QPainter &p);    // 月光驻留进度条
    void drawDoorMessage(QPainter &p);     // 大门提示文字
    void drawClearance(QPainter &p);       // 通关遮罩
    void drawTransition(QPainter &p);      // 楼层切换遮罩
    void drawPlayerSprite(QPainter &p, const Player &player);
    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);

    // 计算当前显示时间（基准 + 偏移）
    // outHour/outMinute：输出参数
    void calcDisplayTime(int &outHour, int &outMinute) const;
    bool m_moonShowDialog;  // 是否显示未完成任务弹窗
    int  m_moonEndingType;  // 结局类型（1=正式结局）

private slots:
    void onCatWindowClosed();
    void onRequestOpenBagFromCat();
    void onCatItemSelected(int /*slotIndex*/, int itemId, const QPixmap &icon);
    void onSheetAddToBag(int itemId, const QPixmap &icon);
    void onSheetLeaveInPlace();
};

#endif // SECONDFLOOR_H
