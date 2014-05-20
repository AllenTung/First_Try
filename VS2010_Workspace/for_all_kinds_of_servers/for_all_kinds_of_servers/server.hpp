#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <deque>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include "connection.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "io_service_pool.hpp"
#include "request_parser.hpp"
#include "encoder.h"
#include "decoder.h"
#include "metadata.h"
using namespace std;


class server/* : private boost::noncopyable*/
{
public:

	static map<string, metadata> obj_meta_table;

	static map<string, string> ip_port_table;

	static boost::mutex cout_lock;

	static boost::mutex table_lock;

	static boost::mutex file_lock;

	static boost::mutex con_deque_lock;

	static boost::mutex ec_port_lock;

	/*Server_id could be treated and used as its own port
	  server_id:Exposed to client and other nodes for receving request and datas
	  inner_server_id: Used as an inner aisle exposed to itself to id the out-going port*/
	int server_id;
	int inner_server_id;
	string local_ip;
	/// The io_service used to perform asynchronous operations.
	io_service_pool io_pool;

	/// The signal_set is used to register for process termination notifications.
	boost::asio::signal_set signals_;

	/// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::acceptor inner_acceptor_;

	/// The connection manager which owns all live connections.
	connection_manager connection_manager_;

	/// The next connection to be accepted.
	connection_ptr new_connection_;

	deque<connection_ptr> connection_pool;

	/// The handler for all incoming requests.
	request_handler request_handler_;

	//File name for the method Transmit file
	string file_name;
	explicit server(string address, string port, const string& root_dir, int thread_num, int s_id);
	explicit server(string address, string port, const string& root_dir, int thread_num, int s_id, const string& filename);
	//Run the server's io_service loop
	void run();
	void start_accept();
	void handle_accept(connection_ptr& current_connection, boost::system::error_code& e);
	void handle_stop();
	int choose_connection();
	void start_handle(connection_ptr& current_connection);
	static void server::show_table_info()
	{
		server::table_lock.lock();
		int table_count = 0;
		map<string, metadata>::iterator it = server::obj_meta_table.begin();
		for (; it != server::obj_meta_table.end(); it++)
		{
			table_count ++;
			cout << it->first << endl;
			cout << it->second.history_record.at(0).request_timestamp << endl << endl;
		}
		server::table_lock.unlock();
		cout << "**********************************************" << endl;
		cout << "Entry counts: " << table_count << " **********" << endl;
		cout << "**********************************************" << endl;
	}
};

#endif