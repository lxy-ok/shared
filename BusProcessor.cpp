#include "BusProcessor.h"
#include "SqlTables.h"

// ����ṹ��һ���û��¼�������
BusinessProcessor::BusinessProcessor(std::shared_ptr<MysqlConnect>mysqlconn) :mysqlconn_( mysqlconn ) , ueh_( new UserEventHandler()){
	
}

bool BusinessProcessor::init() {
	// ����һ����Ķ���
	SqlTables tables( mysqlconn_ );
	tables.CreateUserInfo();   // �����û���
	tables.CreateBikeInfo();   // ����������

	return true;
}

BusinessProcessor::~BusinessProcessor() {
	ueh_.reset();   // reset �������� ,�ͷŵ�ǰ����ָ�뵱ǰָ��Ķ��� , ��������ָ���ÿ�
	mysqlconn_.reset();
}