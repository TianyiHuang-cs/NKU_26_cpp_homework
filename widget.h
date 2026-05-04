#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include "player.h"
#include "basefloor.h"
#include "bagwindow.h"

// 前向声明各楼层（避免头文件互相包含）
class FirstFloor;
#include "secondfloor.h"
class ThirdFloor;
class FourthFloor;

// ============================================================
// Widget：游戏主窗口 & 场景管理器
//
// 职责：
//   1. 持有公共资源：Player、背包、时钟
//   2. 管理当前楼层（BaseFloor*），转发输入与绘制
//   3. 响应楼层发出的切换信号，完成场景切换
//   4. 绘制全局 UI（时钟、背包按钮）—— 始终在最顶层
// ============================================================
class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    // 供各楼层调用：获取背包窗口（用于打开背包）
    BagWindow *bagWindow() { return m_bagWin; }

protected:
    void paintEvent(QPaintEvent *event)   override;
    void keyPressEvent(QKeyEvent *event)  override;
    void keyReleaseEvent(QKeyEvent *event)override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event)  override;

private slots:
    // 主循环（20ms）
    void onGameTick();

    // 楼层切换请求
    void onFloorChangeRequested(int floorIndex);

    // 谜题破解，时间倒流
    void onPuzzleSolved(int minutes);

private:
    // ===== 公共游戏对象 =====
    Player     m_player;    // 玩家（跨楼层共用同一实例）
    BagWindow *m_bagWin;    // 背包弹窗（全局唯一）

    // ===== 时钟 =====
    int m_clockHour;        // 小时（初始 23）
    int m_clockMinute;      // 分钟（初始 0）

    // ===== 场景管理 =====
    BaseFloor  *m_currentFloor; // 当前楼层（多态指针）
    FirstFloor *m_floor1;       // 第一层实例
    SecondFloor*m_floor2;       // 第二层实例
    ThirdFloor *m_floor3;       // 第三层实例
    FourthFloor*m_floor4;       // 第四层实例

    // 执行楼层切换（floorIndex：1~4）
    void switchFloor(int floorIndex);

    // 将各楼层信号连接到 Widget 槽
    void connectFloor(BaseFloor *floor);

    // ===== 全局 UI 绘制 =====
    void drawClock(QPainter &p);         // 左上角时钟
    void drawBagButton(QPainter &p);     // 左上角背包按钮
    void drawPixelRect(QPainter &p,
                       int x, int y, int w, int h,
                       const QColor &fill,
                       const QColor &border);

    // 背包按钮区域与悬停状态
    QRect m_bagBtnRect;
    bool  m_bagBtnHovered;
};

#endif // WIDGET_H
