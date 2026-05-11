#include "sheetmusicwindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>

// ============================================================
// 月光奏鸣曲 Op.27 No.2 第一乐章
// 升c小调，4/4拍，Adagio sostenuto
// 原版织体：右手旋律 + 左手三连音（本谱仅呈现右手高音声部前20音符）
//
// 标准五线谱音符编码：
//   staffLine：音符在五线谱上的位置
//     0  = 第一间（最低）
//     1  = 第二线
//     2  = 第二间
//     3  = 第三线（中线）
//     4  = 第三间
//     5  = 第四线
//     6  = 第四间
//     7  = 第五线（最高）
//   加线用负数（下加线）或8以上（上加线）
//   accidental: 0=无 1=升号 -1=降号 2=还原号
//   duration:   4=四分 8=八分 3=三连八分
// ============================================================
struct MNote {
    int  staffLine;   // 五线谱位置（0=第一间，见上方说明）
    int  accidental;  // 临时记号：0无 1升 -1降
    int  duration;    // 时值：8=八分音符（三连音用8）
    bool stemUp;      // 符干朝上
};

// 月光奏鸣曲开头20个音符（右手旋律声部）
// 第一小节：#G4 E4 #G4（三连音×2）
// 第二小节：#G4 E4 #G4（三连音×2）继续
// 实际为三连八分音符织体，每组3个为一拍
static const MNote NOTES[20] = {
    // 小节1：三连音 #G4 E4 #G4 | #G4 E4 #G4 | #G4 E4 #G4 | #G4 E4 #G4
    // staffLine基于高音谱号：
    //   E4=第一间(0), F#4=第一线上方(1), G#4=第二间(2)
    //   A4=第二线(3不对，重新对应)
    // 高音谱号对应（从下到上）：
    //   下加一线=C4, 下加一间=D4
    //   第一线=E4(line0), 第一间=F4(space0)
    //   第二线=G4(line1), 第二间=A4(space1)
    //   第三线=B4(line2), 第三间=C5(space2)
    //   第四线=D5(line3), 第四间=E5(space3)
    //   第五线=F5(line4)
    // 使用半音阶步长编码（从第一线E4=0开始，每步=半个间距）
    // staffPos: 0=E4线, 2=F4间, 4=G4线, 5=G#4间, 6=A4间, 8=B4线...
    // 简化：直接用像素偏移量（相对第三线中央B4）

    // 用 staffLine 表示相对第三线(B4)的半步数
    // 正=向上，负=向下
    // G#4 = B4 下 3 步 = -3，带升号
    // E4  = B4 下 6 步 = -6
    {-3, 1, 8, false},  // 1  G#4
    {-6, 0, 8, false},  // 2  E4
    {-3, 1, 8, false},  // 3  G#4
    {-6, 0, 8, false},  // 4  E4
    {-3, 1, 8, false},  // 5  G#4
    {-6, 0, 8, false},  // 6  E4
    // 小节1后半：旋律上行
    {-3, 1, 8, false},  // 7  G#4
    {-6, 0, 8, false},  // 8  E4
    {-3, 1, 8, false},  // 9  G#4
    {-6, 0, 8, false},  // 10 E4
    // 小节2：引入F#4
    {-4, 1, 8, false},  // 11 F#4（B4下4步）
    {-6, 0, 8, false},  // 12 E4
    {-4, 1, 8, false},  // 13 F#4
    {-7, 1, 8, false},  // 14 D#4（B4下7步）
    {-4, 1, 8, false},  // 15 F#4
    {-7, 1, 8, false},  // 16 D#4
    // 小节2继续
    {-3, 1, 8, false},  // 17 G#4
    {-6, 0, 8, false},  // 18 E4
    {-3, 1, 8, false},  // 19 G#4
    {-9, 1, 8, false},  // 20 C#4（B4下9步）
};

