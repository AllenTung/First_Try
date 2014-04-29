#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "common.h"
using namespace std;
string get_random_file_name(int thread_id)
{
	//Current setting relates to the thread_id , just to check the concurrent read operation
	if(thread_id%3 == 0)
	{
		string tmp_file = "I:\\test_transmitfile_0.txt";
		return tmp_file;
	}
	else if(thread_id%3 == 1)
	{
		string tmp_file = "I:\\test_transmitfile_1.txt";
		return tmp_file;
	}	
	else if(thread_id%3 == 2)
	{
		string tmp_file = "I:\\test_transmitfile_2.txt";
		return tmp_file;
	}	
}

string get_request_random_file_name(int thread_id)
{
	//Current setting relates to the thread_id , just to check the concurrent read operation
	if(thread_id%3 == 0)
	{
		string tmp_file = "I:\\client_post_request_0.txt";
		return tmp_file;
	}
	else if(thread_id%3 == 1)
	{
		string tmp_file = "I:\\client_post_request_1.txt";
		return tmp_file;
	}	
	else if(thread_id%3 == 2)
	{
		string tmp_file = "I:\\client_post_request_2.txt";
		return tmp_file;
	}	
}
string update_request_random_file_name(int thread_id)
{
	//Current setting relates to the thread_id , just to check the concurrent read operation
	if(thread_id%3 == 0)
	{
		string tmp_file = "I:\\client_post_request_0.txt";
		return tmp_file;
	}
	else if(thread_id%3 == 1)
	{
		string tmp_file = "I:\\client_post_request_1.txt";
		return tmp_file;
	}	
	else if(thread_id%3 == 2)
	{
		string tmp_file = "I:\\client_post_request_2.txt";
		return tmp_file;
	}	
}
string int_to_string(unsigned int tmp_int)
{
	char tmp_char[30] = "";
	_ultoa_s(tmp_int, tmp_char, 10);
/*	_itoa_s(tmp_int, tmp_char, 10);*/
	return tmp_char;
}

void print_info(string client_id, string method, string uri, string detail)
{
	if (client_id.empty())
		client_id = "null";
	if (method.empty())
		method = "null";
	if (uri.empty())
		uri = "null";
	if (detail.empty())
		detail = "null";
	cout << "\nClient: " << client_id << "\nMethod: " << method << "\nTarget: " << uri << "\nDetail: " << detail << endl;
}

string get_systime_string()
{
	SYSTEMTIME sys_time;
	GetLocalTime(&sys_time);

	string temp_string = int_to_string(sys_time.wMonth) + "_" + int_to_string(sys_time.wDay) + "_" + int_to_string(sys_time.wHour) + "_" + 
		int_to_string(sys_time.wMinute) + "_" + int_to_string(sys_time.wSecond) + "_" + int_to_string(sys_time.wMilliseconds);
	
	return temp_string;
	
}

unsigned int get_random_offset(unsigned int content_length)
{
// 	srand((unsigned)time(NULL));
// 	unsigned int random_offset = rand()% content_length;

	unsigned int random_offset = my_random_long() % content_length;
 
	if (content_length > 100)
	{
		cout << "*****Random offset: " << random_offset << "**************in range: " << content_length << endl;
	}

	return random_offset;
}

static long get_my_random_long()
{
	static boost::mt19937 gen(static_cast<unsigned int> (std::time(0)));
	boost::random::uniform_int_distribution<> engine(1, INT_MAX);
	boost::variate_generator<boost::mt19937&,boost::random::uniform_int_distribution<> > rng(gen, engine);

	return rng();
}
long my_random_long()
{
	return get_my_random_long();
}

string get_random_update_content()
{

	int content_count = 88;

	vector<string> update_content_warehouse;
	for(int i = 0; i < content_count; i++)
	{
		string temp_content  = "update_content_" + int_to_string(i) + "_end";
		update_content_warehouse.push_back(temp_content);		
	}

	return update_content_warehouse.at(get_random_offset(content_count));

}

string extract_pure_obj_name(string full_path)
{
	if(full_path.find_last_of("\\") < NO_SUCH_SUBSTRING)
	{
		string obj_name = full_path.substr(full_path.find_last_of("\\") + 1);
		return obj_name;
	}
	else
	{
		return full_path;
	}	
}

string return_full_path(string obj_name)
{
	char* curdir;
	curdir = (char*)malloc(sizeof(char)*1000);	
	_getcwd(curdir, 1000);
	string obj_prefix(curdir);
	string full_obj_name = obj_prefix + "\\" + obj_name;

	return full_obj_name;
}