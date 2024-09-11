#include "TCPServer.h"

TCPServer::TCPServer(const char* mod_name) {
	if(!_db_init()) _err_cnt++;
	if (!_err_cnt) {
		std::string err_descr;
		_log_ptr = std::make_shared<LOG>(mod_name);
		struct sockaddr_in svr_adress,_client_adress;
		_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (_socket < 0) {
			_err_cnt++;
			err_descr = "Could not create socket: "; err_descr += strerror(errno);
			std::cout << err_descr << std::endl;
			_log_ptr->write(err_descr.c_str());
			return;
		}

		const int enable = 1;
		socklen_t cl_length{ sizeof(_client_adress) };

		int iResult = setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		if (iResult < 0) {
			close(_socket);
			err_descr = "Could not set the socket option: "; err_descr += strerror(errno);
			std::cout << err_descr << std::endl;
			_log_ptr->write(err_descr.c_str());
			_err_cnt++;
			return;
		}
		memset(&svr_adress, 0, sizeof(svr_adress));

		svr_adress.sin_family = AF_INET;
		svr_adress.sin_addr.s_addr = htonl(INADDR_ANY);
		svr_adress.sin_port = htons(PORT);

		if (bind(_socket, (struct sockaddr*)&svr_adress, sizeof(svr_adress)) < 0) {
			close(_socket);
			err_descr = "Could not bind the address: "; err_descr += strerror(errno);
			std::cout << err_descr << std::endl;
			_log_ptr->write(err_descr.c_str());
			_err_cnt++;
			return;
		}
		if (listen(_socket, SOMAXCONN) < 0) {
			close(_socket);
			err_descr = "Could not set the backlog: "; err_descr += strerror(errno);
			std::cout << err_descr << std::endl;
			_log_ptr->write(err_descr.c_str());
			_err_cnt++;
			return;
		}else {
			std::cout << "Server is listening...\n";
		}
	}
}

TCPServer::~TCPServer(){
	close(_socket);
}

auto TCPServer::_db_init() -> bool {
	try {
		_hDB = std::make_shared<DBCTX>();
	}
	catch (const std::bad_alloc& ex) {
		std::string err_descr = "Failed to allocate memory for database context: "; err_descr += ex.what();
		std::cout << err_descr << std::endl;
		_log_ptr->write(err_descr.c_str());

		return false;
	}
	catch (...) {
		std::string err_descr = "Failed to init database..."; 
		std::cout << err_descr << std::endl;
		_log_ptr->write(err_descr.c_str());

		return false;
	}
	if (_hDB) {
		if (!_hDB->init_ok()) {
			std::string err_descr = "DB not ready. Error code: "; err_descr += _hDB->get_last_error();
			std::cout << err_descr << std::endl;
			_log_ptr->write(err_descr.c_str());

			return false;
		}
	}else
		return false;

	return true;

}

auto TCPServer::th_process_client(const CLC::CLCPtr& cl_conn,bool& stop_server) ->void{
	std::string info;
	bool exit_loop{ false };
	size_t bytes_in{};
	size_t buf_len{ sizeof(IOMSG) };
	char rcv_buf[sizeof(IOMSG)]{ '\0' };
	IOMSG _msg;
	uint8_t _msg_id{};

	while (!exit_loop) {
		bytes_in = read(cl_conn->get_socket(), rcv_buf, buf_len);
		if (bytes_in) {
			memcpy(&_msg, rcv_buf, buf_len);			
			if (cl_conn->process_client_msg(cl_conn->get_socket(), _msg, exit_loop))
				if (!cl_conn->send_to_client(cl_conn->get_socket(), _msg)) exit_loop = true;
			if (exit_loop) {
				info = "Session terminated.\n";
				break;
			}
		}
	}
	close(cl_conn->get_socket());
}

auto TCPServer::process_clients()->bool {
	if (_err_cnt)
		return false;
	std::string info;
	size_t bytes_in{}, bytes_out{}, iRet{};
	size_t buf_len{ sizeof(IOMSG) };
	char rcv_buf[sizeof(IOMSG)]{'\0'};
	bool stop_server{false};
	IOMSG _msg;
	uint8_t _msg_id{};

	while (!stop_server)
	{
		int _cl_socket;
		struct sockaddr_in addr_c;
		socklen_t addrlen = sizeof(addr_c);
		char cl_ip[15]{(char)'/0'};
		_cl_socket = accept(_socket, (struct sockaddr*)&addr_c, &addrlen);
		if (_cl_socket < 0) {
			close(_socket);
			info = "Could not accept the client: "; info += strerror(errno);
			std::cout << info << std::endl;
			_log_ptr->write(info.c_str());

			_err_cnt++;
			return false;
		}
		else {
			inet_ntop(AF_INET, &(addr_c.sin_addr), cl_ip, 15);
			info = "Incoming connection from client:"; info += cl_ip;
			std::cout << BOLDCYAN << info << RESET << std::endl;
			_log_ptr->write(info.c_str());
		}

		CLC::CLCPtr client_connection(new CLC(_cl_socket, cl_ip,_hDB, _log_ptr));

        std::thread ct(&TCPServer::th_process_client,this, client_connection, std::ref(stop_server));
		ct.detach();

	}
			
	return true;	
}

auto TCPServer::_hash_func(const std::string& user_name)->size_t {
	return cHasher{}(user_name);
}



