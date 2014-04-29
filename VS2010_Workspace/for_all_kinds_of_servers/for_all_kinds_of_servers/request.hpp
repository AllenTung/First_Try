#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <vector>
#include <time.h>
#include "header.hpp"
#include "global_stuff.hpp"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "common.hpp"

using namespace std;

class request
{
	/*****************************************************/
	/*             Format of request            
	client_id:allen\r\n
	method:POST\r\n
	server_id:118\r\n
	content_length:32264\r\n
	request_timestamp:month_day_hour_min_second_millisecond\r\n
	\r\n
	(Content).....
	/*****************************************************/

	/* !!! 注意！！！ request_timestamp 必须是在最后一句，为了根据两个换行符找到content    */
public:
	//Keep everything in string to achieve simplicity
	int server_id;
	string method;		//For now: POST, GET, UPDATE
	string obj_id;

	string client_id;
	string server_status;	//Enumeration of server status: unavailable, ready_for_post, post_done ...  
	unsigned int content_length;
	unsigned int update_offset;
	string request_timestamp;		//Using string to assemble the timestamp in the format as :"month_day_hour_min_second_millisecond"
	string content;		//Used for update request and the content part contains the exact binary stuff 
	Data_Type data_type;

	//These may be discarded
	string request_line;
	string uri;
	int http_version_major;
	int http_version_minor;
	vector<header> headers;

	request();
	request(request& req);
	vector<boost::asio::const_buffer> to_buffers();	

	//all to buffers is mainly for update request
	vector<boost::asio::const_buffer> all_to_buffers();
	//header to buffer is for the regular reqeusts
	vector<boost::asio::const_buffer> header_to_buffers();
	//temporary solution for the content only
	vector<boost::asio::const_buffer> content_to_buffers();
};

#endif
