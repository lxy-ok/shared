#include "userService.h"


UserService::~UserService() {
	sql_conn_.reset();
}

bool UserService::exit(const std::string& mobile) {
	char sql_context[1024] = { 0 };
	sprintf(sql_context, \
		"select * from userinfo where mobile = %s", \
		mobile.c_str());
	SqlRecordSet  sqlRecordSet;
	if (!sql_conn_->Execute(sql_context, sqlRecordSet)) {
		return false;
	}
	return sqlRecordSet.GetRowCount();
}

bool UserService::insert(const std::string& mobile) {
	char sql_context[1024] = { 0 };
	sprintf(sql_context, \
		   "insert into userinfo(mobile) values (%s)",  \
		    mobile.c_str());
	return sql_conn_->Execute(sql_context);
}
