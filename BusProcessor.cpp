#include "BusProcessor.h"
#include "SqlTables.h"

// 这里会构造一个用户事件处理器
BusinessProcessor::BusinessProcessor(std::shared_ptr<MysqlConnect>mysqlconn) :mysqlconn_( mysqlconn ) , ueh_( new UserEventHandler()){
	
}

bool BusinessProcessor::init() {
	// 创建一个表的对象
	SqlTables tables( mysqlconn_ );
	tables.CreateUserInfo();   // 创建用户表
	tables.CreateBikeInfo();   // 创建单车表

	return true;
}

BusinessProcessor::~BusinessProcessor() {
	ueh_.reset();   // reset 不带参数 ,释放当前智能指针当前指向的对象 , 并将智能指针置空
	mysqlconn_.reset();
}