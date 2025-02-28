#include "iniconfig.h"
#include <iniparser/iniparser.h>

Iniconfig* Iniconfig::iniconfig_ = nullptr ;

Iniconfig::Iniconfig( ):_isloaded(false){
	
}

Iniconfig::~Iniconfig( ){
	
}
	
bool Iniconfig::loadfile(const std::string &path){// load ini file
	dictionary *ini = NULL ;
	if( !_isloaded){   //ini file is not load
		ini = iniparser_load( path.c_str());
		if( ini == NULL ){
			fprintf(stderr ," Cannot parser file :%s\n" , path.c_str());
			return false;
		}
		/*
		[database]
		ip	= 127.0.0.1 ;
		port	= 3306 ;
		user	= root ;
		pwd	= ln123456789
		db	= qiniubike ;

		[server]
		port	= 9090 ;

		*/
		const char *ip = iniparser_getstring( ini ,"database:ip" ,"127.0.0.1");
		int port = iniparser_getint(ini ,"database:port" ,3306);
		const char *user = iniparser_getstring( ini ,"database:user" ,"root");
		const char *pwd = iniparser_getstring( ini ,"database:pwd" ,"ln123456789");
		const char *db = iniparser_getstring( ini ,"database:db" ,"qiniubike");
		int sport = iniparser_getint( ini ,"server:port" ,9090);
		
		_config = st_env_config(std::string(ip) , port ,std::string(user) , std::string(pwd),\
		                        std::string(db), sport );
		_isloaded = true;
	}
	return true;
}

st_env_config Iniconfig::getconfig( )const{
	
	return  _config;

} 

Iniconfig* Iniconfig::getInstance() {
	if (!iniconfig_) {
		iniconfig_ = new Iniconfig();
	}
	return iniconfig_;
}




