#include "loginform.h"
#include "ui_loginform.h"
#include <QMessageBox>

LoginForm::LoginForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginForm)
{

    ui->setupUi(this);
}

LoginForm::~LoginForm()
{
    delete ui;
}

void LoginForm::on_RegistrationButton_clicked()
{
    emit registrationRequested();
}

void LoginForm::on_buttonBox_accepted()
{
    auto sz = ui->LoginEdit->text().length();
    if(!sz){
        QMessageBox::critical(this,"Error","Empty login not allowed");
        return;
    }
    sz = ui->PasswordEdit->text().length();
    if(!sz){
        QMessageBox::critical(this,"Error","Empty password not allowed");
        return;
    }

    emit accepted(ui->LoginEdit->text(),ui->PasswordEdit->text());
}

void LoginForm::on_buttonBox_rejected()
{
    emit rejected();
}

