#ifndef CLIENTMAINWINDOW_H
#define CLIENTMAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "TCPClient.h"
#include "startscreen.h"
#include <QTimer>


namespace Ui {
class ClientMainWindow;
}

class ClientMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    ClientMainWindow(QWidget *parent = nullptr);
    ~ClientMainWindow();
    void set_connection(std::shared_ptr<TCPC> cn);
    static ClientMainWindow* create_main_wnd(std::shared_ptr<TCPC> cn=nullptr,QWidget *parent = nullptr);
    void setRefresh_mode(int newRefresh_mode);
    void init_connect_on_creation();

public slots:
    void set_login(const QString&);
    void set_user_id(const std::string& uid){_user_id=uid;};


private slots:
    void on_MessageEdit_returnPressed();
    void on_SendToAllButton_clicked();
    void on_SendPrivateButton_clicked();
    void on_actionExit_triggered();
    void on_actionLaunch_another_session_triggered();
    void _update_chats();
    bool _send_message(const char* sender,const char* text,const char* receiver=nullptr);
    bool _get_messages(std::vector<std::string>&,const char* receiver=nullptr);
    bool _get_user_list(std::vector<std::string>&);

private:
    Ui::ClientMainWindow *ui;
    std::shared_ptr<TCPC> _server_connection{nullptr};
    std::map<std::string, std::string> _users_map;
    QString _login;
    std::string _user_id;
    std::shared_ptr<QTimer> _timer{nullptr};
    int _refresh_mode{-1};


};

#endif // CLIENTMAINWINDOW_H
