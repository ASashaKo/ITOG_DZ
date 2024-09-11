#pragma once
#include "../netcommon.h"
#include "DBAccess.h"

class ClientConnection{
public:
    ClientConnection(int s, char* ip, std::shared_ptr<DBCTX>, std::shared_ptr<LOG>);
	auto get_socket() -> int const { return _socket; };
	auto send_to_client(int, IOMSG&) -> bool;
	auto process_client_msg(int, IOMSG&, bool&) -> bool;
    ClientConnection(const ClientConnection&);
    ClientConnection(ClientConnection&&);
    ~ClientConnection();
    typedef std::shared_ptr<ClientConnection> CLCPtr;

private:
	auto _login_used(const std::string&) -> bool;
	auto _is_valid_user_pwd(const std::string& pwd) -> bool;
	char _s_ip[15]{ '\0' };
	int _socket{ -1 };
	uint64_t _usr_db_id{};
	std::string _user, _pwd_hash;
	std::shared_ptr<DBCTX>_hDB;
    std::shared_ptr<User> _user_ptr;
    std::map<std::string, std::shared_ptr<Message>> _msg_pool;
	std::shared_ptr<LOG> _log_ptr;
	std::shared_mutex _mtxs;
	std::vector<std::string> _buffer;
};

using CLC = ClientConnection;

