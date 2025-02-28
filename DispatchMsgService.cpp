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
	thread_mutex_create(&queue_mutex_);//��ʼ�����еĻ�����  
	tp_ = thread_pool_init( );  //��ʼ���̳߳�
	return tp_ ? TRUE : FALSE;
}

void DispatchMsgService::close() {
	svr_exit_ = TRUE;

	thread_pool_destroy(tp_);
	thread_mutex_destroy(&queue_mutex_);
	subscribers_.clear();

	tp_ = NULL;
}

void DispatchMsgService::subscribe(u32 eid, iEventHandler* handler) // �����¼�id ,��һ���¼�������
{
	LOG_DEBUG("DispatchMsgService::subscribe eid :%u\n", eid);
	T_EventHandlersMap::iterator it = subscribers_.find(eid);

	if (it != subscribers_.end()) {//˵��������key Ϊ eid �ļ�ֵ��
		//��һ���ж� eid ��Ӧ��ֵ�Ƿ�Ϊ���Ǵ���� handler
		T_EventHandlers::iterator its = std::find(it->second.begin() , it->second.end() ,handler);
		if (its == it->second.end()) {  //eid ��Ӧ��ֵ�����Ǵ���� handler�����
			it->second.emplace_back(handler);
		}
	}
	else { //û�� eid �ļ�ֵ�� 
		subscribers_[eid].emplace_back( handler); // ����һ�������¼�
	}
}

// �������
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

// ����ģʽ
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

void DispatchMsgService::svc( void* argv ) {  // void* argv -->����ᴫ��һ���¼�
	DispatchMsgService* dms = DispatchMsgService::getInstance();
	iEvent* ev = (iEvent *)argv ;
	if (!dms->svr_exit_) {   // �ַ��¼��ķ����Ƿ�ֹͣ
		LOG_DEBUG("DispatchMsgService::svc...\n");
		iEvent* rsp = dms->process( ev );   // ���¼����зַ�
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

//���¼�Ͷ�ݵ��̳߳�
i32 DispatchMsgService::enqueue( iEvent* ev ) {
	if (NULL == ev) {
		return -1;
	}
	thread_task_t *task = thread_task_alloc( 0 );

	task->handler = DispatchMsgService::svc;    
	task->ctx = ev ;   // �¼���Ϊ��������

	return thread_task_post( tp_, task );
}

//std::vector<iEvent *> DispatchMsgService::process(const iEvent* ev) {

iEvent *DispatchMsgService::process( const iEvent* ev ) {
	LOG_DEBUG("DispatchMsgService::process ->%p\n", &ev);
	if ( ev == NULL ) {
		//return  process_events_;
		return NULL;
	}
	u32 eid = ev->get_eid();    // ��ȡ�¼���id

	if (eid == EEVENTID_UNKOWN ) {   // δ֪�¼�
		LOG_WARN("DispatchMsgService :unknow evend id %d\n", eid);
		//return process_events_;
		return NULL;
	}
	LOG_DEBUG("DispatchMsgService::process event id %u\n", eid );

	T_EventHandlersMap::iterator handlers = subscribers_.find( eid );
	if ( handlers == subscribers_.end() ) {   // ���¼�û�б�����
		LOG_WARN("DispatchMsgService::process : no way any event handler subscribed %u\n", eid);
		//return process_events_;
		return NULL;
	}

	iEvent* rsp = NULL;
	// �ַ�����ͬ���¼�����������
	for (auto iter = handlers->second.begin(); iter != handlers->second.end(); iter++) {
		iEventHandler* handler = *iter;
		LOG_DEBUG("DispatchMsgService::process get handler %s\n", handler->get_name().c_str());
		rsp = handler->handler( ev ) ;
		//process_events_.emplace_back(rsp);
	}
	//return process_events_;
	return rsp;
}

// �������� ������ ���Լ��¼���id 
iEvent* DispatchMsgService::parseEvent( const char* message , u32 len, u32 eid ) {
	if ( !message ) {
		LOG_DEBUG("DispatchMsgService::parseEvent - message is NULL, eid : % d\n",eid );
		return nullptr;
	}
	if ( eid == EEVENTID_GET_MOBLIE_CODE_REQ ) {  // �ֻ���֤������
		tutorial::mobile_request mr;
		if ( mr.ParseFromArray( message, len )) {
			// ����һ���ֻ���֤�������¼�
			MobileCodeReqEv* ev = new MobileCodeReqEv( mr.mobile() );
			return ev;
		}
	}
	// ��¼�¼�
	else if (eid == EEVENTID_LOGIN_REQ) {
		tutorial::login_request lr;
		if (lr.ParseFromArray(message, len)) {
			// ����һ����¼�����¼�
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
		thread_mutex_lock(&queue_mutex_);  //��������н��м���
		if ( !response_events_.empty()) {
			ev = response_events_.front();
			response_events_.pop();
		}
		else {
			done = true;
		}
		thread_mutex_unlock( &queue_mutex_ );
		if (!done) {
			//1. �ж��ó�������ʲô�¼�
			if ( ev->get_eid() == EEVENTID_GET_MOBLIE_CODE_RSP ) {
				MobileCodeRspEv* mcre = static_cast<MobileCodeRspEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEven id: %d \n",EEVENTID_GET_MOBLIE_CODE_RSP );
				
				ConnectSession* cs = static_cast<ConnectSession*>( ev->get_args() );
				cs->response = ev;
				
				//���л���������
				cs->message_len = mcre->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HANDLER_LEN];

				//��װͷ��
				memmove(cs->write_buf, MESSAGE_HANDLER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EEVENTID_GET_MOBLIE_CODE_RSP;
				*(i32*)(cs->write_buf + 6) = cs->message_len;

				mcre->SerializeToArray(cs->write_buf + MESSAGE_HANDLER_LEN , cs->message_len );
				networkinterface->send_response_message(cs); 
			}
			else if ( ev->get_eid() == EEVENTID_LOGIN_RSP ) {  // ��¼��Ӧ
				//thread_mutex_lock(&queue_mutex_);  //��������н��м���
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
