#ifndef SHEETMUSICWINDOW_H
#define SHEETMUSICWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QPixmap>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QRandomGenerator>

// ============================================================
// SheetMusicWindow：钢琴琴谱弹窗
// 显示贝多芬《月光奏鸣曲》Op.27 No.2 第一乐章开头20音符
// 升c小调，4/4拍，原版三连八分音符织体
// 做旧羊皮纸风格，标准五线谱格式
// ============================================================
class SheetMusicWindow : public QWidget
{
    Q_OBJECT
public:
    explicit SheetMusicWindow(QWidget *parent = nullptr);

signals:
    // 用户选择加入背包（携带物品ID和图标）
    void addToBag(int itemId, const QPixmap &icon);

    // 用户选择放在原地（弹窗关闭，下次仍可触发）
    void leaveInPlace();

    // 窗口关闭
    void windowClosed();

protected:
    void paintEvent(QPaintEvent *event)      override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event)  override;
    void mouseReleaseEvent(QMouseEvent *)    override;

private slots:
    void onAddToBag();       // 加入背包按钮
    void onLeaveInPlace();   // 放在原地按钮 / 关闭按钮

private:
    QPushButton *m_addBagBtn;  // 加入背包
    QPushButton *m_leaveBtn;   // 放在原地
    QPushButton *m_closeBtn;   // 右上角关闭（✕）

    // 无边框窗口拖动支持
    bool   m_dragging;
    QPoint m_dragOffset;

    // ---- 五线谱绘制辅助函数 ----

    // 绘制五线谱（5条横线）
    // x,y：左上角坐标；w：宽度；lineGap：线间距
    void drawStaff(QPainter &p, int x, int y, int w, int lineGap);

    // 绘制升号（#）：cx,cy 为升号中心坐标
    void drawSharp(QPainter &p, int cx, int cy);

    // 绘制单个音符
    // cx,cy：音符椭圆中心；filled：实心；stemUp：符干朝上；beam：有符尾
    void drawNote(QPainter &p, int cx, int cy,
                  bool filled, bool stemUp, bool beam);

    // 绘制加线（超出五线谱范围时）
    void drawLedgerLine(QPainter &p, int cx, int y);

    // 生成背包显示用的乐谱图标（白纸+五线谱线）
    QPixmap generateSheetIcon();

    // 像素风矩形辅助
    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);
};

#endif // SHEETMUSICWINDOW_H
