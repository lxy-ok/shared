#ifndef _SQL_CONNECT_H_
#define _SQL_CONNECT_H_
 
//#ifdef __cplusplus
//extern "C" {
//#endif  /*__cplusplus*/

#include <mysql/mysql.h>
#include <string>
#include <string.h>
#include <mysql/errmsg.h>
#include <assert.h>

#include "glo_def.h"

// 查询结果集 ,封装成类
class SqlRecordSet {
public:
	SqlRecordSet() :m_pRes_( NULL ) {  } 

	// 只能显示构造
	explicit SqlRecordSet( MYSQL_RES* pRes ) {
		m_pRes_ =  pRes;
	}

	~SqlRecordSet() {
		if ( m_pRes_ ) {
			mysql_free_result(m_pRes_);
		}
	}

	inline MYSQL_RES* GetResult() {  
		return m_pRes_; 
	}

	inline void SetResult( MYSQL_RES* pRes ) {
		assert( m_pRes_ == NULL ); // 设置一个断言,如果此时保存了结果集,那么让程序报错,放置内存泄漏
		if (m_pRes_) {
			LOG_WARN("the MYSQL_RES has already strory result ,maybe will cause memory leak!");
		}
		m_pRes_ = pRes;
	}
	// 获取一行的数据
	void FetchRow(MYSQL_ROW& row) {
		row = mysql_fetch_row(m_pRes_);
	}

	// 获取行数
	inline i32 GetRowCount() {
		return m_pRes_->row_count;
	}

	MYSQL_RES* MysqlRes() { return m_pRes_;  }
private:
	MYSQL_RES* m_pRes_;
};

class MysqlConnect {
public:
	MysqlConnect( );
	~MysqlConnect( );

	// 获取mysql句柄
	MYSQL* Mysql() {   return mysql_;    }

	// 连接mysql (  init  )
	bool Init(const char* szHost, int nport, const char* szUser, const char* szPasswd, const char* szDb );

	// 执行,一个没有返回值 ,一个有返回值
	bool Execute(const  char* szSql );
	bool Execute(const  char* szSql, SqlRecordSet& recordSet);  // MYSQL_RSP*

	// 转义字符
	int EscapeString(const char* pSrc, int nStrlen, char* pDest);

	void close();
	// 获取错误信息
	const char* GetErrorInfo();
	
	void Reconnection();
private:
	MYSQL* mysql_;
};

//#ifdef __cplusplus
//}
//#endif // __cplusplus


#endif // _SQL_CONNECTION_H_

