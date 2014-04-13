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

server::server(const string& address, const string& port,
	const string& doc_root, int thread_num, int s_id)
	:io_pool(thread_num),
	signals_(io_pool.get_io_service()),
	acceptor_(io_pool.get_io_service()),
	connection_manager_(),
	new_connection_(),
	request_handler_(doc_root, s_id)
{
	//注册以便去操作那个提示server什么时候该退出的信号
	//在同一个程序里面多次注册同一个信号是安全的，只要是通过asio来搞的
	server_id = s_id;

	signals_.add(SIGINT);
	signals_.add(SIGTERM);
#if defined(SIGQUIT)
	signals_.add(SIGQUIT);
#endif
	signals_.async_wait(boost::bind(&server::handle_stop, this));

	//open the acceptor with the option to reuse the address
	tcp::resolver resolver(acceptor_.get_io_service());
	tcp::resolver::query query(address, port);
	tcp::endpoint endpoint = *resolver.resolve(query);

	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	cout << "server: " << server_id << " is running!" << endl;
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

void server::start_accept()
{
	//每次生成一个新的connection，然后来等，用new_connection这个指针来指向它

	int pick_conn = choose_connection();

	if(pick_conn >= 0)
	{
		acceptor_.async_accept(connection_pool.at(pick_conn)->socket_, boost::bind(&server::handle_accept, this, connection_pool.at(pick_conn), boost::asio::placeholders::error));
	}
	else if (pick_conn == -1)
	{
		connection_ptr  new_conn(new connection(io_pool.get_io_service(), request_handler_, server_id));
		connection_pool.push_back(new_conn);
		int conn_count =  connection_pool.size();
		//For the surveillance of the size of connection_pool
		cout << "\n*************************\n" << conn_count << "\n*************************\n";

		/*	new_connection_.reset(new connection(io_pool.get_io_service(), request_handler_, server_id));*/
		acceptor_.async_accept(new_conn->socket_, boost::bind(&server::handle_accept, this, new_conn, boost::asio::placeholders::error));
	}                      
}

void server::handle_accept(connection_ptr& current_connection, const boost::system::error_code& e)
{
	if (!e)
	{
		cout << "Begin to handle one request !" << endl;
		current_connection->start();
		/*start_handle(current_connection);*/
	}
	start_accept();
}

void server::handle_stop()
{
	io_pool.stop();
}