#pragma once
#include <stdlib.h>
#include <iostream>
#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "server.hpp"

using namespace std;
using boost::asio::ip::tcp;

class server;

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

			if (request_.method == GET_REQUEST)
			{
				string pure_obj_name = extract_pure_obj_name(request_.obj_id);
				map<string, metadata>::iterator it = server::obj_meta_table.find(pure_obj_name);
				if (it!= server::obj_meta_table.end())
				{
					cout << "***************   File found !!!!!!  ****************** " << endl;

					string full_local_path = return_full_path(pure_obj_name);
					boost::system::error_code ec;
					file_.assign(::CreateFile(full_local_path.c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
						OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0), ec);
					if (file_.is_open())
					{
						print_info(request_.client_id, request_.method, pure_obj_name, "Open done!Now transmitting to client....");
						transmit_file(socket_, file_, boost::bind(&connection::handle_write, this, boost::asio::placeholders::error));
					}
					else
					{
						print_info(request_.client_id, request_.method, pure_obj_name, "Error occurs when opening the file!");
						handle_write(ec);
						return;
					}
				}

				else
				{
					//Object doesn't exist, quit
					cout << pure_obj_name << " DOSEN'T EXIST !!!!" << endl;
					boost::system::error_code ec;
					handle_write(ec);
				}
			}
#pragma endregion get_request

