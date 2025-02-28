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

// ��ѯ����� ,��װ����
class SqlRecordSet {
public:
	SqlRecordSet() :m_pRes_( NULL ) {  } 

	// ֻ����ʾ����
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
		assert( m_pRes_ == NULL ); // ����һ������,�����ʱ�����˽����,��ô�ó��򱨴�,�����ڴ�й©
		if (m_pRes_) {
			LOG_WARN("the MYSQL_RES has already strory result ,maybe will cause memory leak!");
		}
		m_pRes_ = pRes;
	}
	// ��ȡһ�е�����
	void FetchRow(MYSQL_ROW& row) {
		row = mysql_fetch_row(m_pRes_);
	}

	// ��ȡ����
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

	// ��ȡmysql���
	MYSQL* Mysql() {   return mysql_;    }

	// ����mysql (  init  )
	bool Init(const char* szHost, int nport, const char* szUser, const char* szPasswd, const char* szDb );

	// ִ��,һ��û�з���ֵ ,һ���з���ֵ
	bool Execute(const  char* szSql );
	bool Execute(const  char* szSql, SqlRecordSet& recordSet);  // MYSQL_RSP*

	// ת���ַ�
	int EscapeString(const char* pSrc, int nStrlen, char* pDest);

	void close();
	// ��ȡ������Ϣ
	const char* GetErrorInfo();
	
	void Reconnection();
private:
	MYSQL* mysql_;
};

//#ifdef __cplusplus
//}
//#endif // __cplusplus


#endif // _SQL_CONNECTION_H_

