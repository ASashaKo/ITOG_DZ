#ifndef LOGINFORM_H
#define LOGINFORM_H

#include <QWidget>

namespace Ui {
class LoginForm;
}

class LoginForm : public QWidget
{
    Q_OBJECT

public:
    explicit LoginForm(QWidget *parent = nullptr);
    ~LoginForm();
signals:
    void registrationRequested();
    void accepted(QString,QString);
    void rejected();

private slots:
    void on_RegistrationButton_clicked();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::LoginForm *ui;
};

#endif // LOGINFORM_H
