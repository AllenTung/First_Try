#pragma once
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <boost/asio.hpp>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "request.h"

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::windows::random_access_handle;
typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

class io_service_pool
{
public:
	int next_io_service;
	vector<io_service_ptr> io_services;

	io_service_pool(int pool_size);
	 
	void stop();
	boost::asio::io_service& get_io_service();

	void launch_client(int request_type, int thread_id);
	void launch_get_request(int thread_id);
	void launch_post_request(int thread_id);
	void handle_write(const boost::system::error_code& e);

	void launch_test_post(int thread_id, random_access_handle& file, tcp::socket& socket);

};