#ifndef INTROANIMATION_H
#define INTROANIMATION_H

#include <QObject>
#include <QPixmap>
#include <QPainter>

class IntroAnimation : public QObject
{
    Q_OBJECT
public:
    explicit IntroAnimation(QObject *parent = nullptr);

    void start();
    void update();
    void render(QPainter &painter);

    bool isFinished() const { return m_phase == PHASE_DONE; }

signals:
    void finished();

public slots:
    void onAnyKey();

private:
    enum Phase {
        PHASE_IDLE,

        PHASE_PAGE1_FADEIN,
        PHASE_PAGE1_HOLD,
        PHASE_PAGE1_FADEOUT,
        PHASE_PAGE1_CLEAR,

        PHASE_PAGE2_FADEIN,
        PHASE_PAGE2_HOLD,
        PHASE_PAGE2_FADEOUT,

        PHASE_EYE_OPEN,
        PHASE_BG_SHOW,
        PHASE_POPUP_SHOW,

        PHASE_DONE
    };

    Phase m_phase;
    int   m_tick;
    float m_alpha;

    QPixmap m_fourthBg;

    bool m_popupVisible;
    bool m_popupAnyKeyReady;

    void nextPhase();

    void drawPage(QPainter &p, const QString &text, float alpha);
    void drawEyeOpen(QPainter &p, float progress);
    void drawPopup(QPainter &p);
};

#endif // INTROANIMATION_H
