#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    auto w = MainWindow::create_console_wnd();
    if(w)
         w->show();
    else
        return 0;
    return a.exec();

}
