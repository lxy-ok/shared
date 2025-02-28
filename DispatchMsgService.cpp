#include "DispatchMsgService.h"
#include "NetworkInterface.h"
#include "eventtype.h"
#include "events_def.h"
#include "bike.pb.h"

#include <algorithm>

DispatchMsgService* DispatchMsgService::DMS_ = NULL;
std::once_flag  DispatchMsgService::flag_;
NetworkInterface* DispatchMsgService::NTIF_ = nullptr;

std::queue<iEvent*>DispatchMsgService::response_events_;
pthread_mutex_t DispatchMsgService::queue_mutex_;
std::vector<iEvent*>DispatchMsgService::process_events_;

DispatchMsgService::DispatchMsgService() {
	tp_ = NULL;
}

DispatchMsgService::~DispatchMsgService() { }

BOOL DispatchMsgService::open() {
	svr_exit_ = false;
	thread_mutex_create(&queue_mutex_);//初始化队列的互斥锁  
	tp_ = thread_pool_init( );  //初始化线程池
	return tp_ ? TRUE : FALSE;
}

void DispatchMsgService::close() {
	svr_exit_ = TRUE;

	thread_pool_destroy(tp_);
	thread_mutex_destroy(&queue_mutex_);
	subscribers_.clear();

	tp_ = NULL;
}

void DispatchMsgService::subscribe(u32 eid, iEventHandler* handler) // 传递事件id ,和一个事件处理器
{
	LOG_DEBUG("DispatchMsgService::subscribe eid :%u\n", eid);
	T_EventHandlersMap::iterator it = subscribers_.find(eid);

	if (it != subscribers_.end()) {//说明存在有key 为 eid 的键值对
		//进一步判断 eid 对应的值是否为我们传入的 handler
		T_EventHandlers::iterator its = std::find(it->second.begin() , it->second.end() ,handler);
		if (its == it->second.end()) {  //eid 对应的值与我们传入的 handler不相等
			it->second.emplace_back(handler);
		}
	}
	else { //没有 eid 的键值对 
		subscribers_[eid].emplace_back( handler); // 新增一个订阅事件
	}
}

// 解除订阅
void DispatchMsgService::unsubscribe(u32 eid, iEventHandler* handler) {
	LOG_DEBUG("DispatchMsgService::unsubscribe eid :%u", eid);
	T_EventHandlersMap::iterator it = subscribers_.find(eid);

	if (it != subscribers_.end()) {
		T_EventHandlers::iterator its = std::find(it->second.begin() ,it->second.end() , handler);
		if (its != it->second.end()) {
			it->second.erase(its);
		}
	}
}

// 懒汉模式
DispatchMsgService* DispatchMsgService::getInstance() {
	/*
	if (DMS_ == NULL) {
		DMS_ = new DispatchMsgService();
	}
	return DMS_;
	*/
	
	if (DMS_ == nullptr ) {
		std::call_once(flag_, []() {
			DMS_ = new DispatchMsgService();
			});
	}
	return DMS_;
	
}

void DispatchMsgService::svc( void* argv ) {  // void* argv -->这里会传递一个事件
	DispatchMsgService* dms = DispatchMsgService::getInstance();
	iEvent* ev = (iEvent *)argv ;
	if (!dms->svr_exit_) {   // 分发事件的服务是否停止
		LOG_DEBUG("DispatchMsgService::svc...\n");
		iEvent* rsp = dms->process( ev );   // 对事件进行分发
		if ( rsp ) {
			rsp->dump(std::cout);
			rsp->set_args( ev->get_args( ) );
		}
		else {
			std::cout << "DispatchMsgService::svc-->process  return nullptr!" << std::endl;
			rsp = new ExitRspEv();
			rsp->set_args( ev->get_args( ) );
		}
		thread_mutex_lock(&queue_mutex_);
		response_events_.push( rsp );
		thread_mutex_unlock(&queue_mutex_);
		std::cout << "DispatchMsgService::svc is end!" << std::endl;
	}
}

//将事件投递到线程池
i32 DispatchMsgService::enqueue( iEvent* ev ) {
	if (NULL == ev) {
		return -1;
	}
	thread_task_t *task = thread_task_alloc( 0 );

	task->handler = DispatchMsgService::svc;    
	task->ctx = ev ;   // 事件作为参数传递

	return thread_task_post( tp_, task );
}

//std::vector<iEvent *> DispatchMsgService::process(const iEvent* ev) {

