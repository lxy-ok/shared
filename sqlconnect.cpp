#include "sqlconnect.h"

MysqlConnect::MysqlConnect( ) : mysql_( mysql_init(nullptr) ) {
	//mysql_ = (MYSQL*)malloc(sizeof(MYSQL));
}

MysqlConnect::~MysqlConnect() {
	if ( mysql_ ) {
		mysql_close(mysql_); 
		//free(mysql_);
		mysql_ = nullptr;
	}
}


bool MysqlConnect::Init( const char* szHost, int nport, const char* szUser,
	                     const char* szPasswd, const char* szDb ) {
	LOG_INFO("Enter Init!\n");
	// 初始化
	//if ( (mysql_ = mysql_init( mysql_ )) == nullptr ) {
	if ( mysql_  == nullptr ) {
		LOG_ERROR("init mysql failed %s. %d\n", this->GetErrorInfo(), errno);
		return false;
	}

	// 设置数据库属性(重连接)
	char cAuto = 1;
	if (mysql_options(mysql_, MYSQL_OPT_RECONNECT, &cAuto) != 0 ) {   // 成功返回  0 
		LOG_ERROR("set mysql options failed. result:%s\n" , this->GetErrorInfo());
		return false;
	}

	// 连接数据库
	if (mysql_real_connect(mysql_, szHost, szUser, szPasswd, szDb, nport, nullptr, 0) == nullptr) {
		LOG_ERROR("mysql connect is failed. %s .%d\n", this->GetErrorInfo(), errno);
		return false;
	}
	
	return true;
}	

bool MysqlConnect::Execute(const char* szSql) {
	if ( mysql_real_query(mysql_, szSql, strlen(szSql)) != 0) {
		if (mysql_errno(mysql_) == CR_SERVER_GONE_ERROR ) {
			Reconnection();
		}
		return false;
	}
	return true;
}

bool MysqlConnect::Execute(const char* szSql, SqlRecordSet& recordSet) {
	if (mysql_real_query( mysql_, szSql, strlen(szSql)) != 0) {
		if (mysql_errno( mysql_) == CR_SERVER_GONE_ERROR) {
			Reconnection();
		}
		return false;
	}
	MYSQL_RES* pRes = mysql_store_result( mysql_ );
	if (!pRes) {
		return false;
	}
	recordSet.SetResult(pRes);
	return true;
}

void MysqlConnect::Reconnection() {
	mysql_ping( mysql_ );
}

const char* MysqlConnect::GetErrorInfo() {
	return mysql_error(mysql_);
}

int MysqlConnect::EscapeString(const char* pSrc, int nStrlen, char* pDest) {
	if ( !mysql_ )  {
		return 0;
	}
	return mysql_real_escape_string( mysql_ ,pDest ,pSrc ,nStrlen);
}

void MysqlConnect::close()
{
	if (mysql_) {
		mysql_close(mysql_);
		//free(mysql_);
		mysql_ = nullptr;
	}
}

