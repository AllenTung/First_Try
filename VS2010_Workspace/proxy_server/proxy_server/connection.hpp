#pragma once
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "common.hpp"
#include <boost/enable_shared_from_this.hpp>
#include "reply.hpp"
#include "request.hpp"
#include "request_handler.hpp"
#include "request_parser.hpp"
#include "io_service_pool.hpp"

/*using namespace std;*/
#if defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)

using boost::asio::ip::tcp;
using boost::asio::windows::overlapped_ptr;
using boost::asio::windows::random_access_handle_service;

class connection_manager;
// Represents a single connection from a client.
class connection: public boost::enable_shared_from_this<connection>
{
public:
	int proxy_server_id;
	//Indication of the busy-or-free status, 0 for free and 1 for busy
	int busy;
	//Client-side connection
	tcp::socket socket_;

	// The handler used to process the incoming request.
	request_handler request_handler_;

	// Buffer for incoming data.
	boost::array<char, BUFFER_SIZE> buffer_;

	// The incoming request.
	request request_;

	// The parser for the incoming request.
	request_parser request_parser_;

	// The reply to be sent back to the client.
	reply reply_;

	//The file name for the transmit file
	string file_name;

	//These are dedicated sockets for erasure coding communication between server and serve

	random_access_handle file_;
	// Construct a connection with the given io_service.
	connection(boost::asio::io_service& io_service, request_handler& handler);

/*	connection(boost::asio::io_service& io_service, const string filename);*/

	// Start the first asynchronous operation for the connection.
	void start();

	// Stop all asynchronous operations associated with the connection.
	void stop();

	// Handle completion of a read operation.
	void handle_read(const boost::system::error_code& e, std::size_t bytes_transferred);

	// Handle completion of a write operation.
	void handle_write(const boost::system::error_code& e);

	//reset_all is called 
	void reset_all();

	//Locate the proper servers, identified by its port(identical to server_id)
	int target_server_location(string obj_id);
};

typedef boost::shared_ptr<connection> connection_ptr;
#else // defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)
# error Overlapped I/O not available on this platform
#endif // defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)