// ============================================================
// 构造函数
// ============================================================
SheetMusicWindow::SheetMusicWindow(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint),
    m_dragging(false)
{
    setFixedSize(580, 400);
    setAttribute(Qt::WA_TranslucentBackground);

    // 右上角关闭按钮
    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setGeometry(550, 8, 22, 22);
    m_closeBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#8B7355;"
        "  border:none; font-size:14px; font-weight:bold; }"
        "QPushButton:hover { color:#cc4444; }"
        );
    connect(m_closeBtn, &QPushButton::clicked,
            this, &SheetMusicWindow::onLeaveInPlace);

    // 加入背包按钮
    m_addBagBtn = new QPushButton("加入背包", this);
    m_addBagBtn->setGeometry(350, 354, 100, 34);
    m_addBagBtn->setStyleSheet(
        "QPushButton { background:#2a3a18; color:#90c850;"
        "  border:2px solid #4a6828; font-family:'Courier New';"
        "  font-size:12px; font-weight:bold; }"
        "QPushButton:hover { background:#3a5020; }"
        );
    connect(m_addBagBtn, &QPushButton::clicked,
            this, &SheetMusicWindow::onAddToBag);

    // 放在原地按钮
    m_leaveBtn = new QPushButton("放在原地", this);
    m_leaveBtn->setGeometry(460, 354, 100, 34);
    m_leaveBtn->setStyleSheet(
        "QPushButton { background:#3a2a10; color:#c8a050;"
        "  border:2px solid #6a5020; font-family:'Courier New';"
        "  font-size:12px; font-weight:bold; }"
        "QPushButton:hover { background:#5a4018; }"
        );
    connect(m_leaveBtn, &QPushButton::clicked,
            this, &SheetMusicWindow::onLeaveInPlace);
}

// ============================================================
// drawPixelRect：辅助绘制像素风矩形
// ============================================================
void SheetMusicWindow::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                                     const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w - 1, h - 1);
}

// ============================================================
// drawStaff：绘制标准五线谱（5条横线）
// x,y：起始坐标；w：宽度；lineGap：线间距
// ============================================================
void SheetMusicWindow::drawStaff(QPainter &p, int x, int y, int w, int lineGap)
{
    // 五线谱用细实线，颜色接近墨水
    p.setPen(QPen(QColor(38, 28, 12), 1));
    for (int i = 0; i < 5; i++)
        p.drawLine(x, y + i * lineGap, x + w, y + i * lineGap);
}

// ============================================================
// drawAccidental：绘制升降号
// type: 1=升号(#)  -1=降号(b)
// ============================================================
void SheetMusicWindow::drawSharp(QPainter &p, int x, int y)
{
    p.setPen(QPen(QColor(30, 20, 8), 1));
    // 两竖线（微微倾斜）
    p.drawLine(x - 2, y - 6, x - 2, y + 6);
    p.drawLine(x + 2, y - 6, x + 2, y + 6);
    // 两横线
    p.drawLine(x - 4, y - 2, x + 4, y - 2);
    p.drawLine(x - 4, y + 2, x + 4, y + 2);
}

// ============================================================
// drawNote：绘制单个音符
// cx,cy：音符椭圆中心坐标
// filled：实心(八分/四分)；stemUp：符干朝上；beam：是否加符尾
// ============================================================
void SheetMusicWindow::drawNote(QPainter &p, int cx, int cy,
                                bool filled, bool stemUp, bool beam)
{
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(28, 18, 6), 1));

    // 音符头：椭圆（稍微倾斜，模拟手写感）
    if (filled)
        p.setBrush(QColor(28, 18, 6));
    else
        p.setBrush(QColor(240, 228, 195)); // 空心用纸色

    // 椭圆：宽9 高6，轻微旋转模拟手写
    p.save();
    p.translate(cx, cy);
    p.rotate(-12); // 轻微倾斜12度
    p.drawEllipse(-5, -3, 10, 6);
    p.restore();

    // 符干（长22px）
    p.setPen(QPen(QColor(28, 18, 6), 1));
    if (stemUp) {
        p.drawLine(cx + 4, cy - 1, cx + 4, cy - 24);
        // 八分音符符尾（向右弯曲的弧线）
        if (beam) {
            QPainterPath tail;
            tail.moveTo(cx + 4, cy - 24);
            tail.cubicTo(cx + 14, cy - 18,
                         cx + 16, cy - 12,
                         cx + 10, cy - 8);
            p.setPen(QPen(QColor(28, 18, 6), 1));
            p.setBrush(Qt::NoBrush);
            p.drawPath(tail);
        }
    } else {
        p.drawLine(cx - 4, cy + 1, cx - 4, cy + 24);
        if (beam) {
            QPainterPath tail;
            tail.moveTo(cx - 4, cy + 24);
            tail.cubicTo(cx + 6,  cy + 18,
                         cx + 8,  cy + 12,
                         cx + 2,  cy + 8);
            p.setPen(QPen(QColor(28, 18, 6), 1));
            p.setBrush(Qt::NoBrush);
            p.drawPath(tail);
        }
    }
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawLedgerLine：绘制加线（音符超出五线谱范围时）
// ============================================================
void SheetMusicWindow::drawLedgerLine(QPainter &p, int cx, int y)
{
    p.setPen(QPen(QColor(38, 28, 12), 1));
    p.drawLine(cx - 8, y, cx + 8, y);
}

