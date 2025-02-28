#ifndef _CONFIGDEF_H_
#define _CONFIGDEF_H_

#include <string>

typedef struct st_env_config{
	//数据库配置
	std::string  db_ip;
	unsigned short  db_port;
    std::string db_user;
	std::string db_pwd ;
	std::string db_name;
	
	//服务器配置
	unsigned short svr_port;
	
	st_env_config( ){
	 
	}
	
	st_env_config( const std::string  &db_ip , unsigned short db_port , const std::string &db_user ,const std::string &db_pwd , 
	              const std::string &db_name , unsigned short svr_port ){
		this->db_ip = db_ip;
		this->db_port = db_port;
		this->db_user = db_user;
		this->db_pwd = db_pwd;
		this->db_name = db_name;
		this->svr_port = svr_port;
	}
	
	//重载 = 
	st_env_config& operator = (const st_env_config& st){
		if( &st != this){
			db_ip = st.db_ip;
			db_port = st.db_port;
			db_user = st.db_user;
			db_pwd =st.db_pwd;
			db_name =st.db_name;
			svr_port =st.svr_port;
		}
		return *this;
	}
	
	

}_st_env_config;

#endif   //  configdef.h