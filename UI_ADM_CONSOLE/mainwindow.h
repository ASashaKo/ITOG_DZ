#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "TCPClient.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    static MainWindow* create_console_wnd(QWidget *parent = nullptr);
signals:


private slots:
    void on_Server_lineEdit_returnPressed();
    void on_Connect_pushButton_clicked();
    void on_BanpushButton_clicked();
    void on_Users_tableWidget_clicked(const QModelIndex &index);
    void _update_data();
    void _update_users();
    void _update_msg();
    bool _set_ban(const std::string&,bool);

    void on_actionLaunch_another_session_triggered();

    void on_actionExit_triggered();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<TCPC> server_connection{nullptr};
    std::shared_ptr<QTimer> _timer{nullptr};

};

#endif // MAINWINDOW_H
