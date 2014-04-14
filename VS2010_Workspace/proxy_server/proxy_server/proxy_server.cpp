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
#include "proxy_server.hpp"

using namespace std;
using boost::asio::ip::tcp;

proxy_server::proxy_server(const string& address, const string& port,
	const string& doc_root, int thread_num, int s_id)
	:io_pool(thread_num),
	signals_(io_pool.get_io_service()),
	acceptor_(io_pool.get_io_service()),
	connection_manager_(),
	new_connection_(),
	request_handler_(doc_root, s_id)
{
	//注册以便去操作那个提示proxy_server什么时候该退出的信号
	//在同一个程序里面多次注册同一个信号是安全的，只要是通过asio来搞的
	proxy_server_id = s_id;

	signals_.add(SIGINT);
	signals_.add(SIGTERM);
#if defined(SIGQUIT)
	signals_.add(SIGQUIT);
#endif
	signals_.async_wait(boost::bind(&proxy_server::handle_stop, this));

	//open the acceptor with the option to reuse the address
	tcp::resolver resolver(acceptor_.get_io_service());
	tcp::resolver::query query(address, port);
	tcp::endpoint endpoint = *resolver.resolve(query);

	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	cout << "proxy_server: " << proxy_server_id << " is running!" << endl;
	start_accept();
}
void proxy_server::run()
{
	io_pool.run();
}

int proxy_server::choose_connection()
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

void proxy_server::start_accept()
{
	new_connection_.reset(new connection(io_pool.get_io_service(), request_handler_));
	acceptor_.async_accept(new_connection_->socket_, boost::bind(&proxy_server::handle_accept, this, boost::asio::placeholders::error));        
}

void proxy_server::handle_accept(const boost::system::error_code& e)
{
	if (!e)
	{
		new_connection_->start();
	}
	start_accept();
}

void proxy_server::handle_stop()
{
	io_pool.stop();
}