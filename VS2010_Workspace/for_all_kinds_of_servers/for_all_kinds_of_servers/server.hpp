#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include "connection.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "io_service_pool.hpp"
#include "request_parser.hpp"
#include "encoder.h"
#include "decoder.h"


using namespace std;


class server : private boost::noncopyable
{
public:

	//Server_id could be treated and used as its own port
	int server_id;
	/// The io_service used to perform asynchronous operations.
	io_service_pool io_pool;

	/// The signal_set is used to register for process termination notifications.
	boost::asio::signal_set signals_;

	/// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor_;

	/// The connection manager which owns all live connections.
	connection_manager connection_manager_;

	/// The next connection to be accepted.
	connection_ptr new_connection_;

	vector<connection_ptr> connection_pool;

	/// The handler for all incoming requests.
	request_handler request_handler_;

	//File name for the method Transmit file
	string file_name;
	explicit server(const string& address, const string& port, const string& root_dir, int thread_num, int s_id);
	explicit server(const string& address, const string& port, const string& root_dir, int thread_num, int s_id, const string& filename);
	//Run the server's io_service loop
	void run();
	void start_accept();
	void handle_accept(connection_ptr& current_connection, const boost::system::error_code& e);
	void handle_stop();
	int choose_connection();
	void start_handle(connection_ptr& current_connection);
};

#endif