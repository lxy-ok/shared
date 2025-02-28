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

//�Ự״̬
enum class SESSION_SATTUS
{
	SS_REQUEST,  
	SS_RESPONSE
};

//���ݴ���״̬
enum class MESSAGE_STATUS
{
	MS_READ_HANDER =0,
	MS_READ_MESSAGE = 1,  // ��Ϣ����δ��ʼ
	MS_READ_DONE = 2,     // ��Ϣ�������
	MS_SENDING = 3        // ��Ϣ������
};

typedef struct _ConnectSession {
	char remote_ip[32];  // Զ�̵�ip��ַ(�ͻ��˵�ip)

	SESSION_SATTUS session_state;

	iEvent* request;
	MESSAGE_STATUS req_state ;

	iEvent* response ;
	MESSAGE_STATUS res_state ;

	u16 eid;    //���浱ǰ������¼�id 
	i32 fd;     //���浱ǰ������ļ����

	struct bufferevent* bev;   // ���ڹ����д�¼�

	char* read_buf;   //�������Ϣ�Ļ�����
	u32 message_len;  //��ǰ��Ҫ��д��Ϣ�ĳ���
	u32 read_message_len; //�Ѿ���ȡ����Ϣ����

	char header[MESSAGE_HANDLER_LEN + 1]; //����ͷ��10�ֽ� + 1 �ֽ�
	u32 read_header_len;                  //�Ѷ�ȡ��ͷ������

	char* write_buf;
	u32 sent_len;     //�ѷ��͵ĳ���
}ConnectSession;

class NetworkInterface
{
public:
	NetworkInterface();
	~NetworkInterface();

	bool start(int port);   // �������� ������event base ���� �ͼ�����
	void close();           // �رշ���

	//������    ��evutil_socket_t --->  ���ڿ�ƽ̨��ʾ socket �ľ�� ��
	static void listener_cb(struct evconnlistener*listener ,evutil_socket_t fd,
								struct sockaddr *sock ,int socklen ,void *arg);

	//���������¼�(����ص�)
	static void handle_request(struct bufferevent* bev, void* arg );
	//������Ӧ�¼�
	static void handle_response(struct bufferevent* bev, void* arg ); 
	//��������¼�
	static void handle_error(struct bufferevent* bev, short events, void* arg);

	void network_event_dispatch( ); // �����¼�

	void send_response_message(ConnectSession* cs); // ������Ӧ�¼�

private:
	struct event_base* base_;          // �¼�ѭ���ĺ��� , ���ڹ��������¼�
	struct evconnlistener* listener_;  // libevent ������
};


#endif // !_NETWORK_INTERFACE_H_



