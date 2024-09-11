#include "TCPClient.h"
#include <vector>
#include <exception>
#include "sha1.h"

TCPClient::TCPClient() {
    if(!_init_net_core()){
        std::logic_error e("Failed to init network");
        throw std::exception(e);
    }
}

TCPClient::TCPClient(const char* peer) :_srv_name(peer) {
    if(!_init_net_core()){
        std::logic_error e("Failed to init network");
        throw std::exception(e);
    }

    if(!_init_connection()){
        _clean_up();
        std::logic_error e("Failed to establish connection to server");
        throw std::exception(e);
    }
}

auto TCPClient::_clean_up()->void{
    close(_socket);
}

TCPClient::~TCPClient(){
    _clean_up();
}

auto TCPClient::close_session()->void{
    IOMSG msg;
    msg.mtype = eQuit;
    if(_is_valid_socket()){
        send_to_server(msg);
    }
}

auto TCPClient::_init_net_core()->bool{
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket < 0) {
        printf("failed to create socket with error: %s\n", strerror(errno));
        _last_err_code = errno;
        return false;
    }
    return true;
}

auto TCPClient::_init_connection() -> bool {
	struct sockaddr_in svr_adress {};
	int conn{ -1 };
	memset(&svr_adress, 0, sizeof(svr_adress));
	svr_adress.sin_family = AF_INET;
	svr_adress.sin_addr.s_addr = inet_addr(_srv_name.c_str());
	svr_adress.sin_port = htons(PORT);

	conn = connect(_socket, (struct sockaddr*)&svr_adress, sizeof(svr_adress));
	if (conn < 0) {
		close(_socket);
		printf("failed to connect to server with error: %s\n", strerror(errno));
		_last_err_code = errno;
        return false;
	}
    return true;
}

auto TCPClient::get_server_msg(IOMSG& msg) -> bool{
    size_t bytes_in{};
    bytes_in = read(_socket, _buffer, _buf_len);
    if (bytes_in > 0) {
        memcpy(&msg, _buffer, _buf_len);
        return true;
    }
    return false;
}

auto TCPClient::check_auth(const char *login, const char *pwd,std::string& ext_data)->bool{
    char buf[32]{'\0'};
    strcpy(buf,pwd);
    std::string hash = hash_to_string(sha1(buf,strlen(pwd)));
    IOMSG _msg;
    uint8_t _msg_id{};
    _msg.mtype=eBatchAuth;
    strcpy(_msg.body,hash.c_str());
    strcpy(_msg.user,login);
    if (!_send_to_server(_msg))
        return false;

    if(get_server_msg(_msg)){
        _msg_id = static_cast<uint8_t>(_msg.mtype);
        switch(_msg_id){
        case eAuthOK:
            ext_data = _msg.user_id;
            return true;
        case eError:
            ext_data = _msg.body;
            return false;
        }
    }
    return false;
}

auto TCPClient::reg_user(const char *login, const char *pwd,std::string& ext_data)->bool{
    char buf[32]{'\0'};
    strcpy(buf,pwd);
    std::string hash = hash_to_string(sha1(buf,strlen(pwd)));
    IOMSG _msg;
    uint8_t _msg_id{};
     _msg.mtype=eBatchReg;
    strcpy(_msg.body,hash.c_str());
    strcpy(_msg.user,login);
    if (!_send_to_server(_msg))
        return false;

    if(get_server_msg(_msg)){
        _msg_id = static_cast<uint8_t>(_msg.mtype);
        switch(_msg_id){
            case eExistingUser:
                ext_data = "user ";ext_data+=login;ext_data+=" already exists";
                break;
            case eError:
                ext_data = _msg.body;
                break;
            case eAuthOK:
                ext_data = _msg.user_id;
                return true;
        }
    }
    return false;
}

