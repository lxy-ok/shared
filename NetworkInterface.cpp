#include "NetworkInterface.h"
#include "DispatchMsgService.h"

#include <string.h>
#include <errno.h>

NetworkInterface::NetworkInterface() {
	std::cout << "Create a NetworkInterface server!" << std::endl;
	base_ = nullptr;
	listener_ = nullptr;
}

NetworkInterface::~NetworkInterface() {
	this->close();
}

bool NetworkInterface::start( int port ) {
	if (Debugs) {
		LOG_DEBUG("coming NetworkInterface::start\n");
	}
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY ;
	sin.sin_port = htons( port );

	base_ = event_base_new();   // 创建一个event核心
	if ( !base_ ) {
		LOG_DEBUG("NetworkInterface::start->event_base_new failed\n ");
		return false;
	}

	// 创建事件监听器
	// 第二个base_ 传递给回调函数的用户定义的数据
	listener_ = evconnlistener_new_bind( base_, NetworkInterface::listener_cb, base_ ,
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
		512, (struct sockaddr*)&sin ,
		sizeof(struct sockaddr_in) ) ;
	if (!listener_) {
		LOG_DEBUG("NetworkInterface::start->evconnlistener_new_bind failed\n");
		//delete base_;
		event_base_free(base_);
		return false;
	}

	LOG_DEBUG("NetworkInterface::start is satrting success!\n ");
	return true;
}

void NetworkInterface::close() {
	if (base_) {
		event_base_free(base_);
		base_ = nullptr;
	}
	if (listener_) {
		evconnlistener_free(listener_);
		listener_ = nullptr;
	}
}

// 初始化连接
static ConnectSession* session_init( int fd, struct bufferevent* bev ) {
	if (Debugs) {
		LOG_DEBUG("coming NetworkInterface::session_init\n");
	}
	ConnectSession* temp = nullptr;
	temp = new ConnectSession;
	if (!temp) {
		fprintf(stderr, "malloc failed .reason:%s", strerror(errno));
		return nullptr;
	}
	memset(temp,'\0', sizeof(ConnectSession));
	temp->bev = bev;
	temp->fd = fd;
	std::cout << "ConnectSession* session_init is end!" << std::endl;
	return temp;
}

static void session_free( ConnectSession* cs) {
	if (cs) {
		if ( cs->read_buf ) {
			delete[] cs->read_buf;
			cs->read_buf = nullptr;
		}
		if ( cs->write_buf ) {
			delete[] cs->write_buf;
			cs->write_buf = nullptr;
		}
		delete cs;
	}
}

static void session_reset(ConnectSession* cs){
	if (cs) {
		if (cs->read_buf) {
			delete[] cs->read_buf;
			cs->read_buf = nullptr;
		}
		if (cs->write_buf) {
			delete[] cs->write_buf;
			cs->write_buf = nullptr;
		}

		//修改状态
		cs->req_state = MESSAGE_STATUS::MS_READ_HANDER;  //读取头部
		cs->session_state = SESSION_SATTUS::SS_REQUEST;  //请求状态

		cs->message_len = 0;
		cs->read_message_len = 0;
		cs->sent_len = 0;
		cs->read_header_len = 0;
	}
}

