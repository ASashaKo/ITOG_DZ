#include "clientmainwindow.h"
#include "ui_clientmainwindow.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QMessageBox>
#include <utility>

ClientMainWindow::ClientMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ClientMainWindow)
{
    ui->setupUi(this);
    ui->statusbar->showMessage("No connection");
    _timer = std::make_shared<QTimer>(this);

}

ClientMainWindow::~ClientMainWindow()
{
    on_actionExit_triggered();
    delete ui;
}

void ClientMainWindow::set_connection(std::shared_ptr<TCPC> cn){
    _server_connection=cn;
    if(_server_connection)
        ui->statusbar->showMessage("Connection established OK");
}

ClientMainWindow *ClientMainWindow::create_main_wnd(std::shared_ptr<TCPC> cn,QWidget *parent){
        StartScreen s(cn);
        auto ret =  s.exec();
        if(ret==QDialog::Rejected)
            return nullptr;

        auto w = new ClientMainWindow(parent);
        if(w){
            std::cout<<__FUNCTION__<<":user_id = "<<s.get_user_id()<<std::endl;

            w->set_connection(s.get_connection());
            w->set_login(s.get_login());
            w->set_user_id(s.get_user_id());
            w->setAttribute(Qt::WA_DeleteOnClose);
            w->init_connect_on_creation();
            return w;
        }else
            return nullptr;

}

void ClientMainWindow::set_login(const QString& login){
    _login = login;
    QString t{"Chat session:"};
    t+=_login;
    this->setWindowTitle(t);
    setRefresh_mode(1);
    _update_chats();
    setRefresh_mode(-1);
}

void ClientMainWindow::on_MessageEdit_returnPressed()
{
    on_SendToAllButton_clicked();
}

void ClientMainWindow::on_SendToAllButton_clicked(){
    if(ui->MessageEdit->text().length())
        bool res = _send_message(_user_id.c_str(),ui->MessageEdit->text().toStdString().c_str());
}

void ClientMainWindow::on_SendPrivateButton_clicked(){
    QDialog dial(this);
    dial.setModal(true);
    QVBoxLayout *l = new QVBoxLayout();
    dial.setLayout(l);
    QListWidget *userListWgt = new QListWidget(&dial);
    l->addWidget(userListWgt);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,&dial);
    l->addWidget(buttonBox);

    connect(buttonBox,&QDialogButtonBox::accepted,&dial,&QDialog::accept);
    connect(buttonBox,&QDialogButtonBox::rejected,&dial,&QDialog::reject);

    std::vector<std::string> userList;
    if(_get_user_list(userList)){
        for (auto user : userList) {
            userListWgt->addItem(QString::fromStdString(user));
        }
        userListWgt->setCurrentRow(0);
        auto result = dial.exec();
        if(result==QDialog::Accepted && userListWgt->currentItem()){
           bool res = _send_message(_user_id.c_str(),
                            ui->MessageEdit->text().toStdString().c_str(),
                            userListWgt->currentItem()->text().toStdString().c_str());
        }
    }
}

void ClientMainWindow::on_actionExit_triggered()
{
    _timer->stop();
    if(_server_connection)
        _server_connection->close_session();
    this->close();
}

void ClientMainWindow::on_actionLaunch_another_session_triggered()
{
    auto w = ClientMainWindow::create_main_wnd();
    if(w)
         w->show();
}

void ClientMainWindow::_update_chats()
{
    QString chat;
    std::vector<std::string> chatMessages;
    if(_refresh_mode==0 || _refresh_mode==-1){
        ui->PublicChatBrowser->clear();
        if(_get_messages(chatMessages)){
            std::vector<std::string> v_fields;
            for (const auto& msg : chatMessages) {
                if(split_msg_data(msg,v_fields)){
                    chat+="Date:";
                    chat.append(QString::fromStdString(v_fields[0]));
                    chat.append(" From:");
                    chat.append(QString::fromStdString(v_fields[1]));
                    chat.append(" To:");
                    chat.append(QString::fromStdString(v_fields[2]));
                    chat.append(" Text:");
                    chat.append(QString::fromStdString(v_fields[3]));
                    chat.append("\n");
                    if(ui->PublicChatBrowser->toPlainText()!=chat)
                        ui->PublicChatBrowser->setText(chat);
                }
            }
        }
    }

    chat.clear();

    if(_refresh_mode==1 || _refresh_mode==-1){
        ui->PrivateChatBrowser->clear();
        if(_get_messages(chatMessages,_login.toStdString().c_str())){
            std::vector<std::string> v_fields;
            for (const auto &msg : chatMessages) {
                if(split_msg_data(msg,v_fields)){
                    chat+="Date:";
                    chat.append(QString::fromStdString(v_fields[0]));
                    chat.append(" From:");
                    chat.append(QString::fromStdString(v_fields[1]));
                    chat.append(" Text:");
                    chat.append(QString::fromStdString(v_fields[3]));
                    chat.append("\n");
                    if(ui->PrivateChatBrowser->toPlainText()!=chat)
                        ui->PrivateChatBrowser->setText(chat);
                }
            }
        }
    }
}

