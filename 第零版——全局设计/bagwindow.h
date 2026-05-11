#ifndef BAGWINDOW_H
#define BAGWINDOW_H

#include <QWidget>
#include <QPixmap>
#include <QVector>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPushButton>

// ============================================================
// BagItem：背包中单个物品的数据结构
// ============================================================
struct BagItem {
    int     id;      // 物品唯一 ID（-1 表示空格）
    QString name;    // 物品名称（显示在格子下方）
    QPixmap icon;    // 物品图标（建议 64×64 像素）
};

// ============================================================
// BagWindow：背包界面弹窗
// 无系统标题栏，自绘像素风外观
// 3×3 网格，点击物品后发射 itemSelected 信号
// ============================================================
class BagWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BagWindow(QWidget *parent = nullptr);

    // --------------------------------------------------------
    // 【接口】添加物品到背包
    // id：物品唯一 ID
    // name：显示名称
    // iconPath：图标资源路径（如 ":/images/item_heart.png"）
    // 背包最多 9 格（3×3），超出忽略
    //
    // 使用示例（在 Widget 构造函数中）：
    //   bagWindow->addItem(1, "心跳声", ":/images/item_heart.png");
    //   bagWindow->addItem(2, "雨声",   ":/images/item_rain.png");
    //   bagWindow->addItem(3, "风声",   ":/images/item_wind.png");
    // --------------------------------------------------------
    void addItem(int id, const QString &name, const QPixmap &icon);
    // 设置当前要填充的目标槽位序号（从 GramophoneWindow 传入）
    void setTargetSlot(int slotIndex) { m_targetSlot = slotIndex; }

signals:
    // 用户选中了某个物品，携带：目标槽位序号、物品ID、物品图标
    void itemSelected(int slotIndex, int itemId, const QPixmap &icon);

    // 背包窗口关闭
    void bagClosed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // 3×3 格子，最多 9 个物品
    QVector<BagItem> m_items;

    // 目标槽位序号（选物品后填入哪个槽）
    int m_targetSlot;

    // 悬停格子序号（-1 表示无悬停，用于高亮反馈）
    int m_hoveredCell;

    // 无边框拖动支持
    bool   m_dragging;
    QPoint m_dragOffset;

    // 格子布局常量
    static const int CELL_SIZE    = 80;  // 每格像素大小
    static const int CELL_PADDING = 10;  // 格子间距
    static const int GRID_OFFSET_X = 28; // 网格左边距
    static const int GRID_OFFSET_Y = 60; // 网格上边距（顶部留标题区）

    // 根据鼠标位置返回格子序号，-1 表示不在格子内
    int cellAt(const QPoint &pos) const;

    QPushButton *m_closeBtn; // 右上角关闭按钮
};

#endif // BAGWINDOW_H
