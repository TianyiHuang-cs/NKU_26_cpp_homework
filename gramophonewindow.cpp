#include "gramophonewindow.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <algorithm>

// ============================================================
// ======================== SlotWidget ========================
// ============================================================

SlotWidget::SlotWidget(QWidget *parent)
    : QWidget(parent), m_slotIndex(0), m_itemId(-1)
{
    // 固定大小：横向排列三个槽，每个 110×110
    setFixedSize(110, 110);
    setCursor(Qt::PointingHandCursor);

}

// ============================================================
// 放入物品：记录 ID 和图标，触发重绘
// ============================================================
// SlotWidget::setItem 加 name 参数
void SlotWidget::setItem(int itemId, const QPixmap &icon, const QString &name)
{
    m_itemId    = itemId;
    m_icon      = icon;
    m_itemName  = name;
    update();
}

// ============================================================
// 清空槽位
// ============================================================
void SlotWidget::clearItem()
{
    m_itemId = -1;
    m_icon   = QPixmap();
    update();
}

// ============================================================
// 绘制槽位：像素风矩形框 + 物品图标（或空槽提示）
// ============================================================
void SlotWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    // 槽位背景（半透明暗色）
    p.fillRect(rect(), QColor(20, 12, 8, 220));

    // 像素风边框（外深内亮双线）
    p.setPen(QPen(QColor(80, 55, 25), 2));
    p.drawRect(1, 1, width() - 3, height() - 3);
    p.setPen(QPen(QColor(50, 32, 10), 1));
    p.drawRect(3, 3, width() - 7, height() - 7);

    if (m_itemId >= 0 && !m_icon.isNull()) {
        // 有物品：居中绘制图标
        QPixmap scaled = m_icon.scaled(80, 80, Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
        int ix = (width()  - scaled.width())  / 2;
        int iy = (height() - scaled.height()) / 2;
        p.drawPixmap(ix, iy, scaled);
    } else {
        // 空槽：显示 "+" 提示
        p.setPen(QColor(100, 75, 40, 180));
        QFont f("Courier New", 28, QFont::Light);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter, "+");

        // 小提示文字
        QFont sf("Courier New", 8);
        p.setFont(sf);
        p.setPen(QColor(80, 60, 30, 150));
        p.drawText(rect().adjusted(0, 0, 0, -6), Qt::AlignBottom | Qt::AlignHCenter, "click");
    }
}

// ============================================================
// 点击槽位 → 发射信号，请求打开背包
// ============================================================
void SlotWidget::mousePressEvent(QMouseEvent *)
{
    emit slotClicked(m_slotIndex);
}


// ============================================================
// ===================== GramophoneWindow =====================
// ============================================================

