#include "DBAccess.h"
#include <chrono>
#pragma warning(disable : 4996)

DBAccess::DBAccess() {
   _init_db_ctx();
   _log_ptr = std::make_shared<Logger>("dbaccess_");
}

DBAccess::~DBAccess() {
    mysql_close(_mysql);
}

auto DBAccess::_db_success(DRC r) -> bool {
    return r == eDatabaseAlreadyExists || r == eDatabaseCreatedOK || r == eInsertOK || r == eUpdateOK || r == eDeleteOK || r == eRecordExists;
}

auto DBAccess::init_ok() -> bool {
    return _db_success(_last_ret_code);
}

auto DBAccess::_init_db_ctx() -> DRC {
    _mysql = mysql_init(_mysql);
    std::string err;
    if (!_mysql) {
        err = "Can't create MySQL-descriptor: "; err += mysql_error(_mysql); err += '\n';
        _last_error = err;
        return eUnableToAllocateEnv;
    }
 
    if (!mysql_real_connect(_mysql, _server.c_str(), _UID.c_str(), _PWD.c_str(), NULL, 0, NULL, 0)) {
        err = "Can't connect to database server: "; err += mysql_error(_mysql); err += '\n';
        _last_error = err;
        return eFailedToConnectToDatabase;
    }else {
        mysql_set_character_set(_mysql, "utf8");
        _last_ret_code = _database_ready();
        return _last_ret_code;
    }
}

auto DBAccess::_database_ready() -> DRC {
    std::string instr = "show databases like "; instr += "\"%"; instr += _dbname; instr += "\"";
    std::string err;
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        std::cout << "Failed to execute query:" << mysql_error(_mysql);
        return eFailedToExecuteQuery;        
    }else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        if (res = mysql_store_result(_mysql)) {
            auto r_cnt = mysql_num_rows(res);
            mysql_free_result(res);
            if (r_cnt) 
                return eDatabaseAlreadyExists;
        }else {
            std::cout << "Failed to execute query:" << mysql_error(_mysql);
            return eFailedToExecuteQuery;
        }

        instr = "CREATE DATABASE "; instr += "`"; instr += _dbname; instr += "`";
        if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
            std::cout << "Failed to create database:" << mysql_error(_mysql) << std::endl;
            return eFailedToCreateDatabase;
        }
        
        instr = "CREATE TABLE if not exists "; instr += _dbname; instr += ".users("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "login varchar(16) CHARACTER SET latin1 COLLATE latin1_general_ci NOT null UNIQUE,"
            "name varchar(32) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci DEFAULT NULL,"
            "email varchar(32) CHARACTER SET latin1 COLLATE latin1_general_ci,"
            "created_at timestamp DEFAULT CURRENT_TIMESTAMP,"
            "deleted timestamp)";
        if (mysql_real_query(_mysql, instr.c_str(),instr.size())) {
            std::cout << "Failed to create table users:" << mysql_error(_mysql) << std::endl;
            return eFailedToCreateTable;
        }

        instr = "CREATE TABLE if not exists "; instr += _dbname; 
        instr += ".user_data (user_id INT NOT NULL REFERENCES users(id),pwdhash character(64) CHARACTER SET latin1 COLLATE latin1_general_ci DEFAULT NULL) ";
        instr += "DEFAULT CHARSET=latin1 COLLATE=latin1_general_ci";
        if (mysql_real_query(_mysql, instr.c_str(),instr.size())) {
            std::cout << "Failed to create table user_data:" << mysql_error(_mysql) << std::endl;
            return eFailedToCreateTable;
        }

        instr = "CREATE TABLE if not exists "; instr += _dbname;
        instr += ".user_ban (user_id INT NOT NULL REFERENCES users(id),deadline date not null)";
        if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
            std::cout << "Failed to create table user_ban:" << mysql_error(_mysql) << std::endl;
            return eFailedToCreateTable;
        }



        instr = "CREATE TRIGGER "; instr += _dbname; instr += ".reserv_new_user_data AFTER INSERT ON users for each row begin insert into user_data(user_id, pwdhash) values(new.id, NULL); end;";
        if (mysql_real_query(_mysql, instr.c_str(),instr.size())) {
            std::cout << "Failed to create trigger reserv_new_user_data:" << mysql_error(_mysql) << std::endl;
            return eFailedToCreateTrigger;
        }

        instr = "CREATE TRIGGER "; instr += _dbname; instr += ".users_before_del BEFORE DELETE ON users FOR EACH ROW "
            "BEGIN "
            "SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'DELETE canceled';"
            "END;";
        if (mysql_real_query(_mysql, instr.c_str(),instr.size())) {
            std::cout << "Failed to create trigger reserv_new_user_data:" << mysql_error(_mysql) << std::endl;
            return eFailedToCreateTrigger;
        }


        instr = "CREATE TABLE if not exists "; instr += _dbname; instr += ".messages("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "from_id INT NOT NULL REFERENCES users(id),"
            "to_id INT NOT NULL REFERENCES users(id),"
            "body varchar(256) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci not NULL,"
            "created timestamp DEFAULT CURRENT_TIMESTAMP)";
        if (mysql_real_query(_mysql, instr.c_str(),instr.size())) {
            std::cout << "Failed to create table messages:" << mysql_error(_mysql) << std::endl;
            return eFailedToCreateTable;
        }
        instr = "CREATE TABLE if not exists "; instr += _dbname; instr += ".message_state("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "message_id INT NOT NULL REFERENCES messages(id),"
            "status INT not null check(status in(0, 1, 2)),"// -- 0 — not sent, 1 — delivered, 2 — viewed.
            "ts_changed timestamp DEFAULT CURRENT_TIMESTAMP)";
        if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
            std::cout << "Failed to create table message_state:" << mysql_error(_mysql) << std::endl;
            return eFailedToCreateTable;
        }
        std::cout << BOLDGREEN << UNDER_LINE << "FIRST LAUNCH: Database is ready" << RESET << std::endl;
    }
    return eDatabaseCreatedOK;
}

