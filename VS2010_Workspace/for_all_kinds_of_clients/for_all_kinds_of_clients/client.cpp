#pragma once
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include "client.h"
#include <fstream>
#include <stdexcept>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "common.h"

using namespace std;
using boost::asio::ip::tcp;


client::client(boost::asio::io_service& io_service, int client_id)
	:client_socket(io_service), client_id(client_id), file_handler(io_service)
{
}
client::client(boost::asio::io_service& io_service, int client_id, int target_port)
	:client_socket(io_service), client_id(client_id), file_handler(io_service), target_port(target_port)
{
}
void client::launch_get_request(int thread_id)
{
	request req;
	req.make_get_request(get_random_file_name(client_id), client_id);
	try
	{
		int tmp = target_port;
		tcp::endpoint end_p(boost::asio::ip::address_v4::from_string("127.0.0.1"), target_port);
		client_socket.connect(end_p);

// 		tcp::resolver client_resolver(file_handler.get_io_service());
// 		tcp::resolver::query query(server_address.c_str(), "http");
// 		tcp::resolver::iterator endpoint_iterator = client_resolver.resolve(query);
// 		boost::asio::connect(client_socket, endpoint_iterator);

		//Set the size of buffer of socket
		boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
		client_socket.set_option(option);

		boost::asio::write(client_socket, req.to_buffers());

		//Below waits and handles the response ,that is to say , the coming file
		int forcin = 0;
		string trans_test_result = "I:/client_get_result_";
		trans_test_result += int_to_string(client_id);
		trans_test_result += ".txt";

		ofstream output_for_trans(trans_test_result.c_str(), ios::app | ios::binary);

		char* for_recv = new char[RECEIVE_BUFFER_SIZE];

		int per_receive = 0;
		int tmp_count = 0;
		for(;;)
		{
			per_receive = recv(client_socket.native_handle(), for_recv, RECEIVE_BUFFER_SIZE, 0);

			if (per_receive <= 0)
			{
				
				//处理刚好上一次读取完了所有的内容的情况
				//若tmp_count还为0，那么就是出现了异常
				if (tmp_count > 0)
				{
					print_info(int_to_string(client_id), "GET", req.uri, "This process finished successfully!");
				}
				break;
			}
			tmp_count += per_receive;

			output_for_trans.write(for_recv, per_receive);
			output_for_trans.flush();

			if (tmp_count >= 1024*1024*200)
			{
				//200M for test is enough
				print_info(int_to_string(client_id), "GET", req.uri, "This process finished successfully!");
				break;
			}
		}

		//Release all related sources
		output_for_trans.close();
		delete []for_recv;
		client_socket.shutdown(tcp::socket::shutdown_both);
		
	}
	catch (exception& e)
	{
		print_info(int_to_string(client_id), "GET", req.uri, "This process finished successfully!");
	} 
}
void client::launch_post_request(int thread_id)
{
	string local_path = get_random_file_name(thread_id);
	string remote_path = "I:/client_post_request_";
	remote_path += int_to_string(thread_id);
	remote_path += ".txt";

	request req;
	req.make_post_request(local_path, remote_path, client_id, 99);
	try
	{
// 		tcp::resolver client_resolver(file_handler.get_io_service());
// 		tcp::resolver::query query(remote_address.c_str(), "http");
// 		tcp::resolver::iterator endpoint_iterator = client_resolver.resolve(query);
// 		boost::asio::connect(client_socket, endpoint_iterator);

		tcp::endpoint end_p(boost::asio::ip::address_v4::from_string("127.0.0.1"), target_port);
		client_socket.connect(end_p);
		boost::asio::write(client_socket, req.to_buffers());

		//The faked response cause we assume the server will accept every post request the client sends
		//And this faked response will end with \r\n
		boost::asio::streambuf response;
		vector<char> tmp_response;
		boost::asio::read_until(client_socket, response, "\r\n");
/*		client_socket.read_some(boost::asio::buffer(tmp_response));*/
		tmp_response.clear();

		cout << "After the ACK message client: " << client_id << " begins to transmit the file!\n";

		//Prepare to transmit the file
		boost::system::error_code ec;
		file_handler.assign(::CreateFile(local_path.c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0), ec);

		if (file_handler.is_open())
		{
			print_info(int_to_string(client_id), "POST", req.uri, "Open done!Now transmitting to server....");
			transmit_file(client_socket, file_handler, boost::bind(&client::handle_write, this, req.obj_id, boost::asio::placeholders::error));
		}

		else
		{
			print_info(int_to_string(client_id), "POST", req.uri, "Error occurs when opening the file!");
			cout << ec.message() << endl;			
			return;
		}
		
	}
	catch (exception& e)
	{
		print_info(int_to_string(client_id), "POST", req.uri, "Exception Occurs!");
		cout << "Exception :" << e.what() << endl;
	}
}
void client::launch_client(int request_type, int thread_id)
{
	if(request_type == POST_REQUEST)
	{
		launch_post_request(thread_id);
		return;
	}
	else if (request_type == GET_REQUEST)
	{
		launch_get_request(thread_id);
		return;
	}
}

void client::handle_write(string uri, const boost::system::error_code& e)
{
	if (!e)
	{
		print_info(int_to_string(client_id), "POST", uri, e.message());
/*		cout << "Now socket is shut down...\n";*/
	}
	else
	{
		print_info(int_to_string(client_id), "POST", uri, "In handle_write and no error_code!");
	}
}