#pragma region update_request

			/* 对于一个object的分块和校验块，命名都跟master node上的一样的，但是在data type里面有所不同
			   因此update的操作的时候，client肯定也是以原文件的名字来发送update request，然后在server端的话，也是用这个来在obj_meta_table里面进行查找*/


			else if (request_.method == UPDATE_REQUEST)
			{
				//request_handler_.handle_post_request(request_, reply_);
				string pure_obj_name = extract_pure_obj_name(request_.obj_id);
				string full_local_path = return_full_path(pure_obj_name);
				
				boost::system::error_code err_code;
				reply_.server_status = READY_FOR_UPDATE_STATUS;
				boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code);

				boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
				socket_.set_option(option);

				//Updated content is all that the strembuf contains
				boost::asio::streambuf update_buf;
				boost::asio::read_until(socket_, update_buf, "\r\n");
				istream update_stream(&update_buf);
				string update_content;
				update_stream >> update_content;
				request_.content = update_content;

				int done = -1;
				try
				{
					
					map<string, metadata>::iterator obj_it = server::obj_meta_table.find(pure_obj_name);
					if (obj_it != server::obj_meta_table.end())
					{
						vector<Data_Type> data_type_vector(types, types + ERASURE_CODE_K + ERASURE_CODE_M + 1);
						vector<Data_Type>::iterator data_it = find(data_type_vector.begin(), data_type_vector.end(), obj_it->second.data_type);
						unsigned int real_offset = request_.update_offset - ((*data_it) - 1) * (obj_it->second.content_length);

						done = update_file(full_local_path, update_content, real_offset, 0);
					}

					if (done == 1)
					{
						//Just for the close of connection
						boost::system::error_code err_code_done;
						reply_.server_status = UPDATE_DONE_STATUS;
						boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code_done);

						//Record the update client and according timestamp
						map<string ,metadata>::iterator it = server::obj_meta_table.find(pure_obj_name);
						if(it != server::obj_meta_table.end())
						{
							it->second.insert_new_record(request_.client_id, request_.request_timestamp, server_id, false);
						}

						//Next,send the delta-update part to server holding the parity block, and a heads-up to the master node
						//Hence the total number of request is M+1

						int enough_for_rec = 0;

						for (int i = 1; i <= ERASURE_CODE_M + 1 ; i++)
						{

							//Connect the next targeted server and send a transmit_update_content or transmit_delta_content request
							int next_target_server = -1;
							if(i == 1)
							{
								next_target_server = it->second.full_copy_location;
							}
							else if(i == 2)
							{
								next_target_server = it->second.first_parity_location;
							}
							else if(i == 3)
							{
								next_target_server = it->second.second_parity_location;
							}

							tcp::endpoint end_p(boost::asio::ip::address_v4::from_string("127.0.0.1"), next_target_server);
							ec_socket.at(i - 1)->connect(end_p);

							request transmit_data_request(request_);

							//Set obj_id directly to pure form.
							transmit_data_request.method = TRANSMIT_UPDATE_CONTENT_REQUEST;
							if (i > 1)
							{
								transmit_data_request.method = TRANSMIT_DELTA_CONTENT_REQUEST;
							}
							
							transmit_data_request.content_length = update_content.length();
							transmit_data_request.obj_id = pure_obj_name;

							//This is used to inform parity and master node the original target node of this update operation
							transmit_data_request.server_id = server_id;

							boost::asio::write(*(ec_socket.at(i - 1)), transmit_data_request.header_to_buffers());	

							//Confirm the ready_for_update and send again if no confirmation is found
							for(;;)
							{
								boost::asio::streambuf tmp_response;
								boost::asio::read_until(*(ec_socket.at(i - 1)), tmp_response, "\r\n");
								istream update_stream(&tmp_response);
								string response_string;
								update_stream >> response_string;

								if(response_string.find(READY_FOR_UPDATE_STATUS) < NO_SUCH_SUBSTRING)
								{
									cout << "***************************************" << endl;
									cout << "Server: " << next_target_server << "is totally ready !!!!!" << endl;
									cout << "***************************************" << endl;
									break;
								}

								else if(response_string.find(TRY_AGAIN_STATUS) < NO_SUCH_SUBSTRING)
								{
									cout << "***************************************" << endl;
									cout << "Server: " << next_target_server << "is somehow blocked !!!!!" << endl;
									cout << "***************************************" << endl;

									boost::asio::write(*(ec_socket.at(i - 1)), transmit_data_request.header_to_buffers());	

								}
							}

							//Cause there's no need to transmit the actual content to master node
							if (i > 1)
							{
								boost::system::error_code execution_err_code;
								boost::asio::write(*(ec_socket.at(i - 1)), transmit_data_request.content_to_buffers(), execution_err_code);

								//Wait for ACK and then close the socket and end this journey
								boost::asio::streambuf response_ack;
								boost::asio::read_until(*(ec_socket.at(i - 1)), response_ack, "\r\n");
								istream ack_stream(&response_ack);
								string ack_string;
								ack_stream >> ack_string;

								if (ack_string.find(UPDATE_DONE_STATUS) < NO_SUCH_SUBSTRING)
								{
									boost::system::error_code ignored_ec;
									ec_socket.at(i - 1)->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
									ec_socket.at(i - 1)->close();

									//Accumulate the ack for assuring reconstruction
									enough_for_rec ++;
								}
								else
								{
									cout << "No ACK from the server !" << endl;
								}
							}

							//This target node has received enough ack to ask the master node to reconstruct
							//After finish writing this reply, ec_sockets could be closed and just wait for the master node to re-initiate a 
							//reconstruct request, to all accountable nodes.
							if (enough_for_rec >= ERASURE_CODE_M)
							{
								boost::system::error_code err_code_done;
								reply_.server_status = UPDATE_DONE_STATUS;
								boost::asio::write(*(ec_socket.at(0)), reply_.simple_ready_buffers(), err_code_done);
							}


						}
						after_ec = 1;						
						handle_write(err_code);
					}
					else 
					{
						cout << "Error occurs when update the file : " << pure_obj_name << endl;
					}

				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, pure_obj_name, e.what());
				} 

			}
#pragma endregion update_request

#pragma region transmit_delta_content

			else if (request_.method == TRANSMIT_DELTA_CONTENT_REQUEST)
			{
				//This type of request indicates this server contains the according request object's parity
				string pure_obj_name = extract_pure_obj_name(request_.obj_id);
				string full_local_path = return_update_path(pure_obj_name);

				boost::system::error_code err_code;
				reply_.server_status = READY_FOR_UPDATE_STATUS;
				boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code);

				boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
				socket_.set_option(option);

				//Updated content is all that the streambuf contains
				boost::asio::streambuf update_buf;
				boost::asio::read_until(socket_, update_buf, "\r\n");
				istream update_stream(&update_buf);
				string update_content;
				update_stream >> update_content;
				
				try
				{
					int forcin = 0;
					ofstream output_for_trans(full_local_path.c_str(), ios::app | ios::binary);
					output_for_trans.write(update_content.c_str(), update_content.length());
					output_for_trans.flush();
					output_for_trans.close();
					
					cout << "Post finished , about to update the record!" << endl;
					
					//Update the record

					map<string ,metadata>::iterator it = server::obj_meta_table.find(pure_obj_name);
					if(it != server::obj_meta_table.end())
					{
						//First it's dirty, only when the master node inform of the finish of recon. , it turns clean
						it->second.insert_new_record(request_.client_id, request_.request_timestamp, request_.server_id, false);
					}
					
					//Just for the close of connection
					boost::system::error_code err_code_done;
					reply_.server_status = UPDATE_DONE_STATUS;
					boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code_done);

					//After sending ack to inform the target node that dirty version of delta has been stored
					//The socket between these two guys could be closed for the target node could use this very ack to be the 
					//sufficient condition to send an Ultimate ACK to according master node
					handle_write(err_code);
				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, request_.obj_id, e.what());
				} 

			}
