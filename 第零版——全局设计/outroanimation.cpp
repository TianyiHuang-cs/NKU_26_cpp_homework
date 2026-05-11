#include "outroanimation.h"
#include <QPainter>
#include <QFont>
#include <QRadialGradient>
#include <QLinearGradient>
#include <cmath>

OutroAnimation::OutroAnimation(QObject *parent)
    : QObject(parent),
    m_phase(PHASE_IDLE),
    m_tick(0),
    m_alpha(0.0f),
    m_pageIndex(0),
    m_exitBtnHovered(false)
{
    // 五页故事文字
    m_pages[0] = "……咦 什么声音……";
    m_pages[1] = "同学醒醒，闭馆了！";
    m_pages[2] = "原来我没有被困在图书馆，\n这只是一个梦。";
    m_pages[3] = "我收起手下压着的推理小说，\n整理好书包，在闭馆音乐中，\n'终于'走出了图书馆。";
    m_pages[4] = "如果你也因为ddl而焦虑……\n也许可以……祝你晚安好梦";

    // 退出按钮区域（Game Over页）
    m_exitBtnRect = QRect(300, 430, 200, 52);
}

void OutroAnimation::start()
{
    m_phase      = PHASE_COVER;   // 新增一个遮罩阶段
    m_tick       = 0;
    m_alpha      = 0.0f;
    m_coverAlpha = 0.0f;
    m_pageIndex  = 0;
    m_exitBtnHovered = false;
}

void OutroAnimation::nextPage()
{
    m_pageIndex++;
    if (m_pageIndex >= PAGE_COUNT) {
        // 进入 Game Over 页
        m_phase = PHASE_GAMEOVER;
        m_tick  = 0;
        m_alpha = 0.0f;
    } else {
        m_phase = PHASE_FADEIN;
        m_tick  = 0;
        m_alpha = 0.0f;
    }
}

void OutroAnimation::update()
{
    m_tick++;

    if (m_phase == PHASE_COVER) {
        m_coverAlpha += 0.012f;
        if (m_coverAlpha >= 1.0f) {
            m_coverAlpha = 1.0f;
            // 遮罩完全黑后停留一下再进第一页
            if (m_tick > 60) {
                m_phase = PHASE_FADEIN;
                m_tick  = 0;
                m_alpha = 0.0f;
            }
        }
        return;
    }

    switch (m_phase) {
    case PHASE_FADEIN:
        m_alpha += FADE_SPEED;
        if (m_alpha >= 1.0f) {
            m_alpha = 1.0f;
            m_phase = PHASE_HOLD;
            m_tick  = 0;
        }
        break;

    case PHASE_HOLD:
        if (m_tick > HOLD_TICKS) {
            m_phase = PHASE_FADEOUT;
            m_tick  = 0;
        }
        break;

    case PHASE_FADEOUT:
        m_alpha -= FADE_SPEED;
        if (m_alpha <= 0.0f) {
            m_alpha = 0.0f;
            nextPage();
        }
        break;

    case PHASE_GAMEOVER:
        // Game Over 页淡入
        if (m_alpha < 1.0f) {
            m_alpha += FADE_SPEED;
            if (m_alpha > 1.0f) m_alpha = 1.0f;
        }
        break;

    default:
        break;
    }
}

void OutroAnimation::onAnyKey(QKeyEvent *event)
{
    Q_UNUSED(event)
    // 故事页可按任意键快进到下一页
    if (m_phase == PHASE_HOLD) {
        m_phase = PHASE_FADEOUT;
        m_tick  = 0;
    }
}

void OutroAnimation::onMousePress(QMouseEvent *event)
{
    if (m_phase == PHASE_GAMEOVER &&
        event->button() == Qt::LeftButton &&
        m_exitBtnRect.contains(event->pos()))
    {
        m_phase = PHASE_DONE;
        emit finished();
    }
}