// ============================================================
// paintEvent：绘制完整琴谱弹窗
// ============================================================
void SheetMusicWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    int W = width(), H = height();

    // ============================================================
    // 背景：做旧羊皮纸质感
    // ============================================================
    // 基底色：泛黄米色
    p.fillRect(0, 0, W, H, QColor(238, 224, 190));

    // 纸张边缘深色（做旧感）
    QLinearGradient edgeL(0, 0, 18, 0);
    edgeL.setColorAt(0, QColor(180, 155, 100, 160));
    edgeL.setColorAt(1, Qt::transparent);
    p.fillRect(0, 0, 18, H, edgeL);

    QLinearGradient edgeR(W - 18, 0, W, 0);
    edgeR.setColorAt(0, Qt::transparent);
    edgeR.setColorAt(1, QColor(180, 155, 100, 160));
    p.fillRect(W - 18, 0, 18, H, edgeR);

    QLinearGradient edgeT(0, 0, 0, 18);
    edgeT.setColorAt(0, QColor(180, 155, 100, 140));
    edgeT.setColorAt(1, Qt::transparent);
    p.fillRect(0, 0, W, 18, edgeT);

    QLinearGradient edgeB(0, H - 18, 0, H);
    edgeB.setColorAt(0, Qt::transparent);
    edgeB.setColorAt(1, QColor(180, 155, 100, 140));
    p.fillRect(0, H - 18, W, 18, edgeB);

    // 随机污渍斑点（做旧）
    QRandomGenerator rng(42);
    for (int i = 0; i < 18; i++) {
        int sx = rng.bounded(W);
        int sy = rng.bounded(H);
        int sr = rng.bounded(2, 8);
        int sa = rng.bounded(15, 50);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(140, 110, 60, sa));
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawEllipse(sx, sy, sr, sr);
        p.setRenderHint(QPainter::Antialiasing, false);
    }

    // 外框（仿旧书装帧线）
    p.setPen(QPen(QColor(100, 75, 35), 2));
    p.drawRect(6, 6, W - 13, H - 13);
    p.setPen(QPen(QColor(140, 108, 52), 1));
    p.drawRect(9, 9, W - 19, H - 19);

    // ============================================================
    // 标题区
    // ============================================================
    p.setRenderHint(QPainter::Antialiasing, true);

    // 主标题
    QFont titleFont("Georgia", 15, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor(35, 22, 8));
    p.drawText(0, 14, W, 26, Qt::AlignCenter,
               "Piano Sonata No.14 in C# minor");

    // 副标题（手写风格斜体）
    QFont subFont("Georgia", 11);
    subFont.setItalic(true);
    p.setFont(subFont);
    p.setPen(QColor(65, 45, 18));
    p.drawText(0, 38, W, 20, Qt::AlignCenter,
               "\"Moonlight\"  Op.27, No.2");

    // 作曲家
    QFont compFont("Courier New", 9);
    p.setFont(compFont);
    p.setPen(QColor(85, 60, 25));
    p.drawText(12, 56, W - 24, 16, Qt::AlignRight,
               "L. van Beethoven  (1770–1827)");

    // 标题下分隔线（双线）
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(QColor(100, 75, 35), 1));
    p.drawLine(14, 75, W - 14, 75);
    p.setPen(QPen(QColor(140, 108, 52), 1));
    p.drawLine(14, 77, W - 14, 77);

    // ============================================================
    // 五线谱参数
    // ============================================================
    const int LINE_GAP  = 9;    // 线间距（标准比例）
    const int STAFF_X   = 22;   // 五线谱起始X
    const int STAFF_W   = W - 44; // 五线谱宽度
    const int HALF_STEP = LINE_GAP / 2; // 半步距离（4px）

    // 两行五线谱Y坐标（第一线位置）
    const int ROW1_Y = 100;  // 第一行五线谱第一线
    const int ROW2_Y = 230;  // 第二行五线谱第一线

    // 中线（第三线）Y = ROW_Y + 2*LINE_GAP
    const int MID1_Y = ROW1_Y + 2 * LINE_GAP;
    const int MID2_Y = ROW2_Y + 2 * LINE_GAP;

    // ============================================================
    // 绘制第一行五线谱
    // ============================================================
    drawStaff(p, STAFF_X, ROW1_Y, STAFF_W, LINE_GAP);

    // 左竖线（谱表起始线）
    p.setPen(QPen(QColor(35, 22, 8), 1));
    p.drawLine(STAFF_X, ROW1_Y, STAFF_X, ROW1_Y + 4 * LINE_GAP);

    // ---- 高音谱号（简化像素手绘版，避免字体不支持） ----
    // 用贝塞尔曲线模拟高音谱号形状
    p.setRenderHint(QPainter::Antialiasing, true);
    int gx = STAFF_X + 6, gy = ROW1_Y;
    p.setPen(QPen(QColor(28, 18, 6), 2));
    p.setBrush(Qt::NoBrush);

    // 谱号竖线主干
    QPainterPath clef;
    clef.moveTo(gx + 8, gy + 40);
    clef.cubicTo(gx + 8, gy - 6,   gx + 18, gy - 8,  gx + 18, gy + 6);
    clef.cubicTo(gx + 18, gy + 22, gx + 2,  gy + 26, gx + 2,  gy + 18);
    clef.cubicTo(gx + 2,  gy + 10, gx + 14, gy + 8,  gx + 14, gy + 18);
    p.drawPath(clef);

    // 谱号下部螺旋
    QPainterPath curl;
    curl.moveTo(gx + 8, gy + 40);
    curl.cubicTo(gx - 2, gy + 44, gx - 2, gy + 50, gx + 8, gy + 50);
    curl.cubicTo(gx + 18, gy + 50, gx + 20, gy + 44, gx + 16, gy + 40);
    p.drawPath(curl);
    p.setRenderHint(QPainter::Antialiasing, false);

    // ---- 调号：4个升号（升c小调=升F C G D） ----
    // 位置：第五线、第三线、第五间（上加线下方）、第三间
    int keyX = STAFF_X + 34;
    int sharpPositions1[] = {
        ROW1_Y,                       // #F：第五线（最高线）
        ROW1_Y + 2 * LINE_GAP,        // #C：第三线
        ROW1_Y - LINE_GAP / 2,        // #G：第五线上一间（上加一间）
        ROW1_Y + LINE_GAP + LINE_GAP / 2  // #D：第三间
    };
    for (int i = 0; i < 4; i++) {
        p.setRenderHint(QPainter::Antialiasing, true);
        drawSharp(p, keyX + i * 8, sharpPositions1[i]);
        p.setRenderHint(QPainter::Antialiasing, false);
    }

    // ---- 拍号：4/4 ----
    int timeX = keyX + 36;
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont timeFont("Georgia", 16, QFont::Bold);
    p.setFont(timeFont);
    p.setPen(QColor(28, 18, 6));
    // 上方"4"（位于第三间和第四线之间）
    p.drawText(timeX, ROW1_Y - 4, 18, 2 * LINE_GAP + 2, Qt::AlignCenter, "4");
    // 下方"4"
    p.drawText(timeX, ROW1_Y + 2 * LINE_GAP - 2, 18, 2 * LINE_GAP + 2, Qt::AlignCenter, "4");
    p.setRenderHint(QPainter::Antialiasing, false);

    // ---- 速度标记 ----
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont tempoFont("Georgia", 9);
    tempoFont.setItalic(true);
    p.setFont(tempoFont);
    p.setPen(QColor(50, 35, 12));
    p.drawText(STAFF_X, ROW1_Y - 22, 200, 18, Qt::AlignLeft, "Adagio sostenuto");
    // 三连音标记
    QFont tripletFont("Courier New", 7);
    p.setFont(tripletFont);
    p.setPen(QColor(70, 50, 18));
    p.drawText(STAFF_X + 80, ROW1_Y - 14, 60, 14, Qt::AlignLeft, "（三连音）");
    p.setRenderHint(QPainter::Antialiasing, false);

    // ============================================================
    // 绘制第一行音符（1~10）
    // 音符起始X（调号拍号之后留空）
    // ============================================================
    int noteStartX = timeX + 28;
    // 10个音符均匀分布在剩余宽度
    int noteAreaW  = STAFF_X + STAFF_W - noteStartX - 10;
    int noteSpacing = noteAreaW / 10;

    for (int i = 0; i < 10; i++) {
        const MNote &n = NOTES[i];
        int nx = noteStartX + i * noteSpacing + noteSpacing / 2;
        // staffLine=-3 → G#4：B4(中线)向下3个半步
        // 每个半步 = HALF_STEP像素，向下=Y增大
        int ny = MID1_Y - n.staffLine * HALF_STEP;

        // 加线判断（超出五线谱范围）
        if (ny > ROW1_Y + 4 * LINE_GAP + HALF_STEP) {
            // 下方加线
            for (int ly = ROW1_Y + 5 * LINE_GAP;
                 ly <= ny + HALF_STEP; ly += LINE_GAP)
                drawLedgerLine(p, nx, ly);
        }
        if (ny < ROW1_Y - HALF_STEP) {
            // 上方加线
            for (int ly = ROW1_Y - LINE_GAP;
                 ly >= ny - HALF_STEP; ly -= LINE_GAP)
                drawLedgerLine(p, nx, ly);
        }

        // 升号
        if (n.accidental == 1) {
            p.setRenderHint(QPainter::Antialiasing, true);
            // 相邻音符避免重叠：奇偶号偏移
            bool prevSameAcc = (i > 0 && NOTES[i-1].accidental == 1 &&
                                std::abs(NOTES[i-1].staffLine - n.staffLine) < 3);
            drawSharp(p, nx - 10 - (prevSameAcc ? 8 : 0), ny);
            p.setRenderHint(QPainter::Antialiasing, false);
        }

        // 三连音括号（每3个一组，在符干上方绘制括号+数字"3"）
        if (i % 3 == 0 && i + 2 < 10) {
            int x1 = noteStartX + i * noteSpacing + noteSpacing / 2;
            int x2 = noteStartX + (i + 2) * noteSpacing + noteSpacing / 2;
            int ty = MID1_Y - 6 * HALF_STEP - 18; // 括号Y（音符上方）
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(QPen(QColor(45, 32, 12), 1));
            p.setBrush(Qt::NoBrush);
            // 括号横线
            p.drawLine(x1, ty, x2, ty);
            // 左竖线
            p.drawLine(x1, ty, x1, ty + 5);
            // 右竖线
            p.drawLine(x2, ty, x2, ty + 5);
            // 数字"3"
            QFont tripFont("Georgia", 8, QFont::Bold);
            p.setFont(tripFont);
            p.setPen(QColor(35, 22, 8));
            p.drawText((x1 + x2) / 2 - 4, ty - 11, 10, 12,
                       Qt::AlignCenter, "3");
            p.setRenderHint(QPainter::Antialiasing, false);
        }

        // 绘制音符（八分音符，实心，有符尾）
        drawNote(p, nx, ny, true, n.stemUp, true);
    }

    // ============================================================
    // 绘制第二行五线谱
    // ============================================================
    drawStaff(p, STAFF_X, ROW2_Y, STAFF_W, LINE_GAP);
    p.setPen(QPen(QColor(35, 22, 8), 1));
    p.drawLine(STAFF_X, ROW2_Y, STAFF_X, ROW2_Y + 4 * LINE_GAP);

    // 第二行高音谱号
    p.setRenderHint(QPainter::Antialiasing, true);
    int gx2 = STAFF_X + 6, gy2 = ROW2_Y;
    p.setPen(QPen(QColor(28, 18, 6), 2));
    p.setBrush(Qt::NoBrush);
    QPainterPath clef2;
    clef2.moveTo(gx2 + 8, gy2 + 40);
    clef2.cubicTo(gx2 + 8, gy2 - 6,   gx2 + 18, gy2 - 8,  gx2 + 18, gy2 + 6);
    clef2.cubicTo(gx2 + 18, gy2 + 22, gx2 + 2,  gy2 + 26, gx2 + 2,  gy2 + 18);
    clef2.cubicTo(gx2 + 2,  gy2 + 10, gx2 + 14, gy2 + 8,  gx2 + 14, gy2 + 18);
    p.drawPath(clef2);
    QPainterPath curl2;
    curl2.moveTo(gx2 + 8, gy2 + 40);
    curl2.cubicTo(gx2 - 2, gy2 + 44, gx2 - 2, gy2 + 50, gx2 + 8,  gy2 + 50);
    curl2.cubicTo(gx2 + 18, gy2 + 50, gx2 + 20, gy2 + 44, gx2 + 16, gy2 + 40);
    p.drawPath(curl2);
    p.setRenderHint(QPainter::Antialiasing, false);

    // 第二行调号（同第一行）
    int sharpPositions2[] = {
        ROW2_Y,
        ROW2_Y + 2 * LINE_GAP,
        ROW2_Y - LINE_GAP / 2,
        ROW2_Y + LINE_GAP + LINE_GAP / 2
    };
    for (int i = 0; i < 4; i++) {
        p.setRenderHint(QPainter::Antialiasing, true);
        drawSharp(p, keyX + i * 8, sharpPositions2[i]);
        p.setRenderHint(QPainter::Antialiasing, false);
    }

    // 第二行拍号
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setFont(timeFont);
    p.setPen(QColor(28, 18, 6));
    p.drawText(timeX, ROW2_Y - 4, 18, 2 * LINE_GAP + 2, Qt::AlignCenter, "4");
    p.drawText(timeX, ROW2_Y + 2 * LINE_GAP - 2, 18, 2 * LINE_GAP + 2, Qt::AlignCenter, "4");
    p.setRenderHint(QPainter::Antialiasing, false);

    // ============================================================
    // 绘制第二行音符（11~20）
    // ============================================================
    for (int i = 0; i < 10; i++) {
        const MNote &n = NOTES[i + 10];
        int nx = noteStartX + i * noteSpacing + noteSpacing / 2;
        int ny = MID2_Y - n.staffLine * HALF_STEP;

        // 加线
        if (ny > ROW2_Y + 4 * LINE_GAP + HALF_STEP) {
            for (int ly = ROW2_Y + 5 * LINE_GAP;
                 ly <= ny + HALF_STEP; ly += LINE_GAP)
                drawLedgerLine(p, nx, ly);
        }

        // 升号
        if (n.accidental == 1) {
            p.setRenderHint(QPainter::Antialiasing, true);
            bool prevSameAcc = (i > 0 && NOTES[i + 9].accidental == 1 &&
                                std::abs(NOTES[i + 9].staffLine - n.staffLine) < 3);
            drawSharp(p, nx - 10 - (prevSameAcc ? 8 : 0), ny);
            p.setRenderHint(QPainter::Antialiasing, false);
        }

        // 三连音括号
        if (i % 3 == 0 && i + 2 < 10) {
            int x1 = noteStartX + i * noteSpacing + noteSpacing / 2;
            int x2 = noteStartX + (i + 2) * noteSpacing + noteSpacing / 2;
            int ty = MID2_Y - 6 * HALF_STEP - 18;
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(QPen(QColor(45, 32, 12), 1));
            p.setBrush(Qt::NoBrush);
            p.drawLine(x1, ty, x2, ty);
            p.drawLine(x1, ty, x1, ty + 5);
            p.drawLine(x2, ty, x2, ty + 5);
            QFont tripFont("Georgia", 8, QFont::Bold);
            p.setFont(tripFont);
            p.setPen(QColor(35, 22, 8));
            p.drawText((x1 + x2) / 2 - 4, ty - 11, 10, 12,
                       Qt::AlignCenter, "3");
            p.setRenderHint(QPainter::Antialiasing, false);
        }

        drawNote(p, nx, ny, true, n.stemUp, true);
    }

    // ============================================================
    // 结尾双竖线
    // ============================================================
    int endX = STAFF_X + STAFF_W;
    p.setPen(QPen(QColor(35, 22, 8), 1));
    p.drawLine(endX - 3, ROW2_Y, endX - 3, ROW2_Y + 4 * LINE_GAP);
    p.setPen(QPen(QColor(35, 22, 8), 3));
    p.drawLine(endX,     ROW2_Y, endX,     ROW2_Y + 4 * LINE_GAP);

    // ============================================================
    // 底部分隔线 + 版权信息
    // ============================================================
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(QColor(110, 82, 38), 1));
    p.drawLine(14, H - 54, W - 14, H - 54);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont footFont("Courier New", 7);
    footFont.setItalic(true);
    p.setFont(footFont);
    p.setPen(QColor(110, 85, 40, 160));
    p.drawText(0, H - 52, W, 14, Qt::AlignCenter,
               "Public Domain — First published 1802, Cappi, Vienna");
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// generateSheetIcon：生成背包图标（白纸 + 五线谱线条）
// 简洁风格：白色方块 + 黑色线条模拟谱纸
// ============================================================
QPixmap SheetMusicWindow::generateSheetIcon()
{
    QPixmap icon(64, 64);
    icon.fill(Qt::transparent);
    QPainter p(&icon);
    p.setRenderHint(QPainter::Antialiasing, false);

    // 纸张阴影
    p.fillRect(5, 5, 54, 56, QColor(80, 65, 40, 100));
    // 纸张主体（白色偏暖）
    p.fillRect(3, 3, 52, 54, QColor(245, 238, 218));
    // 纸张外框
    p.setPen(QPen(QColor(130, 100, 50), 1));
    p.drawRect(3, 3, 51, 53);
    // 纸张内框（做旧感）
    p.setPen(QPen(QColor(160, 130, 70, 80), 1));
    p.drawRect(6, 6, 45, 47);

    // 五线谱线（三组，每组5条）
    p.setPen(QPen(QColor(30, 20, 8, 200), 1));
    // 第一行（上方）
    for (int i = 0; i < 5; i++)
        p.drawLine(8, 14 + i * 3, 52, 14 + i * 3);
    // 第二行（下方）
    for (int i = 0; i < 5; i++)
        p.drawLine(8, 38 + i * 3, 52, 38 + i * 3);

    // 高音谱号（极简两笔）
    p.setPen(QPen(QColor(30, 20, 8), 1));
    p.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath g;
    g.moveTo(10, 28); g.cubicTo(10, 12, 16, 10, 15, 17);
    g.cubicTo(14, 24, 8, 24, 9, 20);
    p.setBrush(Qt::NoBrush);
    p.drawPath(g);
    p.setRenderHint(QPainter::Antialiasing, false);

    // 音符点（小实心椭圆）
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 20, 8));
    int noteYs1[] = {14, 17, 14, 20, 14};
    int noteYs2[] = {38, 41, 38, 44, 38};
    for (int i = 0; i < 5; i++) {
        int nx = 20 + i * 7;
        p.drawEllipse(nx - 2, noteYs1[i] - 1, 5, 3);
        p.drawEllipse(nx - 2, noteYs2[i] - 1, 5, 3);
    }

    return icon;
}

// ============================================================
// onAddToBag：加入背包
// ============================================================
void SheetMusicWindow::onAddToBag()
{
    emit addToBag(10, generateSheetIcon());
    hide();
    emit windowClosed();
}

// ============================================================
// onLeaveInPlace：放在原地（不触发失效）
// ============================================================
void SheetMusicWindow::onLeaveInPlace()
{
    hide();
    emit leaveInPlace();
    emit windowClosed();
}

// ============================================================
// 鼠标事件：拖动（标题区）
// ============================================================
void SheetMusicWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->pos().y() < 80) {
        m_dragging   = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
    }
}
void SheetMusicWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) move(event->globalPos() - m_dragOffset);
}
void SheetMusicWindow::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}
