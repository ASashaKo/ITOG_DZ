#pragma once
#include "netcommon.h"
#include <map>
#include "Message.h"
#include "utils.h"

class TCPClient{
public:
	TCPClient();
	TCPClient(const char*);
	~TCPClient();
	void start();
	void set_peer(const char* p) { _srv_name = p; };
    auto is_ready()->bool const {return _last_err_code==0;};
    auto send_to_server(IOMSG&) -> bool;
    auto get_server_msg(IOMSG&) -> bool;
    auto check_auth(const char* login, const char* pwd,std::string&)->bool;
    auto reg_user(const char* login, const char* pwd, std::string&)->bool;
    auto close_session()->void;

private:
	int _socket{-1};	
	int _last_err_code{ 0 };
	char _buffer[sizeof(IOMSG)]{'\0'};
	size_t _buf_len{ sizeof(IOMSG) };

	std::string _srv_name{"localhost"};
    auto _clean_up()->void;
    auto _init_connection() -> bool;
    auto _init_net_core()->bool;
	auto _unpack_available_users(const std::string&, std::map<size_t, std::string>&) -> void;
    auto _send_to_server(IOMSG&) -> bool;
    auto _is_valid_socket()->bool;
	uint64_t _usr_db_id{};
};

using TCPC = TCPClient;