void TCPClient::start()
{
	if (!_last_err_code) {
		size_t bytes_in{}, bytes_out{}, iRet{};
		bool exit_loop{ false }, server_down{false};
		std::string input;
		IOMSG _msg;
        _msg.mtype = eAuthAdmin;
		uint8_t _msg_id{};

		if (!_send_to_server(_msg)) {
			return;
		}

		while (!exit_loop)
		{
			bytes_in = read(_socket, _buffer, _buf_len);
			if (bytes_in > 0) {
				memcpy(&_msg, _buffer, _buf_len);
				_msg_id = static_cast<uint8_t>(_msg.mtype);
				switch (_msg_id) {
                case eUserNextAdmin:{
                    std::string inc_msg{_msg.body};
                    std::vector<std::string> v;
                    size_t pos = inc_msg.find('\n');
                    while(pos){
                        v.emplace_back(inc_msg.substr(0,pos));
                        inc_msg = inc_msg.substr(pos);
                        pos = inc_msg.find('\n');
                    }
                    break;
                }
				case eWelcome:
					std::cout << _msg.body;
					std::getline(std::cin, input);
					if (input.size() == 1 && input.at(0) == 'x') {
						_msg.mtype = eQuit;
						strcpy(_msg.body, "Good buy\n");
					}else {
						clear_message(_msg);
						if (input.size() == 1 && input.at(0) == 'y')
							_msg.mtype = eNewUser;
						else
							_msg.mtype = eExistingUser;
					}
					break;
				case eChooseLogin:
					std::cout << _msg.body;
					std::getline(std::cin, input);
					if (input.size() == 1 && input.at(0) == 'x') {
						_msg.mtype = eQuit;
						strcpy(_msg.body, "Good buy\n");
					}else{
						clear_message(_msg);
						_msg.mtype = eLogin;
						strcpy(_msg.body, input.c_str());
						strcpy(_msg.user, "n"); //new user
					}
					break;
				case eLogin:
					std::cout << _msg.body;
					std::getline(std::cin, input);
					if (input.size() == 1 && input.at(0) == 'x') {
						_msg.mtype = eQuit;
						strcpy(_msg.body, "Good buy\n");
					}
					else {  
						clear_message(_msg);
						_msg.mtype = eLogin;
						strcpy(_msg.body, input.c_str());
						strcpy(_msg.user, "e"); //existing user
					}
					break;

				case eChoosePassword:
					std::cout << _msg.body;
					std::getline(std::cin, input);
					if (input.size() == 1 && input.at(0) == 'x') {
						_msg.mtype = eQuit;
						strcpy(_msg.body, "Good buy\n");
					}
					else {
						clear_message(_msg);
						_msg.mtype = ePassword;
					}

					break;
				case ePassword:
					std::cout << _msg.body;
					std::getline(std::cin, input);
					if (input.size() == 1 && input.at(0) == 'x') {
						_msg.mtype = eQuit;
						strcpy(_msg.body, "Good buy\n");
					}
					else {
						clear_message(_msg); 
						_msg.mtype = ePassword;
					}
					break;

				case eWrongLogin:
					std::cout << _msg.body;
					std::getline(std::cin, input);
					if (input.size() == 1 && input.at(0) == 'x') {
						_msg.mtype = eQuit;
						strcpy(_msg.body, "Good buy\n");
					}
					else {						
						if (*_msg.user == 'n')
							_msg.mtype = eChooseLogin;
						else {
							_msg.mtype = eLogin;
							strcpy(_msg.user, "e"); //existing user
						}
						strcpy(_msg.body, input.c_str());						
					}
					break;
				case eWrongPassword:
					std::cout << _msg.body;
					std::getline(std::cin, input);
					if (input.size() == 1 && input.at(0) == 'x') {
						_msg.mtype = eQuit;
						strcpy(_msg.body, "Good buy\n");
					}
					else {
						clear_message(_msg);
						_msg.mtype = ePassword;
					}

					break;
				case eAuthOK:
					std::cout << _msg.body;
					if (_msg.user) {
						try {
							_usr_db_id = std::stol(_msg.user);
						}catch (const std::invalid_argument& ex) {
							std::cout << "Bad user id sent by server - " << _msg.user << ": " << ex.what() << std::endl;
						}
					}
					clear_message(_msg);
					_msg.mtype = eGetUserMsg;
					break;
				case eNoMsg:
					if(_msg.body) std::cout << _msg.body;
					clear_message(_msg);
					_msg.mtype = eGetMainMenu;
					break;
				case eMsgNext:
				{
					if (_msg.user) std::cout << _msg.user;
					std::string message_id;
					Message::show_unpacked(_msg.body, message_id);
					clear_message(_msg);
					_msg.mtype = eGetNextMsg;
					strcpy(_msg.body, message_id.c_str()); //this message must be marked as viewed by server
					break;
				}
				case eMsgMainMenu:
					std::cout << _msg.body << std::endl;
					clear_message(_msg);
					std::getline(std::cin, input);
					if (input.size() == 1 && input.at(0) == 'x') {
						_msg.mtype = eQuit;
						strcpy(_msg.body, "Good buy\n");
					}else {
						if (input.at(0) == 'l') {
							_msg.mtype = eLogOut;
						}else if (input.at(0) == 'u') { //write to user
							clear_message(_msg);
							_msg.mtype = eSendToUser;
							strcpy(_msg.user,std::to_string(_usr_db_id).c_str());
						}else if (input.at(0) == 'a') { //write to all
							std::cout << "Type your message (x - exit):\n";
							std::getline(std::cin, input);
							if (input.size() == 1 && input.at(0) == 'x') {
								_msg.mtype = eQuit;
								strcpy(_msg.body, "Good buy\n");
							}else{
								clear_message(_msg);
								_msg.mtype = eSendToAll;
								strcpy(_msg.body, input.c_str());
								strcpy(_msg.user, std::to_string(_usr_db_id).c_str());
							}
						}else {
							_msg.mtype = eNone;
							strcpy(_msg.body, "Undefined choice\n");
						}
					}
					break;
				case eMsgDelivered:
					std::cout << BOLDCYAN << _msg.body << RESET << std::endl;
					clear_message(_msg);
					_msg.mtype = eGetMainMenu;

					break;
				case eErrMsgNotDelivered:
					if (_msg.body)
						std::cout << BOLDCYAN << _msg.body << RESET << std::endl;
					clear_message(_msg);
					_msg.mtype = eGetMainMenu;
					
					break;
				case eAvailableUsers:
				{
					std::map<size_t, std::string> usrs;
					std::cout << "Available users:\n";
					_unpack_available_users(_msg.body, usrs);
				USER_ID_INPUT:
					std::cout << "Type user id (in brackets) (x - exit):\n";
					std::getline(std::cin, input);
					while (!input.size()) {
						std::cout << BOLDRED << "You`ve typed nothing." << RESET << std::endl;
						std::cout << "Type user id (in brackets) (x - exit):\n";
						std::getline(std::cin, input);
					}

					if (input.at(0) == 'x') {
						_msg.mtype = eQuit;
						strcpy(_msg.body, "Good buy\n");
					}
					else {
						size_t uid{};
						try {
							uid = std::stol(input);
						}
						catch (std::invalid_argument const& ex) {
							std::cout << BOLDRED << "You`ve mistyped user id:" << ex.what() << RESET << std::endl;
							goto USER_ID_INPUT;
						}

						auto it = usrs.find(uid);
						if (it != usrs.end())
							std::cout << "Type your message to user " << it->second << " (x - exit):" << std::endl;
						else {
							std::cout << "You`ve mistyped user id - No such id\n";
							goto USER_ID_INPUT;
						}
						std::getline(std::cin, input);
						while (!input.size()) {
							std::cout << BOLDRED << "You`ve typed nothing." << RESET << std::endl;
							std::cout << "Type your message to user " << it->second << " (x - exit):" << std::endl;
							std::getline(std::cin, input);
						}
						if (input.at(0) == 'x') {
							_msg.mtype = eQuit;
							strcpy(_msg.body, "Good buy\n");
						}
						else {
							clear_message(_msg);
							_msg.mtype = eSendToUserMsgReady;
							strcpy(_msg.body, input.c_str());
							strcpy(_msg.user, std::to_string(uid).c_str());
						}
					}

					break;
				}
				case eQuit:
					if(_msg.body)
						std::cout << _msg.body  <<"Connection closed by server" << std::endl;

					exit_loop = true;
					server_down = true;
					break;

				}

				if (!_send_to_server(_msg))
					break;


			}else if (bytes_in == 0) {
				printf("Connection closing...\n");
				close(_socket);
				return;
			}else {
				printf("recv failed: %s\n", strerror(errno));
				close(_socket);
				return;
            }
            sleep(1);
		}
	}else
		std::cout << "\nClient init failure\n";
	
}

