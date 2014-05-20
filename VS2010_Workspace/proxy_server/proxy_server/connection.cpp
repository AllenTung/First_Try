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


connection::connection(boost::asio::io_service& io_service, request_handler& handler, string run_mode)
	:socket_(io_service), request_handler_(handler),file_(io_service),mode(run_mode)
{
	busy = 1;
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

int connection::target_server_location(string obj_id)
{
	//Using consistent-hashing structure to organize the cluster
	int located_server = STARTING_SERVER_ID + get_hash_value(const_cast<char*>(obj_id.c_str()));
	return located_server;
}


void connection::handle_read(const boost::system::error_code& e, size_t bytes_transferred)
{
	if (!e)
	{
		boost::tribool result;
		//Parse the request_buffer received to request object
		result = request_parser_.simple_parse(request_, buffer_);
		cout << socket_.remote_endpoint().port() << endl;
		if (result)
		{
			if (request_.method == "GET")
			{
				try
				{
					reply_.server_id = target_server_location(request_.obj_id);
					if(mode == "test")
						reply_.server_id = STARTING_SERVER_ID;
					boost::system::error_code err_code;
					boost::asio::write(socket_, reply_.simple_location_buffers(), err_code);
					handle_write(err_code);
				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, request_.obj_id, e.what());
				} 
			}			
			else if (request_.method == "POST")
			{
				cout << "POST received !" << endl;
				try
				{
					reply_.server_id = target_server_location(request_.obj_id);
					if(mode == "test")
						reply_.server_id = STARTING_SERVER_ID;
					boost::system::error_code err_code;
					boost::asio::write(socket_, reply_.simple_location_buffers(), err_code);
					handle_write(err_code);
				}
				catch (exception& e)
				{
					print_info(request_.client_id, request_.method, request_.obj_id, e.what());
				} 
			}
			else if (request_.method == "UPDATE")
			{
				try
				{
					//The content-length here represents the original file's length but not the updated part
					//The updated part is directly contained in the request header
					//Third thing: the server_id begins at: 888 
					int starting_id = target_server_location(request_.obj_id);
					if(mode == "test")
					{
						starting_id = STARTING_SERVER_ID;
					}
					reply_.server_id = starting_id + request_.update_offset / (request_.content_length / ERASURE_CODE_K) + 1;

					// Round circle calculation
					if (reply_.server_id > STARTING_SERVER_ID + NUMBER_OF_SERVER - 1)
					{
						reply_.server_id -= NUMBER_OF_SERVER;
					}

					boost::system::error_code err_code;
					boost::asio::write(socket_, reply_.simple_location_buffers(), err_code);
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
}