#pragma once
#include "decoder.h"
#include "server.hpp"
class server;
decoder::decoder()
{
	encode_method = "cauchy_good";
	readins = 1;
	buffer_size = 8192;
	n = 1;
}

decoder::decoder(int r_ins, int buf_size)
{
	encode_method = "cauchy_good";
	readins = r_ins;
	buffer_size = buf_size;
	n = 1;
}

int decoder::decode_file(ec_io_service_pool& ec_io_service, vector<socket_ptr> ec_socket, request& ori_req, int server_id, string local_ip, string obj_id, unsigned int ori_size, int master_node, int decode_level)
{
	int argc = 2;
	char* source_path = "I://VS2010_Workspace//for_all_kinds_of_servers//for_all_kinds_of_servers//test.txt";

	FILE *fp;				
	char **data;
	char **coding;
	int *erasures = NULL;
	int *erased = NULL;
	int *matrix = NULL;
	int *bitmatrix = NULL;
	
	//int k, m, w, packetsize, buffersize;

	int k = ERASURE_CODE_K;
	int m = ERASURE_CODE_M;
	int w = ERASURE_CODE_W;
	int packetsize = ERASURE_CODE_PACKETSIZE;		
	int buffersize = buffer_size;
	
	/* If decode_level is 0, ori_size stands for the whole file size, req.content_length stands for the size of one segment
	   If decode_level > 0, ori_size stands for the size of one segment ,req.content_length stands for the size of updated content which could be ignored */
	int origsize = ori_size;
	if (decode_level > 0)
		origsize = ori_size * k;

	int i, j;				// loop control variables
	int blocksize;			// size of individual files

	int total;				// used to write data, not padding to file
	int numerased = 0;			// number of erased files
		
	/* Used to recreate file names */
	char *temp;
	char *cs1, *cs2;
	char *fname;
	char *curdir;

	/* Used to time decoding */
	/* Timing variables */
	LARGE_INTEGER t1, t2, t3, t4, tc;
	/* tsec and totalsec are expressed in second */
	double tsec, totalsec;
	/* Initial time counting */
	QueryPerformanceFrequency(&tc);
	totalsec = 0.0;
	
	/* Start timing */
	QueryPerformanceCounter(&t1);

	/* Error checking parameters */
	curdir = (char *)malloc(sizeof(char)*100);
	_getcwd(curdir, 100);
	
	/* Begin recreation of file names */
	cs1 = (char*)malloc(sizeof(char)*strlen(source_path));
	cs2 = strrchr(source_path, '/');
	if (cs2 != NULL)
	{
		cs2++;
		strcpy(cs1, cs2);
	}
	else 
	{
		strcpy(cs1, source_path);
	}
	cs2 = strchr(cs1, '.');
	if (cs2 != NULL) 
	{
		*cs2 = '\0';
	}	
	cs2 = (char*)malloc(sizeof(char)*strlen(source_path));
	fname = strchr(source_path, '.');
	strcpy(cs2, fname);
	fname = (char *)malloc(sizeof(char*)*(100+strlen(source_path)+10));


	/* Allocate memory */
	erased = (int *)malloc(sizeof(int)*(k+m));
	for (i = 0; i < k + m; i++)
		erased[i] = 0;
	erasures = (int *)malloc(sizeof(int)*(k+m));

	data = (char **)malloc(sizeof(char *)*k);
	coding = (char **)malloc(sizeof(char *)*m);

	int *decoding_matrix;
	int *dm_ids;

	decoding_matrix = talloc(int, k*k);
	dm_ids = talloc(int, k);

	/* Allocating ends */

	if (buffersize != origsize) 
	{
		for (i = 0; i < k; i++)
		{
			data[i] = (char *)malloc(sizeof(char)*(buffersize/k));
		}
		for (i = 0; i < m; i++) 
		{
			coding[i] = (char *)malloc(sizeof(char)*(buffersize/k));
		}
		blocksize = buffersize/k;
	}

	/* ***************************************************************************** */
	/* Before preparation has been done and the distributed contact should be on board*/

	/* First connect all the accountable nodes, and if the first K node response all well, then conduct a light-weight reconstruction.
	   And here are some points worthy of noticing : 
	   If this is a full reconstruct behaviour initiated by the master node ,than it follows the simpliest process.
	   If this is a one-segment reconstruct initiated by one certain data or parity node ,than the server id should be carefully set.*/


	/* These to control the http visit sequence and the right servers */
	int positive_response = 0;
	int next_target_server = -1;
	int loop_control = 0;
	if (server_id = master_node)
	{
		loop_control = k + m;
	}
	else if (server_id != master_node)
	{
		/* If this a individual segment reconstruction, then connect the server starting from 
	       the first data node */
		loop_control = k + m - 1;
	}


	cout << "Decoding ! " << endl << "Level: " << decode_level << endl << "Connecting all the accountable nodes.." << endl;
	server::ec_port_lock.lock();
	int temp_ec_port = get_random_ec_port(server_id);
	server::ec_port_lock.unlock();
	for (int i = 1; i <= loop_control ; i++)
	{
		if (decode_level == i)
		{
			continue;
		}
		//Connect the next targeted server and send a transmit_data request
		next_target_server = master_node + i;

		// Round circle calculation
		if (next_target_server > STARTING_SERVER_ID + NUMBER_OF_SERVER - 1)
		{
			next_target_server -= NUMBER_OF_SERVER;
		}

		string remote_ip = "";
		map<string, string>::iterator temp_it = server::ip_port_table.find(int_to_string(next_target_server));
		if (temp_it != server::ip_port_table.end())
		{
			remote_ip = temp_it->second;
		}
		tcp::endpoint remote_end_point(boost::asio::ip::address_v4::from_string(remote_ip.c_str()), next_target_server);
		tcp::endpoint local_end_point(boost::asio::ip::address_v4::from_string(local_ip.c_str()), temp_ec_port);
		ec_socket.at(i - 1)->open(local_end_point.protocol());
		ec_socket.at(i - 1)->set_option(boost::asio::ip::tcp::socket::reuse_address(true));
		ec_socket.at(i - 1)->bind(local_end_point);
		ec_socket.at(i - 1)->connect(remote_end_point);

		/* The "ori_req" is the exact same one from client's update, except for the content-length 
		   has been modified to whole object size or one segment size depending on decode level,
		   and server_id stands for the target updating server */
		request recon_request(ori_req);
		recon_request.method = RECONSTRUCT_REQUEST;

		boost::system::error_code err_code;
		boost::asio::write(*(ec_socket.at(i - 1)), recon_request.header_to_buffers(), err_code);		

		//After ready response , prepare the socket for the transmition from the client
		boost::asio::socket_base::receive_buffer_size option(blocksize);
		ec_socket.at(i - 1)->set_option(option);
	}

	/* Set the server which initiate this process to erased status */
	if (decode_level > 0)
	{
		erased[decode_level - 1] = 1;
		erasures[numerased] = decode_level - 1;
		numerased ++;
	}

	
	for (int i = 1; i <= k + m ; i++)
	{
		/* Confirm all the status and version condition on all related servers */
		if (decode_level == i)
		{
			continue;
		}
		for(;;)
		{
			cout << "Checking the ready-or-not status of server: " << master_node + i << endl;
			boost::asio::streambuf tmp_response;
			boost::asio::read_until(*(ec_socket.at(i - 1)), tmp_response, "\r\n");
			istream response_stream(&tmp_response);
			string response_string;
			response_stream >> response_string;

			if(response_string.find(READY_FOR_RECONSTRUCTION_STATUS) < NO_SUCH_SUBSTRING)
			{
				positive_response ++;
				break;
			}

			else if(response_string.find(OBJECT_NOT_FOUND_STATUS) < NO_SUCH_SUBSTRING)
			{
				erased[i - 1] = 1;
				erasures[numerased] = i - 1;
				numerased ++;

				break;
			}

			else if(response_string.find(TRY_AGAIN_STATUS) < NO_SUCH_SUBSTRING)
			{

				//The "ori_req" is the exact same one from client's update, and server_id stands for the target updating server
				request recon_request_again(ori_req);
				recon_request_again.method = RECONSTRUCT_REQUEST;

				boost::system::error_code err_code;
				boost::asio::write(*(ec_socket.at(i - 1)), recon_request_again.header_to_buffers(), err_code);		
			}
		}
#pragma region light_weight_reconstruct
		if ((positive_response == ERASURE_CODE_K) && (i == ERASURE_CODE_K) && (decode_level == 0))
		{
			/* It means the K data node is normal, light weight reconstruction
			   could be executed, and it's totally ok to happen entirely in this
			   for-loop. And the last handshake-like communication is send in a 
			   reply way but not a request way */

			/* If decode_level isn't equal to 0 ,then this for-loop is just for building connection */
			cout << "A light weight reconstruction is being conducting !!!!!!!" << endl;
			/* Inform the M nodes that they could take a rest */
			for (int ec_count = ERASURE_CODE_K; ec_count < ERASURE_CODE_K + ERASURE_CODE_M ; ec_count ++)
			{
				boost::asio::streambuf tmp_response;
				boost::asio::read_until(*(ec_socket.at(ec_count)), tmp_response, "\r\n");

				boost::system::error_code err_code;
				reply tmp_reply;
				tmp_reply.server_status = MISSION_ABORT_STATUS;
				boost::asio::write(*(ec_socket.at(ec_count)), tmp_reply.simple_ready_buffers(), err_code);
			}

			for (int ec_count = 0; ec_count < ERASURE_CODE_K ; ec_count ++)
			{
				boost::system::error_code err_code;
				reply tmp_reply;
				tmp_reply.server_status = CALL_FOR_LIGHT_WEIGHT_RECONSTRUCT_STATUS;
				boost::asio::write(*(ec_socket.at(ec_count)), tmp_reply.simple_ready_buffers(), err_code);
			}

			/* Open the local path to get ready for the overwritting */
			string local_path = return_full_path(ori_req.obj_id);
			string locking_file = get_locking_file_path(local_path);
			boost::interprocess::file_lock temp_lock(locking_file.c_str());
			temp_lock.lock();
			ofstream output_for_trans(local_path.c_str(), ios::out | ios::binary);

			for(int ec_count = 0; ec_count < ERASURE_CODE_K; ec_count ++)
			{
				unsigned int per_receive = 0;
				unsigned int tmp_count = 0;
				char* for_recv = new char[RECEIVE_BUFFER_SIZE];
				cout << endl << "Now collecting sub data block : " << ec_count + 1 << " with the size of" << ori_req.content_length << endl;
				for(;;)
				{
					//per_receive = boost::asio::read(socket_, boost::asio::buffer(for_recv, RECEIVE_BUFFER_SIZE));
					per_receive = (unsigned int)recv(ec_socket.at(ec_count)->native_handle(), for_recv, RECEIVE_BUFFER_SIZE, 0);

					if (per_receive <= 0 && tmp_count > 0)
					{
						cout << ori_req.obj_id << "Per_receive is ZERO !!! Receiving quits !!!Total amount: " << tmp_count << endl;
						break;
					}
					if (per_receive <=0 && tmp_count <= 0)
					{
						cout << "Something realy really weired happens!" << endl;
						cin >> tmp_count;
						break;
					}
					tmp_count += per_receive;
					cout << "Accumulated received size: " << tmp_count << endl;

					output_for_trans.write(for_recv, per_receive);
					output_for_trans.flush();

					/* Cause the content_length in ori_req is equal to the size of one data segment which is formed by the data node in update request
					   Therefore, the whole object reconstruction needs the original size .*/
					if (tmp_count >= ori_req.content_length)
					{
						break;
					}
				}
				delete []for_recv; 
			}
			output_for_trans.close();
			temp_lock.unlock();
			return 0;
		}
#pragma endregion light_weight_reconstruct

	}

#pragma region heavy_failure_situation

	/* If decode level is 0, and positive response is less than K, then this decoding should be aborted, and whole new encoding should be conducted.
	   If decode level>0 also positive* response is less than K, then TOTAL RECONSTRUCT REQUEST IS INVOKED to the master node */
	if (positive_response < ERASURE_CODE_K)
	{
		if (decode_level == 0)
		{
			boost::system::error_code ignored_ec;
			for (int i = 0; i < ec_socket.size(); i++)
			{
				ec_socket.at(i)->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
				ec_socket.at(i)->close();
			}

			/* Return to Region " transimit_update_content_request"  */
			return 1;
		}
		else if (decode_level > 0)
		{
			boost::system::error_code ignored_ec;
			for (int i = 0; i < ec_socket.size(); i++)
			{
				ec_socket.at(i)->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
				ec_socket.at(i)->close();
			}
			/*Return to region "update request"*/
			return 1;
		}
	}

#pragma endregion heavy_failure_situation
	QueryPerformanceCounter(&t3);

	/* ******* Create coding matrix or bitmatrix ************* */
	matrix = cauchy_good_general_coding_matrix(k, m, w);
	bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, matrix);
	/* ******************************************************* */

	QueryPerformanceCounter(&t4);
	tsec = 0.0;
