#ifndef VERSION_H
#define VERSION_H

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "common.hpp"

using namespace std;

struct version
{
	string client_id;
	string request_timestamp;

	//For simplicity, true is for clean, false is for dirty
	bool clean;

	//Used to id where the update operation targets
	int target_server_id;

	version():client_id(""), request_timestamp(""), target_server_id(0), clean(true)
	{

	}

	version(string c_id, string r_time, int t_server_id, bool c_or_d): client_id(c_id), request_timestamp(r_time), target_server_id(t_server_id),clean(c_or_d)
	{
		
	}
	~version()
	{

	}
};

#endif