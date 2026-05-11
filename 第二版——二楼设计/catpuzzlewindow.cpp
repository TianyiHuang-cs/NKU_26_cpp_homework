#include "catpuzzlewindow.h"
#include <QPainter>
#include <QFont>
#include <QRadialGradient>
#include <QLinearGradient>
#include <cmath>
// ============================================================
// 构造函数：初始化两页式弹窗
// ============================================================
CatPuzzleWindow::CatPuzzleWindow(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
    m_page(0), m_itemId(-1), m_isCatSnack(false),
    m_showResult(false), m_dragging(false)
{
    setFixedSize(420, 300);
    setAttribute(Qt::WA_TranslucentBackground);

    // 投喂槽区域（第二页中央）
    m_slotRect = QRect(155, 130, 110, 110);

    // --- 右上角红叉关闭按钮 ---
    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setGeometry(390, 8, 22, 22);
    m_closeBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  color: #cc4444;"
        "  border: none;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { color: #ff6666; }"
        );
    connect(m_closeBtn, &QPushButton::clicked, this, &CatPuzzleWindow::onClose);

    // --- 第一页：右下角右箭头按钮 ---
    m_nextBtn = new QPushButton("▶", this);
    m_nextBtn->setGeometry(356, 252, 48, 32);
    m_nextBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3a2208;"
        "  color: #d4aa60;"
        "  border: 2px solid #6a4010;"
        "  font-size: 16px;"
        "}"
        "QPushButton:hover { background-color: #5a3410; }"
        );
    connect(m_nextBtn, &QPushButton::clicked, this, &CatPuzzleWindow::onNextPage);

    // --- 第二页：投喂按钮（初始隐藏） ---
    m_feedBtn = new QPushButton("投 喂", this);
    m_feedBtn->setGeometry(286, 252, 88, 32);
    m_feedBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3a2208;"
        "  color: #d4aa60;"
        "  border: 2px solid #6a4010;"
        "  font-family: 'Courier New';"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #5a3410; }"
        );
    m_feedBtn->hide();
    connect(m_feedBtn, &QPushButton::clicked, this, &CatPuzzleWindow::onFeed);
}

// ============================================================
// placeItem：外部将物品放入投喂槽
// ============================================================
void CatPuzzleWindow::placeItem(int itemId, const QPixmap &icon, const QString &itemName, bool isCatSnack)
{
    m_itemId     = itemId;
    m_itemIcon   = icon;
    m_itemName   = itemName;   // 直接存调用方传来的名字
    m_isCatSnack = isCatSnack;
    m_showResult = false;
    update();
}
// ============================================================
// paintEvent：根据当前页绘制内容
// ============================================================
void CatPuzzleWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int W = width(), H = height();

    // 主体背景
    p.fillRect(4, 4, W - 8, H - 8, QColor(18, 12, 6, 248));

    // 三层像素边框
    p.setPen(QPen(QColor(28, 18, 6), 2));
    p.drawRect(2, 2, W - 5, H - 5);
    p.setPen(QPen(QColor(100, 68, 22), 2));
    p.drawRect(4, 4, W - 9, H - 9);
    p.setPen(QPen(QColor(60, 40, 12), 1));
    p.drawRect(7, 7, W - 15, H - 15);

    // 顶部标题区
    p.fillRect(8, 8, W - 16, 44, QColor(35, 20, 7));
    p.setPen(QPen(QColor(90, 58, 20), 1));
    p.drawLine(8, 52, W - 9, 52);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont titleFont("Courier New", 12, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor(195, 150, 55));
    p.drawText(0, 8, W - 30, 44, Qt::AlignCenter, "— 神秘的猫猫 —");
    p.setRenderHint(QPainter::Antialiasing, false);

    if (m_page == 0)
        drawPage1(p);
    else
        drawPage2(p);
}

// ============================================================
// drawPixelRect：像素风矩形辅助
// ============================================================
void CatPuzzleWindow::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                                    const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

// ============================================================
// drawPage1：第一页内容
// ============================================================
void CatPuzzleWindow::drawPage1(QPainter &p)
{
    p.setRenderHint(QPainter::Antialiasing, true);

    // 正文
    QFont textFont("Courier New", 11);
    p.setFont(textFont);
    p.setPen(QColor(185, 155, 100));
    p.drawText(30, 65, width() - 60, 160,
               Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
               "猫猫好像知道什么，\n\n可是有点饿了，\n\n不想理你。");

    // 猫咪像素图标（简单像素猫轮廓）
    p.setRenderHint(QPainter::Antialiasing, false);
    // 猫身
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(60, 55, 50));
    p.drawRect(310, 100, 60, 45);
    // 猫头
    p.drawRect(318, 72, 44, 36);
    // 猫耳（左）
    p.drawRect(318, 62, 12, 14);
    // 猫耳（右）
    p.drawRect(348, 62, 12, 14);
    // 猫眼（黄色）
    p.setBrush(QColor(200, 170, 40));
    p.drawRect(324, 82, 8, 6);
    p.drawRect(348, 82, 8, 6);
    // 猫鼻（粉色）
    p.setBrush(QColor(200, 100, 110));
    p.drawRect(337, 91, 6, 4);
    // 猫尾
    p.setBrush(QColor(60, 55, 50));
    p.drawRect(368, 128, 8, 28);
    p.drawRect(360, 150, 10, 8);
    // 猫胡须
    p.setPen(QPen(QColor(200, 195, 185), 1));
    p.drawLine(316, 93, 328, 91); // 左胡须上
    p.drawLine(316, 97, 328, 95); // 左胡须下
    p.drawLine(352, 91, 362, 93); // 右胡须上
    p.drawLine(352, 95, 362, 97); // 右胡须下
}

