#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <map>
#include <direct.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <time.h>
#include "proxy_server.hpp"
#include "common.hpp"
#include <Windows.h>

using namespace std;

int main()
{
	int forcin = 0, argc = 4;


// #pragma region test_zone
// 
// 	int r1=0,r2=0,r3=0,r4=0;
// 	string temp_hash1 = "client_1.txt";
// 	string temp_hash2 = "client_2.txt";
// 	string temp_hash3 = "client_1.txt";
// 
// 	cout << get_hash_value(const_cast<char*>(temp_hash1.c_str()));
// 		cout << get_hash_value(const_cast<char*>(temp_hash2.c_str()));
// 			cout << get_hash_value(const_cast<char*>(temp_hash3.c_str()));
// 
// 	cin >> forcin;
// 
// 
// 	srand((unsigned)time(NULL));
// 
// 	for (int i = 0; i < 10; i ++)
// 	{
// 		cout << rand()%256 << endl;
// 	}
// 
// 	cin >> forcin;
// 
// 
// 
// // 	string temp_string_test = "fuck";
// // 	string temp2= "me";
// // 	temp_string_test += temp2 + " please";
// // 	cout << temp_string_test;
// // 
// // 	cin >> forcin;
// // 
// // 
// // 	string file_path = "I:/test_random.txt";
// // 	ofstream random_writer(file_path.c_str(), ios::in | ios::out | ios::binary);
// // 	long cur_position = random_writer.tellp();
// // 	random_writer.seekp(cur_position + 8);
// // 	string content_string = "skeptical";
// // 	random_writer.write(content_string.c_str(), content_string.length());
// // 
// // 	random_writer.close();
// 	 
// 
// // 	SYSTEMTIME sys_time;
// // 	GetLocalTime(&sys_time);
// // 	printf( "%d %d %d:%d:%d %d\n\n", sys_time.wMonth, sys_time.wDay, sys_time.wHour, 
// // 		sys_time.wMinute, sys_time.wSecond, sys_time.wMilliseconds);
// // 
// // 	string request_string = "user_id:allen\r\nserver_id:12\r\n\r\nmethod:GET\r\ncontent_length:33567\r\n";
// // 	string user_id = request_string.substr(request_string.find("method") + 7, request_string.find_first_of("\r\n", request_string.find("method:")) - request_string.find("method:") - 7);
// // 	cout << request_string.find_first_of("\r\n\r\n", request_string.find("server_id")) << endl;
// // 	cout << user_id <<endl;
// /*	cin >> forcin;*/
// // 	map<string,string> mymap;
// // 	mymap.insert(pair<string, string>("obj_1", "alice"));
// // 	mymap.insert(pair<string, string>("obj_2", "bob"));
// // 	mymap.insert(make_pair("obj_3", "allen"));
// // 	pair<map<string, string>::iterator, bool> test_pair;
// // 	test_pair = mymap.insert(make_pair("obj_4", "jeremy"));
// // 
// // 	if(test_pair.second == true) {
// // 		cout << "successfully inserted the value :" << test_pair.first->second << endl;
// // 	}
// // 	else 
// // 	{
// // 		cout << "Insertion failed!" << endl;
// // 	}
// // 	if (mymap.count("obj_1") > 0)
// // 	{
// // 		cout << mymap["obj_1"] << endl;
// // 	}
// // 	map<string,string>::iterator it = mymap.find("obj_3");  
// // 	if(it != mymap.end())  
// // 	{  
// // 		cout << it->second << endl;  
// // 	}  
// // 
// // 	mymap["obj_3"] = "new_allen";
// // 	map<string,string>::iterator it2 = mymap.find("obj_3");  
// // 	if(it2 != mymap.end())  
// // 	{  
// // 		cout << it2->second << endl;  
// // 	}  
// // 
// // 	cin >> forcin;
// #pragma endregion test_zone


#pragma region config_init
	char* cur_dir = (char*)malloc(sizeof(char)*100);
	_getcwd(cur_dir, 1000);

	string config_file = "\\conf.txt";
	char* config_path = strcat(cur_dir, config_file.c_str());

	FILE* fp = fopen(config_path, "rb");
	int lSize;
	char * buffer;
	size_t result;

	// obtain file size:
	fseek (fp , 0 , SEEK_END);
	lSize = ftell (fp);
	rewind (fp);

	buffer = (char*)malloc(sizeof(char)*lSize);
	result = fread(buffer,1,lSize,fp);

	string config_content(buffer);

	fclose (fp);
	free (buffer);	
#pragma endregion config_init

	int thread_num = -1;
	int proxy_server_id = -1;
	string ip ="";
	string port = "";
	string doc_root ="";
	
	if (config_content.find("proxy_server_id:") < NO_SUCH_SUBSTRING)
	{
		string temp_proxy_server_id = config_content.substr(config_content.find("proxy_server_id:") + 16, config_content.find_first_of("\r\n", config_content.find("proxy_server_id:")) - config_content.find("proxy_server_id") - 16);
		proxy_server_id = atoi(temp_proxy_server_id.c_str());
	}
	if (config_content.find("thread_num:") < NO_SUCH_SUBSTRING)
	{
		string temp_num = config_content.substr(config_content.find("thread_num:") + 11, config_content.find_first_of("\r\n", config_content.find("thread_num:")) - config_content.find("thread_num:") - 11);
		thread_num = atoi(temp_num.c_str());
	}
	if (config_content.find("ip:") < NO_SUCH_SUBSTRING)
	{
		ip = config_content.substr(config_content.find("ip:") + 3, config_content.find_first_of("\r\n", config_content.find("ip:")) - config_content.find("ip:") - 3);
	}
	if (config_content.find("port:") < NO_SUCH_SUBSTRING)
	{
		port = config_content.substr(config_content.find("port:") + 5, config_content.find_first_of("\r\n", config_content.find("port:")) - config_content.find("port:") - 5);
	}
	if (config_content.find("doc_root:") < NO_SUCH_SUBSTRING)
	{
		doc_root = config_content.substr(config_content.find("doc_root:") + 9, config_content.find_first_of("\r\n", config_content.find("doc_root:")) - config_content.find("doc_root:") - 9);
	}

	proxy_server new_proxy_server(ip, port, doc_root, thread_num, proxy_server_id);	
	new_proxy_server.run();

	return 0;
}