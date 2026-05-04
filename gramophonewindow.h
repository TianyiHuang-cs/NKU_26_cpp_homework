#ifndef GRAMOPHONEWINDOW_H
#define GRAMOPHONEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QMouseEvent>
#include <QPaintEvent>

// ============================================================
// SlotWidget：留声机谜题的单个"投放槽"控件
// 显示已放入的物品图标，支持点击打开背包界面
// ============================================================
class SlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SlotWidget(QWidget *parent = nullptr);

    // 放入物品：传入物品ID和图标
    void setItem(int itemId, const QPixmap &icon);

    // 清空此槽位
    void clearItem();

    // 返回当前槽位的物品ID（-1 表示空）
    int  getItemId() const { return m_itemId; }

    // 返回此槽是否已有物品
    bool hasItem()   const { return m_itemId >= 0; }

    //获取私有数据
    void setSlotIndex(int index) { m_slotIndex = index; }

    QPixmap getIcon() const { return m_icon; }

signals:
    // 用户点击此槽（请求打开背包选择物品放入此槽）
    void slotClicked(int slotIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    int     m_slotIndex; // 槽位序号（0/1/2）
    int     m_itemId;    // 当前放入物品的 ID（-1=空）
    QPixmap m_icon;      // 当前物品图标
};


// ============================================================
// GramophoneWindow：留声机谜题弹窗
// 无系统标题栏，自绘暗黑像素风外观
// 包含：谜题说明、三个投放槽、确认/取消按钮、结果提示
// ============================================================
class GramophoneWindow : public QWidget
{
    Q_OBJECT
public:
    explicit GramophoneWindow(QWidget *parent = nullptr);

    // --------------------------------------------------------
    // 【接口】设置正确答案
    // correctIds：长度为 3 的 vector，依次是三个槽位应放的物品 ID
    // 顺序不要求完全一致（只要三个 ID 的集合相同即判断正确）
    // 传入空 vector 表示暂不设置（当前初始化为空，始终判错）
    // --------------------------------------------------------
    void setCorrectAnswer(const QVector<int> &correctIds);

    // 外部调用：往指定槽放入物品（从背包界面选择后回调）
    // slotIndex：0~2；itemId：物品ID；icon：图标
    void placeItemIntoSlot(int slotIndex, int itemId, const QPixmap &icon);

signals:
    // 用户点击槽位，请求打开背包（携带槽位序号）
    void requestOpenBag(int slotIndex);

    // 谜题成功破解（时间倒流信号，Widget 监听）
    void puzzleSolved();

    // 窗口关闭（Widget 监听，恢复游戏焦点）
    void windowClosed();

    // 答案错误，将槽位物品退回背包（携带槽位序号）
    void returnItemToBag(int slotIndex, int itemId, const QPixmap &icon);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onConfirmClicked();  // 确认按钮：校验答案
    void onCancelClicked();   // 取消/关闭按钮

private:
    // --- 三个投放槽 ---
    SlotWidget *m_slots[3];

    // --- 按钮 ---
    QPushButton *m_confirmBtn; // 确认
    QPushButton *m_cancelBtn;  // 取消/关闭

    // --- 结果标签 ---
    QLabel *m_resultLabel;

    // --- 正确答案集合（无序匹配） ---
    QVector<int> m_correctIds;

    // --- 无边框窗口拖动支持 ---
    bool   m_dragging;
    QPoint m_dragOffset;

    // 校验三个槽是否与正确答案匹配
    bool checkAnswer() const;
};

#endif // GRAMOPHONEWINDOW_H
