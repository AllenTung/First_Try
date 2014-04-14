#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <time.h>
#include <ctime>
#include <boost/lexical_cast.hpp>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "request_handler.hpp"

request_handler::request_handler(const string& doc_root, int s_id) : doc_root_(doc_root)
{
	server_id = s_id;
}
void request_handler::handle_post_request(const request& req, reply& rep)
{
	cout << "Now making an ready_for_post reply from proxy_server: " << server_id << "...\n";

	rep.server_status = "ready_for_post";
	rep.server_id = server_id;
	rep.headers.resize(3);
	char tmp_char[30];
	_itoa(req.content_length, tmp_char, 10);
	time_t tt = time(0);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = tmp_char;
	                              
	//Amend the hearder to simply answer ACK
	rep.headers[1].name = "Server-Status";
	rep.headers[1].value = "Ready";

	// Time
	rep.headers[2].name = "Date";
	rep.headers[2].value = ctime(&tt);
}

//从connection.cpp那里会写处理和判断这个请求是否是完整的，只有完整的请求才会调用到这个类的这个函数
//也就是说这个函数就是给需要正常搞个请求的时候用到的
void request_handler::handle_get_request(const request& req, reply& rep)
{
	//open the file to send back
	string full_path = doc_root_ + req.uri;
	ifstream file_input(full_path.c_str(), ios::in | ios::binary);

	if (!file_input)
	{
		cout << "Error occurs when opening the file!\n";
		return;
	}

	char buf[512];
	while (file_input.read(buf, sizeof(buf)).gcount() > 0)
	{
		rep.content.append(buf, file_input.gcount());
	}
	rep.headers.resize(2);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[1].name = "Content-Type";
	rep.headers[1].value = "text/plain";

	file_input.close();
}