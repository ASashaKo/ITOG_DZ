#ifndef STARTSCREEN_H
#define STARTSCREEN_H

#include <QDialog>
#include <memory>
#include "TCPClient.h"

namespace Ui {
class StartScreen;
}

class StartScreen : public QDialog
{
    Q_OBJECT

public:
    explicit StartScreen(std::shared_ptr<TCPC> conn = nullptr,QWidget *parent = nullptr);
    ~StartScreen();
    void setLoginForm();
    void setRegistrationForm();
    void on_rejected();
    void on_accepted(const QString &,const QString &);
    void on_accepted_reg(const QString &,const QString &);
    bool auth_ok(const QString &,const QString &);
    bool reg_ok(const QString&,const QString&,std::string&);
    const QString &srv_adress() const;
    const QString& get_login() const {return _login;};
    const std::string& get_user_id() const {return _user_id;};
    void setSrv_adress(const QString &newSrv_adress);
    std::shared_ptr<TCPC> get_connection() const {return _dbctx;};
    void set_connection(std::shared_ptr<TCPC> cn){_dbctx = cn;};

private slots:
    void on_ServerAdressEdit_returnPressed();

private:
    Ui::StartScreen *ui;
    QString _srv_adress;
    QString _login;
    std::string _user_id;
    std::shared_ptr<TCPC> _dbctx{nullptr};
    std::string _last_error;

};

#endif // STARTSCREEN_H