#pragma endregion transmit_delta_content

#pragma region transmit_update_content

			else if (request_.method == TRANSMIT_UPDATE_CONTENT_REQUEST)
			{
				//This type of request indicates this server contains the full copy of the according object
				//It would first perceives a "transmit update content" request
				//Then, it returns a status of ready for update
				//Next, wait for a ack which is shown in "update done" server status
				//Finally, it calls for a request of reconstruction, which is "RECONSTRUCT"



				//For now, client_id and request_timestamp are simply used for id the newest version
				//Cause one client is not able to launch mutiple update to the same object without confirming the last one
				string pure_obj_name = extract_pure_obj_name(request_.obj_id);
				string full_local_path = return_update_path(pure_obj_name);

				version newest_version(request_.client_id, request_.request_timestamp, request_.server_id, false);

				boost::system::error_code err_code;
				reply_.server_status = READY_FOR_UPDATE_STATUS;
				boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code);

				boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
				socket_.set_option(option);

				//Wait for the target node to send ultimate ack
				boost::asio::streambuf response_buf;
				boost::asio::read_until(socket_, response_buf, "\r\n");
				istream response_stream(&response_buf);
				string response_string;
				response_stream >> response_string;

				try
				{
					int forcin = 0;

					//Receive the ultimate ack from the target node, which means the update content and delta content are all in place
					//Then launch the reconstruct request
					if (response_string.find(UPDATE_DONE_STATUS) < NO_SUCH_SUBSTRING)
					{
						//Connect those guys
						for (int i = 1; i <= ERASURE_CODE_K + ERASURE_CODE_M ; i++)
						{

							//Connect the next targeted server and send a transmit_data request
							int next_target_server = server_id + i;
							// Round circle calculation
							if (next_target_server > STARTING_SERVER_ID + NUMBER_OF_SERVER - 1)
							{
								next_target_server -= NUMBER_OF_SERVER;
							}

							tcp::endpoint end_p(boost::asio::ip::address_v4::from_string("127.0.0.1"), next_target_server);
							ec_socket.at(i - 1)->connect(end_p);

							request recon_request;
							
							//Use the info from the newest version: client, request timestamp, 
							recon_request.method = RECONSTRUCT_REQUEST;
							recon_request.obj_id = pure_obj_name;
							recon_request.server_id = newest_version.target_server_id;
							recon_request.client_id = newest_version.client_id;
							recon_request.request_timestamp = newest_version.request_timestamp;

							boost::system::error_code err_code;
							boost::asio::write(*(ec_socket.at(i - 1)), recon_request.header_to_buffers(), err_code);		
						}
					}

					boost::system::error_code err_code_done;
					reply_.server_status = UPDATE_DONE_STATUS;
					boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code_done);
					handle_write(err_code);
				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, request_.obj_id, e.what());
				} 

			}
#pragma endregion transmit_update_content

