#pragma once
#include <stdlib.h>
#include <iostream>
#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include "connection_manager.hpp"
#include "request_handler.hpp"

using namespace std;
using boost::asio::ip::tcp;


connection::connection(boost::asio::io_service& io_service, request_handler& handler, int s_id)
	:socket_(io_service),ec_io_service(ERASURE_CODE_K+ERASURE_CODE_M), request_handler_(handler),file_(io_service),encoder_(),decoder_()
{
	server_id = s_id;
	busy = 1;
	for (int i = 0; i < ERASURE_CODE_K + ERASURE_CODE_M; i++)
	{
		socket_ptr temp_socket(new tcp::socket(ec_io_service.get_ec_io_service()));
		ec_socket.push_back(temp_socket);
	}
}

// connection::connection(boost::asio::io_service& io_service, const string& filename)
// 	: socket_(io_service), file(io_service), file_name(filename)
// {
// 	cout << "The connection constructor for transmit file is triggered !\n";
// }


//从start函数分开处理，这里为了简化就默认写成了handle_read了。
void connection::start()
{
	socket_.async_read_some(boost::asio::buffer(buffer_),
		boost::bind(&connection::handle_read, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void connection::stop()
{
	socket_.close();
}

void connection::reset_all()
{
	busy = 1;

}
void connection::handle_read(const boost::system::error_code& e, size_t bytes_transferred)
{
	if (!e)
	{
		boost::tribool result;

		//Parse the request_buffer received to request object
		result = request_parser_.simple_parse(request_, buffer_);

		if (result)
		{
			//这里要分开处理，分别添加handle_get_request，即原来的handle_get_request，
			//然后要有handle_post_request，用来处理用户发来的POST请求也就是上传文件
			//然后要有handle_update_request, 用来处理用户发来的update请求也就是从指定的offset开始写入部分内容
			if (request_.method == "GET")
			{
				boost::system::error_code ec;
				file_.assign(::CreateFile((request_.obj_id).c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
					OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0), ec);
				if (file_.is_open())
				{
					print_info(request_.client_id, request_.method, request_.obj_id, "Open done!Now transmitting to client....");
					transmit_file(socket_, file_, boost::bind(&connection::handle_write, this, boost::asio::placeholders::error));
				}

				else
				{
					print_info(request_.client_id, request_.method, request_.obj_id, "Error occurs when opening the file!");
					handle_write(ec);
					return;
				}
			}
			else if (request_.method == "UPDATE")
			{

			}
			else if (request_.method == "TRANSMIT_DATA_BLOCK" || request_.method == "TRANSMIT_PARITY_BLOCK")
			{
				//Here's the handle process of receiving the data block or parity block
				//Eliminate the reply for brevity, for now



			}
			else if (request_.method == "POST")
			{
				request_handler_.handle_post_request(request_, reply_);
				boost::system::error_code err_code;
				boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code);
				// 				boost::asio::async_write(socket_, reply_.to_buffers(),
				// 					boost::bind(&connection::handle_write, shared_from_this(), 
				// 					boost::asio::placeholders::error));

				//After a faked response , prepare the socket for the transmition from the client
				boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
				socket_.set_option(option);

				try
				{
					int forcin = 0;
					ofstream output_for_trans((request_.obj_id).c_str(), ios::app | ios::binary);

					char* for_recv = new char[RECEIVE_BUFFER_SIZE];

					int per_receive = 0;
					int tmp_count = 0;

					for(;;)
					{
						per_receive = recv(socket_.native_handle(), for_recv, RECEIVE_BUFFER_SIZE, 0);
						cout << "******************client: " << request_.client_id << "  *****per_receive: " << per_receive << "********" << endl;
						
						if (per_receive <= 0)
						{
/*							print_info(request_.client_id, request_.method, request_.obj_id, "This time got nothing and stop!");*/

							//处理刚好上一次读取完了所有的内容的情况
							//若tmp_count还为0，那么就是出现了异常
							if (tmp_count > 0)
							{
								print_info(request_.client_id, request_.method, request_.obj_id, "This process finished successfully!");
							}
							break;
						}
						tmp_count += per_receive;

						output_for_trans.write(for_recv, per_receive);
						output_for_trans.flush();

						

						if (tmp_count >= 1024*1024*200)
						{
							//200M for test is enough                         
							print_info(request_.client_id, request_.method, request_.obj_id, "200M works ! This process finished successfully!");
							break;
						}
					}

					output_for_trans.close();
					delete []for_recv;


					/******************************************************
					              Endoing process activated
					*******************************************************/
                    int ec_done = encoder_.encode_file(ec_io_service, ec_socket, request_, server_id);

					// Not sure if it's the right place !!!!!!!!!!!!!!
					ec_io_service.run();
					// Not sure if it's the right place !!!!!!!!!!!!!!
					//Just for the close of connection
					handle_write(err_code);

				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, request_.obj_id, e.what());
				} 
			}
		}
	}

	else 
	{       
		cout << e.message() << endl;

	}
}

void connection::handle_write(const boost::system::error_code& e)
{
	if (!e)
	{
		print_info(request_.client_id, request_.method, request_.uri, "Service ends for above circumstance!");
	}
	else
	{
		print_info(request_.client_id, request_.method, request_.uri, "In method handle_write and the error code is null");
	}

	//Initiate graceful connection closure
	boost::system::error_code ignored_ec;
	file_.close();
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
	socket_.close();
	
	busy = 0;
}