// ============================================================
// drawPage2：第二页内容（投喂槽 + 结果）
// ============================================================
void CatPuzzleWindow::drawPage2(QPainter &p)
{
    p.setRenderHint(QPainter::Antialiasing, true);

    // 提示文字
    QFont hintFont("Courier New", 10);
    p.setFont(hintFont);
    p.setPen(QColor(155, 125, 75));
    p.drawText(0, 60, width(), 30, Qt::AlignCenter, "请选择投喂的东西：");

    // 投喂槽（居中）
    p.setRenderHint(QPainter::Antialiasing, false);
    drawPixelRect(p, m_slotRect.x(), m_slotRect.y(),
                  m_slotRect.width(), m_slotRect.height(),
                  QColor(22, 14, 6, 220), QColor(100, 68, 25));
    p.setPen(QPen(QColor(60, 40, 12), 1));
    p.drawRect(m_slotRect.x() + 3, m_slotRect.y() + 3,
               m_slotRect.width() - 7, m_slotRect.height() - 7);

    if (m_itemId >= 0 && !m_itemIcon.isNull()) {
        // 有物品：绘制图标
        QPixmap scaled = m_itemIcon.scaled(80, 80, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
        int ix = m_slotRect.x() + (m_slotRect.width()  - scaled.width())  / 2;
        int iy = m_slotRect.y() + (m_slotRect.height() - scaled.height()) / 2;
        p.drawPixmap(ix, iy, scaled);
    } else {
        // 空槽提示
        p.setRenderHint(QPainter::Antialiasing, true);
        QFont emptyFont("Courier New", 9);
        p.setFont(emptyFont);
        p.setPen(QColor(90, 68, 35, 180));
        p.drawText(m_slotRect, Qt::AlignCenter, "点击\n选择");
        p.setRenderHint(QPainter::Antialiasing, false);
    }

    // 结果文字（投喂后显示）
    if (m_showResult) {
        p.setRenderHint(QPainter::Antialiasing, true);
        QFont resFont("Courier New", 10);
        p.setFont(resFont);
        QColor resColor = m_isCatSnack ? QColor(100, 200, 100) : QColor(200, 120, 80);
        p.setPen(resColor);
        p.drawText(16, 68, width() - 120, 160,
                   Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                   m_resultText);
    }
}

// ============================================================
// onNextPage：翻到第二页，切换按钮显示
// ============================================================
void CatPuzzleWindow::onNextPage()
{
    m_page = 1;
    m_nextBtn->hide();
    m_feedBtn->show();
    update();
}

// ============================================================
// onFeed：投喂按钮点击，根据物品判断结果
// ============================================================
void CatPuzzleWindow::onFeed()
{
    if (m_itemId < 0) {
        m_resultText = "放点东西再来~";
        m_isCatSnack = false;
        m_showResult = true;
        update();
        return;
    }
    if (m_isCatSnack) {
        m_resultText = "嘿嘿，猫猫啥也不知道 <(0 ^ 0)>";
    } else {
        m_resultText = "猫猫给了你个白眼。。。";
        // 退回时带上原名
        emit returnItemToBag(m_itemId, m_itemIcon, m_itemName);
        m_itemId   = -1;
        m_itemName = "";
        m_itemIcon = QPixmap();
    }
    m_showResult = true;
    update();
}

// ============================================================
// onClose：关闭弹窗，重置状态
// ============================================================
void CatPuzzleWindow::onClose()
{
    // 重置到第一页
    m_page       = 0;
    m_itemId     = -1;
    m_showResult = false;
    m_nextBtn->show();
    m_feedBtn->hide();
    hide();
    emit windowClosed();
}

// ============================================================
// 鼠标事件：拖动（标题区）+ 点击槽位（打开背包）
// ============================================================
void CatPuzzleWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    // 标题区拖动
    if (event->pos().y() < 55) {
        m_dragging   = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        return;
    }

    // 第二页点击槽位 → 打开背包
    if (m_page == 1 && m_slotRect.contains(event->pos()))
        emit requestOpenBag();
}

void CatPuzzleWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging)
        move(event->globalPos() - m_dragOffset);
}

void CatPuzzleWindow::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}