#pragma region transmit_request

			else if (request_.method == TRANSMIT_DATA_BLOCK_REQUEST || request_.method == TRANSMIT_PARITY_BLOCK_REQUEST)
			{
				//Here's the handle process of receiving the data block or parity block
				//Reply seems could not be eliminated

				cout << "In handle transmit data block : Dealing with the block with size of : " << request_.content_length << "B ***" << endl;

				request_handler_.handle_transmit_block_request(request_, reply_);

				//Master node uses pure object name to construct the request, no extraction is needed
				string pure_obj_name = request_.obj_id;
				string full_local_path = return_full_path(pure_obj_name);

				//Send back the ready status
				reply_.server_status = READY_FOR_POST_STATUS;
				boost::system::error_code err_code;
				boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code);
				boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
				socket_.set_option(option);

				try
				{
					int forcin = 0;
					ofstream output_for_trans(full_local_path.c_str(), ios::app | ios::binary);
					char* for_recv = new char[RECEIVE_BUFFER_SIZE];
					int per_receive = 0;
					int tmp_count = 0;

					for(;;)
					{
						cout << "In the receiving loop !" << endl;
						//per_receive = boost::asio::read(socket_, boost::asio::buffer(for_recv, RECEIVE_BUFFER_SIZE));
						//per_receive = boost::asio::read(socket_, for_recv, err_code);
						per_receive = recv(socket_.native_handle(), for_recv, RECEIVE_BUFFER_SIZE, 0);
						cout << "*****per_receive: " << per_receive << "B ********" << endl;
						
						if (per_receive <= 0)
						{
/*							print_info(request_.client_id, request_.method, request_.obj_id, "This time got nothing and stop!");*/

							//处理刚好上一次读取完了所有的内容的情况
							//若tmp_count还为0，那么就是出现了异常
							if (tmp_count > 0)
							{
								cout << pure_obj_name << "::::::::Per_receive is ZERO !!! Receiving quits !!!Total amount: " << tmp_count << endl;
								break;
							}
							
						}
						tmp_count += per_receive;

						output_for_trans.write(for_recv, per_receive);
						output_for_trans.flush();
											
						if (tmp_count >= request_.content_length)
						{                       
							cout << pure_obj_name << "::::::::Total amount is equal to content-length : " << tmp_count << endl;
							break;
						}
					}

					cout << "About to insert into the obj_meta table:" << pure_obj_name << endl;

					output_for_trans.close();
					delete []for_recv;

					//Insert record: and the id of full copy server is the "server_id" property coming along with  request

// 					vector<Data_Type> data_type_vector(types, types + ERASURE_CODE_K + ERASURE_CODE_M + 1);
// 					vector<Data_Type>::iterator data_it = find(data_type_vector.begin(), data_type_vector.end(), request_.data_type);
// 
					int temp_first = request_.server_id + ERASURE_CODE_K + 1;
					int temp_second = temp_first + 1;
					

					if (temp_first > STARTING_SERVER_ID + NUMBER_OF_SERVER - 1)
					{
						temp_first -= NUMBER_OF_SERVER;
					}

					if (temp_second > STARTING_SERVER_ID + NUMBER_OF_SERVER - 1)
					{
						temp_second -= NUMBER_OF_SERVER;
					}

					cout << "temp first: " << temp_first << endl << "temp_second: " << temp_second << endl;

					pair<map<string, metadata>::iterator, bool> test_pair;
					metadata obj_meta(request_.content_length, request_.data_type, request_.client_id, request_.request_timestamp, true, request_.server_id, temp_first, temp_second);
					test_pair = server::obj_meta_table.insert(make_pair(pure_obj_name, obj_meta));

					if(test_pair.second == true)
					{
						cout << "Successfully inserted the value :" << test_pair.first->first << "   Data type : " << request_.data_type << endl;
					}
					else 
					{
						cout << "Insertion failed!" << endl;
					}

					handle_write(err_code);
				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, pure_obj_name, e.what());
				} 
			}
#pragma endregion transmit_request

