#ifndef CATPUZZLEWINDOW_H
#define CATPUZZLEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QPaintEvent>
#include <QMouseEvent>

// ============================================================
// CatPuzzleWindow：猫猫机关弹窗
// 两页式设计：
//   第一页：介绍文字 + 右下角右箭头翻页
//   第二页：投喂槽位 + 投喂按钮 + 结果显示
// 无系统标题栏，自绘像素风外观，可拖动
// ============================================================
class CatPuzzleWindow : public QWidget
{
    Q_OBJECT
public:
    explicit CatPuzzleWindow(QWidget *parent = nullptr);

    // 外部调用：将物品放入投喂槽（从背包选择后回调）
    // itemId：物品ID；icon：图标；isCatSnack：是否为猫条
    void placeItem(int itemId, const QPixmap &icon, const QString &itemName, bool isCatSnack);

signals:
    // 请求打开背包（选择投喂物品）
    void requestOpenBag();

    // 窗口关闭
    void windowClosed();

    // 无效物品返回背包：
    void returnItemToBag(int itemId, const QPixmap &icon,const QString &itemName);

protected:
    void paintEvent(QPaintEvent *event)      override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event)  override;
    void mouseReleaseEvent(QMouseEvent *)    override;

private slots:
    void onNextPage();      // 翻到第二页
    void onFeed();          // 投喂按钮
    void onClose();         // 关闭窗口

private:
    QString m_itemName;
    int  m_page;            // 当前页（0=第一页，1=第二页）

    // 投喂槽状态
    int     m_itemId;       // 放入物品ID（-1=空）
    QPixmap m_itemIcon;     // 物品图标
    bool    m_isCatSnack;   // 是否为猫条

    // 结果文字（投喂后显示）
    QString m_resultText;
    bool    m_showResult;

    // 按钮
    QPushButton *m_nextBtn;   // 第一页右箭头
    QPushButton *m_feedBtn;   // 第二页投喂按钮
    QPushButton *m_closeBtn;  // 右上角红叉

    // 投喂槽点击区域
    QRect m_slotRect;

    // 无边框拖动
    bool   m_dragging;
    QPoint m_dragOffset;

    void drawPage1(QPainter &p);
    void drawPage2(QPainter &p);
    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);
};

#endif // CATPUZZLEWINDOW_H