// 	tsec += t4.tv_usec;
// 	tsec -= t3.tv_usec;
// 	tsec /= 1000000.0;
// 	tsec += t4.tv_sec;
// 	tsec -= t3.tv_sec;
// 	totalsec += tsec;
	totalsec += (t4.QuadPart - t3.QuadPart) * 1.0 / tc.QuadPart;
	
	/* Begin decoding process */
	total = 0;
	n = 1;	

	string local_path = return_full_path(obj_id);
	string temp_locking_file = get_locking_file_path(local_path);
	boost::interprocess::file_lock temp_lock(temp_locking_file.c_str());
	temp_lock.lock();
	ofstream decoded_output(local_path.c_str(), ios::out | ios::binary);
#pragma region while_loop

	while (n <= readins) 
	{
		/*Now for simplicity , we assume that all the server won't neccessarily go down,
		  but to stimulate the scene of the failure object miss could happen.
		  And thus, the RECONSTRUCT request could always receive K+M replies.
		  Data and parity server inject the miss-or-not into the server status. Therefore, 
		  this master node should before the while-loop collect all information to decide whether or not
		  skip the decoding matrix computation phase. And surely , it could just simply solicit transmit-like
		  transfer way to finish the process. But if some node fails ,decoding should be called 
		  and the decoding matrix process and while-loop should be included .*/

		/* Open files, check for erasures, read in data/coding,
		   and this could be completed by the previous connect-phase */	
		for (i = 1; i <= k; i++)
		{
			if (erased[i - 1] != 1) 
			{
				if (buffersize == origsize) 
				{
					/* If decode level is 0 and buffersize could contain the whole file, then blocksize is equal to size of one 
					   segment which is contained in ori_req.content_length ;
					   If decode level > 0 and buffersize could contain the whole segment ,than blocksize should be equal to one segment*/

					blocksize = ori_size;
					data[i-1] = (char *)malloc(sizeof(char)*blocksize);
				}
				else 
				{
					//When the file is bigger than the buffersize, then every time deal with size of buffersize, thus sizeof(data[i]) should be
					//buffersize/k, so it is with coding[i]
				}
				boost::system::error_code err_code;
				reply tmp_reply;
				tmp_reply.server_status = CALL_FOR_NORMAL_RECONSTRUCT_STATUS;
				boost::asio::write(*(ec_socket.at(i - 1)), tmp_reply.simple_ready_buffers(), err_code);

				unsigned int percev = 0;
				unsigned int total_recv = 0;
				char* for_recv = new char[blocksize];
				
				for(;;)
				{
					/* Set up a temporary buffer region for make sure the exact size of data has been read in*/
					percev = recv(ec_socket.at(i - 1)->native_handle(), for_recv, blocksize, 0);
					if(percev >= blocksize)
					{
						strcpy(data[i - 1], (const char*)for_recv);
						for_recv = NULL;
						delete for_recv;
						break;
					}

				}

			}
		}
		for (i = 1; i <= m; i++) 
		{
			if (erased[k + i - 1] != 1)
			{
				if (buffersize == origsize)
				{
					blocksize = ori_size;
					coding[i-1] = (char *)malloc(sizeof(char)*blocksize);
				}
				else 
				{
				}	
				boost::system::error_code err_code;
				reply tmp_reply;
				tmp_reply.server_status = CALL_FOR_NORMAL_RECONSTRUCT_STATUS;
				boost::asio::write(*(ec_socket.at(k + i - 1)), tmp_reply.simple_ready_buffers(), err_code);

				unsigned int percev = 0;
				char* for_recv = new char[blocksize];
				for(;;)
				{
					/* Set up a temporary buffer region for make sure the exact size of data has been read in*/
					percev = recv(ec_socket.at(k + i - 1)->native_handle(), for_recv, blocksize, 0);
					if(percev >= blocksize)
					{
						for(int cpy_count = 0; cpy_count < blocksize; cpy_count ++)
						{
							coding[i - 1][cpy_count] = for_recv[cpy_count];					
						}
						for_recv = NULL;
						delete for_recv;
						break;
					}

				}
			}
		}


		/* Finish allocating data/coding if needed */
		if (n == 1)
		{
			for (i = 0; i < numerased; i++) 
			{
				if (erasures[i] < k) 
				{
					data[erasures[i]] = (char *)malloc(sizeof(char)*blocksize);
				}
				else 
				{
					coding[erasures[i] - k] = (char *)malloc(sizeof(char)*blocksize);
				}
			}
		}
		
		erasures[numerased] = -1;
		QueryPerformanceCounter(&t3);

		//这里的bitmatrix是上已经生成好了的，不需要自己再次调用matrix_to_bitmatrix
		//这里其实运行完这个函数之后，已经把所有的data 和 coding 都已经恢复好了，然后就可以根据相应的erasures数组里面的
		//记录，恢复出相应的东东，erasures最后一个元素是-1，然后用是否大于k来判断是data还是coding
		//然后下面的就直接写进文件即可
		//i = jerasure_bitmatrix_decode(k, m, w, bitmatrix, 1, erasures, data, coding, blocksize, packetsize);
		i = jerasure_schedule_decode_lazy(k, m, w, bitmatrix, erasures, data, coding, blocksize, packetsize, 1);
		QueryPerformanceCounter(&t4);
	
		/* Exit if decoding was unsuccessful */
		if (i == -1) 
		{
			fprintf(stderr, "Unsuccessful!\n");
			exit(0);
		}

		/* ***********************************************************
		                        Create decoded file 
		If the the decoding level is 0, then the for-loop should be controlled in
		k-times in order to creat the whole file , if the plan is greater than 0, 
		than the for-loop should be controlled in once, and the inner variable should
		exactly be the data or parity array that needs to be recovered     */

		if(decode_level == 0)
		{
			//The whole file should be recovered
			for (i = 0; i < k; i++) 
			{
				if (total+blocksize <= origsize) 
				{
					//total 应该是用来表示目前总共写入了多少，然后blocksize相当于是每次以多大的缓冲来写入，其实就是给写入的过程yy了一个文件
					//系统的block,orgsize就是那个完整的文件本来的大小了

					decoded_output.write((const char*)data[i], blocksize);
					total+= blocksize;
				}
				else
				{
					decoded_output.write((const char*)data[i], origsize - total);
					break;
				}
			}
		}
		else if (decode_level > 0)
		{
			if(decode_level <= ERASURE_CODE_K)
			{
				//Recover the data block
				if (total+blocksize <= origsize) 
				{
					//total 应该是用来表示目前总共写入了多少，然后blocksize相当于是每次以多大的缓冲来写入，其实就是给写入的过程yy了一个文件
					//系统的block,orgsize就是那个完整的文件本来的大小了
					decoded_output.write((const char*)data[decode_level - 1], blocksize);
					total+= blocksize;
				}
				else
				{
					decoded_output.write((const char*)data[decode_level - 1], origsize - total);
					break;
				}
			}
			else if (decode_level <= ERASURE_CODE_K + ERASURE_CODE_M)
			{
				//Recover the parity block
				if (total+blocksize <= origsize) 
				{
					//total 应该是用来表示目前总共写入了多少，然后blocksize相当于是每次以多大的缓冲来写入，其实就是给写入的过程yy了一个文件
					//系统的block,orgsize就是那个完整的文件本来的大小了
					//rintf("%s\n", data[i]);
					decoded_output.write((const char*)coding[decode_level - ERASURE_CODE_K - 1], blocksize);
					total+= blocksize;
				}
				else
				{
					decoded_output.write((const char*)coding[decode_level - ERASURE_CODE_K - 1], origsize - total);
					break;
				}
			}
		}

		n++;
		fclose(fp);

// 		tsec = 0.0;
// 		tsec += t4.tv_usec;
// 		tsec -= t3.tv_usec;
// 		tsec /= 1000000.0;
// 		tsec += t4.tv_sec;
// 		tsec -= t3.tv_sec;
// 		totalsec += tsec;
// 
// 		QueryPerformanceCounter(&t4);
// 		totalsec += (t4.QuadPart - t3.QuadPart) * 1.0 / tc.QuadPart;
	}
	decoded_output.close();
#pragma endregion while_loop
	temp_lock.unlock();

	/* Free allocated memory */
	free(cs1);
	free(fname);
	free(data);
	free(coding);
	free(erasures);
	free(erased);
	
	/* Stop timing and print time */
// 	tsec = 0;
// 	tsec += t2.tv_usec;
// 	tsec -= t1.tv_usec;
// 	tsec /= 1000000.0;
// 	tsec += t2.tv_sec;
// 	tsec -= t1.tv_sec;
//     QueryPerformanceCounter(&t2);
// 	tsec = (t2.QuadPart - t1.QuadPart) * 1.0 / tc.QuadPart;
// 	printf("Decoding (MB/sec): %0.10f\n", (origsize/1024/1024)/(totalsec));
// 	printf("De_Total (MB/sec): %0.10f\n", (origsize/1024/1024)/(tsec));

	return 0;
}	
