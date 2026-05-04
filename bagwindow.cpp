#include "bagwindow.h"
#include <QPainter>
#include <QFont>

// ============================================================
// 构造函数：初始化背包弹窗
// ============================================================
BagWindow::BagWindow(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
    m_targetSlot(-1), m_hoveredCell(-1),
    m_dragging(false)
{
    // 背包窗口固定大小：3×3 格子 + 边距 + 标题区
    int W = GRID_OFFSET_X * 2 + 3 * CELL_SIZE + 2 * CELL_PADDING;
    int H = GRID_OFFSET_Y + 3 * CELL_SIZE + 2 * CELL_PADDING + 30;
    setFixedSize(W, H);

    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true); // 开启鼠标追踪以支持悬停高亮
    // 右上角关闭按钮（透明底色白叉）
    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setGeometry(W - 28, 6, 22, 22);
    m_closeBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  color: #aaaaaa;"
        "  border: none;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { color: #ffffff; }"
        );
    connect(m_closeBtn, &QPushButton::clicked, this, [this](){
        hide();
        emit bagClosed();
    });
}

// ============================================================
// 添加物品到背包（最多9格）
// ============================================================
void BagWindow::addItem(int id, const QString &name, const QPixmap &iconPath)
{
    if (m_items.size() >= 9) return; // 背包已满，忽略
    BagItem item;
    item.id   = id;
    item.name = name;
    item.icon = QPixmap(iconPath).scaled(
        CELL_SIZE - 16, CELL_SIZE - 16,
        Qt::KeepAspectRatio, Qt::SmoothTransformation
        );
    m_items.append(item);
}

// ============================================================
// 根据鼠标坐标返回格子序号（0~8），-1 表示不在格子内
// ============================================================
int BagWindow::cellAt(const QPoint &pos) const
{
    for (int i = 0; i < 9; i++) {
        int col = i % 3;
        int row = i / 3;
        int cx  = GRID_OFFSET_X + col * (CELL_SIZE + CELL_PADDING);
        int cy  = GRID_OFFSET_Y + row * (CELL_SIZE + CELL_PADDING);
        QRect cellRect(cx, cy, CELL_SIZE, CELL_SIZE);
        if (cellRect.contains(pos)) return i;
    }
    return -1;
}

// ============================================================
// 绘制背包窗口：像素风面板 + 3×3 格子 + 物品图标
// ============================================================
void BagWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int W = width(), H = height();

    // --- 主体背景 ---
    p.fillRect(4, 4, W - 8, H - 8, QColor(15, 10, 5, 240));

    // --- 像素风边框（三层） ---
    p.setPen(QPen(QColor(25, 15, 5), 2));
    p.drawRect(2, 2, W - 5, H - 5);
    p.setPen(QPen(QColor(100, 68, 25), 2));
    p.drawRect(4, 4, W - 9, H - 9);
    p.setPen(QPen(QColor(60, 40, 14), 1));
    p.drawRect(7, 7, W - 15, H - 15);

    // --- 顶部标题区 ---
    p.fillRect(8, 8, W - 16, 44, QColor(35, 20, 7));
    p.setPen(QPen(QColor(90, 58, 20), 1));
    p.drawLine(8, 52, W - 9, 52);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont titleFont("Courier New", 12, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor(195, 150, 55));
    p.drawText(0, 8, W, 44, Qt::AlignCenter, "— 背 包 —");
    p.setRenderHint(QPainter::Antialiasing, false);

    // --- 绘制 3×3 格子 ---
    for (int i = 0; i < 9; i++) {
        int col = i % 3;
        int row = i / 3;
        int cx  = GRID_OFFSET_X + col * (CELL_SIZE + CELL_PADDING);
        int cy  = GRID_OFFSET_Y + row * (CELL_SIZE + CELL_PADDING);

        // 格子背景：悬停时高亮
        QColor bgColor = (i == m_hoveredCell && i < m_items.size())
                             ? QColor(55, 35, 12, 200)
                             : QColor(25, 14, 5, 200);
        p.fillRect(cx, cy, CELL_SIZE, CELL_SIZE, bgColor);

        // 格子边框
        p.setPen(QPen(QColor(75, 50, 18), 2));
        p.drawRect(cx, cy, CELL_SIZE - 1, CELL_SIZE - 1);
        p.setPen(QPen(QColor(45, 28, 8), 1));
        p.drawRect(cx + 2, cy + 2, CELL_SIZE - 5, CELL_SIZE - 5);

        if (i < m_items.size()) {
            // 有物品：绘制图标（居中）
            const BagItem &item = m_items[i];
            if (!item.icon.isNull()) {
                int ix = cx + (CELL_SIZE - item.icon.width())  / 2;
                int iy = cy + (CELL_SIZE - item.icon.height()) / 2 - 8;
                p.drawPixmap(ix, iy, item.icon);
            }

            // 物品名称（格子底部小字）
            p.setRenderHint(QPainter::Antialiasing, true);
            QFont nameFont("Courier New", 7);
            p.setFont(nameFont);
            p.setPen(QColor(160, 130, 70));
            p.drawText(cx, cy + CELL_SIZE - 16, CELL_SIZE, 16,
                       Qt::AlignCenter, item.name);
            p.setRenderHint(QPainter::Antialiasing, false);
        }
    }

    // --- 底部提示 ---
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont hintFont("Courier New", 8);
    p.setFont(hintFont);
    p.setPen(QColor(80, 60, 25, 180));
    p.drawText(0, H - 26, W, 20, Qt::AlignCenter, "点击物品放入槽位");
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// 鼠标按下：拖动窗口 或 选择物品
// ============================================================
void BagWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    // 点击标题区域 → 拖动
    if (event->pos().y() < 55) {
        m_dragging   = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        return;
    }

    // 点击格子区域 → 选择物品
    int cell = cellAt(event->pos());
    if (cell >= 0 && cell < m_items.size()) {
        const BagItem item = m_items[cell]; // 先拷贝
        emit itemSelected(m_targetSlot, item.id, item.icon);
        // 从背包中移除（物品移入槽位）
        m_items.removeAt(cell);
        m_hoveredCell = -1;
        hide();
        emit bagClosed();
    }
}

// ============================================================
// 鼠标移动：更新悬停格子序号 + 拖动窗口
// ============================================================
void BagWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        move(event->globalPos() - m_dragOffset);
        return;
    }
    int newHovered = cellAt(event->pos());
    if (newHovered != m_hoveredCell) {
        m_hoveredCell = newHovered;
        update();
    }
}

// ============================================================
// 鼠标释放：结束拖动
// ============================================================
void BagWindow::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}