GramophoneWindow::GramophoneWindow(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
    m_dragging(false)
{
    // 固定弹窗尺寸
    setFixedSize(520, 380);

    // 半透明背景支持
    setAttribute(Qt::WA_TranslucentBackground);

    // --------------------------------------------------------
    // 【接口】正确答案初始化为空（始终判错）
    // 后续调用 setCorrectAnswer({id0, id1, id2}) 进行设置
    // --------------------------------------------------------
    m_correctIds.clear();

    // --- 三个槽位（水平排列，居中） ---
    for (int i = 0; i < 3; i++) {
        m_slots[i] = new SlotWidget(this);
        // 将槽位序号注入（通过 lambda 捕获）
        // Qt 信号不直接携带发送者序号，改用手动设置
        // 这里通过 connect 时固定捕获 i
        connect(m_slots[i], &SlotWidget::slotClicked,
                this, [this](int idx){ emit requestOpenBag(idx); });
    }

    // 手动给每个槽设置序号（SlotWidget 内部 m_slotIndex 需要外部赋值）
    for (int i = 0; i < 3; i++) {
        m_slots[i]->setSlotIndex(i);
    }

    // --- 槽位布局（距顶部 170px，水平居中） ---
    int slotAreaY = 175;
    int totalSlotW = 3 * 110 + 2 * 18; // 三槽 + 两个间距
    int slotStartX = (520 - totalSlotW) / 2;
    for (int i = 0; i < 3; i++) {
        m_slots[i]->move(slotStartX + i * (110 + 18), slotAreaY);
    }

    // --- 确认按钮（右下角） ---
    m_confirmBtn = new QPushButton("确 认", this);
    m_confirmBtn->setGeometry(320, 318, 88, 36);
    m_confirmBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #4a2e0a;"
        "  color: #d4aa60;"
        "  border: 2px solid #7a5020;"
        "  font-family: 'Courier New';"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #6a4010; }"
        "QPushButton:pressed { background-color: #2a1400; }"
        );
    connect(m_confirmBtn, &QPushButton::clicked, this, &GramophoneWindow::onConfirmClicked);

    // --- 取消/关闭按钮（右下角） ---
    m_cancelBtn = new QPushButton("✕", this);
    m_cancelBtn->setGeometry(420, 318, 60, 36);
    m_cancelBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2a1010;"
        "  color: #c04040;"
        "  border: 2px solid #601010;"
        "  font-family: 'Courier New';"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #401010; }"
        );
    connect(m_cancelBtn, &QPushButton::clicked, this, &GramophoneWindow::onCancelClicked);

    // --- 结果提示标签（底部中央，初始隐藏） ---
    m_resultLabel = new QLabel(this);
    m_resultLabel->setGeometry(20, 312, 290, 44);
    m_resultLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setStyleSheet(
        "color: #d4aa60;"
        "font-family: 'Courier New';"
        "font-size: 12px;"
        "background: transparent;"
        );
    m_resultLabel->hide();
}

// ============================================================
// 设置正确答案接口
// correctIds：三个物品 ID 的集合（顺序无关）
// ============================================================
void GramophoneWindow::setCorrectAnswer(const QVector<int> &correctIds)
{
    m_correctIds = correctIds;
}

// ============================================================
// 外部回调：将背包中选中的物品放入指定槽位
// ============================================================
// placeItemIntoSlot 加 name 参数
void GramophoneWindow::placeItemIntoSlot(int slotIndex, int itemId,
                                         const QPixmap &icon, const QString &name)
{
    if (slotIndex < 0 || slotIndex > 2) return;
    m_slots[slotIndex]->setItem(itemId, icon, name);
    m_resultLabel->hide();
}

// ============================================================
// 绘制弹窗外观：自定义暗黑像素风面板
// ============================================================
void GramophoneWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int W = width(), H = height();

    // --- 主体背景（深棕/暗黑） ---
    p.fillRect(4, 4, W - 8, H - 8, QColor(18, 10, 5, 245));

    // --- 外层像素边框（三层，由外到内） ---
    p.setPen(QPen(QColor(30, 18, 6), 2));
    p.drawRect(2, 2, W - 5, H - 5);
    p.setPen(QPen(QColor(90, 60, 22), 2));
    p.drawRect(4, 4, W - 9, H - 9);
    p.setPen(QPen(QColor(55, 35, 12), 1));
    p.drawRect(7, 7, W - 15, H - 15);

    // --- 顶部装饰横条（模拟木框顶梁） ---
    p.fillRect(8, 8, W - 16, 42, QColor(38, 22, 8));
    p.setPen(QPen(QColor(90, 60, 22), 1));
    p.drawLine(8, 50, W - 9, 50);
    p.setPen(QPen(QColor(55, 35, 10), 1));
    p.drawLine(8, 51, W - 9, 51);

    // --- 标题文字："— 留声机的秘密 —" ---
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont titleFont("Courier New", 13, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor(200, 155, 60));
    p.drawText(0, 8, W, 42, Qt::AlignCenter, "— 留声机的秘密 —");

    // --- 谜题提示文字（分两行） ---
    QFont promptFont("Courier New", 11);
    p.setFont(promptFont);
    p.setPen(QColor(180, 150, 100));
    p.drawText(30, 60, W - 60, 60,
               Qt::AlignCenter | Qt::TextWordWrap,
               "最大的声音不一定是最重要的声音，\n请放入三个最重要的声音。");

    // --- 分隔线（槽位上方） ---
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(QColor(70, 45, 15), 1));
    p.drawLine(30, 165, W - 30, 165);
    p.setPen(QPen(QColor(40, 25, 8), 1));
    p.drawLine(30, 167, W - 30, 167);

    // --- 槽位区域标注文字 ---
    QFont slotFont("Courier New", 8);
    p.setFont(slotFont);
    p.setPen(QColor(100, 75, 35, 180));
    p.drawText(0, 150, W, 18, Qt::AlignCenter, "[ 点击槽位打开背包 ]");

    // --- 底部分隔线 ---
    p.setPen(QPen(QColor(70, 45, 15), 1));
    p.drawLine(30, 308, W - 30, 308);
}

