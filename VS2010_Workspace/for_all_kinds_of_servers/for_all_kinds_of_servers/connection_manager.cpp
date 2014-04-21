#include <algorithm>
#include <boost/bind.hpp>
#include "connection_manager.hpp"

using namespace std;

//������������һ�����ӳأ������set�������ݽṹ��Ȼ��startһ������insertһ����������ӳ�����ȥ
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

