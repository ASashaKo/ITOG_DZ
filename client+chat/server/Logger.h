#pragma once
#include <thread>
#include <shared_mutex>
#include <fstream>
#include <string>
#include <ctime>

#define MAX_LOG_LINE_LEN 512

class Logger{
public:
    Logger(const char* prefix);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
	auto write(const char* line) -> bool;
	auto read_line(std::string& l,int idx=-1)->bool; //idx:	-1 -> read last line, 0 -> first line

private:
	const char* _log_fn{ "log.txt" };
	const char* _log_pref{ "" };
	auto _get_file_name()-> const std::string;
	std::fstream _file;
	std::shared_mutex _mtxs;
	bool _init_ok{ false };

};

using LOG = Logger;
