#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include "request.hpp"
#include "common.hpp"
#include <vector>

using namespace std;
typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

class ec_io_service_pool : private boost::noncopyable
{
public:
	int next_ec_io_service;
	vector<io_service_ptr> ec_io_services;
	vector<work_ptr> ec_works;
	vector<boost::shared_ptr<boost::thread>> ec_threads;

	explicit ec_io_service_pool(int pool_size);
	void run();
	void stop();
	boost::asio::io_service& get_ec_io_service();
	boost::asio::io_service& get_ec_io_service(int fetch_mode);

};