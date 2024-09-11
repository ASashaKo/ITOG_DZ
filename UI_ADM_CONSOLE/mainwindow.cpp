#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clientmainwindow.h"
#include "ui_clientmainwindow.h"
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->Users_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _timer = std::make_shared<QTimer>(this);
    ui->statusbar->showMessage("No connection");
}

MainWindow::~MainWindow()
{
    delete ui;
}

MainWindow *MainWindow::create_console_wnd(QWidget *parent)
{
    auto w = new MainWindow(parent);
    if(w){
        return w;
    }else
        return nullptr;
}

void MainWindow::on_Server_lineEdit_returnPressed()
{
    on_Connect_pushButton_clicked();
}

void MainWindow::on_Connect_pushButton_clicked(){
    if(ui->Server_lineEdit->text().length()){
        if(!server_connection){
            try{
                server_connection = std::make_shared<TCPC>(ui->Server_lineEdit->text().toStdString().c_str());
            }catch(const std::exception& ex){
                QMessageBox::critical(this,"Error",ex.what());
                return;
            }
        }
        ui->statusbar->showMessage("Connection to chat server established OK");
        _update_data();
        connect(_timer.get(),&QTimer::timeout,this,&MainWindow::_update_data);
        _timer->start(1000);

    }else{
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Lack of data"));
        msgBox.setText(tr("No server adress set"));
        msgBox.exec();

    }
}

void MainWindow::on_BanpushButton_clicked(){
    auto cr = ui->Users_tableWidget->currentRow();
    if(cr<0) return;
    auto item = ui->Users_tableWidget->item(cr,0);
    std::string user_id;
    bool is_banned{false};
    if(item){
        user_id=item->text().toStdString();
    }

    item = ui->Users_tableWidget->item(cr,5);
    if(item){
       std::cout<<"Ban date:"<<item->text().toStdString()<<std::endl;
       auto sz = item->text().size();
       QMessageBox mbox;
       mbox.setWindowTitle("Confirm the action");
       if(sz && sz>1){
          mbox.setText("The user is banned.");
          mbox.setInformativeText("Do you want to unban the user?");
          is_banned=true;

       }else{
          mbox.setText("The user is active.");
          mbox.setInformativeText("Do you want to ban the user?");
          is_banned=false;
       }
       mbox.setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
       mbox.setDefaultButton(QMessageBox::Cancel);
       int ret = mbox.exec();
       if(ret==QMessageBox::Ok){
           if(_set_ban(user_id,!is_banned)){
               _update_users();
               ui->statusbar->showMessage("User ban flag changed OK");
           }else
               ui->statusbar->showMessage("Failed to set user ban flag");
       }
    }
}

void MainWindow::on_Users_tableWidget_clicked(const QModelIndex &index)
{
    ui->Users_tableWidget->selectRow(index.row());
}

void MainWindow::_update_data(){
    if(!server_connection) return;
    auto cr = ui->Users_tableWidget->currentRow();
    _update_users();
    _update_msg();
    if(cr>=0){
        ui->Users_tableWidget->selectRow(cr);
    }
}

void MainWindow::_update_users(){
    if(!server_connection) return;
    IOMSG msg;
    msg.mtype = eAuthAdmin;

    try {
        server_connection->send_to_server(msg);
    }catch(const std::exception& ex) {
        QMessageBox::critical(this,"Error",ex.what());
        return;
    }

    ui->Users_tableWidget->clearContents();
    ui->Users_tableWidget->setRowCount(0);

    uint8_t _msg_id{};
    bool exit_loop{false};
    std::vector<std::string> v_fields;
    size_t row_count{0};
    while(!exit_loop){
        if(server_connection->get_server_msg(msg)){
            _msg_id = static_cast<uint8_t>(msg.mtype);
            switch(_msg_id){
            case eUserNextAdmin:
            {
               std::string r{msg.body};
               if(split_msg_data(r,v_fields)){
                   size_t col_count{0};
                   ui->Users_tableWidget->insertRow(row_count);
                   for (const auto& f : v_fields) {
                       QString line = f.c_str();
                       QTableWidgetItem *item = new QTableWidgetItem(tr("%1").arg(line));
                       item->setFlags(Qt::ItemIsSelectable);
                       ui->Users_tableWidget->setItem(row_count,col_count++,item);
                   }
                   row_count++;
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
            server_connection->send_to_server(msg);
    }
}

void MainWindow::_update_msg(){
    if(!server_connection) return;
    IOMSG msg;
    msg.mtype = eGetMsgAdmin;

    try {
        server_connection->send_to_server(msg);
    }catch(const std::exception& ex) {
        QMessageBox::critical(this,"Error",ex.what());
        return;
    }

    ui->Messages_tableWidget->clearContents();
    ui->Messages_tableWidget->setRowCount(0);


    uint8_t _msg_id{};
    bool exit_loop{false};
    std::vector<std::string> v_fields;
    size_t row_count{0};
    while(!exit_loop){
        if(server_connection->get_server_msg(msg)){
            _msg_id = static_cast<uint8_t>(msg.mtype);
            switch(_msg_id){
            case eMsgNext:
            {
               std::string r{msg.body};
               if(split_msg_data(r,v_fields)){
                   size_t col_count{0};
                   ui->Messages_tableWidget->insertRow(row_count);
                   for (const auto& f : v_fields) {
                       QTableWidgetItem *item = new QTableWidgetItem(tr("%1").arg(QString(f.c_str())));
                       item->setFlags(Qt::ItemIsSelectable);
                       ui->Messages_tableWidget->setItem(row_count,col_count++,item);
                   }
                   row_count++;
               }
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
            server_connection->send_to_server(msg);
    }
}

bool MainWindow::_set_ban(const std::string& user_id, bool ban_flag){
    IOMSG msg;
    msg.mtype = eSetUserBanFlag;
    strcpy(msg.user, user_id.c_str());
    strcpy(msg.body,std::to_string(ban_flag).c_str());

    try {
        server_connection->send_to_server(msg);
    }catch(const std::exception& ex) {
        QMessageBox::critical(this,"Error",ex.what());
        return false;
    }
    uint8_t _msg_id{};
    bool exit_loop{false};
    while(!exit_loop){
        if(server_connection->get_server_msg(msg)){
           _msg_id = static_cast<uint8_t>(msg.mtype);
           if(_msg_id==eUserBanFlagSet){
                return true;
           }
        }
        break;
    }
    return false;
}

void MainWindow::on_actionLaunch_another_session_triggered(){
    auto w = ClientMainWindow::create_main_wnd();
    if(w)
         w->show();
}

void MainWindow::on_actionExit_triggered(){
    _timer->stop();
    if(server_connection)
        server_connection->close_session();

    this->close();
}

