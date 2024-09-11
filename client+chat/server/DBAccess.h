#pragma once
#include <iostream>
#include <memory>

#include <mysql/mysql.h>
#include <vector>
#include <map>
#include <unordered_map>

#include "Message.h"
#include "User.h"
#include "Logger.h"

#include "../utils.h"

enum DatabaseRetCodes {
    eUnableToAllocateEnv,
    eUnableToAllocateConnection,
    eUnableToAllocateQuery,
    eFailedToConnectToServer,
    eFailedToConnectToDatabase,
    eUnableToDefineODBCversion,
    eFailedToExecuteQuery,
    eFailedToCreateDatabase,
    eFailedToCreateTable,
    eFailedToCreateTrigger,
    eDatabaseAlreadyExists,
    eDatabaseCreatedOK,
    eInsertFailed,
    eInsertOK,
    eUpdateFailed,
    eUpdateOK,
    eDeleteFailed,
    eDeleteOK,
    eRecordExists,
    eRecordNotFound
};
using DRC = DatabaseRetCodes;

constexpr auto SQL_RESULT_LEN = 240;
constexpr auto SQL_QUERY_SIZE = 1024;

class DBAccess{
public:
    DBAccess();
    ~DBAccess();
    DBAccess(const DBAccess&) = delete;
    DBAccess& operator=(const DBAccess&)=delete;
    auto show_version(void) -> void;
    template<typename S> auto set_uid(const S& s)-> void { _UID = s;};
    template<typename S> auto set_pwd(const S& s)-> void { _PWD = s; };
    template<typename S> auto set_db_name(const S& s) -> void { _dbname = s; };
    template<typename S> auto set_server(const S& s) -> void { _server = s; };
    template<typename S> auto set_odbc_port(const S& s) -> void { _odbc_port = s; };
    auto set_mysql_port(unsigned int p)->void { _mysql_port = p; };


    auto init_ok()-> bool;
    auto get_last_error() -> const char* const { return _last_error.c_str();};
    auto add_user(const char *l, uint64_t&, const char *n=nullptr, const char *e=nullptr)-> bool;
    auto get_user(const char* , uint64_t&, std::string&, std::string&, std::string&) -> bool;
    auto ban_user(const char* user_id,const char*)->bool;
    auto get_users(std::vector<std::string>&) -> bool const;
    
    auto get_user_msgs(const uint64_t&, const std::string&, std::map<std::string, std::shared_ptr<Message>>&)->void;
    auto get_user_msgs(const char*, const std::string&, std::map<std::string, std::shared_ptr<Message>>&) -> void;
    auto get_all_msgs(std::map<std::string, std::shared_ptr<Message>>&) -> void;


    auto read_users(std::unordered_map<size_t, std::shared_ptr<User>>&)->void;
    auto pack_users(const char* client_id, std::string& ) -> bool const;
    auto write_users(const std::unordered_map<size_t, std::shared_ptr<User>>&) -> void;


    auto user_auth_ok(const char*, const std::string&, std::string&) -> bool;
    auto user_pwdh_ok(const uint64_t&, const std::string&) -> bool;
    auto login_used(const char*) -> bool;

    auto set_user_attr(const uint64_t&, const char*, const char*) -> bool;
    auto set_msg_state(const char*, const char*) -> bool;
    auto set_user_pwdhash(const uint64_t&, const char*) -> bool;
    auto user_is_banned(const char*, std::string&) -> bool;
    auto deliver_msg(const char* msg, const char* from_id, const char* to_id = nullptr) -> bool;


private:
    auto _init_db_ctx()-> DRC;
    auto _database_ready() -> DRC;
    auto _db_success(DRC) -> bool;
    auto _query_exec(const std::string& ,uint64_t* id = nullptr) -> bool;

    MYSQL *_mysql{nullptr};
    std::string _server{"localhost"};
    std::string _odbc_port{"3306"};
    unsigned int _mysql_port{0};
    std::string _dbname{"chatvy28"};
    std::string _UID{"root"};
    std::string _PWD{"1234"};
    std::string _last_error;
    DRC _last_ret_code;
    std::shared_ptr<Logger> _log_ptr{nullptr};

};
using DBCTX = DBAccess;