// ============================================================
// 鼠标按下：判断是否拖动窗口
// ============================================================
void GramophoneWindow::mousePressEvent(QMouseEvent *event)
{
    // 只有点击顶部标题区域才允许拖动
    if (event->pos().y() < 55 && event->button() == Qt::LeftButton) {
        m_dragging   = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
    }
}

// ============================================================
// 鼠标移动：拖动窗口
// ============================================================
void GramophoneWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging)
        move(event->globalPos() - m_dragOffset);
}

// ============================================================
// 鼠标释放：结束拖动
// ============================================================
void GramophoneWindow::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}

// ============================================================
// 确认按钮点击：校验答案
// 正确 → 显示成功文字 + 发射 puzzleSolved
// 错误 → 显示提示，等待用户重新选择
// ============================================================
void GramophoneWindow::onConfirmClicked()
{
    if (checkAnswer()) {
        // 答案正确
        m_resultLabel->setStyleSheet(
            "color: #60dd80;"
            "font-family: 'Courier New';"
            "font-size: 12px;"
            "background: transparent;"
            );
        m_resultLabel->setText("♪ 心声最重要——时间倒流10min");
        m_resultLabel->show();

        // 通知主窗口时间倒流
        emit puzzleSolved();

        // 隐藏确认按钮，只保留关闭按钮（防止重复触发）
        m_confirmBtn->setEnabled(false);
    } else {
        // 答案错误
        m_resultLabel->setStyleSheet(
            "color: #dd6040;"
            "font-family: 'Courier New';"
            "font-size: 12px;"
            "background: transparent;"
            );
        m_resultLabel->setText("✗ 有噪声，请重新选择。");
        m_resultLabel->show();

        // 将三个槽位的物品全部退回背包
        for (int i = 0; i < 3; i++) {
            if (m_slots[i]->hasItem()) {
                emit returnItemToBag(i, m_slots[i]->getItemId(),
                                     m_slots[i]->getIcon(),
                                     m_slots[i]->getItemName());
                m_slots[i]->clearItem();
            }
        }
    }
}

// ============================================================
// 取消/关闭按钮：隐藏弹窗，通知主窗口恢复焦点
// ============================================================
void GramophoneWindow::onCancelClicked()
{
    m_resultLabel->hide();
    m_confirmBtn->setEnabled(true);
    hide();
    emit windowClosed();
}

// ============================================================
// 校验答案：将三个槽位的 ID 集合与正确答案比较（顺序无关）
// m_correctIds 为空时始终返回 false
// ============================================================
bool GramophoneWindow::checkAnswer() const
{
    if (m_correctIds.size() != 3) return false;

    // 收集槽位 ID（必须三格全满）
    QVector<int> slotIds;
    for (int i = 0; i < 3; i++) {
        if (!m_slots[i]->hasItem()) return false;
        slotIds.append(m_slots[i]->getItemId());
    }

    // 排序后比较（顺序无关）
    QVector<int> answer = m_correctIds;
    std::sort(slotIds.begin(), slotIds.end());
    std::sort(answer.begin(),  answer.end());
    return slotIds == answer;
}
