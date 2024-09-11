#pragma once
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#define PORT 55169
#include <string>
#include <iostream>
#include <thread>

#pragma warning(disable : 5208)
#pragma warning(disable : 4996)

#define MSG_LENGTH 1024
#define USR_NAME_LEN 32
#define USR_ID_LEN 5

enum MessageTypes {
	//To send by client
    eAuthAdmin = 34,   //cl
	eAuth,				//cl
    eBatchAuth,         //cl
    eBatchReg,         //cl
	eNewUser,			//cl
	eGetAvailableUsers,	//cl
	eGetUsersAdmin	,	//cl
    eSetUserBanFlag,    //cl
	eGetMsgAdmin,		//cl
	eGetUserMsg,        //cl
	eGetNextMsg,        //cl
	eGetMainMenu,       //cl
	eSendToAll,			//cl
	eSendToUser,		//cl
	eSendToUserMsgReady,//cl
	eLogOut,			//cl
	//To send by server
	eWelcome,			//srv
	eChooseLogin,       //srv
	eChoosePassword,	//srv
	eWrongLogin,		//srv
	eWrongPassword,		//srv
	eLoginUsed,			//srv
	eLoginOK,			//srv
	eAuthOK,			//srv
	eAvailableUsers,	//srv
	eMsgDelivered,		//srv
	eErrMsgNotDelivered,//srv
	eErrMsgBadFormat,	//srv 
	eNoMsg,				//srv
	eMsgNext,			//srv
	eMsgMainMenu,		//srv
    eUserBanFlagSet,    //srv
	//To send by both
    eExistingUser,		//cl-srv
    eError,             //cl-srv
    eUserNextAdmin,		//cl-srv
	eNone,				//cl-srv
	eLogin,				//cl-srv
	ePassword,			//cl-srv
	eQuit				//cl-srv
};
using MT = MessageTypes;

typedef struct _msg{
	MT mtype{ eAuth };
	char body[MSG_LENGTH]{'\0'};
	char user[USR_NAME_LEN]{'\0'};
    char user_id[USR_ID_LEN]{'\0'};
}IOMSG;

static void clear_message(IOMSG& msg) {
	msg.mtype = eNone;
	memset(msg.body, '\0', MSG_LENGTH); 
	memset(msg.user, '\0', USR_NAME_LEN);
    memset(msg.user_id, '\0', USR_ID_LEN);
}

static void print_message(IOMSG& msg) {
    std::cout << "mtype: "<<msg.mtype << " body: " << msg.body << " user: " << msg.user<<" user id:"<<msg.user_id<< std::endl;
}

static auto str_wsa_error(size_t err)->const std::string {
	return "";
}

