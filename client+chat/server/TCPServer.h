#pragma once
#include "ClientConnection.h"
#include "DBAccess.h"
#include <chrono>

#include "../utils.h"

class TCPServer{
public:
    TCPServer(const char*);
    ~TCPServer();
	auto process_clients()-> bool;
	auto th_process_client(const CLC::CLCPtr&,bool&) -> void;
	auto init_ok()->bool const { return !_err_cnt;};
private:
	int _socket{-1};	
	std::string _name{ "localhost" };
	size_t _err_cnt{};
	std::shared_ptr<DBCTX>_hDB;
	size_t _active_conns{ 0 };
    std::map<std::string, std::shared_ptr<Message>> _msg_pool;
	std::shared_ptr<LOG> _log_ptr;
	auto _db_init() -> bool;
	auto _hash_func(const std::string&) -> size_t;

};

using TSPS = TCPServer;