auto DBAccess::show_version(void) -> void {
    if (_mysql) {
        MYSQL_RES* res;
        mysql_query(_mysql, "SELECT @@VERSION");
        if (res = mysql_store_result(_mysql)) {
            std::cout << BOLDBLUE << UNDER_LINE << "Database server: MySQL version:" << mysql_fetch_row(res)[0] << RESET << std::endl;
            mysql_free_result(res);
        }
    }
}

auto DBAccess::add_user(const char* login, uint64_t& id,const char* name, const char* email)->bool {
    std::string instr = "select login from "; instr += _dbname; instr += ".users where login= '"; instr += login; instr += "' limit 1";
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error+= mysql_error(_mysql);
        return false;
    }
    else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        if (res = mysql_store_result(_mysql)) {
            auto r_cnt = mysql_num_rows(res);
            mysql_free_result(res);
            if (r_cnt) {
                _last_error = "Record with login :"; _last_error += login; _last_error += " already exists\n";
                return false;
            }
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);            
            return false;
        }


        instr = "insert into "; instr += _dbname; instr += ".users (login,name,email) values ('";
        instr += login; instr += "','";
        instr += (!name ? "":name); instr += "','";
        instr += (!email ? "":email); instr += "')";
        if (!mysql_real_query(_mysql, instr.c_str(), instr.size())) {
            auto r_cnt = mysql_affected_rows(_mysql);
            if (r_cnt) {
                id = mysql_insert_id(_mysql);
                return true;
            }else {
                _last_error = "No rows were affected by insert statement\n";
                return false;
            }
                
        }else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }
    }
}

auto DBAccess::login_used(const char* login)->bool {
    std::string instr = "select login from ";
    instr += _dbname;instr += ".users where login = '";
    instr += login; instr += "' limit 1";
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return false;
    }else {
        MYSQL_RES* res{ nullptr };
        if (res = mysql_store_result(_mysql)) {
            auto r_cnt = mysql_num_rows(res);
            mysql_free_result(res);
            if (r_cnt)
                return true;
            else
                return false;
        }
        else {
            _last_error = "Failed to get query results:"; _last_error += mysql_error(_mysql);
            return false;
        }           
    }

}

auto DBAccess::user_auth_ok(const char* name, const std::string& pwd_hash, std::string& user_id) -> bool {
    std::string instr = "select id, login, ud.pwdhash from ";
    instr += _dbname;
    instr += ".users usr join ";
    instr += _dbname;
    instr += ".user_data ud on usr.id = ud.user_id where usr.login = '";
    instr += name;
    instr += "' and  ud.pwdhash='"; instr += pwd_hash;
    instr += "' limit 1";
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return false;
    }
    else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        if (res = mysql_store_result(_mysql)) {
            auto r_cnt = mysql_num_rows(res);
            if (r_cnt) {
                if (row = mysql_fetch_row(res))
                    user_id = row[0];
                
                mysql_free_result(res);
                return true;
            }else
                mysql_free_result(res);
            return false;
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }
    }
}

