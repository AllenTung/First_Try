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

	//�����metadata��һ�����ɵ�ʱ�򣬱ض�����update���������Ե�һ���汾����Ҫ��target server�ı�ʾ������ֱ���������-1��֪�����������
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