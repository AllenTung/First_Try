#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include "request.hpp"

using namespace std;

namespace misc_strings 
{
	const char name_value_separator[] = { ':', ' ' };
	const char crlf[] = { '\r', '\n' };
} 

request::request():client_id("-1"), method(""), server_id(88), request_timestamp(""), content(""), content_length(0), update_offset(0), data_type(full_copy)
{

}
request:: request(request& req)
{
	client_id = req.client_id;
	server_id = req.server_id;
	request_timestamp = req.request_timestamp;
	obj_id = req.obj_id;
	content_length = req.content_length;
	update_offset = req.update_offset;
	data_type = req.data_type;
}
/*****************************************************/
	/*             Format of request            
	client_id:allen\r\n
	method:POST\r\n
	server_id:118\r\n
	obj_id:read_me.txt\r\n
	content_length:32264\r\n
	update_offset:33333\r\n
	request_timestamp:month_day_hour_min_second_millisecond\r\n
	\r\n
	(Content).....
	/*****************************************************/

	/* !!! 注意！！！ request_timestamp 必须是在最后一句，为了根据两个换行符找到content    */
vector<boost::asio::const_buffer> request::to_buffers()
{
	vector<boost::asio::const_buffer> buffers;

	//生成POST请求的必须的Header Lines
	headers.resize(9);
	headers[0].name = "client_id:";
	headers[0].value = client_id;
	headers[1].name = "method:";
	headers[1].value = method;
	headers[2].name = "server_id:"; 
	headers[2].value = int_to_string(server_id);
	headers[3].name = "obj_id:";
	headers[3].value = obj_id;

	headers[4].name = "content_length:";
	headers[4].value = int_to_string(content_length);

	headers[5].name = "update_offset:";
	headers[5].value = int_to_string(update_offset);

	headers[6].name = "data_type:";
	headers[6].value = int_to_string(data_type);

	headers[7].name = "request_timestamp:";
	headers[7].value = request_timestamp;
	headers[8].name = "content:";
	headers[8].value = content;

	for (size_t i = 0; i < headers.size(); i++)
	{
		header& h = headers[i];
		if (i == headers.size() - 1)
		{		
			buffers.push_back(boost::asio::buffer(misc_strings::crlf));
			buffers.push_back(boost::asio::buffer(h.value));
			continue;
		}
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	}
	return buffers;
}