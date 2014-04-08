#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "io_service_pool.hpp"

using namespace std;

io_service_pool::io_service_pool(int pool_size)
	:next_io_service(0)
{
	for(int i = 0; i < pool_size; i++)
	{
		io_service_ptr io_ptr(new boost::asio::io_service);
		work_ptr work_pointer(new boost::asio::io_service::work(*io_ptr));

		io_services.push_back(io_ptr);
		works.push_back(work_pointer);	
	}
}

void io_service_pool::run()
{

	for (int i =0;i < io_services.size(); i++)
	{
		boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&boost::asio::io_service::run, io_services[i])));
		threads.push_back(thread);
	}

	for (int k = 0; k < threads.size(); k++)
	{
		threads[k]->join();
	}
} 

void io_service_pool::stop()
{
	for (int i = 0; i < io_services.size(); i++)
	{
		io_services[i]->stop();
	}
}

boost::asio::io_service& io_service_pool::get_io_service()
{
	next_io_service++;
	if (next_io_service == io_services.size())
	{
		next_io_service = 0;
	}

	boost::asio::io_service& tmp_io = *io_services[next_io_service];
	return tmp_io;
}

boost::asio::io_service& io_service_pool::get_io_service(int fetch_mode)
{
	next_io_service += fetch_mode;
	
	if (next_io_service == io_services.size())
	{
		next_io_service = 0;
	}

	else if (next_io_service == -1)
	{
		next_io_service = io_services.size() - 1;
	}

    boost::asio::io_service& tmp_io = *io_services[next_io_service];
	return tmp_io;
}