void OutroAnimation::render(QPainter &p)
{
    int W = 800, H = 600;

    if (m_phase == PHASE_COVER) {
        // 夜色渐变遮罩（和原来 drawClearance 一样的视觉效果）
        QLinearGradient grad(0, 0, 0, H);
        grad.setColorAt(0.0, QColor(10, 15, 40, static_cast<int>(m_coverAlpha * 255)));
        grad.setColorAt(1.0, QColor(5, 8, 25,  static_cast<int>(m_coverAlpha * 255)));
        p.fillRect(0, 0, W, H, grad);

        if (m_coverAlpha > 0.3f) {
            int textAlpha = qMin(255, static_cast<int>((m_coverAlpha - 0.3f) / 0.7f * 220));
            p.setRenderHint(QPainter::Antialiasing, true);
            QFont f("Courier New", 18, QFont::Bold);
            f.setItalic(true);
            p.setFont(f);
            p.setPen(QColor(180, 200, 240, textAlpha));
            p.drawText(0, 0, W, H, Qt::AlignCenter,
                       "既然出不去了，那就好好享受一下夜色吧");
            p.setRenderHint(QPainter::Antialiasing, false);
        }
        return;
    }

    if (m_phase == PHASE_GAMEOVER || m_phase == PHASE_DONE) {
        drawGameOverPage(p);
        return;
    }

    // 故事页：柔和暖白背景
    p.fillRect(0, 0, W, H, QColor(242, 236, 220));

    // 柔和光晕
    QRadialGradient glow(W/2, H/2, 320);
    glow.setColorAt(0, QColor(255, 250, 235, 60));
    glow.setColorAt(1, QColor(242, 236, 220, 0));
    p.fillRect(0, 0, W, H, glow);

    if (m_pageIndex < PAGE_COUNT) {
        bool isLastStoryPage = (m_pageIndex == PAGE_COUNT - 1);
        drawStoryPage(p, m_pages[m_pageIndex], m_alpha);
        // 第5页（index=4）额外绘制 zzz 图标
        if (isLastStoryPage) {
            drawZzzIcon(p, m_alpha);
        }
    }
}

// ============================================================
// drawPixelRect
// ============================================================
void OutroAnimation::drawPixelRect(QPainter &p, int x, int y, int w, int h,
                                   const QColor &fill, const QColor &border)
{
    p.fillRect(x, y, w, h, fill);
    p.setPen(border);
    p.drawRect(x, y, w-1, h-1);
}