auto DBAccess::user_pwdh_ok(const uint64_t& id, const std::string& pwd_hash) -> bool {
    std::string instr = "select id, ud.pwdhash from ";
    instr += _dbname;
    instr += ".users usr join ";
    instr += _dbname;
    instr += ".user_data ud on usr.id = ud.user_id where usr.id = ";
    instr += std::to_string(id);
    instr += " limit 1";
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return false;
    }
    else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        if (res = mysql_store_result(_mysql)) {
            if (row = mysql_fetch_row(res)) {
                mysql_free_result(res);
                return pwd_hash == row[4];
            }
            return false;
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }
    }
}

auto DBAccess::get_user(const char* login, uint64_t& id, std::string& name, std::string& email, std::string& pwd_hash) -> bool {
    std::string instr = "select id, login, name, email, ud.pwdhash from "; 
                        instr += _dbname;
                        instr += ".users usr join ";
                        instr += _dbname;
                        instr += ".user_data ud on usr.id = ud.user_id where usr.login = '";
                        instr += login;
                        instr += "' limit 1";
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return false;
    }else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        if (res = mysql_store_result(_mysql)) {
            if (row = mysql_fetch_row(res)) {
                id = strtoll(row[0],NULL,0);
                name = row[2];
                email = row[3];
                pwd_hash = row[4];
            }
            mysql_free_result(res);
            return true;
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }
    }
}

auto DBAccess::ban_user(const char* user_id, const char* action)->bool{
    if (*action == '1') {
        std::string instr = "delete from "; instr += _dbname; instr += ".user_ban where user_id = "; instr += user_id;
        if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }

        instr = "insert into "; instr += _dbname; instr += ".user_ban values("; instr += user_id; instr += ",date_add(now(), interval 1 day))";
        if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }
    }
    else {
        std::string instr = "delete from "; instr += _dbname; instr += ".user_ban where user_id = "; instr += user_id;
        if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }
    }
    return true;
}

auto DBAccess::get_users(std::vector<std::string>& v) -> bool const {
    std::string instr = "select id, login, name, email, ud.pwdhash, ifnull(ub.deadline,\"\") as banned_till from ";
    instr += _dbname;
    instr += ".users usr join ";
    instr += _dbname;
    instr += ".user_data ud on usr.id = ud.user_id left join ";
    instr += _dbname;
    instr += ".user_ban ub on usr.id =ub.user_id";

    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return false;
    }
    else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        size_t num_fields{};
        if (res = mysql_store_result(_mysql)) {
            num_fields = mysql_num_fields(res);
            while (row = mysql_fetch_row(res)) {
                std::string str_row;
                for (size_t i = 0; i < num_fields; i++)
                {
                    str_row += row[i]; str_row += '~';
                }
                str_row += '\0';
                v.emplace_back(str_row);
            }
            mysql_free_result(res);
            return v.size()>0;
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }
    }


}

auto DBAccess::set_user_attr(const uint64_t& id, const char* attr,const char *new_value) -> bool {
    std::string instr = "update ";
    instr += _dbname;
    instr += ".users set "; instr += attr; instr += "='";
    instr += new_value;
    instr += "' where id = ";
    instr += std::to_string(id);
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return false;
    }
    else
        return true;
}

auto DBAccess::set_msg_state(const char* msg_id, const char* new_state) -> bool {
    std::string instr = "update ";
    instr += _dbname;
    instr += ".message_state set status = "; instr += new_state; instr += " where message_id =";
    instr += msg_id;
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return false;
    }
    else
        return true;

}

auto DBAccess::set_user_pwdhash(const uint64_t& id, const char* new_pwdh) -> bool {
    std::string instr = "update ";
    instr += _dbname;
    instr += ".user_data set pwdhash='"; 
    instr += new_pwdh;
    instr += "' where user_id = ";
    instr += std::to_string(id);
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return false;
    }
    else
        return true;
}

auto DBAccess::user_is_banned(const char* usr_id, std::string& ban) -> bool{
    std::string instr = "select deadline from ";
    instr += _dbname;
    instr += ".user_ban where user_id = "; instr += usr_id; instr += " limit 1";
    if (_query_exec(instr)) {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        if (res = mysql_store_result(_mysql)) {
            if (row = mysql_fetch_row(res)) {
                ban = row[0];
                mysql_free_result(res);
                return true;
            }else {
                mysql_free_result(res);
                return false;
            }
        }else{
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }
    }else
        return false;
    return true;

}

auto DBAccess::pack_users(const char* client_id, std::string& str_res) ->bool const{
    std::string instr = "select id, login from ";
    instr += _dbname;
    instr += ".users where  id!="; instr += client_id;
    if (_query_exec(instr)) {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        std::string name, id;
        if (res = mysql_store_result(_mysql)) {
            str_res.clear();
            while (row = mysql_fetch_row(res)) {                
                str_res += row[0]; str_res +="|";
                str_res += row[1]; str_res.push_back('#');
            }
            mysql_free_result(res);
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return false;
        }
    }
    else
        return false;
    return true;
}