// 回调函数的几个参数是由libevent内部传递过去的（struct evconnlistener* listener, evutil_socket_t fd,struct sockaddr* sock, int socklen）
// void* arg--->传递过来的是 libevent_base  
void NetworkInterface::listener_cb( struct evconnlistener* listener, evutil_socket_t fd ,
	struct sockaddr* sock, int socklen, void* arg ) {

	if (Debugs) {
		LOG_DEBUG("coming NetworkInterface::listener_cb\n");   
	}
	struct event_base* base = (struct event_base* )arg ;

	// 为这个客户端分配一个bufferevent( 用于管理读写事件 )
	// BEV_OPT_CLOSE_ON_FREE-->释放 bufferevent 时，自动关闭其底层的文件描述符 
	struct bufferevent* bev = bufferevent_socket_new( base, fd , BEV_OPT_CLOSE_ON_FREE  );
	if ( !bev ) {
		std::cout << "NetworkInterface::listener_cb->bufferevent_socket_new is failed" << std::endl;
		return;
	}

	ConnectSession* cs = session_init( fd , bev );  
	if (!cs) {
		std::cout <<"NetworkInterface::listener_cb->session_init is  failed\n";
	}

	//LOG_DEBUG("read from clien :%s\n", cs->bev->ev_read);

	// 设置会话状态 以及 请求状态
	cs->session_state = SESSION_SATTUS::SS_REQUEST ;   // 消息传输未开始
	cs->req_state = MESSAGE_STATUS::MS_READ_HANDER ;   // 读取头部

	strcpy( cs->remote_ip , inet_ntoa( ((struct sockaddr_in*)sock)->sin_addr ) );

	LOG_DEBUG("NetworkInterface::listener_cb .accept a new connection fd: %d. remote_ip : %s\n", cs->fd , cs->remote_ip );

	// 设置当前事件的回调函数   cs作为参数传递
	bufferevent_setcb( bev , handle_request, handle_response, handle_error,  cs );
	// EV_READ-->可读    , EV_PERSIST 持续( 可以持续的读取 )
	bufferevent_enable(bev, EV_READ | EV_PERSIST);
	bufferevent_settimeout( bev, 10, 10 );    // 超时会回调-->handle_error
	std::cout << "NetworkInterface::listener_cb is end!\n";
}

// 处理请求事件(请求回调)
void NetworkInterface::handle_request(struct bufferevent* bev, void* arg) {
	if (Debugs) {
		LOG_DEBUG("coming NetworkInterface::handle_request\n");
	}

	ConnectSession* cs = (ConnectSession*)arg;

	// 判断会话状态
	if ( cs->session_state != SESSION_SATTUS::SS_REQUEST ) {
		LOG_WARN(" NetworkInterface::handle_request. wrong session state[%d]\n", cs->session_state );
		return;
	}

	if ( cs->req_state == MESSAGE_STATUS::MS_READ_HANDER ) {  // 正在读取消息头部
		i32 len = bufferevent_read( bev, cs->header + cs->read_header_len , MESSAGE_HANDLER_LEN - cs->read_header_len );
		cs->read_header_len += len;

		cs->header[cs->read_header_len] = '\0';

		LOG_DEBUG("NetworkInterface::handle_request read of headler[%s]\n",  cs->header );

		if ( cs->read_header_len == MESSAGE_HANDLER_LEN ) { // 头部数据读取完毕
			// 校验 头部 "FBEB"
			if ( strncmp(cs->header, MESSAGE_HANDLER_ID, strlen(MESSAGE_HANDLER_ID)) == 0 ) {
				// 解析事件 id( 2个字节)
				cs->eid = *((u16*)(cs->header + strlen(MESSAGE_HANDLER_ID)));
				// 拿到数据长度
				cs->message_len = *((i32*)(cs->header + strlen(MESSAGE_HANDLER_ID) + sizeof(u16)));
				LOG_DEBUG("NetworkInterface::handle_request -> read eid :%d . message_len :%d\n",
					cs->eid, cs->message_len);
				// 合法性检查
				if ( cs->message_len < 1 || cs->message_len > MAX_MESSAGE_LEN ) {
					LOG_WARN("NetworkInterface::handle_request. wrong msg_len:%u\n", cs->message_len);
					bufferevent_free(bev);
					session_free(cs);
					return ;
				}
				cs->read_buf = new char[ cs->message_len + 1 ];      
				cs->req_state = MESSAGE_STATUS::MS_READ_MESSAGE;
				cs->read_message_len = 0;
			}
			else { // 校验 头部 "FBEB" 失败
				LOG_ERROR("NetworkInterface::handle_request .NOT FBEB --> invalid request from %s\n",cs->remote_ip );
				bufferevent_free(bev);
				session_free(cs);
				return;
			}
		}
	}
	// evbuffer_get_length( bufferevent_get_input(bev) ->获取当前bev的输入缓冲区中待处理的字节数。
	if ( cs->req_state == MESSAGE_STATUS::MS_READ_MESSAGE && evbuffer_get_length( bufferevent_get_input(bev) ) > 0 ) {
		// 开始读数据
		//while ( cs->read_message_len < cs->message_len ) {
		i32 len = bufferevent_read(bev, cs->read_buf + cs->read_message_len, cs->message_len - cs->read_message_len);
		cs->read_message_len += len;
		//}

		LOG_DEBUG("NetworkInterface::handle_request - . totail len:%d  .is readly len :%d . read msg:%s \n",
			 cs->message_len ,cs->read_message_len , cs->read_buf ) ;
		// 数据读取完毕
		if ( cs->read_message_len == cs->message_len ) {  
			cs->session_state = SESSION_SATTUS::SS_RESPONSE ;
			cs->req_state = MESSAGE_STATUS::MS_READ_DONE ;

			// 解析成事件
			iEvent* ev = DispatchMsgService::getInstance()->parseEvent( cs->read_buf ,cs->message_len ,cs->eid);
			delete[] cs->read_buf ;
			cs->read_buf = nullptr ;
			cs->read_message_len = 0;

			if ( ev ) { // 投入消息队列，让其处理
				ev->set_args( cs );
				std::cout << "NetworkInterface::handle_request-->push event to enqueue!" << std::endl;
				DispatchMsgService::getInstance()->enqueue( ev );
			}
			else {
				LOG_ERROR("NetworkInterface::handle_request -ev is null .ip = %s ,eid :%d\n", cs->remote_ip, cs->eid);
				bufferevent_free(bev);
				session_free( cs );
				return;
			}
		}
	}
}

