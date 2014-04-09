#pragma once
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <boost/asio.hpp>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "request.h"

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::windows::random_access_handle;
typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

class client
{
public:
	tcp::socket client_socket;
	random_access_handle file_handler;
/*	tcp::resolver client_resolver;*/

	int request_type;
	int client_id;
	int target_port;

	client(boost::asio::io_service& io_service, int client_id);
	//The one below maybe just for temporary test
	client(boost::asio::io_service& io_service, int client_id, int target_port);

	void launch_client(int request_type, int thread_id);
	void launch_get_request(int thread_id);
	void launch_post_request(int thread_id);

	void handle_write(string uri, const boost::system::error_code& e);
};

typedef boost::shared_ptr<client> client_ptr;