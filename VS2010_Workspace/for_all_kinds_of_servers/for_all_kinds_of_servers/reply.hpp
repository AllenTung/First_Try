#ifndef HTTP_REPLY_HPP
#define HTTP_REPLY_HPP

#include <string>
#include <vector>
#include <time.h>
#include <boost/asio.hpp>
#include "header.hpp"

using namespace std;
class reply
{
public:
	int server_id;
	string server_status;	//Enumeration of server status: unavailable, ready_for_post, ready_for_update, post_done, update_done, try_again...  
	unsigned int content_length;
	
	vector<header> headers;
	string content;
	
	//used to construct a full reply containing all needed headers
	vector<boost::asio::const_buffer> to_buffers();

	//Used to construct a simple reply containing all the simple server status, no additional headers needed
	vector<boost::asio::const_buffer> simple_ready_buffers();
};
#endif // HTTP_REPLY_HPP