iEvent *DispatchMsgService::process( const iEvent* ev ) {
	LOG_DEBUG("DispatchMsgService::process ->%p\n", &ev);
	if ( ev == NULL ) {
		//return  process_events_;
		return NULL;
	}
	u32 eid = ev->get_eid();    // 获取事件的id

	if (eid == EEVENTID_UNKOWN ) {   // 未知事件
		LOG_WARN("DispatchMsgService :unknow evend id %d\n", eid);
		//return process_events_;
		return NULL;
	}
	LOG_DEBUG("DispatchMsgService::process event id %u\n", eid );

	T_EventHandlersMap::iterator handlers = subscribers_.find( eid );
	if ( handlers == subscribers_.end() ) {   // 该事件没有被订阅
		LOG_WARN("DispatchMsgService::process : no way any event handler subscribed %u\n", eid);
		//return process_events_;
		return NULL;
	}

	iEvent* rsp = NULL;
	// 分发给不同的事件处理器处理
	for (auto iter = handlers->second.begin(); iter != handlers->second.end(); iter++) {
		iEventHandler* handler = *iter;
		LOG_DEBUG("DispatchMsgService::process get handler %s\n", handler->get_name().c_str());
		rsp = handler->handler( ev ) ;
		//process_events_.emplace_back(rsp);
	}
	//return process_events_;
	return rsp;
}

// 数据内容 ，长度 ，以及事件的id 
iEvent* DispatchMsgService::parseEvent( const char* message , u32 len, u32 eid ) {
	if ( !message ) {
		LOG_DEBUG("DispatchMsgService::parseEvent - message is NULL, eid : % d\n",eid );
		return nullptr;
	}
	if ( eid == EEVENTID_GET_MOBLIE_CODE_REQ ) {  // 手机验证码请求
		tutorial::mobile_request mr;
		if ( mr.ParseFromArray( message, len )) {
			// 生成一个手机验证码请求事件
			MobileCodeReqEv* ev = new MobileCodeReqEv( mr.mobile() );
			return ev;
		}
	}
	// 登录事件
	else if (eid == EEVENTID_LOGIN_REQ) {
		tutorial::login_request lr;
		if (lr.ParseFromArray(message, len)) {
			// 生成一个登录请求事件
			LoginReqEv* ev = new LoginReqEv(lr.mobile(), lr.icode());
			return ev;
		}
	}

	return nullptr;
}

void DispatchMsgService::handleAllResponseEvent( NetworkInterface* networkinterface ) {
	std::cout << "Into DispatchMsgService::handleAllResponseEvent.\n";
	bool done = false;
	while (!done) {
		iEvent* ev = nullptr;
		thread_mutex_lock(&queue_mutex_);  //对任务队列进行加锁
		if ( !response_events_.empty()) {
			ev = response_events_.front();
			response_events_.pop();
		}
		else {
			done = true;
		}
		thread_mutex_unlock( &queue_mutex_ );
		if (!done) {
			//1. 判断拿出来的是什么事件
			if ( ev->get_eid() == EEVENTID_GET_MOBLIE_CODE_RSP ) {
				MobileCodeRspEv* mcre = static_cast<MobileCodeRspEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEven id: %d \n",EEVENTID_GET_MOBLIE_CODE_RSP );
				
				ConnectSession* cs = static_cast<ConnectSession*>( ev->get_args() );
				cs->response = ev;
				
				//序列化请求数据
				cs->message_len = mcre->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HANDLER_LEN];

				//组装头部
				memmove(cs->write_buf, MESSAGE_HANDLER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EEVENTID_GET_MOBLIE_CODE_RSP;
				*(i32*)(cs->write_buf + 6) = cs->message_len;

				mcre->SerializeToArray(cs->write_buf + MESSAGE_HANDLER_LEN , cs->message_len );
				networkinterface->send_response_message(cs); 
			}
			else if ( ev->get_eid() == EEVENTID_LOGIN_RSP ) {  // 登录响应
				//thread_mutex_lock(&queue_mutex_);  //对任务队列进行加锁
				//if ( response_events_.empty() )   std::cout << "empty" << std::endl;
				//thread_mutex_unlock(&queue_mutex_);

				LoginRspEv* lrs = static_cast<LoginRspEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEven id: %d \n", EEVENTID_LOGIN_RSP );

				ConnectSession* cs = static_cast<ConnectSession*>( ev->get_args() );

				cs->response = ev;
				cs->message_len = lrs->ByteSize();
				std::cout << "LoginRspEv msg size: " << cs->message_len << std::endl;
				cs->write_buf = new char[ cs->message_len + MESSAGE_HANDLER_LEN + 1];
		
				memmove( cs->write_buf, MESSAGE_HANDLER_ID , 4 );
				*(u16*)(cs->write_buf + 4 ) = EEVENTID_LOGIN_RSP ;
				*(i32*)(cs->write_buf + 6 ) = cs->message_len;
				int ret = lrs->SerializeToArray( cs->write_buf + MESSAGE_HANDLER_LEN , cs->message_len );

				networkinterface->send_response_message( cs );
			}
			else if (ev->get_eid() == EEVENTID_EXIT_RSP) {
				ConnectSession* cs = static_cast<ConnectSession*>(ev->get_args());
				cs->response = ev;
				networkinterface->send_response_message(cs);
			}
		}
	}
}
