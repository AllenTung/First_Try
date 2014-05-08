#ifndef METADATA_H
#define METADATA_H

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include "common.hpp"
#include "version.h"

using namespace std;

class metadata
{
public:
	unsigned int content_length;
	Data_Type data_type;

	//Indicates the property of the hybrid chain
	int full_copy_location;
	int first_parity_location;
	int second_parity_location;

	vector<version> history_record;	

	metadata();
	metadata(unsigned int cont_length, Data_Type d_type, string client_id, string request_timestamp, bool c_or_d,int full_copy_location, int first_parity_location, int second_parity_location);
	void insert_new_record(string client_id, string request_timestamp, int t_server_id, bool c_or_d);
	void insert_new_record(version new_v);
};

#endif