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

void request_handler::handle_transmit_block_request(const request& req, reply& rep)
{
	cout << "Now making an ready for transmit block from server: " << server_id << "....\n";

	rep.server_id = server_id;
	rep.headers.resize(3);

	time_t tt = time(0);
	rep.headers[0].name = "content_length:";
	rep.headers[0].value = int_to_string(req.content_length);

	//Amend the hearder to simply answer ACK
	rep.headers[1].name = "server_status:";
	rep.headers[1].value = READY_FOR_POST_STATUS;

	// Time
	rep.headers[2].name = "date:";
	rep.headers[2].value = ctime(&tt);

}


void request_handler::handle_post_request(const request& req, reply& rep)
{
	cout << "Now making an ready for transmit block from server: " << server_id << "....\n";

	rep.server_id = server_id;
	rep.headers.resize(3);

	time_t tt = time(0);
	rep.headers[0].name = "content_length:";
	rep.headers[0].value = int_to_string(req.content_length);

	//Amend the hearder to simply answer ACK
	rep.headers[1].name = "server_status:";
	rep.headers[1].value = READY_FOR_POST_STATUS;

	// Time
	rep.headers[2].name = "date:";
	rep.headers[2].value = ctime(&tt);
}

void request_handler::handle_get_request(const request& req, reply& rep)
{
	//open the file to send back
	string full_path = req.obj_id;
	FILE* tempfile = fopen(full_path.c_str(),"rb");

	if(!tempfile)
	{
		cout << "Error occurs when opening the file: " << full_path << endl;
		return;
	}
	fseek(tempfile, 0, SEEK_END);
	rep.content_length = ftell(tempfile); 
	fseek(tempfile, 0,SEEK_SET);
	fclose(tempfile);

	rep.headers.resize(1);
	rep.headers[0].name = "content_length:";
	rep.headers[0].value = int_to_string(rep.content_length);

	
}