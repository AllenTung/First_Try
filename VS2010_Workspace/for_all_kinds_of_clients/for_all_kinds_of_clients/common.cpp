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
	srand((unsigned)time(NULL));
	return rand()% content_length;
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