#ifndef DECODER_H
#define DECODER_H

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
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <signal.h>
#include <direct.h>
#include <vector>
#include "common.hpp"
#include "request.hpp"
#include "external_tool.h"
#include "ec_io_service_pool.h"
#include "reply.hpp"


using namespace std; 

class server;
class decoder
{
public:

	int readins;
	int buffer_size;
	int n; // for control
	string encode_method;
	decoder(int r_ins, int buf_size);
	decoder();
	//Decode level: 0 is for the whole file which is reconstruction, [1, K+M] means recovery for one certain block
	//If decode level is 0 ,then the ori_size could be used. AND 0 is always came up by master node thus the size of whole object is accesable.
	//If decode level is greater than 0 ,than decoding only need the size of one data segment which is contained in request's content_length
	
	/* Return Value: 0 is for success , 1 is for failure ,which could be divided into these sub-situation :
	   return 1 and this funtion is devoked by master node , then it must happen during the transmit update content request ,then a TOTAL_RECONSTRUCT_REQUEST
	   should be initiated by master node and the newest update would be discarded
	   return 1 and this funtion is devoked by one of the data or parity node : it could happen during the update, or get , or transmit_delta request,
	   the node should invoke this fucntion by identifying the last variant decode_level to specify the exact segment to recover. If less than K nodes response, than
	   it's when the 1 is returned and it has to send a TOTAL_RECONSTRUCT_REQUEST to master node */
	int decode_file(ec_io_service_pool& ec_io_service, vector<socket_ptr> ec_socket, request& ori_req, int server_id, string local_ip, string obj_id, unsigned int ori_size, int master_node, int decode_level);

};




#endif