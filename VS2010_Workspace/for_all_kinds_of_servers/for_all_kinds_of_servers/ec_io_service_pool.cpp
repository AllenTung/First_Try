#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "ec_io_service_pool.h"

using namespace std;

ec_io_service_pool::ec_io_service_pool(int pool_size)
	:next_ec_io_service(5)
{
	for(int i = 0; i < pool_size; i++)
	{
		io_service_ptr io_ptr(new boost::asio::io_service);
		work_ptr work_pointer(new boost::asio::io_service::work(*io_ptr));

		ec_io_services.push_back(io_ptr);
		ec_works.push_back(work_pointer);	
	}
}

void ec_io_service_pool::run()
{

	for (int i =0;i < ec_io_services.size(); i++)
	{
		boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&boost::asio::io_service::run, ec_io_services[i])));
		ec_threads.push_back(thread);
	}

	for (int k = 0; k < ec_threads.size(); k++)
	{
		ec_threads[k]->join();
	}
} 

void ec_io_service_pool::stop()
{
	for (int i = 0; i < ec_io_services.size(); i++)
	{
		ec_io_services[i]->stop();
	}
}

boost::asio::io_service& ec_io_service_pool::get_ec_io_service()
{
// 	if (next_ec_io_service == -1)
// 	{
// 		next_ec_io_service = ec_io_services.size() - 1;
// 	}
// 	boost::asio::io_service& tmp_io = *ec_io_services[next_ec_io_service];
// 	next_ec_io_service--;
// 	return tmp_io;
	next_ec_io_service++;
	if (next_ec_io_service == ec_io_services.size())
	{
		next_ec_io_service = 0;
	}
	boost::asio::io_service& tmp_io = *ec_io_services[next_ec_io_service];

	return tmp_io;
}

boost::asio::io_service& ec_io_service_pool::get_ec_io_service(int fetch_mode)
{
	next_ec_io_service += fetch_mode;

	if (next_ec_io_service == ec_io_services.size())
	{
		next_ec_io_service = 0;
	}

	else if (next_ec_io_service == -1)
	{
		next_ec_io_service = ec_io_services.size() - 1;
	}

	boost::asio::io_service& tmp_io = *ec_io_services[next_ec_io_service];
	return tmp_io;
}