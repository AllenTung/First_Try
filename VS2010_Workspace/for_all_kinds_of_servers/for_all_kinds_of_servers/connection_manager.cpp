#include <algorithm>
#include <boost/bind.hpp>
#include "connection_manager.hpp"

using namespace std;

//等于它管理了一个连接池，这个用set来做数据结构，然后start一个就是insert一个到这个连接池里面去
void connection_manager::start(connection_ptr c)
{
	cout << "Insert a new connection_ptr and gets it start\n";

	connections_.insert(c);
/*	c->start();*/
}
void connection_manager::stop(connection_ptr c)
{
	cout << "Stop a connection_ptr...\n";

	connections_.erase(c);
/*	c->stop();*/
}

void connection_manager::stop_all()
{
	cout << "Stop all the connections and clear the set!!!\n";

	for_each(connections_.begin(), connections_.end(), boost::bind(&connection::stop, _1));
	connections_.clear();
}

