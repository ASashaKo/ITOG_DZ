#include "registrationform.h"
#include "ui_registrationform.h"
#include <QMessageBox>

RegistrationForm::RegistrationForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegistrationForm)
{
    ui->setupUi(this);
}

RegistrationForm::~RegistrationForm()
{
    delete ui;
}

void RegistrationForm::on_LoginButton_clicked()
{
    emit loginRequested();
}

void RegistrationForm::on_buttonBox_accepted()
{

    auto sz = ui->LoginEdit->text().size();
    if(!sz){
        QMessageBox::critical(this,"Error","Type proper login");
        return;
    }
    sz = ui->PasswordEdit->text().size();
    if(!sz){
        QMessageBox::critical(this,"Error","Empty password not allowed");
        return;
    }

    if(ui->PasswordEdit->text() != ui->ConfirmPasswordEdit->text()){
        QMessageBox::critical(this,"Error","Passwords don`t match");
        return;
    }

    emit accepted(ui->LoginEdit->text(),ui->PasswordEdit->text());
}

void RegistrationForm::on_buttonBox_rejected()
{
    emit rejected();
}

