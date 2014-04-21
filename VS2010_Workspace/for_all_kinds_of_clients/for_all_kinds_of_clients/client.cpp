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
	:client_proxy_socket(io_service), client_storage_server_socket(io_service), client_id(client_id), file_handler(io_service),target_port(777)
{
}
client::client(boost::asio::io_service& io_service, int client_id, int target_port)
	:client_proxy_socket(io_service), client_storage_server_socket(io_service), client_id(client_id), file_handler(io_service), target_port(target_port)
{
}


void client::launch_get_request(int thread_id)
{
	request req;
	req.make_get_request(get_random_file_name(client_id), client_id);
	try
	{
#pragma region proxy_phase
		tcp::endpoint end_point_proxy(boost::asio::ip::address_v4::from_string("127.0.0.1"), target_port);
		client_proxy_socket.connect(end_point_proxy);
		boost::system::error_code proxy_err_code;
		boost::asio::write(client_proxy_socket, req.header_to_buffers(), proxy_err_code);

		//Read the location info from proxy server
		boost::asio::streambuf response;
		boost::asio::read_until(client_proxy_socket, response, "\r\n");
		istream temp_stream(&response);
		string temp_string;
		temp_stream >> temp_string;
		
		if (temp_string.find("server_id:") < NO_SUCH_SUBSTRING)
		{
			string temp_server_id = temp_string.substr(temp_string.find("server_id:") + 10, temp_string.find_first_of("\r\n", temp_string.find("server_id:")) - temp_string.find("server_id") - 10);
			req.server_id = atoi(temp_server_id.c_str());
			cout << "Accountable server from the proxy server: " << req.server_id << endl;
		}

		boost::system::error_code ignored_ec;
		client_proxy_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		client_proxy_socket.close();
#pragma endregion proxy_phase
		
#pragma region storage_phase
		//Connect to the dedicated storage server using the server_id as the port
		tcp::endpoint end_point_storage(boost::asio::ip::address_v4::from_string("127.0.0.1"), req.server_id);
		client_storage_server_socket.connect(end_point_storage);

		//Set the size of buffer of socket
		boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
		client_storage_server_socket.set_option(option);

		//Send request to the accountable storage server
		boost::system::error_code execution_err_code;
		boost::asio::write(client_storage_server_socket, req.header_to_buffers(), execution_err_code);

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
			per_receive = recv(client_storage_server_socket.native_handle(), for_recv, RECEIVE_BUFFER_SIZE, 0);
			
			if (per_receive <= 0)
			{
				if (tmp_count > 0)
				{
					print_info(int_to_string(client_id), "GET", req.uri, "This process finished successfully!");
				}
				break;
			}
			tmp_count += per_receive;

			output_for_trans.write(for_recv, per_receive);
			output_for_trans.flush();
		}

		cout << "Receive finished !" << endl;

		//Release all related sources
		output_for_trans.close();
		delete []for_recv;
		client_storage_server_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		client_storage_server_socket.close();
#pragma endregion storage_phase
		
	}
	catch (exception& e)
	{
		print_info(int_to_string(client_id), "GET", req.uri, "This process finished successfully!");
	} 
}
void client::launch_update_request(int thread_id)
{
// 	string local_path = get_random_file_name(thread_id);
// 	string remote_path = "I:/client_post_request_";
// 	remote_path += int_to_string(thread_id);
// 	remote_path += ".txt";

	string local_path = "I:/test_random.txt";
	string remote_path = "I:/test_random.txt";

	request req;
	req.make_update_request(local_path, remote_path, client_id, get_random_update_content());
	try
	{
#pragma region proxy_phase

		tcp::endpoint end_point_proxy(boost::asio::ip::address_v4::from_string("127.0.0.1"), target_port);
		client_proxy_socket.connect(end_point_proxy);
		boost::system::error_code proxy_err_code;
		boost::asio::write(client_proxy_socket, req.header_to_buffers(), proxy_err_code);

		//Read the location info from proxy server
		boost::asio::streambuf response;
		boost::asio::read_until(client_proxy_socket, response, "\r\n");
		istream temp_stream(&response);
		string temp_string;
		temp_stream >> temp_string;

		if (temp_string.find("server_id:") < NO_SUCH_SUBSTRING)
		{
			string temp_server_id = temp_string.substr(temp_string.find("server_id:") + 10, temp_string.find_first_of("\r\n", temp_string.find("server_id:")) - temp_string.find("server_id") - 10);
			req.server_id = atoi(temp_server_id.c_str());
			cout << "Accountable server from the proxy server: " << req.server_id << endl;
		}

		boost::system::error_code ignored_ec;
		client_proxy_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		client_proxy_socket.close();
#pragma endregion proxy_phase

#pragma region storage_phase

		//Connect to the dedicated storage server using the server_id as the port
		tcp::endpoint end_point_storage(boost::asio::ip::address_v4::from_string("127.0.0.1"), req.server_id);
		client_storage_server_socket.connect(end_point_storage);

		//Set the size of buffer of socket
		boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
		client_storage_server_socket.set_option(option);

		cout << "After connect!" << endl;

		boost::system::error_code storage_prepare_err_code;
		boost::asio::write(client_storage_server_socket, req.header_to_buffers(), storage_prepare_err_code);

		cout << "After write !" << endl;

		//Wait for the ready for update from the storage server
		boost::asio::streambuf response_ready;
		boost::asio::read_until(client_storage_server_socket, response_ready, "\r\n");
		istream ready_stream(&response_ready);
		string ready_string;
		ready_stream >> ready_string;



		if (ready_string.find("ready_for_update") < NO_SUCH_SUBSTRING)
		{
			//Send only the content to the accountable server for simplicity, ends with "\r\n"
			boost::system::error_code execution_err_code;
			boost::asio::write(client_storage_server_socket, req.content_to_buffers(), execution_err_code);
		}
		else
		{
			cout << "Something wrong with the server: " << req.server_id << endl;
			return;
		}
		//Wait for ACK and then close the socket and end this journey
		boost::asio::streambuf response_ack;
		boost::asio::read_until(client_storage_server_socket, response_ack, "\r\n");
		istream ack_stream(&response_ack);
		string ack_string;
		ack_stream >> ack_string;

		cout << "ack_string:" << ack_string << endl;

		if (ack_string.find("update_done") < NO_SUCH_SUBSTRING)
		{
			boost::system::error_code ignored_ec;
			client_storage_server_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			client_storage_server_socket.close();
		}
		else
		{
			cout << "No ACK from the server !" << endl;
		}
#pragma endregion storage_phase
	}
	catch (exception& e)
	{
		print_info(int_to_string(client_id), "POST", req.uri, "Exception Occurs!");
		cout << "Exception :" << e.what() << endl;
	}
}
void client::launch_post_request(int thread_id)
{
	string local_path = get_random_file_name(thread_id);
	string remote_path = "I:/client_post_request_";
	remote_path += int_to_string(thread_id);
	remote_path += ".txt";

	request req;
	req.make_post_request(local_path, remote_path, client_id, target_port);
	try
	{
#pragma region proxy_phase
		tcp::endpoint end_p(boost::asio::ip::address_v4::from_string("127.0.0.1"), target_port);
		client_proxy_socket.connect(end_p);
		boost::asio::write(client_proxy_socket, req.header_to_buffers());

		//Read the location info from proxy server
		boost::asio::streambuf response;
		boost::asio::read_until(client_proxy_socket, response, "\r\n");
		istream temp_stream(&response);
		string temp_string;
		temp_stream >> temp_string;

		if (temp_string.find("server_id:") < NO_SUCH_SUBSTRING)
		{
			string temp_server_id = temp_string.substr(temp_string.find("server_id:") + 10, temp_string.find_first_of("\r\n", temp_string.find("server_id:")) - temp_string.find("server_id") - 10);
			req.server_id = atoi(temp_server_id.c_str());
		}

		boost::system::error_code ignored_ec;
		client_proxy_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		client_proxy_socket.close();
#pragma endregion proxy_phase

#pragma region storage_phase
		//Connect to the dedicated storage server using the server_id as the port
		tcp::endpoint end_point_storage(boost::asio::ip::address_v4::from_string("127.0.0.1"), req.server_id);
		client_storage_server_socket.connect(end_point_storage);

		//Set the size of buffer of socket
		boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
		client_storage_server_socket.set_option(option);

		boost::system::error_code storage_prepare_err_code;
		boost::asio::write(client_storage_server_socket, req.header_to_buffers(), storage_prepare_err_code);

		boost::asio::streambuf response_ready;
		boost::asio::read_until(client_storage_server_socket, response_ready, "\r\n");
		istream ready_stream(&response_ready);
		string ready_string;
		ready_stream >> ready_string;

		if (ready_string.find("ready_for_post") < NO_SUCH_SUBSTRING)
		{
			//Prepare to transmit the file
			boost::system::error_code ec;
			file_handler.assign(::CreateFile(local_path.c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
				OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0), ec);

			if (file_handler.is_open())
			{
				print_info(int_to_string(client_id), "POST", req.obj_id, "Open done!Now transmitting to server....");
				transmit_file(client_storage_server_socket, file_handler, boost::bind(&client::handle_write, this, req.obj_id, boost::asio::placeholders::error));
			}

			else
			{
				print_info(int_to_string(client_id), "POST", req.obj_id, "Error occurs when opening the file!");
				cout << ec.message() << endl;
				return;
			}
		}

		else
		{
			//Something wrong with the server
			boost::system::error_code ignored_ec;
			client_storage_server_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			client_storage_server_socket.close();
		}

		cout << "After the tramsmit and wait for the ack !" << endl;


		//Wait for ACK and then close the socket and end this journey
		boost::asio::streambuf response_ack;
		boost::asio::read_until(client_storage_server_socket, response_ack, "\r\n");
		istream ack_stream(&response_ack);
		string ack_string;
		ack_stream >> ack_string;
		if (ack_string.find("post_done") < NO_SUCH_SUBSTRING)
		{
			cout << "Confirmed that the server has done its job !!!! post done !!!" << endl;
			boost::system::error_code ignored_ec;
			client_storage_server_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			client_storage_server_socket.close();
		}
		else
		{
			cout << "No ACK from the server !" << endl;
		}
#pragma endregion storage_phase		
	}
	catch (exception& e)
	{
		print_info(int_to_string(client_id), "POST", req.obj_id, "Exception Occurs!");
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
	else if (request_type == UPDATE_REQUEST)
	{
		launch_update_request(thread_id);
		return;
	}
}

void client::handle_write(string uri, const boost::system::error_code& e)
{
	if (!e)
	{
		print_info(int_to_string(client_id), "POST", uri, e.message());
	}
	else
	{
		print_info(int_to_string(client_id), "POST", uri, "In handle_write and no error_code!");
	}
// 	boost::system::error_code ignored_ec;
// 	client_storage_server_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
// 	client_storage_server_socket.close();

	cout << "HANDLE WRITE DONE !!!!!!!!!" << endl;

}