//处理响应事件
void NetworkInterface::handle_response(struct bufferevent* bev, void* arg) {
	LOG_DEBUG("NetworkInterface::handle_response\n");
}

//处理出错事件(超时、连接关闭、读写出错等异常情况指定回调函数)
void NetworkInterface::handle_error(struct bufferevent* bev, short events, void* arg) {
	ConnectSession* cs = static_cast<ConnectSession*>(arg);

	LOG_DEBUG("NetworkInterface::handle_error......\n");
	//连接关闭
	if ( events & BEV_EVENT_EOF ) {
		LOG_DEBUG("NetworkInterface::handle_error. Connection is close.....\n");
	}
	//连接超时
	else if ((events & BEV_EVENT_TIMEOUT ) && ( events & BEV_EVENT_READING ) ) {
		LOG_WARN("NetworkInterface::handle_error. Connection is read timeout......\n");
	}
	else if ((events & BEV_EVENT_TIMEOUT) && (events & BEV_EVENT_WRITING)) {
		LOG_WARN("NetworkInterface::handle_error. Connection is write timeout......\n");
	}
	else if (events & BEV_EVENT_ERROR) {
		LOG_ERROR("NetworkInterface::handle_error. Connetion is error some other....\n");
	}
	//关闭bufferevent 缓冲区 
	bufferevent_free(bev);
	session_free(cs);  //关闭会话

}

void NetworkInterface::network_event_dispatch( ) {
	// libevent 事件循环
	event_base_loop( base_,  EVLOOP_NONBLOCK );

	//处理响应事件
	DispatchMsgService::getInstance()->handleAllResponseEvent(this);
}

void NetworkInterface::send_response_message(ConnectSession* cs) {
	if ( cs->response == nullptr ) {   //响应事件为空
		bufferevent_free(cs->bev);
		if ( cs->request )  delete cs->request;
		session_free(cs);
	}
	else {   
		bufferevent_write(cs->bev, cs->write_buf, cs->message_len + MESSAGE_HANDLER_LEN );
		//重置会话状态
		session_reset(cs);
	}
}
