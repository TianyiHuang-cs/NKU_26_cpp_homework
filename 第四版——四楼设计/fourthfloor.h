#ifndef FOURTHFLOOR_H
#define FOURTHFLOOR_H

#include "basefloor.h"
#include "bagwindow.h"
#include "notedialog.h"
#include <QPixmap>
#include <QRect>
#include <QWidget>
#include <QPushButton>
#include <QTimer>
#include <QLabel>

// ============================================================
// AIPuzzleDialog：电脑处机关弹窗（第一页：问AI / 开始做题）
// ============================================================
class AIPuzzleDialog : public QWidget
{
    Q_OBJECT
public:
    explicit AIPuzzleDialog(QWidget *parent = nullptr);

signals:
    void requestAI();       // 用户选择"问问AI"
    void requestQuiz();     // 用户选择"开始做题"
    void dialogClosed();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *)  override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    QPushButton *m_aiBtn;
    QPushButton *m_quizBtn;
    QPushButton *m_closeBtn;
    bool   m_dragging;
    QPoint m_dragOffset;

    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);
};

// ============================================================
// AIChatDialog：与AI对话界面
// ============================================================
class AIChatDialog : public QWidget
{
    Q_OBJECT
public:
    explicit AIChatDialog(QWidget *parent = nullptr);

signals:
    void dialogClosed();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *)  override;
    void mouseReleaseEvent(QMouseEvent *) override;

private slots:
    void onSendMessage();
    void onThinkingTimer();

private:
    QPushButton *m_sendBtn;
    QPushButton *m_closeBtn;
    QLabel      *m_replyLabel;
    QTimer      *m_thinkTimer;
    int          m_thinkPhase; // 0=显示第一句, 1=显示第二句
    bool         m_sent;

    bool   m_dragging;
    QPoint m_dragOffset;

    QString m_replyText;
    bool    m_showFirstLine;
    bool    m_showSecondLine;

    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);
};

// ============================================================
// QuizDialog：排序做题弹窗
// ============================================================
class QuizDialog : public QWidget
{
    Q_OBJECT
public:
    explicit QuizDialog(QWidget *parent = nullptr);

signals:
    void quizCorrect(int minutes);  // 答对，倒流 minutes 分钟
    void quizWrong();
    void dialogClosed();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *)  override;
    void mouseReleaseEvent(QMouseEvent *) override;

private slots:
    void onOptionClicked(int idx);
    void onSubmit();

private:
    QPushButton *m_opts[3];     // 三个选项按钮
    QPushButton *m_submitBtn;
    QPushButton *m_closeBtn;

    int  m_clickOrder[3];       // 记录点击顺序 (0=未点击, 1/2/3=第几个点)
    int  m_clickCount;          // 已点击数

    QLabel *m_resultLabel;
    bool    m_submitted;

    bool   m_dragging;
    QPoint m_dragOffset;

    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);
};

// ============================================================
// DeskItemsDialog：桌面物品弹窗（6个可拾取物品）
// ============================================================
class DeskItemsDialog : public QWidget
{
    Q_OBJECT
public:
    explicit DeskItemsDialog(QWidget *parent = nullptr);

signals:
    void itemPickedUp(int itemId, const QString &name, const QPixmap &icon);
    void dialogClosed();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *)  override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    QPushButton *m_closeBtn;

    struct DeskItem {
        int     id;
        QString name;
        QString imagePath;
        bool    pickedUp;
        QRect   rect;
    };
    DeskItem m_items[6];

    bool   m_dragging;
    QPoint m_dragOffset;

    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);
};

// ============================================================
// FourthFloor：第四层——图书馆阅览室
//
// 场景元素（对应背景图 library_16_9__1_.png）：
//   - 电脑处机关（SPACE触发）：问AI / 做题
//   - 右侧台灯处机关（SPACE触发）：桌面物品
//   - 最右侧门（下键/鼠标触发）：去三层
//
// *** 出生点：椅子旁边，约 x=400, y=groundY ***
// ============================================================
class FourthFloor : public BaseFloor
{
    Q_OBJECT
public:
    explicit FourthFloor(QObject *parent = nullptr);
    ~FourthFloor() override;

    void onEnter(int windowW, int windowH) override;
    void onExit()  override;
    void update(Player &player, int windowW, int windowH) override;
    void render(QPainter &painter, const Player &player)  override;
    void onKeyPress(QKeyEvent *event,   Player &player) override;
    void onKeyRelease(QKeyEvent *event, Player &player) override;
    void onMousePress(QMouseEvent *event) override;
    void onMouseMove(QMouseEvent *event)  override;

    void setBagWindow(BagWindow *bag) { m_bagWin = bag; }
signals:
    void gameEnd(); // 游戏结束（预留）

private:
    // ===== 背景 =====
    QPixmap m_bgPixmap;
    int     m_groundY;
    // *** 地面Y：windowH * 0.78，微调请改此比例 ***

    // ===== 接近检测区域（屏幕坐标，onEnter中计算）=====
    QRect   m_computerRect;   // 电脑区域
    QRect   m_lampRect;       // 右侧台灯区域
    QRect   m_doorRect;       // 最右侧门

    bool    m_nearComputer;
    bool    m_nearLamp;
    bool    m_nearDoor;

    // *** 各区域X位置（微调请修改 onEnter 中对应的比例系数）***
    // computerRect：x = windowW * 0.45 ～ 0.60
    // lampRect：x = windowW * 0.72 ～ 0.80
    // doorRect：x = windowW - 60 ～ windowW - 10

    // ===== 按键状态 =====
    bool    m_keyLeft, m_keyRight, m_keyDown, m_keySpace;

    // ===== 跳跃 =====
    bool    m_isJumping;
    float   m_jumpVelocity;
    static constexpr float GRAVITY = 0.6f;

    // ===== 帧计数 =====
    int     m_tick;

    // ===== 机关状态 =====
    bool    m_lampItemsTriggered;  // 台灯物品弹窗已打开（物品可重复拾取，弹窗一直可开）
    // ★ 排序做题机关：答对后永久失效，答错可重复触发
    bool    m_quizSolved;          // 排序做题是否已答对（true=机关失效）

    // ===== 过渡动画 =====
    bool    m_transitioning;
    float   m_transitionAlpha;

    // ===== 弹窗 =====
    AIPuzzleDialog *m_aiPuzzleDlg;
    AIChatDialog   *m_aiChatDlg;
    QuizDialog     *m_quizDlg;
    DeskItemsDialog *m_deskDlg;

    // ===== 背包 =====
    BagWindow *m_bagWin;

    // ===== 时间倒流提示弹窗 =====
    NoteDialog *m_timeRewindDlg;

    // ===== 绘制 =====
    void drawBackground(QPainter &p);
    void drawDoor(QPainter &p);
    void drawHints(QPainter &p);
    void drawTransition(QPainter &p);
    void drawPlayerSprite(QPainter &p, const Player &player);
    void drawPixelRect(QPainter &p, int x, int y, int w, int h,
                       const QColor &fill, const QColor &border);

    void showAIPuzzleDialog();
    void showDeskItemsDialog();
};

#endif // FOURTHFLOOR_H
