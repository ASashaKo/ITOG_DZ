#include "startscreen.h"
#include "ui_startscreen.h"
#include <QMessageBox>

StartScreen::StartScreen(std::shared_ptr<TCPC> conn,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StartScreen)
{
    ui->setupUi(this);
    setLoginForm();
    connect(ui->LoginPage,&LoginForm::registrationRequested,this,&StartScreen::setRegistrationForm);
    connect(ui->LoginPage,&LoginForm::rejected,this,&StartScreen::on_rejected);
    connect(ui->LoginPage,&LoginForm::accepted,this,&StartScreen::on_accepted);

    connect(ui->RegistrationPage,&RegistrationForm::loginRequested,this,&StartScreen::setLoginForm);
    connect(ui->RegistrationPage,&RegistrationForm::rejected,this,&StartScreen::on_rejected);
    connect(ui->RegistrationPage,&RegistrationForm::accepted,this,&StartScreen::on_accepted_reg);
    setSrv_adress(ui->ServerAdressEdit->text());
    if(conn)
        _dbctx = conn;
    else
        _dbctx = std::make_shared<TCPC>(_srv_adress.toStdString().c_str());

}

StartScreen::~StartScreen(){
    delete ui;
}

void StartScreen::setLoginForm(){
    ui->stackedWidget->setCurrentIndex(0);
}

void StartScreen::setRegistrationForm(){
    ui->stackedWidget->setCurrentIndex(1);
}

void StartScreen::on_rejected(){
    this->close();
}

void StartScreen::on_accepted(const QString& login,const QString& pwd){
    if(!_dbctx){
        try{
            _dbctx = std::make_shared<TCPC>(_srv_adress.toStdString().c_str());
        }catch(const std::exception& ex){
            QMessageBox::critical(this,"Error",ex.what());
            return;
        }
    }
    if(!auth_ok(login,pwd)){
        QString descr{"Authentication failed:"};
        descr.append(_last_error.c_str());
        QMessageBox::critical(this,"Bad news",descr);
        return;
    }
    _login = login;

    accept();
}

void StartScreen::on_accepted_reg(const QString& login, const QString& pwd){
    if(!_dbctx){
        try{
            _dbctx = std::make_shared<TCPC>(_srv_adress.toStdString().c_str());
        }catch(const std::exception& ex){
            QMessageBox::critical(this,"Error",ex.what());
            return;
        }
    }
    std::string ext_data;
    if(!reg_ok(login,pwd,ext_data)){
        QMessageBox::critical(this,"Error",ext_data.c_str());
        return;
    }
    _login = login;
    _user_id = ext_data;
    accept();
}

bool StartScreen::auth_ok(const QString & login, const QString & pwd){
    std::string ext_info;
    bool res = _dbctx->check_auth(login.toStdString().c_str(),pwd.toStdString().c_str(),ext_info);
    if(res)
        _user_id = ext_info;
    else
        _last_error = ext_info;
    return res;
}

bool StartScreen::reg_ok(const QString & login, const QString & pwd,std::string& err_descr){
    return _dbctx->reg_user(login.toStdString().c_str(),pwd.toStdString().c_str(),err_descr);
}


const QString &StartScreen::srv_adress() const{
    return _srv_adress;
}

void StartScreen::setSrv_adress(const QString &newSrv_adress){
    _srv_adress = newSrv_adress;
}

void StartScreen::on_ServerAdressEdit_returnPressed()
{
    setSrv_adress(ui->ServerAdressEdit->text());
}