auto DBAccess::read_users(std::unordered_map<size_t, std::shared_ptr<User>>& users)->void {
    std::string instr = "select id, login, name, ud.pwdhash from ";
    instr += _dbname;
    instr += ".users usr join ";
    instr += _dbname;
    instr += ".user_data ud on usr.id = ud.user_id";
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return;
    }
    else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        std::string name, pwd_hash;
        uint64_t id;

        if (res = mysql_store_result(_mysql)) {
            while (row = mysql_fetch_row(res)) {
                id = strtoll(row[0], NULL, 0);
                name = row[2];
                pwd_hash = row[4];
                auto usr = std::make_shared<User>(name.c_str(), pwd_hash.c_str());
                usr->set_id(id);
                users.emplace(std::make_pair(cHasher{}(name), usr));
            }
            mysql_free_result(res);
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        }
    }
}

auto DBAccess::get_all_msgs(std::map<std::string, std::shared_ptr<Message>>& msg_pool) -> void {
    std::string t_msg{ _dbname }; t_msg += ".messages msg ";
    std::string instr = "select msg.id as msg_id, msg.body, usrsnd.login from_name, usrrcv.login to_name, msg.created ,msg.from_id, to_id, ms.status, ms.ts_changed from ";
    instr += t_msg;
    instr += "inner join ";instr += _dbname;instr += ".users usrsnd on usrsnd.id = msg.from_id ";
    instr += "inner join "; instr += _dbname; instr += ".users usrrcv on usrrcv.id = msg.to_id ";
    instr += "inner join "; instr += _dbname; instr += ".message_state ms on ms.message_id = msg.id";
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return;
    }
    else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        std::string name, pwd_hash;
        uint64_t id;

        if (res = mysql_store_result(_mysql)) {
            msg_pool.clear();
            while (row = mysql_fetch_row(res)) {
                auto msg_ptr = std::make_shared<Message>(row[1], row[2], row[3]);
                msg_ptr->set_ts(row[4]);
                msg_ptr->set_message_id(row[0]);
                msg_pool.insert(std::make_pair(row[0], msg_ptr));
            }
            mysql_free_result(res);
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        }
    }
}

auto DBAccess::get_user_msgs(const char* id, const std::string& usrname, std::map<std::string, std::shared_ptr<Message>>& msg_pool)->void {
    std::string instr = "select ms.ts_changed,msg.body,usr.login as sender,ms.message_id from ";
    instr += _dbname;
    instr += ".messages msg inner join ";
    instr += _dbname;
    instr += ".users usr on usr.id = msg.from_id ";
    instr += "inner join "; instr += _dbname; instr += ".message_state ms on ms.message_id = msg.id ";
    instr += "where msg.to_id ="; instr += id;
    instr += " or msg.from_id = "; instr += id;
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return;
    }
    else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;

        if (res = mysql_store_result(_mysql)) {
            msg_pool.clear();
            while (row = mysql_fetch_row(res)) {
                auto msg_ptr = std::make_shared<Message>(row[1], row[2], usrname.c_str());
                msg_ptr->set_ts(row[0]);
                msg_ptr->set_message_id(row[3]);
                msg_pool.insert(std::make_pair(row[3], msg_ptr));
            }
            mysql_free_result(res);
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        }
    }
}

auto DBAccess::get_user_msgs(const uint64_t& id, const std::string& usrname,std::map<std::string, std::shared_ptr<Message>>& msg_pool)->void {
    std::string instr = "select ms.ts_changed,msg.body,usr.login,ms.message_id from ";
    instr += _dbname;
    instr += ".messages msg inner join ";
    instr += _dbname;
    instr += ".users usr on usr.id = msg.from_id ";
    instr += "inner join "; instr += _dbname; instr += ".message_state ms on ms.message_id = msg.id ";
    instr += "where ms.status = 1 and msg.to_id =";instr += std::to_string(id);
    if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return;
    }
    else {
        MYSQL_RES* res{ nullptr };
        MYSQL_ROW row;
        std::string name, pwd_hash;
        uint64_t id;

        if (res = mysql_store_result(_mysql)) {
            msg_pool.clear();
            while (row = mysql_fetch_row(res)) {
                auto msg_ptr = std::make_shared<Message>(row[1], row[2], usrname.c_str());
                msg_ptr->set_ts(row[0]);
                msg_ptr->set_message_id(row[3]);
                msg_pool.emplace(msg_ptr->get_ts(), msg_ptr);
            }
            mysql_free_result(res);
        }
        else {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        }
    }

}

