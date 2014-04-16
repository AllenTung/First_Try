#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include "request.h"

using namespace std;

namespace misc_strings 
{
	const char name_value_separator[] = { ':', ' ' };
	const char crlf[] = { '\r', '\n' };
} 

request::request():client_id("-1"), method(""), server_id(88), request_timestamp(""), content(""), content_length(0), update_offset(0)
{

}

/*****************************************************/
	/*             Format of request            
	client_id:allen\r\n
	method:POST\r\n
	server_id:118\r\n
	obj_id:read_me.txt\r\n
	content_length:32264\r\n
	request_timestamp:month_day_hour_min_second_millisecond\r\n
	\r\n
	(Content).....
	/*****************************************************/

	/* !!! ע�⣡���� request_timestamp �����������һ�䣬Ϊ�˸����������з��ҵ�content    */



vector<boost::asio::const_buffer> request::all_to_buffers()
{
	vector<boost::asio::const_buffer> buffers;

	//����POST����ı����Header Lines
	headers.resize(8);
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

	headers[6].name = "request_timestamp:";
	headers[6].value = request_timestamp;

	headers[7].name = "content:";
	headers[7].value = content;

	for (size_t i = 0; i < headers.size(); i++)
	{
		header& h = headers[i];
		if (i == headers.size() - 1)
		{		
			//Two line break leads to the updated content, no need for "content:"
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

vector<boost::asio::const_buffer> request::header_to_buffers()
{
	vector<boost::asio::const_buffer> buffers;

	headers.resize(7);
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

	headers[6].name = "request_timestamp:";
	headers[6].value = request_timestamp;

	for (size_t i = 0; i < headers.size(); i++)
	{
		header& h = headers[i];
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	}
	return buffers;
}

vector<boost::asio::const_buffer> request::content_to_buffers()
{
	vector<boost::asio::const_buffer> buffers;

	headers.resize(1);
	headers[0].name = "content:";
	headers[0].value = content;

	for(int i = 0; i < headers.size(); i++)
	{
		header& h = headers[i];
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	}

	return buffers;
}


void request::make_post_request(string local_path, string remote_path, int tmp_client_id, int ser_id)
{
	//ip_string ����header��file_string����_line���õ���
	method = "POST";
	obj_id = remote_path;
	server_id = ser_id;
	client_id = int_to_string(tmp_client_id);
	request_timestamp = get_systime_string();
	uri = remote_path;

	/*Get the size of the to-be-transmitted file */
	FILE* tempfile = fopen(local_path.c_str(),"rb");
	if(!tempfile)
	{
		cout << "Error occurs when opening the file: " << local_path << endl;
		return;
	}
	fseek(tempfile, 0, SEEK_END);
	content_length = ftell(tempfile); 
	fseek(tempfile, 0,SEEK_SET);
	fclose(tempfile);

}
void request::make_get_request(string file_string, int tmp_client_id)
{
	//ip_string ����header��file_string����_line���õ���
	method = "GET";
	obj_id = file_string;
	uri = file_string;
	client_id = int_to_string(tmp_client_id);
	request_timestamp = get_systime_string();
	
}
void request::make_update_request(string local_path, string file_string, int tmp_client_id, string update_content)
{
	/*�и������Ҫ���ǣ���������Ȱ����client����֪��content length��ȷ�����offset
	  ������ȥproxy server֮��Ҳ�ǰ�������������������ȥ��Ӧ��server��������ʵ��server��
	  �����consistency����Ĳ�һ�µĻ������ǳ��ȵĸı䣬�����������serverȥ��ѯversion controller
	  ֮��ģ�����master node�������ʹﵽһ��transfer iff needed ��Ч�� */
	method = "UPDATE";
	obj_id = file_string;
	client_id = int_to_string(tmp_client_id);
	request_timestamp = get_systime_string();
	content = update_content;

	/*Get the size of the to-be-transmitted file */
	FILE* tempfile = fopen(local_path.c_str(),"rb");
	if(!tempfile)
	{
		cout << "Error occurs when opening the file: " << local_path << endl;
		return;
	}
	fseek(tempfile, 0, SEEK_END);
	content_length = ftell(tempfile); 
	fseek(tempfile, 0,SEEK_SET);
	fclose(tempfile);

	//Use the content length to calculate random update offset
	update_offset = get_random_offset(content_length);
}



