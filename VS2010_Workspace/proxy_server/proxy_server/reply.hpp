#ifndef HTTP_REPLY_HPP
#define HTTP_REPLY_HPP
#include <string>
#include <vector>
#include <time.h>
#include <boost/asio.hpp>
#include "common.hpp"
#include "header.hpp"

using namespace std;
class reply
{
public:
	int server_id;
	string server_status;	//Enumeration of server status: unavailable, ready_for_post, post_done ...  
	unsigned long content_length;
	
	vector<header> headers;
	string content;
	
	vector<boost::asio::const_buffer> to_buffers();

	//Used to construct a simple reply indicating the server is ready , for post especially
	vector<boost::asio::const_buffer> simple_ready_buffers();
	vector<boost::asio::const_buffer> simple_location_buffers();
};
#endif // HTTP_REPLY_HPP