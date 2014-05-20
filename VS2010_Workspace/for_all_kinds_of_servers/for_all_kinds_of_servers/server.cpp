#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <signal.h>
#include "server.hpp"

using namespace std;
using boost::asio::ip::tcp;

boost::mutex server::cout_lock;
boost::mutex server::file_lock;
boost::mutex server::table_lock;
boost::mutex server::con_deque_lock;
boost::mutex server::ec_port_lock;

server::server(string address, string port,
	const string& doc_root, int thread_num, int s_id)
	:io_pool(thread_num),
	signals_(io_pool.get_io_service()),
	acceptor_(io_pool.get_io_service()),
	inner_acceptor_(io_pool.get_io_service()),
	connection_manager_(),
	new_connection_(),
	request_handler_(doc_root, s_id)
{
	server_id = s_id;
	local_ip = address;
	inner_server_id = server_id + 1000;
	signals_.add(SIGINT);
	signals_.add(SIGTERM);
#if defined(SIGQUIT)
	signals_.add(SIGQUIT);
#endif
	signals_.async_wait(boost::bind(&server::handle_stop, this));

	//open the acceptor with the option to reuse the address
	tcp::resolver resolver(acceptor_.get_io_service());
	tcp::resolver::query query(address.c_str(), port.c_str());
	tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

// 	tcp::resolver inner_resolver(inner_acceptor_.get_io_service());
// 	const string inner_port = int_to_string(inner_server_id).c_str();
// 	tcp::resolver::query inner_query(address, inner_port);
// 	tcp::endpoint inner_endpoint = *inner_resolver.resolve(inner_query);
// 	inner_acceptor_.open(inner_endpoint.protocol());
// 	inner_acceptor_.set_option(tcp::acceptor::reuse_address(true));
// 	inner_acceptor_.bind(inner_endpoint);
// 	inner_acceptor_.listen();

	cout << "Server: " << local_ip << " :" << server_id << " is running!" << endl;
	start_accept();
}
void server::run()
{
	io_pool.run();
}

int server::choose_connection()
{
	int free_connectoin = -1;

	for(int i = 0; i < connection_pool.size(); i++)
	{
		if (connection_pool.at(i)->busy == 0)
		{
			free_connectoin = i;
		}		
	}

	//If all are occupied ,then -1 is returned
	return free_connectoin;
}

void server::handle_accept(connection_ptr& current_connection, boost::system::error_code& e)
{
	if (!e)
	{
		cout << "NEW REQUEST dealed by the port: " << current_connection->socket_.local_endpoint() << endl;
		current_connection->start();
	}
	start_accept();
}


void server::start_accept()
{
	boost::system::error_code err_code;
// #pragma region conn_pool_method
// 	int pick_conn = choose_connection();
// 	if(pick_conn >= 0)
// 	{
// 		acceptor_.async_accept(connection_pool.at(pick_conn)->socket_, boost::bind(&server::handle_accept, this, connection_pool.at(pick_conn), err_code));
// 	}
// 	else if (pick_conn == -1)
// 	{
// 		connection_ptr  new_conn(new connection(io_pool.get_io_service(), request_handler_, server_id));
// 		connection_pool.push_back(new_conn);
// 		int conn_count =  connection_pool.size();
// 		//For the surveillance of the size of connection_pool
// 		cout << "\n*************************\n" << conn_count << "\n*************************\n";
// 
// 		/*	new_connection_.reset(new connection(io_pool.get_io_service(), request_handler_, server_id));*/
// 		acceptor_.async_accept(new_conn->socket_, boost::bind(&server::handle_accept, this, new_conn, err_code));
// 	}
// #pragma endregion conn_pool_method
	             
// #pragma region reset_conn_method
// 	new_connection_.reset(new connection(io_pool.get_io_service(), request_handler_, server_id));
// 	acceptor_.async_accept(new_connection_->socket_, boost::bind(&server::handle_accept, this, new_connection_, err_code));
// #pragma endregion reset_conn_method

#pragma region soly_create_conn
	connection_ptr  new_conn(new connection(io_pool.get_io_service(), request_handler_, server_id, local_ip));
	connection_pool.push_back(new_conn);

	/* Eliminate those used connections */
// 	if(connection_pool.front()->busy == 0)
// 	{
// 		connection_pool.pop_front();
// 	}	

	acceptor_.async_accept(new_conn->socket_, boost::bind(&server::handle_accept, this, new_conn, err_code));
/*	inner_acceptor_.async_accept(new_conn->socket_, boost::bind(&server::handle_accept, this, new_conn, err_code));*/
#pragma endregion soly_create_conn

}


void server::handle_stop()
{
	io_pool.stop();
}

