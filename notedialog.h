#ifndef NOTEDIALOG_H
#define NOTEDIALOG_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QPaintEvent>
#include <QMouseEvent>

// ============================================================
// NoteDialog：泛黄便签弹窗（两页式）
// 风格与前几层弹窗一致：像素风边框 + 羊皮纸底色 + 可拖动
// 支持富文本（加粗关键词）
// ============================================================
class NoteDialog : public QWidget
{
    Q_OBJECT
public:
    explicit NoteDialog(QWidget *parent = nullptr);

    // 设置两页内容
    // page1Html/page2Html 支持 <b>加粗</b> 标签
    void setContent(const QString &page1Html,
                    const QString &page2Html);

    // 设置弹窗标题
    void setTitle(const QString &title);

signals:
    void dialogClosed();

protected:
    void paintEvent(QPaintEvent *event)      override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event)  override;
    void mouseReleaseEvent(QMouseEvent *)    override;

private slots:
    void onNext();
    void onClose();

private:
    int     m_page;          // 0 或 1
    QString m_title;
    QString m_page1Html;
    QString m_page2Html;

    QPushButton *m_nextBtn;
    QPushButton *m_closeBtn;

    bool   m_dragging;
    QPoint m_dragOffset;

    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);
};

#endif // NOTEDIALOG_H
