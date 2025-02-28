#ifndef _NETWORK_INTERFACE_H_
#define _NETWORK_INTERFACE_H_

#include <event.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <string>

#include "ievent.h"

#include "glo_def.h"
#include <sys/socket.h>
#include<netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>


#define	MAX_MESSAGE_LEN 367280   //
#define MESSAGE_HANDLER_LEN 10
#define MESSAGE_HANDLER_ID "FBEB"

//会话状态
enum class SESSION_SATTUS
{
	SS_REQUEST,  
	SS_RESPONSE
};

//数据传输状态
enum class MESSAGE_STATUS
{
	MS_READ_HANDER =0,
	MS_READ_MESSAGE = 1,  // 消息传输未开始
	MS_READ_DONE = 2,     // 消息传输完毕
	MS_SENDING = 3        // 消息传输中
};

typedef struct _ConnectSession {
	char remote_ip[32];  // 远程的ip地址(客户端的ip)

	SESSION_SATTUS session_state;

	iEvent* request;
	MESSAGE_STATUS req_state ;

	iEvent* response ;
	MESSAGE_STATUS res_state ;

	u16 eid;    //保存当前请求的事件id 
	i32 fd;     //保存当前传输的文件句柄

	struct bufferevent* bev;   // 用于管理读写事件

	char* read_buf;   //保存读消息的缓冲区
	u32 message_len;  //当前需要读写消息的长度
	u32 read_message_len; //已经读取的消息长度

	char header[MESSAGE_HANDLER_LEN + 1]; //保存头部10字节 + 1 字节
	u32 read_header_len;                  //已读取的头部长度

	char* write_buf;
	u32 sent_len;     //已发送的长度
}ConnectSession;

class NetworkInterface
{
public:
	NetworkInterface();
	~NetworkInterface();

	bool start(int port);   // 启动服务 ，创建event base 核心 和监听器
	void close();           // 关闭服务

	//监听器    （evutil_socket_t --->  用于跨平台表示 socket 的句柄 ）
	static void listener_cb(struct evconnlistener*listener ,evutil_socket_t fd,
								struct sockaddr *sock ,int socklen ,void *arg);

	//处理请求事件(请求回调)
	static void handle_request(struct bufferevent* bev, void* arg );
	//处理响应事件
	static void handle_response(struct bufferevent* bev, void* arg ); 
	//处理出错事件
	static void handle_error(struct bufferevent* bev, short events, void* arg);

	void network_event_dispatch( ); // 处理事件

	void send_response_message(ConnectSession* cs); // 发送响应事件

private:
	struct event_base* base_;          // 事件循环的核心 , 用于管理所有事件
	struct evconnlistener* listener_;  // libevent 监听器
};


#endif // !_NETWORK_INTERFACE_H_



