#pragma once
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include "header.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "common.h"

using namespace std;
/*****************************************************/
	/*             Format of request            
	client_id:allen\r\n
	method:POST\r\n
	server_id:118\r\n
	request_timestamp:month_day_hour_min_second_millisecond\r\n
	\r\n
	(Content).....
	/*****************************************************/

	/* !!! ע�⣡���� request_timestamp �����������һ�䣬Ϊ�˸����������з��ҵ�content    */
class request
{
public:
	string request_line;
	string method;
	string uri;
	string client_id;
	int server_id;
	string obj_id;
	string request_timestamp;		//Using string to assemble the timestamp in the format as :"month_day_hour_min_second_millisecond"
	string content;		//Used for update request and the content part contains the exact binary stuff 

	unsigned long content_length;
	vector<header> headers;

	//Header plus content converted to vectors of boost buffer 
	//in order to apply with the socket forms

	request();
	vector<boost::asio::const_buffer> to_buffers();
	void make_get_request(string file_string, int client_id);
	void make_post_request(string local_path, string file_string, int client_id, int ser_id);
};