#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <map>
#include <algorithm>
#include <deque>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/upgradable_lock.hpp>
#include <boost/asio/windows/random_access_handle.hpp>
#include <string>
#include "server.hpp"
#include "common.hpp"
#include "encoder.h"
#include "decoder.h"
#include "metadata.h"
#include <Windows.h>

using namespace std;
using namespace boost::interprocess;
//Initialization
map<string, metadata> server::obj_meta_table = map<string, metadata>();
map<string, string> server::ip_port_table = map<string, string>();
int main()
{
	int forcin = 0, argc = 4;
#pragma region test_zone
// 	deque<int> test_queue;
// 	test_queue.push_back(3);
// 	test_queue.push_back(4);
// 	cout << test_queue.at(1) << endl;
// 	test_queue.pop_front();
// 	cout << test_queue.size() << endl;
// 	cin >> forcin;
// 	string locking_suffix = ".lock";
// 	string full_local_path = "I:\\testtt.txt";
// 
// 	creat_locking_file(full_local_path);
// 	cin >> forcin;
// 
// 	file_lock test_lock("I:\\test_lock.lock");
// 	test_lock.lock();
// 		ofstream file_writer("I:\\test_lock.txt", ios::app | ios::binary);
// 		const char* w = "fuckserver";
// 		cin >> forcin;
// 		file_writer.write(w,4);
// 		file_writer.flush();
// 		file_writer.close();
// 		test_lock.unlock();
// 	
// 	cin >> argc;
// 		char* testchar2 = (char*)malloc(sizeof(char)*4);	
// 		char* new_insert = new char[5];
// 		new_insert[0] = 'f';
// 		new_insert[1] = 'u';
// 		new_insert[2] = 'c';
// 		new_insert[3] = 'k';
// 		new_insert[4] = 'm';
// 		strcpy(testchar2, (const char*)new_insert);
// 		cout << testchar2 << endl;
// 		new_insert = NULL;
// 		delete new_insert;
// 		//free((char*)new_insert);
// 		//cout << testchar2 << endl;
// 		cout << testchar2 << endl;
// 		cin >> forcin;
// 		return 0;
// 	testchar = teststring;
// 	free(teststring);
// 	cout << testchar << endl;
// 	cin >> forcin;
// 	int fuck[2] = {0,1};
// 	int* res = test_pointer(fuck);
// 	cout << fuck[0] << endl;
// 	cout << *(res+1) << endl;
// 	cin >> forcin;
	
// 	ofstream test_open("I:\\fuck_up3.txt", ios::_Nocreate);
// 	if (test_open)
// 		cout << "true" << endl;
// 	if (!test_open)
// 		cout << "false" << endl;
// 	cout << test_open.is_open() << endl;
// 	cin >> forcin;
// 	string lc = "I:\\test_random.txt";
// 	string lc2 = "fuck";
// 	ofstream test_out(lc.c_str(), ios::out | ios::binary);
// 
// 	test_out.write(lc.c_str(), lc.length());
// 	test_out.flush();
// 	test_out.write(lc2.c_str(),lc2.length());
// 	test_out.flush();
// 	test_out.close();
// 
// 	ofstream test_out2(lc.c_str(), ios::app | ios::binary);
// 
// 	test_out2.write(lc2.c_str(), lc2.length());
// 	test_out2.flush();
// 	test_out2.close();
// 
// 	 cin>> forcin;
// 	for(int i = 0; i<100 ;i++)
// 		cout << my_random_long() % 196608 << endl;
// 
// 	cin >> forcin;
// 	cout << data_type_vector.size() << endl;
// 	cin >> forcin;
// 	string temp_fuck = "fuck";
// 	string temp2 = "fuck";
// 	if (temp_fuck == temp2)
// 		cout << "fuck" << endl;
// 	cin >> forcin;
//  	string temp_string_test = "fuck";
// 	cout << temp_string_test.length() << endl;
// 	cin >> forcin
// 	string temp2= "me";
// 	temp_string_test += temp2 + " please";
// 	cout << temp_string_test;
// 
// 	cin >> forcin;
// 
// 
// 	string file_path = "I:/test_random.txt";
// 	ofstream random_writer(file_path.c_str(), ios::in | ios::out | ios::binary);
// 	long cur_position = random_writer.tellp();
// 	random_writer.seekp(cur_position + 24);
// 	string content_string = "skeptical";
// 	random_writer.write(content_string.c_str(), content_string.length());
// 
// 	random_writer.close();

//	cin >> forcin;

// 	SYSTEMTIME sys_time;
// 	GetLocalTime(&sys_time);
// 	printf( "%d %d %d:%d:%d %d\n\n", sys_time.wMonth, sys_time.wDay, sys_time.wHour, 
// 		sys_time.wMinute, sys_time.wSecond, sys_time.wMilliseconds);
// 
// 	string request_string = "user_id:allen\r\nserver_id:12\r\n\r\nmethod:GET\r\ncontent_length:33567\r\n";
// 	string user_id = request_string.substr(request_string.find("method") + 7, request_string.find_first_of("\r\n", request_string.find("method:")) - request_string.find("method:") - 7);
// 	cout << request_string.find_first_of("\r\n\r\n", request_string.find("server_id")) << endl;
// 	cout << user_id <<endl;
/*	cin >> forcin;*/
// 	map<string,string> mymap;
// 	mymap.insert(pair<string, string>("obj_1", "alice"));
// 	mymap.insert(pair<string, string>("obj_2", "bob"));
// 	mymap.insert(make_pair("obj_3", "allen"));
// 	pair<map<string, string>::iterator, bool> test_pair;
// 	test_pair = mymap.insert(make_pair("obj_4", "jeremy"));
// 
// 	if(test_pair.second == true) {
// 		cout << "successfully inserted the value :" << test_pair.first->second << endl;
// 	}
// 	else 
// 	{
// 		cout << "Insertion failed!" << endl;
// 	}
// 	if (mymap.count("obj_1") > 0)
// 	{
// 		cout << mymap["obj_1"] << endl;
// 	}
// 	map<string,string>::iterator it = mymap.find("obj_3");  
// 	if(it != mymap.end())  
// 	{  
// 		cout << it->second << endl;  
// 	}  
// 
// 	mymap["obj_3"] = "new_allen";
// 	map<string,string>::iterator it2 = mymap.find("obj_3");  
// 	if(it2 != mymap.end())  
// 	{  
// 		cout << it2->second << endl;  
// 	}  
// 
// 	cin >> forcin;
#pragma endregion test_zone

#pragma region config_init
	char* cur_dir = (char*)malloc(sizeof(char)*100);	
	_getcwd(cur_dir, 1000);
	string current_dir = cur_dir;
	string config_file = "\\conf.txt";
	string ip_port_f = "\\ip_port.txt";

	string config_path = current_dir + config_file;
	string ip_port_path = current_dir + ip_port_f;

	FILE* fp = fopen(config_path.c_str(), "rb");
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

	string temp_record = "";
	ifstream ip_port_file(ip_port_path.c_str(), ios::in | ios::binary);
	while(getline(ip_port_file, temp_record))
	{
		string temp_port = temp_record.substr(0, temp_record.find_first_of(","));
		string temp_ip = temp_record.substr(temp_record.find_first_of(",") + 1);
		server::ip_port_table.insert(make_pair(temp_port, temp_ip));
	}
	ip_port_file.close();

#pragma endregion config_init

	int thread_num = -1;
	int server_id = -1;
	string ip ="";
	string port = "";
	string doc_root ="";
	
	if (config_content.find("server_id:") < NO_SUCH_SUBSTRING)
	{
		string temp_server_id = config_content.substr(config_content.find("server_id:") + 10, config_content.find_first_of("\r\n", config_content.find("server_id:")) - config_content.find("server_id") - 10);
		server_id = atoi(temp_server_id.c_str());
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

	server new_server(ip, port, doc_root, thread_num, server_id);
	new_server.run();

	return 0;
}