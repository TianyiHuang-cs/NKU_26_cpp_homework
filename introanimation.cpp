#include "introanimation.h"

IntroAnimation::IntroAnimation(QObject *parent)
    : QObject(parent),
    m_phase(PHASE_IDLE),
    m_tick(0),
    m_alpha(0.0f),
    m_popupVisible(false),
    m_popupAnyKeyReady(false)
{
    m_fourthBg.load(":/images/fourth_floor.png");
}

void IntroAnimation::start()
{
    m_phase = PHASE_PAGE1_FADEIN;
    m_tick  = 0;
    m_alpha = 0.0f;
    m_popupVisible = false;
    m_popupAnyKeyReady = false;
}

void IntroAnimation::nextPhase()
{
    m_phase = static_cast<Phase>(static_cast<int>(m_phase) + 1);
    m_tick = 0;
}

void IntroAnimation::update()
{
    m_tick++;

    switch (m_phase) {

    // ===== PAGE 1 =====
    case PHASE_PAGE1_FADEIN:
        m_alpha += 0.02f;
        if (m_alpha >= 1.0f) {
            m_alpha = 1.0f;
            nextPhase(); // → HOLD
        }
        break;

    case PHASE_PAGE1_HOLD:
        if (m_tick > 300) {
            nextPhase(); // → FADEOUT
        }
        break;

    case PHASE_PAGE1_FADEOUT:
        m_alpha -= 0.02f;
        if (m_alpha <= 0.0f) {
            m_alpha = 0.0f;
            nextPhase(); // → CLEAR
        }
        break;

    case PHASE_PAGE1_CLEAR:
        if (m_tick > 15) {
            nextPhase(); // → PAGE2_FADEIN
        }
        break;

    // ===== PAGE 2 =====
    case PHASE_PAGE2_FADEIN:
        m_alpha += 0.02f;
        if (m_alpha >= 1.0f) {
            m_alpha = 1.0f;
            nextPhase(); // → HOLD
        }
        break;

    case PHASE_PAGE2_HOLD:
        if (m_tick > 300) {
            nextPhase(); // → FADEOUT
        }
        break;

    case PHASE_PAGE2_FADEOUT:
        m_alpha -= 0.02f;
        if (m_alpha <= 0.0f) {
            m_alpha = 0.0f;
            nextPhase(); // → EYE_OPEN
        }
        break;

    // ===== EYE OPEN =====
    case PHASE_EYE_OPEN:
        m_alpha += 0.02f;
        if (m_alpha >= 1.0f) {
            m_alpha = 1.0f;
            nextPhase();
        }
        break;

    case PHASE_BG_SHOW:
        if (m_tick > 60) {
            m_popupVisible = true;
            nextPhase();
        }
        break;

    case PHASE_POPUP_SHOW:
        if (m_tick > 30)
            m_popupAnyKeyReady = true;
        break;

    default:
        break;
    }
}

void IntroAnimation::onAnyKey()
{
    if (m_phase == PHASE_POPUP_SHOW && m_popupAnyKeyReady) {
        m_phase = PHASE_DONE;
        emit finished();
    }
    else if (m_phase == PHASE_PAGE1_HOLD ||
             m_phase == PHASE_PAGE2_HOLD) {
        nextPhase();
    }
}

void IntroAnimation::render(QPainter &p)
{
    const int W = 800;
    const int H = 600;

    switch (m_phase) {

    // ===== PAGE 1 =====
    case PHASE_PAGE1_FADEIN:
    case PHASE_PAGE1_HOLD:
    case PHASE_PAGE1_FADEOUT:
        p.fillRect(0,0,W,H, QColor(240,235,220));
        drawPage(p, "今天，我一定要努力地赶ddl！！！", m_alpha);
        break;

    // ===== CLEAR =====
    case PHASE_PAGE1_CLEAR:
        p.fillRect(0,0,W,H, Qt::black);
        break;

    // ===== PAGE 2 =====
    case PHASE_PAGE2_FADEIN:
    case PHASE_PAGE2_HOLD:
    case PHASE_PAGE2_FADEOUT:
        p.fillRect(0,0,W,H, QColor(240,235,220));
        drawPage(p, "但是好困啊。。。睡一觉吧", m_alpha);
        break;

    // ===== EYE OPEN =====
    case PHASE_EYE_OPEN:
        if (!m_fourthBg.isNull())
            p.drawPixmap(0,0,W,H, m_fourthBg);
        else
            p.fillRect(0,0,W,H, Qt::black);

        drawEyeOpen(p, m_alpha);
        break;

    // ===== POPUP =====
    case PHASE_BG_SHOW:
    case PHASE_POPUP_SHOW:
        if (!m_fourthBg.isNull())
            p.drawPixmap(0,0,W,H, m_fourthBg);
        else
            p.fillRect(0,0,W,H, Qt::black);

        if (m_popupVisible)
            drawPopup(p);
        break;

    case PHASE_DONE:
        p.fillRect(0,0,W,H, Qt::black);
        break;

    default:
        break;
    }
}

// ================== DRAW HELPERS ==================

void IntroAnimation::drawPage(QPainter &p, const QString &text, float alpha)
{
    int W = 800, H = 600;

    QRadialGradient glow(W/2, H/2, 300);
    glow.setColorAt(0, QColor(255,255,240, int(alpha*120)));
    glow.setColorAt(1, QColor(240,235,220, 0));
    p.fillRect(0,0,W,H,glow);

    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f("Georgia", 22, QFont::Normal);
    f.setItalic(true);
    p.setFont(f);

    int a = int(alpha * 220);
    p.setPen(QColor(180,165,130, a/2));
    p.drawText(2,2,W,H,Qt::AlignCenter,text);
    p.setPen(QColor(75,52,22,a));
    p.drawText(0,0,W,H,Qt::AlignCenter,text);
}

void IntroAnimation::drawEyeOpen(QPainter &p, float progress)
{
    int W=800,H=600;
    int coverH = int((1.0f - progress) * H/2);

    p.fillRect(0,0,W,coverH,Qt::black);
    p.fillRect(0,H-coverH,W,coverH,Qt::black);
}

void IntroAnimation::drawPopup(QPainter &p)
{
    int W=800,H=600;
    int pw=500,ph=200;
    int px=(W-pw)/2,py=(H-ph)/2;

    p.fillRect(0,0,W,H,QColor(0,0,0,100));

    p.fillRect(px+4,py+4,pw-8,ph-8,QColor(235,220,180,248));
    p.setPen(QPen(QColor(100,75,35),2));
    p.drawRect(px+3,py+3,pw-7,ph-7);

    QFont tf("Georgia",12,QFont::Bold); tf.setItalic(true);
    p.setFont(tf); p.setPen(QColor(55,35,10));
    p.drawText(px,py+8,pw-28,44,Qt::AlignCenter,"— 糟糕！ —");

    QFont cf("Courier New",10); p.setFont(cf);
    p.setPen(QColor(60,38,12));
    p.drawText(px+20,py+60,pw-40,100,
               Qt::AlignLeft|Qt::TextWordWrap,
               "糟糕，睡过头了——\n现在已经是23:30了\n 图书馆23:00就闭馆了，怎么出去呢？！\n——如果有让时间倒流的魔法就好了‘QAQ’");

    QFont hf("Courier New",9); p.setFont(hf);
    p.setPen(QColor(120,88,38,(m_tick/20)%2==0?200:80));
    p.drawText(px,py+ph-30,pw,24,Qt::AlignCenter,"按任意键继续...");
}
