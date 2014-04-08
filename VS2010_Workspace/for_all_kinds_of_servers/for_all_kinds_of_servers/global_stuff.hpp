#pragma  once
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>

using namespace std;

class global_val
{
public:
	static const int buf_size = 8192;


	static vector<string> split(string& str,const char* c)
	{
		char *cstr, *p;
		vector<string> res;
		cstr = new char[str.size()+1];
		strcpy(cstr,str.c_str());
		p = strtok(cstr,c);
		while(p!=NULL)
		{
			res.push_back(p);
			p = strtok(NULL,c);
		}
		return res;
	}
};

