#ifndef _INICONFIG_H_
#define _INICONFIG_H_

#include <iostream>
#include <string>
#include "configdef.h"


class Iniconfig{
protected:
	Iniconfig();
public:
	~Iniconfig();
	
	bool loadfile(const std::string &path);
	st_env_config  getconfig()const;      //获取配置文件的结构体

	static Iniconfig* getInstance();

private:
	st_env_config _config;
	bool _isloaded;
	static Iniconfig* iniconfig_;
};


#endif    // iniconfig.h