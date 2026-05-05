#include "notedialog.h"
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QFont>

NoteDialog::NoteDialog(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
    m_page(0), m_dragging(false)
{
    setFixedSize(400, 280);
    setAttribute(Qt::WA_TranslucentBackground);

    // 右上角关闭按钮
    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setGeometry(370, 8, 22, 22);
    m_closeBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#8B7355;"
        "  border:none; font-size:14px; font-weight:bold; }"
        "QPushButton:hover { color:#cc4444; }"
        );
    connect(m_closeBtn, &QPushButton::clicked, this, &NoteDialog::onClose);

    // 右下角翻页按钮（第一页显示，第二页隐藏）
    m_nextBtn = new QPushButton("▶", this);
    m_nextBtn->setGeometry(346, 236, 42, 30);
    m_nextBtn->setStyleSheet(
        "QPushButton { background:#3a2a10; color:#c8a050;"
        "  border:2px solid #6a5020; font-size:14px; }"
        "QPushButton:hover { background:#5a4018; }"
        );
    connect(m_nextBtn, &QPushButton::clicked, this, &NoteDialog::onNext);
}

void NoteDialog::setContent(const QString &p1, const QString &p2)
{
    m_page1Html = p1;
    m_page2Html = p2;
    m_page = 0;
    m_nextBtn->show();
    update();
}

void NoteDialog::setTitle(const QString &title)
{
    m_title = title;
    update();
}

void NoteDialog::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                               const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

void NoteDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    int W = width(), H = height();

    // 做旧纸张背景
    p.fillRect(4, 4, W - 8, H - 8, QColor(235, 220, 180, 248));

    // 纸张边缘渐变（做旧）
    QLinearGradient el(0,0,16,0); el.setColorAt(0,QColor(160,130,70,140)); el.setColorAt(1,Qt::transparent);
    p.fillRect(0,0,16,H,el);
    QLinearGradient er(W-16,0,W,0); er.setColorAt(0,Qt::transparent); er.setColorAt(1,QColor(160,130,70,140));
    p.fillRect(W-16,0,16,H,er);
    QLinearGradient et(0,0,0,14); et.setColorAt(0,QColor(160,130,70,120)); et.setColorAt(1,Qt::transparent);
    p.fillRect(0,0,W,14,et);
    QLinearGradient eb(0,H-14,0,H); eb.setColorAt(0,Qt::transparent); eb.setColorAt(1,QColor(160,130,70,120));
    p.fillRect(0,H-14,W,14,eb);

    // 像素外框
    p.setPen(QPen(QColor(100, 75, 35), 2));
    p.drawRect(3, 3, W - 7, H - 7);
    p.setPen(QPen(QColor(145, 110, 50), 1));
    p.drawRect(6, 6, W - 13, H - 13);

    // 标题区
    p.fillRect(8, 8, W - 16, 38, QColor(180, 148, 80, 120));
    p.setPen(QPen(QColor(120, 88, 38), 1));
    p.drawLine(8, 46, W - 9, 46);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont tf("Georgia", 11, QFont::Bold);
    tf.setItalic(true);
    p.setFont(tf);
    p.setPen(QColor(55, 35, 10));
    p.drawText(0, 8, W - 28, 38, Qt::AlignCenter, m_title);

    // 正文（富文本渲染）
    QString html = (m_page == 0) ? m_page1Html : m_page2Html;

    QTextDocument doc;
    doc.setDefaultFont(QFont("Courier New", 11));
    doc.setDefaultStyleSheet(
        "body { color: #3a2508; line-height: 1.6; }"
        "b { color: #1a0a00; font-weight: bold; }"
        );
    doc.setHtml("<body>" + html + "</body>");
    doc.setTextWidth(W - 40);

    p.save();
    p.translate(20, 54);
    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, QColor(55, 35, 12));
    doc.documentLayout()->draw(&p, ctx);
    p.restore();

    p.setRenderHint(QPainter::Antialiasing, false);
}

void NoteDialog::onNext()
{
    m_page = 1;
    m_nextBtn->hide();
    update();
}

void NoteDialog::onClose()
{
    m_page = 0;
    m_nextBtn->show();
    hide();
    emit dialogClosed();
}

void NoteDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->pos().y() < 50) {
        m_dragging   = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
    }
}
void NoteDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) move(event->globalPos() - m_dragOffset);
}
void NoteDialog::mouseReleaseEvent(QMouseEvent *) { m_dragging = false; }