// ============================================================
// drawStoryPage：柔和渐变的分页文字
// ============================================================
void OutroAnimation::drawStoryPage(QPainter &p, const QString &text, float alpha)
{
    int W = 800, H = 600;
    int textAlpha = static_cast<int>(alpha * 220);

    p.setRenderHint(QPainter::Antialiasing, true);

    // 文字主体（居中，多行）
    QFont f("Georgia", 20, QFont::Normal);
    f.setItalic(true);
    p.setFont(f);

    // 阴影
    p.setPen(QColor(200, 185, 155, textAlpha / 3));
    p.drawText(QRect(2, 2, W, H - 80), Qt::AlignCenter | Qt::TextWordWrap, text);

    // 主色（深棕）
    p.setPen(QColor(75, 52, 22, textAlpha));
    p.drawText(QRect(0, 0, W, H - 80), Qt::AlignCenter | Qt::TextWordWrap, text);

    // 底部"按任意键继续"提示（闪烁）
    if (m_phase == PHASE_HOLD && (m_tick / 20) % 2 == 0) {
        QFont hf("Courier New", 10);
        p.setFont(hf);
        p.setPen(QColor(140, 110, 65, 180));
        p.drawText(0, H - 60, W, 30, Qt::AlignCenter, "— 按任意键继续 —");
    }

    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawZzzIcon：打瞌睡的三个 Z（手绘像素风）
// 显示在页面右下角
// ============================================================
void OutroAnimation::drawZzzIcon(QPainter &p, float alpha)
{
    int baseAlpha = static_cast<int>(alpha * 200);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 三个 Z，大小递增，右下角位置
    // 小 Z：右下角
    // 中 Z：稍左上
    // 大 Z：更左上
    struct ZInfo {
        int x, y, size;
        int extraAlpha; // 各Z的透明度差异（营造漂浮感）
    } zz[3] = {
        {620, 440, 22, 0},   // 小Z
        {648, 400, 32, -30}, // 中Z
        {680, 350, 44, -60}  // 大Z
    };

    for (int i = 0; i < 3; i++) {
        int a = qMax(0, baseAlpha + zz[i].extraAlpha);
        QColor zColor(90, 65, 25, a);
        QColor zBg(242, 236, 220, a / 2);

        QFont zf("Georgia", zz[i].size, QFont::Bold);
        zf.setItalic(true);
        p.setFont(zf);

        // 文字阴影
        p.setPen(QColor(200, 175, 130, a / 3));
        p.drawText(zz[i].x + 2, zz[i].y + 2, "Z");

        // Z 主体
        p.setPen(zColor);
        p.drawText(zz[i].x, zz[i].y, "Z");
    }

    // 绘制一个小小的卡通脸（闭眼打瞌睡）
    // 位置：右下角 580, 460
    int fx = 575, fy = 455;
    int fR = 28; // 脸半径

    // 脸轮廓
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(245, 220, 170, static_cast<int>(alpha * 220)));
    p.drawEllipse(fx - fR, fy - fR, fR*2, fR*2);

    // 脸边框
    p.setPen(QPen(QColor(140, 100, 50, static_cast<int>(alpha * 180)), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(fx - fR, fy - fR, fR*2, fR*2);

    // 闭眼（弧线）
    QPen eyePen(QColor(80, 50, 20, static_cast<int>(alpha * 200)), 2);
    p.setPen(eyePen);
    // 左眼
    p.drawArc(fx - 16, fy - 6, 10, 8, 0, -180*16);
    // 右眼
    p.drawArc(fx + 6,  fy - 6, 10, 8, 0, -180*16);

    // 嘴（小弧，微笑）
    p.drawArc(fx - 8, fy + 5, 16, 10, 200*16, -200*16);

    // 腮红
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(230, 150, 130, static_cast<int>(alpha * 80)));
    p.drawEllipse(fx - fR + 4, fy + 6, 12, 8);
    p.drawEllipse(fx + fR - 16, fy + 6, 12, 8);

    p.setRenderHint(QPainter::Antialiasing, false);
}

// ============================================================
// drawGameOverPage："Game over , Life moves on" + 退出按钮
// ============================================================
void OutroAnimation::drawGameOverPage(QPainter &p)
{
    int W = 800, H = 600;

    // 深色背景（夜晚感）
    p.fillRect(0, 0, W, H, QColor(8, 5, 2));

    // 柔和中央光晕
    QRadialGradient glow(W/2, H/2, 280);
    glow.setColorAt(0, QColor(40, 28, 10, static_cast<int>(m_alpha * 80)));
    glow.setColorAt(1, QColor(8, 5, 2, 0));
    p.fillRect(0, 0, W, H, glow);

    int ta = static_cast<int>(m_alpha * 230);

    p.setRenderHint(QPainter::Antialiasing, true);

    // 主标题："Game over , Life moves on"
    QFont mainF("Georgia", 28, QFont::Bold);
    mainF.setItalic(true);
    p.setFont(mainF);

    // 阴影
    p.setPen(QColor(180, 140, 50, ta / 4));
    p.drawText(2, 182, W, 80, Qt::AlignCenter, "Game over , Life moves on");

    // 主文字
    p.setPen(QColor(210, 170, 65, ta));
    p.drawText(0, 180, W, 80, Qt::AlignCenter, "Game over , Life moves on");

    // 分隔线
    int lineAlpha = static_cast<int>(m_alpha * 120);
    p.setPen(QPen(QColor(150, 110, 40, lineAlpha), 1));
    p.drawLine(200, 278, 600, 278);

    // 副标题
    QFont subF("Courier New", 13);
    p.setFont(subF);
    p.setPen(QColor(160, 130, 60, static_cast<int>(m_alpha * 180)));
    p.drawText(0, 295, W, 40, Qt::AlignCenter, "感谢游玩 / Thanks for playing");

    // zzz 图标（右下角，小一号）
    if (m_alpha > 0.5f) {
        float zAlpha = (m_alpha - 0.5f) * 2.0f;
        p.setFont(QFont("Georgia", 18, QFont::Bold));
        p.setPen(QColor(150, 120, 50, static_cast<int>(zAlpha * 160)));
        p.drawText(630, 340, "z");
        p.setFont(QFont("Georgia", 24, QFont::Bold));
        p.setPen(QColor(150, 120, 50, static_cast<int>(zAlpha * 130)));
        p.drawText(655, 310, "z");
        p.setFont(QFont("Georgia", 32, QFont::Bold));
        p.setPen(QColor(150, 120, 50, static_cast<int>(zAlpha * 100)));
        p.drawText(688, 275, "Z");
    }

    // 退出按钮（像素风）
    if (m_alpha > 0.6f) {
        int btnAlpha = static_cast<int>((m_alpha - 0.6f) / 0.4f * 255);
        int bx = m_exitBtnRect.x(), by = m_exitBtnRect.y();
        int bw = m_exitBtnRect.width(), bh = m_exitBtnRect.height();

        QColor btnBg  = m_exitBtnHovered
                           ? QColor(80, 55, 18, btnAlpha)
                           : QColor(30, 18, 6, btnAlpha);
        QColor btnRim = m_exitBtnHovered
                            ? QColor(210, 165, 60, btnAlpha)
                            : QColor(100, 72, 25, btnAlpha);

        p.fillRect(bx, by, bw, bh, btnBg);
        p.setPen(QPen(btnRim, 2));
        p.drawRect(bx, by, bw-1, bh-1);
        p.setPen(QPen(QColor(60, 40, 12, btnAlpha / 2), 1));
        p.drawRect(bx+3, by+3, bw-7, bh-7);

        QFont bf("Courier New", 13, QFont::Bold);
        p.setFont(bf);
        p.setPen(QColor(10, 5, 2, btnAlpha / 2));
        p.drawText(bx+1, by+1, bw, bh, Qt::AlignCenter, "退出游戏");
        p.setPen(m_exitBtnHovered
                     ? QColor(240, 200, 80, btnAlpha)
                     : QColor(195, 155, 55, btnAlpha));
        p.drawText(bx, by, bw, bh, Qt::AlignCenter, "退出游戏");
    }

    p.setRenderHint(QPainter::Antialiasing, false);
}