auto DBAccess::write_users(const std::unordered_map<size_t, std::shared_ptr<User>>& users) -> void {
    if (!users.size())
        return;

    size_t rec_count{ 0 };

    for (const auto& i : users) {
        auto _login = i.second->get_name();
        auto _pwdh = i.second->get_pwd_hash();

        std::string instr = "select login from "; instr += _dbname; instr += ".users where id = ";
        instr += std::to_string(i.second->get_id()); instr += " limit 1";
        if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
            _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
            return;
        }
        else {
            MYSQL_RES* res{ nullptr };
            MYSQL_ROW row;
            uint64_t id{};
            if (res = mysql_store_result(_mysql)) {
                auto r_cnt = mysql_num_rows(res);
                if (!r_cnt) {//if record not exists
                    instr = "insert into "; instr += _dbname; instr += ".users (login,name,email) values ('";
                    instr += _login; instr += "','";
                    instr += _login; instr += "','";
                    instr += ""; instr += "')";
                    if (_query_exec(instr, &id)) {
                       set_user_pwdhash(id, _pwdh.c_str());
                    }else 
                        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
                    
                    mysql_free_result(res);
                }
            }
        }
    }
}

auto DBAccess::_query_exec(const std::string& instr, uint64_t* key) -> bool {
    if (!mysql_real_query(_mysql, instr.c_str(), instr.size())) {
        auto kwi_pos = instr.find("insert"); auto kwu_pos = instr.find("update"); auto msg_pos = instr.find(".messages");
        if (kwi_pos != std::string::npos || kwu_pos != std::string::npos) {
            auto r_cnt = mysql_affected_rows(_mysql);
            if (!r_cnt) {
                _last_error = "No rows were affected by the statement\n";
                return false;
            }
            auto _id_new = static_cast<uint64_t>(mysql_insert_id(_mysql));
            if (kwi_pos != std::string::npos && key!=nullptr) 
                *key = _id_new;
        }
    }else {
        _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
        return false;
    }
    return true;
}

auto DBAccess::deliver_msg(const char* msg, const char* from_id, const char* to_id) -> bool {
    std::string instr{ "insert into " };
    char ts_buf[70]{};
    std::time_t now_ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm now_tm = *std::localtime(&now_ts);
    if (!strftime(ts_buf, sizeof ts_buf, "%Y-%m-%e %H:%M:%S", &now_tm)) {
        std::cout << "Time to string failed:\n";
        _last_error = "strftime failed in DBAccess::deliver_msg(";
        return false;
    }
        
    instr += _dbname; instr += ".messages (from_id,to_id,body) ";
    if (to_id) {
        instr += "values ("; instr += from_id; instr += ",";
        instr += to_id; instr += ",'"; instr += msg; instr += "')";
    }else {
        instr += "select "; instr += from_id; instr += ",";
        instr += "id, '"; instr += msg; instr += "' from "; instr += _dbname; instr += ".users where id !="; instr += from_id;
    }
    uint64_t msg_id;
    if (_query_exec(instr, &msg_id)) {//record added to messages table
        //Make changes in message_state table
        if (to_id) {//sending to defined user
            instr = "insert into "; instr += _dbname; instr += ".message_state (message_id,status) ";
            instr += "values ("; instr += std::to_string(msg_id); instr += ",1)"; //1 = delivered
            return _query_exec(instr);
        }else {//broadcast sending
            instr = "select id from ";instr += _dbname; instr += ".messages where from_id ="; instr += from_id;
            instr += " and created>='"; instr += ts_buf; instr += "'";
            if (mysql_real_query(_mysql, instr.c_str(), instr.size())) {
                _last_error = "Failed to execute query:"; _last_error += mysql_error(_mysql);
                return false;
            }
            else {
                MYSQL_RES* res{ nullptr };
                MYSQL_ROW row;
                uint64_t id{};
                if (res = mysql_store_result(_mysql)) {
                    while (row = mysql_fetch_row(res)) {
                        instr = "insert into "; instr += _dbname; instr += ".message_state (message_id,status) ";
                        instr += "values ("; instr += row[0]; instr += ",1)"; //1 = delivered
                        if (!_query_exec(instr)) {
                            _last_error = "Failed to update message state:"; _last_error += mysql_error(_mysql);
                            return false;
                        }
                    }
                    mysql_free_result(res);
                    return true;
                }else {
                    _last_error = "Failed to access query results:"; _last_error += mysql_error(_mysql);
                    return false;
                }
            }
        }
    }
    else
        return false;
    
}