#pragma region post_request
			else if (request_.method == "POST")
			{

				request_handler_.handle_post_request(request_, reply_);
				boost::system::error_code err_code;
				reply_.server_status = READY_FOR_POST_STATUS;
				boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code);

				//After ready response , prepare the socket for the transmition from the client
				boost::asio::socket_base::receive_buffer_size option(RECEIVE_BUFFER_SIZE);
				socket_.set_option(option);

				string pure_obj_name = extract_pure_obj_name(request_.obj_id);
				string full_local_path = return_full_path(pure_obj_name);

				try
				{
					int forcin = 0;
					ofstream output_for_trans(full_local_path.c_str(), ios::app | ios::binary);

					char* for_recv = new char[RECEIVE_BUFFER_SIZE];
					unsigned int per_receive = 0;
					unsigned int tmp_count = 0;

					for(;;)
					{
						//per_receive = boost::asio::read(socket_, boost::asio::buffer(for_recv, RECEIVE_BUFFER_SIZE));
						per_receive = (unsigned int)recv(socket_.native_handle(), for_recv, RECEIVE_BUFFER_SIZE, 0);

						cout << "Per-receive :" << per_receive << " B *********** " << endl;

						tmp_count += per_receive;

						output_for_trans.write(for_recv, per_receive);
						output_for_trans.flush();

						if (tmp_count >= request_.content_length)
						{
							print_info(request_.client_id, request_.method, pure_obj_name, "This process finished successfully!");
							break;
						}
					}
					output_for_trans.close();
					delete []for_recv;

					cout << "Post finished , about to insert the record!" << endl;

					//Insert the record
					int temp_first = server_id + ERASURE_CODE_K + 1;
					int temp_second = temp_first + 1;

					if (temp_first > STARTING_SERVER_ID + NUMBER_OF_SERVER - 1)
					{
						temp_first -= NUMBER_OF_SERVER;
					}

					if (temp_second > STARTING_SERVER_ID + NUMBER_OF_SERVER - 1)
					{
						temp_second -= NUMBER_OF_SERVER;
					}

					cout << endl << "temp_first:" << temp_first << endl << "temp_second:" << temp_second << endl;

					pair<map<string, metadata>::iterator, bool> test_pair;
					metadata obj_meta(request_.content_length, full_copy, request_.client_id, request_.request_timestamp, true, server_id, temp_first, temp_second);
					test_pair = server::obj_meta_table.insert(make_pair(pure_obj_name, obj_meta));

					if(test_pair.second == true) 
					{
						cout << "Successfully inserted the value :" << test_pair.first->first<< endl;
					}
					else 
					{
						cout << "Insertion failed!" << endl;
					}


					/******************************************************
					              Endoing process activated
					*******************************************************/
                    cout << "Now server : " << server_id << " begins to do the erasure coding part !" << endl;
					int ec_done = encoder_.encode_file(ec_io_service, ec_socket, request_, server_id, pure_obj_name, full_local_path);
					after_ec = 1;
					cout << "Now behind the function of encode_file !" << endl;
					
					//Just for the close of connection
					boost::system::error_code err_code_done;
					reply_.server_status = POST_DONE_STATUS;
					boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code_done);
					handle_write(err_code_done);
				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, request_.obj_id, e.what());
				} 
			}
#pragma endregion post_request

#pragma region reconstruct_request
			if (request_.method == RECONSTRUCT_REQUEST)
			{
				string pure_obj_name = extract_pure_obj_name(request_.obj_id);
				map<string, metadata>::iterator it = server::obj_meta_table.find(pure_obj_name);
				if (it!= server::obj_meta_table.end())
				{
					//If this server is exactly the target server of this request, sth should be different.
					string full_local_path = return_full_path(pure_obj_name);
					boost::system::error_code ec;
					file_.assign(::CreateFile(full_local_path.c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
						OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0), ec);
					if (file_.is_open())
					{
						print_info(request_.client_id, request_.method, pure_obj_name, "Open done!Now transmitting to client....");
						transmit_file(socket_, file_, boost::bind(&connection::handle_write, this, boost::asio::placeholders::error));
					}
					else
					{
						print_info(request_.client_id, request_.method, pure_obj_name, "Error occurs when opening the file!");
						handle_write(ec);
						return;
					}
				}

				else
				{
					//If,obj could not be found, also return a special reply
					//Giving the name "object_not_found"
					//Just for the close of connection

					cout << "Object not found: " << request_.obj_id << endl;

					boost::system::error_code err_code_done;
					reply_.server_status = OBJECT_NOT_FOUND_STATUS;
					boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code_done);
					handle_write(err_code_done);
				}
			}
#pragma endregion reconstruct_request
		}
	}

	else 
	{       
		cout << "Not entered into the handle-read zone because of :" << e.message() << endl;
		if (e.message().find(END_OF_FILE_EXCEPTION) < NO_SUCH_SUBSTRING)
		{
			boost::system::error_code err_code_done;
			reply_.server_status = TRY_AGAIN_STATUS;
			boost::asio::write(socket_, reply_.simple_ready_buffers(), err_code_done);
			start();
		}
		else
		{
			//Do something
		}
	}
}

void connection::handle_write(const boost::system::error_code& e)
{
	if (!e)
	{
		print_info(request_.client_id, request_.method, request_.obj_id, "Service ends for above circumstance!");
	}
	else
	{
		print_info(request_.client_id, request_.method, request_.obj_id, "In method handle_write and the error code is null");
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