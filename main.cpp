#include "widget.h"
#include <QApplication>

// ============================================================
// main：程序入口，创建应用与主窗口
// ============================================================
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
    return a.exec();
}
