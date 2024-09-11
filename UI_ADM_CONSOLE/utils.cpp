#include "utils.h"
#include <unistd.h>

auto getPassword(std::string& pwd) -> void {
    pwd.erase();
	pwd = std::string(getpass(pwd.c_str()));
	std::cout << RESET<<std::endl;
}

auto print_os_data() -> void {
    struct utsname utsname;
    uname(&utsname);
    printf("OS name: %s (%s:%s)\n", utsname.sysname, utsname.release, utsname.version);
}

auto get_login_by_id_from_packed_string(const std::string& sid) -> const std::string {
    return "";
}

bool split_msg_data(const std::string & r, std::vector<std::string>& v, const char* div){
    std::string tmp{r};
    size_t pos = tmp.find(div);
    v.clear();
    while(pos!=std::string::npos){
        std::string line = tmp.substr(0,pos).c_str();
        v.emplace_back(line);
        tmp = tmp.substr(pos+1);
        pos = tmp.find(div);
    }
    if(tmp.size()){
       pos = tmp.find('\n');
       if(pos)
            tmp =tmp.substr(0,pos);
       v.emplace_back(tmp);
    }
    return v.size()>0;
}
