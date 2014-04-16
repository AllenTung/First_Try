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
	after_ec = 0;
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

#pragma region get_request

			if (request_.method == "GET")
			{
				//暂时一收到get请求就发送文件过去

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
#pragma endregion get_request
#pragma region update_request

			else if (request_.method == "UPDATE")
			{
				//request_handler_.handle_post_request(request_, reply_);
				boost::system::error_code err_code;

				reply_.server_status = "ready_for_update";
				boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code);

				boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
				socket_.set_option(option);

				//Updated content is all that the strembuf contains
				boost::asio::streambuf update_buf;
				boost::asio::read_until(socket_, update_buf, "\r\n");
				istream update_stream(&update_buf);
				string update_content;
				update_stream >> update_content;

				try
				{
					int done = update_file(request_.obj_id, update_content, request_.update_offset);
					ofstream output_for_trans((request_.obj_id).c_str(), ios::app | ios::binary);

					if (done == 1)
					{
						//Just for the close of connection
						boost::system::error_code err_code_done;
						reply_.server_status = "update_done";
						boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code_done);
						handle_write(err_code);
					}
					else 
					{
						cout << "Error occurs when update the file : " << request_.obj_id << endl;
					}

				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, request_.obj_id, e.what());
				} 

			}
#pragma endregion update_request
#pragma region transmit_request

			else if (request_.method == "TRANSMIT_DATA_BLOCK" || request_.method == "TRANSMIT_PARITY_BLOCK")
			{
				//Here's the handle process of receiving the data block or parity block
				//Reply seems could not be eliminated

				cout << "In handle transmit data block : Dealing with the block with size of : " << request_.content_length << "B ***" << endl;

				request_handler_.handle_transmit_block_request(request_, reply_);
				
				reply_.server_status = "ready_for_post";
				boost::system::error_code err_code;
				boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code);
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
						cout << "In the receiving loop !" << endl;
						per_receive = boost::asio::read(socket_, boost::asio::buffer(for_recv, RECEIVE_BUFFER_SIZE));
						//per_receive = boost::asio::read(socket_, for_recv, err_code);
						//per_receive = recv(socket_.native_handle(), for_recv, RECEIVE_BUFFER_SIZE, 0);
						cout << "*****per_receive: " << per_receive << "B ********" << endl;
						
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
											
						if (tmp_count >= request_.content_length)
						{
							//200M for test is enough                         
							print_info(request_.client_id, request_.method, request_.obj_id, "It works ! This process finished successfully!");
							break;
						}
					}

					cout << "Reception of data or parity block done!" << endl;

					output_for_trans.close();
					delete []for_recv;
					handle_write(err_code);
				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, request_.obj_id, e.what());
				} 
			}
#pragma endregion transmit_request
#pragma region post_request
			else if (request_.method == "POST")
			{
				request_handler_.handle_post_request(request_, reply_);
				boost::system::error_code err_code;

				reply_.server_status = "ready_for_post";
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
					char* for_recv_tiny = new char[32];
					int per_receive = 0;
					int tmp_count = 0;

					for(;;)
					{
						per_receive = recv(socket_.native_handle(), for_recv, RECEIVE_BUFFER_SIZE, 0);						
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
					}

					output_for_trans.close();
					delete []for_recv;
					//ec_io_service.run();
					/******************************************************
					              Endoing process activated
					*******************************************************/
                    cout << "Now server : " << server_id << " begins to do the erasure coding part !" << endl;
					int ec_done = encoder_.encode_file(ec_io_service, ec_socket, request_, server_id);
					after_ec = 1;
					cout << "Now behind the function of encode_file !" << endl;
					
					//Just for the close of connection
					boost::system::error_code err_code_done;
					reply_.server_status = "post_done";
					boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code_done);
					handle_write(err_code);
				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, request_.obj_id, e.what());
				} 
			}
#pragma endregion post_request
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

	//Release those erasure-coded sockets as well
	if (after_ec == 1)
	{
		for (int i = 0; i < ec_socket.size(); i++)
		{
			ec_socket.at(i)->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			ec_socket.at(i)->close();
		}

		after_ec = 0;
	}

	cout << "At the end of this server's handle_write !" << endl;
	//Reset the overall status to busy
	busy = 0;
}