#ifndef OUTROANIMATION_H
#define OUTROANIMATION_H

#include <QObject>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPushButton>

// ============================================================
// OutroAnimation：游戏结束动画
//
// 流程（每页停留5秒，柔和渐变切换）：
//   页1："既然出不去了，那就好好欣赏一下夜色吧~"
//   页2："同学醒醒，闭馆了！"
//   页3："原来我没有被困在图书馆，这只是一个梦。"
//   页4："我收起手下压着的推理小说，整理好书包，在闭馆音乐中，'终于'走出了图书馆。"
//   页5："如果你也因为ddl而焦虑……哈哈哈晚安好梦" + zzz图标
//   页6："Game over , Life moves on" + 退出按钮
// ============================================================
class OutroAnimation : public QObject
{
    Q_OBJECT
public:
    explicit OutroAnimation(QObject *parent = nullptr);

    void start();
    void update();
    void render(QPainter &p);
    void onAnyKey(QKeyEvent *event);
    void onMousePress(QMouseEvent *event);

signals:
    void finished();    // 退出游戏

private:
    enum Phase {
        PHASE_COVER,
        PHASE_IDLE,
        PHASE_FADEIN,   // 渐入当前页
        PHASE_HOLD,     // 停留
        PHASE_FADEOUT,  // 渐出
        PHASE_GAMEOVER, // 最后一页（含退出按钮）
        PHASE_DONE
    };

    Phase   m_phase;
    int     m_tick;
    float   m_alpha;        // 文字透明度
    int     m_pageIndex;    // 当前页（0~4 为故事页，5=Game Over页）
    float m_coverAlpha;  // 初始遮罩透明度

    // 每页文字
    static const int PAGE_COUNT = 5;
    QString m_pages[PAGE_COUNT];

    // 最后一页退出按钮区域
    QRect   m_exitBtnRect;
    bool    m_exitBtnHovered;

    // 页面背景色（柔和渐变）
    QColor  m_bgColor;

    void nextPage();
    void drawStoryPage(QPainter &p, const QString &text, float alpha);
    void drawZzzIcon(QPainter &p, float alpha);  // zzz打瞌睡图标
    void drawGameOverPage(QPainter &p);
    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);

    static constexpr int HOLD_TICKS = 250; // 每页停留 250帧 ≈5秒
    static constexpr float FADE_SPEED = 0.02f;
};

#endif // OUTROANIMATION_H
