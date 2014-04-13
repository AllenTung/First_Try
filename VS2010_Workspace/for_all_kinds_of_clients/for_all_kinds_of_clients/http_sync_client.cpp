#pragma once
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "request.h"
#include "common.h"
#include <vector>
#include "client.h"
#include <fstream>
#include <Windows.h>
#include <direct.h>

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::windows::overlapped_ptr;
using boost::asio::windows::random_access_handle;
typedef boost::shared_ptr<boost::thread> thread_ptr;
typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;

static int next_io_service = 0;
vector<thread_ptr> thread_pool;
vector<io_service_ptr> io_service_pool;

boost::asio::io_service& get_next_io_service()
{
	next_io_service++;
	if (next_io_service == CLIENT_POOL_SIZE)
	{
		next_io_service = 0;
	}
	boost::asio::io_service& tmp_io = *io_service_pool[next_io_service];
	return tmp_io;	
}

int main()
{
	 int forcin = 0;

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
	int client_id = -1;
	int client_pool = -1;
	int target_port = -1;

	if (config_content.find("client_id:") < NO_SUCH_SUBSTRING) {
		string temp_server_id = config_content.substr(config_content.find("client_id:") + 10, config_content.find_first_of("\r\n", config_content.find("client_id:")) - config_content.find("client_id") - 10);
		client_id = atoi(temp_server_id.c_str());
	}
	if (config_content.find("thread_num:") < NO_SUCH_SUBSTRING) {
		string temp_num = config_content.substr(config_content.find("thread_num:") + 11, config_content.find_first_of("\r\n", config_content.find("thread_num:")) - config_content.find("thread_num:") - 11);
		thread_num = atoi(temp_num.c_str());
	}
	if (config_content.find("client_pool:") < NO_SUCH_SUBSTRING) {
		string temp_pool_size = config_content.substr(config_content.find("client_pool:") + 12, config_content.find_first_of("\r\n", config_content.find("client_pool:")) - config_content.find("client_pool:") - 12);
		client_pool = atoi(temp_pool_size.c_str());
	}
	if (config_content.find("target_port:") < NO_SUCH_SUBSTRING) {
		string temp_target_port = config_content.substr(config_content.find("target_port:") + 12, config_content.find_first_of("\r\n", config_content.find("target_port:")) - config_content.find("target_port:") - 12);
		target_port = atoi(temp_target_port.c_str());
	}

	//Creat some io_service
	for (int i = 0; i < thread_num; i++)
	{
		io_service_ptr io_ptr(new boost::asio::io_service);
		io_service_pool.push_back(io_ptr);
	}

	//Creat some different clients
 
	vector<client_ptr> clients;
	for (int i = 0; i < client_pool; i++)
	{
		client_ptr new_client(new client(get_next_io_service(), next_io_service, target_port));
		clients.push_back(new_client);
	}

	//Launch the request method of client
 	for (int thread_id = 0; thread_id < client_pool; thread_id ++)
	{
		if (thread_id%2 == 0)
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&client::launch_client, clients[thread_id], POST_REQUEST, thread_id)));	
			thread_pool.push_back(thread);
		}

		else 
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&client::launch_client, clients[thread_id], GET_REQUEST, thread_id)));
			thread_pool.push_back(thread);
		}	
	}

	//Run all the io_services
	for (int io_num = 0; io_num < thread_num; io_num ++)
	{
		io_service_pool[io_num]->run();
	}
	
	//Waits all the threads to stop
	for (int t = 0; t < thread_num; t ++)
	{
		cout << "Waiting for thread:" << t << "to finish...\n";
		thread_pool.at(t)->join();
 	}

	//Stop all io_services
	for (int io_num = 0; io_num < thread_num; io_num ++)
	{
		io_service_pool[io_num]->stop();
	}

	cin >> forcin;
	return 0;
}