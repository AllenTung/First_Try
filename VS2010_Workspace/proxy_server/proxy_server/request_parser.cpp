#include "request_parser.hpp"
#include "request.hpp"

using namespace std;

string return_string_1 = "Now the consume method return indeterminate\n";
string return_string_2 = "Now the consume method return false\n";


request_parser::request_parser()
{

}

boost::tribool request_parser::simple_parse(request& req, boost::array<char, BUFFER_SIZE>& buf)
{

	/*****************************************************/
	/*             Format of request            
	client_id:allen\r\n
	method:POST\r\n
	server_id:118\r\n
	request_timestamp:month_day_hour_min_second_millisecond\r\n
	\r\n
	(Content).....
	/*****************************************************/
	string req_string(buf.begin(), buf.end());

	if (req_string.find("client_id:") < NO_SUCH_SUBSTRING) {
		req.client_id = req_string.substr(req_string.find("client_id:") + 10, req_string.find_first_of("\r\n", req_string.find("client_id:")) - req_string.find("client_id:") - 10);
	}
	if (req_string.find("server_id:") < NO_SUCH_SUBSTRING) {
		string temp_server_id = req_string.substr(req_string.find("server_id:") + 10, req_string.find_first_of("\r\n", req_string.find("server_id:")) - req_string.find("server_id") - 10);
		req.server_id = atoi(temp_server_id.c_str());
	}
	if (req_string.find("method:") < NO_SUCH_SUBSTRING) {
		req.method = req_string.substr(req_string.find("method:") + 7, req_string.find_first_of("\r\n", req_string.find("method:")) - req_string.find("method:") - 7);
	}
	if (req_string.find("obj_id:") < NO_SUCH_SUBSTRING) {
		req.obj_id = req_string.substr(req_string.find("obj_id:") + 7, req_string.find_first_of("\r\n", req_string.find("obj_id:")) - req_string.find("obj_id:") - 7);
	}
	if (req_string.find("content_length:") < NO_SUCH_SUBSTRING) {
		string temp_content_length = req_string.substr(req_string.find("content_length:") + 15, req_string.find_first_of("\r\n", req_string.find("content_length:")) - req_string.find("content_length:") - 15);
		req.content_length = strtoul(temp_content_length.c_str(), NULL, 10);
	}
	if (req_string.find("update_offset:") < NO_SUCH_SUBSTRING) {
		string temp_update_offset = req_string.substr(req_string.find("update_offset:") + 14, req_string.find_first_of("\r\n", req_string.find("update_offset:")) - req_string.find("update_offset:") - 14);
		req.update_offset = strtoul(temp_update_offset.c_str(), NULL, 10);
	}
	if (req_string.find("request_timestamp:") < NO_SUCH_SUBSTRING) {
		req.request_timestamp = req_string.substr(req_string.find("request_timestamp:") + 18, req_string.find_first_of("\r\n", req_string.find("request_timestamp:")) - req_string.find("request_timestamp:") - 15);
	}
	if (req.method == "UPDATE")
	{
		/* 若是UPDATE请求，默认会在 request 的\r\n 空行之后携带上content的内容
		根据header的最后一行也就是request timestamp后出现的第一个换行符来决定content的实际开始位置 */

		req.content = req_string.substr(req_string.find_first_of("\r\n", req_string.find_first_of("request_timestamp")) + 4);
	}

	return true;
}