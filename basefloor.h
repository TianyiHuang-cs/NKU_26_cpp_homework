#ifndef BASEFLOOR_H
#define BASEFLOOR_H

#include <QObject>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include "player.h"

// ============================================================
// BaseFloor：所有楼层场景的抽象基类
//
// 每一层继承此类并实现纯虚函数，Widget 通过基类指针统一调度。
// 公共资源（Player、时钟、背包）由 Widget 持有并以指针传入，
// 各楼层只负责自己的场景逻辑与绘制。
// ============================================================
class BaseFloor : public QObject
{
    Q_OBJECT

public:
    explicit BaseFloor(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~BaseFloor() = default;

    // --------------------------------------------------------
    // 生命周期
    // --------------------------------------------------------

    // 进入本层时调用：初始化场景元素、设置玩家出生点
    // windowW/windowH：当前窗口尺寸
    virtual void onEnter(int windowW, int windowH) = 0;

    // 离开本层时调用：清理临时状态（弹窗、动画等）
    virtual void onExit() = 0;

    // --------------------------------------------------------
    // 每帧更新（由 Widget 的 gameTimer 驱动，20ms/帧）
    // player：Widget 持有的玩家对象（引用传入，各层直接操作）
    // --------------------------------------------------------
    virtual void update(Player &player, int windowW, int windowH) = 0;

    // --------------------------------------------------------
    // 绘制（由 Widget::paintEvent 调用）
    // 绘制顺序：背景 → 场景元素 → 角色 → UI覆盖层
    // 角色由各层自行绘制（方便控制层次）
    // --------------------------------------------------------
    virtual void render(QPainter &painter, const Player &player) = 0;

    // --------------------------------------------------------
    // 输入事件转发
    // --------------------------------------------------------
    virtual void onKeyPress(QKeyEvent *event,   Player &player) = 0;
    virtual void onKeyRelease(QKeyEvent *event, Player &player) = 0;
    virtual void onMousePress(QMouseEvent *event) = 0;
    virtual void onMouseMove(QMouseEvent *event)  = 0;

signals:
    // 请求切换到指定楼层（floorIndex：1~4）
    void requestFloorChange(int floorIndex);

    // 谜题破解成功，时间倒流（minutes：正数表示倒流分钟数）
    void puzzleSolved(int minutes);
};

#endif // BASEFLOOR_H
