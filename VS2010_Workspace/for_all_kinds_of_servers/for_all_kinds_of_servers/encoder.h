#pragma once
#ifndef ENCODER_HPP
#define ENCODER_HPP

extern "C"
{
#include "jerasure.h"
#include "cauchy.h"
#include "galois.h"
#include "liberation.h"
#include "reed_sol.h"
};
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <math.h>
#include <WinSock2.h>
#include <windows.h>
#include "external_tool.h"
#include <boost/random.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <signal.h>
#include <direct.h>
#include "common.hpp"
#include <vector>
#include "ec_io_service_pool.h"
#include "request.hpp"
using namespace std;
using boost::asio::ip::tcp;


class encoder
{
public:
	int readins;
	int n;		//for control


	//To indicate the size of each cahr in the experimental text.
	int unit_size;

	int ack_received;
	string encode_tech;
	
	encoder();
	int jfread(void *ptr, int size, int nmembers, FILE* stream);
	int encode_file (ec_io_service_pool& ec_io_service, vector<socket_ptr> ec_socket, request& ori_req, int server_id, string pure_obj_name, string full_local_path);

	//These two funtions are called when POST_REQUEST is done being handled at master node
	void make_transmit_block_request(request ori_request);
	void make_transmit_data_request(request ori_request);

	void gather_ack(const boost::system::error_code& e);
	void handle_write(const boost::system::error_code& e);
};

#endif