auto TCPClient::_unpack_available_users(const std::string& msg_body, std::map<size_t, std::string>& usrs) -> void {
	std::string packed_usrs = msg_body;
	auto pos_b = packed_usrs.find("|");
	while (pos_b != std::string::npos) {
		auto pos_cr = packed_usrs.find("#");
		if (pos_cr != std::string::npos) {
			size_t uid = std::stol(packed_usrs.substr(0, pos_b));
			usrs.insert(std::make_pair(uid, packed_usrs.substr(pos_b + 1, pos_cr - (pos_b + 1))));
		}
		packed_usrs = packed_usrs.substr(pos_cr + 1);
		pos_b = packed_usrs.find("|");
	}
	for (const auto& it : usrs)
		std::cout << "[" << it.first << "] " << it.second << std::endl;

}

auto TCPClient::send_to_server(IOMSG& msg) -> bool{
   if(_is_valid_socket())
        return _send_to_server(msg);
   return false;
}

auto TCPClient::_is_valid_socket()->bool{
    return _socket!=-1;
}

auto TCPClient::_send_to_server(IOMSG& msg) -> bool {
	size_t bytes_out{};
	bytes_out = write(_socket, reinterpret_cast<void*>(&msg), sizeof(IOMSG));
	if (!bytes_out) {
		close(_socket);
        std::logic_error e("Failed to send message to server\n");
        throw std::exception(e);
	}

	return true;
}
