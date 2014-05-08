#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "metadata.h"

using namespace std;

metadata::metadata()
{
	content_length = 0;
	data_type = full_copy;

	full_copy_location = 0;
	first_parity_location = 0;
	second_parity_location = 0;

	version tmp_version;
	history_record.push_back(tmp_version);
}

metadata::metadata(unsigned int cont_length, Data_Type d_type, string client_id, string request_timestamp, bool c_or_d, int full_copy_loc, int first_parity_loc, int second_parity_loc)
{
	content_length = cont_length;
	data_type = d_type;

	full_copy_location = full_copy_loc;
	first_parity_location = first_parity_loc;
	second_parity_location = second_parity_loc;

	//如果是metadata第一次生成的时候，必定不是update操作，所以第一个版本不需要作target server的标示，或者直接如果读到-1就知道是这回事了
	version tmp_version(client_id, request_timestamp, -1, c_or_d);

	history_record.push_back(tmp_version);
}

void metadata::insert_new_record(string client_id, string request_timestamp, int t_server_id, bool c_or_d)
{
	version tmp_version(client_id, request_timestamp, t_server_id, c_or_d);

	history_record.push_back(tmp_version);
}

void metadata::insert_new_record(version new_v)
{
	version tmp_version(new_v);
	history_record.push_back(tmp_version);
}