bool ClientMainWindow::_send_message(const char *sender, const char *text, const char *receiver){
    if(!_server_connection) return false;

    IOMSG msg;
    msg.mtype = eSendToAll;
    strcpy(msg.user_id,sender);
    memcpy(msg.body,text,strlen(text));

    if(receiver){
        auto usr_id = _users_map.find(receiver);
        msg.mtype = eSendToUserMsgReady;
        strcpy(msg.user,_user_id.c_str());
        if(usr_id != _users_map.end())
            strcpy(msg.user_id,usr_id->second.c_str());
    }

    try {
        _server_connection->send_to_server(msg);
    }catch(const std::exception& ex) {
        QMessageBox::critical(this,"Error",ex.what());
        return false;
    }

    uint8_t _msg_id{};
    bool exit_loop{false},delivered{false};
    while(!exit_loop){
        if(_server_connection->get_server_msg(msg)){
            _msg_id = static_cast<uint8_t>(msg.mtype);
            switch(_msg_id){
            case eMsgDelivered:
            {
               delivered=true;
               exit_loop=true;
            }
                break;
            case eErrMsgNotDelivered:
            case eError:
                QMessageBox::critical(this,"Error",msg.body);
                exit_loop=true;
                break;
            }
        }else
            exit_loop=true;
    }
    return delivered;
}

bool ClientMainWindow::_get_messages(std::vector<std::string>& v,const char *receiver)
{

    if(!_server_connection) return false;
    IOMSG msg;

    if(!receiver)
        msg.mtype = eGetMsgAdmin;
    else{
        msg.mtype = eGetUserMsg;
        memset(msg.user_id,'\0',sizeof(msg.user_id));
        strcpy(msg.user_id,_user_id.c_str());

    }

    try {
        _server_connection->send_to_server(msg);
    }catch(const std::exception& ex) {
        QMessageBox::critical(this,"Error",ex.what());
        return false;
    }

    v.clear();


    uint8_t _msg_id{};
    bool exit_loop{false};
    while(!exit_loop){
        if(_server_connection->get_server_msg(msg)){
            _msg_id = static_cast<uint8_t>(msg.mtype);
            switch(_msg_id){
            case eMsgNext:
            {
               v.emplace_back(msg.body);
               msg.mtype = eGetNextMsg;
               memset(msg.body, '\0', MSG_LENGTH);
            }
                break;
            case eNoMsg:
                exit_loop=true;
                break;
            }
        }else
            exit_loop=true;
        if(!exit_loop)
            _server_connection->send_to_server(msg);
    }
    return true;
}

bool ClientMainWindow::_get_user_list(std::vector<std::string>& ul){

    if(!_server_connection) return false;
    IOMSG msg;
    msg.mtype = eAuthAdmin;

    try {
        _server_connection->send_to_server(msg);
    }catch(const std::exception& ex) {
        QMessageBox::critical(this,"Error",ex.what());
        return false;
    }


    ul.clear();
    _users_map.clear();

    uint8_t _msg_id{};
    bool exit_loop{false};
    std::vector<std::string> v_fields;
    while(!exit_loop){
        if(_server_connection->get_server_msg(msg)){
            _msg_id = static_cast<uint8_t>(msg.mtype);
            switch(_msg_id){
            case eUserNextAdmin:
            {
               std::string r{msg.body},lst_item;
               if(split_msg_data(r,v_fields)){
                   lst_item=v_fields[1];
                   _users_map.insert(std::make_pair(v_fields[1],v_fields[0]));
                   ul.emplace_back(lst_item);
               }
               msg.mtype = eUserNextAdmin;
               memset(msg.body, '\0', MSG_LENGTH);
            }
                break;
            case eNoMsg:
                exit_loop=true;
                break;

            }
        }else
            exit_loop=true;
        if(!exit_loop)
            _server_connection->send_to_server(msg);
    }
    return true;
}

void ClientMainWindow::setRefresh_mode(int newRefresh_mode){
    _refresh_mode = newRefresh_mode;
}

void ClientMainWindow::init_connect_on_creation(){
    connect(_timer.get(),&QTimer::timeout,this,&ClientMainWindow::_update_chats);
     _update_chats();
    _timer->start(1000);
}

