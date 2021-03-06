#pragma once
#include "encoder.h"
#include "server.hpp"
#define talloc(type, num) (type *) malloc(sizeof(type)*(num))

using namespace std;
using boost::asio::ip::tcp;

class server;

encoder::encoder()
{
	encode_tech = "cauchy_good";
	readins = 0;
	n = 1;
	ack_received = 0;
	/* sizeof(char) == 1 */
	unit_size = sizeof(char);
}
void encoder::gather_ack(const boost::system::error_code& e)
{
	ack_received ++;

	cout << "********************************************" << endl;
	cout << "********** Now the number of ACK is: " << ack_received << "********" << endl;
	cout << "********************************************" << endl;
}
int encoder::jfread(void *ptr, int size, int nmembers, FILE *stream)
{
	int nd;
	int *li, i;
	if (stream != NULL)
	{
		return fread(ptr, size, nmembers, stream);
	}
	nd = size/unit_size;
	li = (int *) ptr;
	for (i = 0; i < nd; i++)
	{
		li[i] = my_random_long();
	}
	return size;
}

int* encoder::encode_file (int recorder[], ec_io_service_pool& ec_io_service, vector<socket_ptr> ec_socket, request& ori_req, int server_id, string local_ip, string pure_obj_name, string full_local_path) 
{
	//word_size,即系将一个文件，比如一个大文本，分成了很多个word，然后每个word是多大
	//Packetsize, 必须是word的整数倍，这个encode时的单位
	//buffersize，在下面还要通过上下取整来调整到最佳

	char* source_path = const_cast<char*>(full_local_path.c_str());

	int k = ERASURE_CODE_K;
	int m = ERASURE_CODE_M;
	int w = ERASURE_CODE_W;
	int packetsize = ERASURE_CODE_PACKETSIZE;		
	int buffersize = 8192;


	int i;				
	int blocksize;					
	int total;
	int extra;
	
	
	FILE *fp;				// file pointers
	char *block;				// padding file
	int size, newsize;			// size of file and temp size 

	/* Jerasure Arguments */

	/* Encoding 的过程中存放数据块和parity */
	char **data;				
	char **coding;
	/**********************************/

	int *matrix = NULL;
	int *bitmatrix = NULL;
	int **schedule = NULL;
	int *erasure = NULL;
	int *erased = NULL;
	
	/* Creation of file name variables */
	char temp[5];
	char *s1, *s2;
	char *fname;
	int md;
	char *curdir;
	
	/* Timing variables */
	LARGE_INTEGER t1, t2, t3, t4, tc, start/*, stop*/;
	/* tsec and totalsec are expressed in second */
	double tsec, totalsec;

	/* Initial time counting */
	QueryPerformanceFrequency(&tc);

	/* Find buffersize */
	int up, down;

	/* Start timing */
	/*gettimeofday(&t1, &tz);*/
	QueryPerformanceCounter(&t1);
	totalsec = 0.0;

	if (buffersize != 0) 
	{
		/* unit_size * w * k * packetsize = 1 * 8 * 4 * 1024 = 32k
		   According to current setting ,buffersize would remain at 32k, and blocksize 8k, thus it's ok to skip the trivialness of segmentation transmission*/
		if (packetsize != 0 && buffersize%(unit_size*w*k*packetsize) != 0) 
		{ 
			up = buffersize;
			down = buffersize;
			while (up%(unit_size*w*k*packetsize) != 0 && (down%(unit_size*w*k*packetsize) != 0)) 
			{
				up++;
				if (down == 0)
				{
					down--;
				}
			}
			if (up%(unit_size*w*k*packetsize) == 0) 
			{
				buffersize = up;
			}
			else 
			{
				if (down != 0) 
				{
					buffersize = down;
				}
			}
		}
		else if (packetsize == 0 && buffersize%(unit_size*w*k) != 0)
		{
			up = buffersize;
			down = buffersize;
			while (up%(unit_size*w*k) != 0 && down%(unit_size*w*k) != 0) 
			{
				up++;
				down--;
			}
			if (up%(unit_size*w*k) == 0) 
			{
				buffersize = up;
			}
			else 
			{
				buffersize = down;
			}
		}
	}

	/* Get current working directory for construction of file names */
	curdir = (char*)malloc(sizeof(char)*1000);	
	_getcwd(curdir, 1000);

	fp = fopen(source_path, "rb");
	if (fp == NULL) 
	{
		fprintf(stderr,  "Unable to open file.\n");
		exit(0);
	}

	size = size_of_file(full_local_path);
	newsize = size;

	cout << "The size of the to-be-encoded file is : " << size << "B !" << endl;
	
	/* Find new size by determining next closest multiple */
	if (packetsize != 0) 
	{
		if (size%(k*w*packetsize*unit_size) != 0) 
		{
			while (newsize%(k*w*packetsize*unit_size) != 0)
			{
				newsize++;
			}
		}
	}
	else
	{
		if (size%(k*w*unit_size) != 0)
		{
			while (newsize%(k*w*unit_size) != 0) 
				newsize++;
		}
	}

	if (buffersize != 0) 
	{
		while (newsize%buffersize != 0) 
		{
			newsize++;
		}
	}

	/* Determine size of k+m files */
	blocksize = newsize/k;

	/* Allow for buffersize and determine number of read-ins */
	if (size > buffersize && buffersize != 0) 
	{
		if (newsize%buffersize != 0)
		{
			readins = newsize/buffersize;
		}
		else
		{
			readins = newsize/buffersize;
		}
		block = (char *)malloc(unit_size*buffersize);
		blocksize = buffersize/k;
	}
	else 
	{
		readins = 1;
		buffersize = size;
		block = (char *)malloc(unit_size*newsize);
	}
	
	cout << "buffersize:" << buffersize << endl;
	cout << "newsize:" << newsize << endl;
	cout << "size:" << size << endl;
	cout << "blocksize:" << blocksize << endl;

	/* Break inputfile name into the filename and extension */	
	s1 = (char*)malloc(sizeof(char)*((strlen(source_path) + 10)));
	s2 = strrchr(source_path, '/');

	if (s2 != NULL) 
	{
		s2++;
		strcpy(s1, s2);
	}
	else 
	{
		strcpy(s1, source_path);
	}
	s2 = strchr(s1, '.');
	if (s2 != NULL) 
	{
		*s2 = '\0';
	}
	fname = strchr(source_path, '.');
	s2 = (char*)malloc(sizeof(char)*(strlen(source_path) + 5));
	if (fname != NULL)
	{
		strcpy(s2, fname);
	}
	
	/* Allocate for full file name */
	fname = (char*)malloc(sizeof(char)*(strlen(source_path) + strlen(curdir) + 10));
	sprintf(temp, "%d", k);
	md = strlen(temp);
	
	/* Allocate data and coding */
	data = (char**)malloc(sizeof(char*)*k);
	coding = (char**)malloc(sizeof(char*)*m);

	for (i = 0; i < m; i++)
	{
		coding[i] = (char *)malloc(unit_size*blocksize);
	}

	/* Create coding matrix or bitmatrix and schedule */
	/*gettimeofday(&t3, &tz);*/
	QueryPerformanceCounter(&t3);

	//准备encode所需要的一切材料
	matrix = cauchy_good_general_coding_matrix(k, m, w);
	bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, matrix);
	schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, bitmatrix);
	//*****************************************************************


// 	gettimeofday(&start, &tz);	
// 	gettimeofday(&t4, &tz);
	QueryPerformanceCounter(&start);
	QueryPerformanceCounter(&t4);

	totalsec += (t4.QuadPart - t3.QuadPart) * 1.0 / tc.QuadPart;
	total = 0;
	server::ec_port_lock.lock();
	int temp_ec_port = get_random_ec_port(server_id);
	server::ec_port_lock.unlock();
	for (int i = 1; i <= k + m ; i++)
	{

		//Connect the next targeted server and send a transmit_data request
		int next_target_server = server_id + i;
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

		request transmit_data_request(ori_req);
		if (i <= k)
		{
			transmit_data_request.method = "TRANSMIT_DATA_BLOCK";
			sprintf(fname, "%s\\%s_k%0*d%s", curdir, s1, md, i, s2);
		}
		else
		{
			transmit_data_request.method = "TRANSMIT_PARITY_BLOCK";
			sprintf(fname, "%s\\%s_m%0*d%s", curdir, s1, md, i - k, s2);
		}
		//Use the pure object name to construct the request
		transmit_data_request.content_length /= k;
		transmit_data_request.obj_id = pure_obj_name;
		transmit_data_request.data_type = types[i];
		transmit_data_request.server_id = server_id;

		boost::asio::write(*(ec_socket.at(i - 1)), transmit_data_request.to_buffers());		
	}

	string temp_locking_file = get_locking_file_path(full_local_path);
	boost::interprocess::file_lock temp_lock(temp_locking_file.c_str());
	temp_lock.lock();
	fp = fopen(source_path, "rb");

#pragma region while_loop

	while (n <= readins) 
	{
		/* Check if padding is needed, if so, add appropriate number of zeros */

		cout << "N:  " << n << endl;
		cout << "Readins: " << readins << endl;

		if (total < size && total+buffersize <= size) 
		{
			total += jfread(block, unit_size, buffersize, fp);
		}
		else if (total < size && total+buffersize > size) 
		{
			extra = jfread(block, unit_size, buffersize, fp);
			for (i = extra; i < buffersize; i++) {
				block[i] = '0';
			}
		}
		else if (total == size) 
		{
			for (i = 0; i < buffersize; i++) 
			{
				block[i] = '0';
			}
		}
	
		/* Set pointers to point to file data */
		for (i = 0; i < k; i++) 
		{
			data[i] = block+(i*blocksize);
		}


		/* This is where we encode ! Encode according to coding method 
		*********************************************************************************/

		QueryPerformanceCounter(&t3);
		jerasure_schedule_encode(k, m, w, schedule, data, coding, blocksize, packetsize);
		QueryPerformanceCounter(&t4);

		/**********************************************************************************
		Write data and encoded data to k+m files 
		这里是写文件的模块，需要扩展成分布式环境下往不同的server上面写文件，要用到tranmit file的函数
		*********************************************************************************/

		//Data Segment
		for	(i = 0; i < k + m; i++) 
		{
			if (fp == NULL) 
			{
				memset(data[i], 0, blocksize);
			}

			if (n == 1)
			{
				//Consume the simple reply from other servers as ACK for the first transmit
				//If no ready status is perceived, then re-send the request
				for(;;)
				{
					boost::asio::streambuf tmp_response;
					boost::asio::read_until(*(ec_socket.at(i)), tmp_response, "\r\n");
					istream update_stream(&tmp_response);
					string response_string;
					update_stream >> response_string;


					if(response_string.find(READY_FOR_POST_STATUS) < NO_SUCH_SUBSTRING)
					{
						cout << "***************************************" << endl;
						cout << "Server: " << server_id + i + 1 << "is totally ready !!!!!" << endl;
						cout << "***************************************" << endl;
						break;
					}

					else if(response_string.find(TRY_AGAIN_STATUS) < NO_SUCH_SUBSTRING)
					{
						cout << "***************************************" << endl;
						cout << "Server: " << server_id + i + 1 << "is somehow blocked !!!!!" << endl;
						cout << "***************************************" << endl;

// 						int forcin = 0;
// 						cin >> forcin;

						request transmit_data_request(ori_req);
						if (i < k)
						{
							transmit_data_request.method = TRANSMIT_DATA_BLOCK_REQUEST;
							sprintf(fname, "%s\\%s_k%0*d%s", curdir, s1, md, i + 1, s2);
						}
						else
						{
							transmit_data_request.method = TRANSMIT_PARITY_BLOCK_REQUEST;
							sprintf(fname, "%s\\%s_m%0*d%s", curdir, s1, md, i - k + 1, s2);
						}
						//Use pure object name
						transmit_data_request.content_length /= k;
						transmit_data_request.obj_id = pure_obj_name;
						transmit_data_request.data_type = types[i + 1];
						transmit_data_request.server_id = server_id;
						boost::asio::write(*(ec_socket.at(i)), transmit_data_request.to_buffers());	
					}

				}
			}

			cout << "********* Child Block: = " << i << " ******" << endl;

			boost::system::error_code err_code;
			if (i < k)
			{
				boost::asio::write(*(ec_socket.at(i)), boost::asio::buffer(data[i], blocksize), err_code);
				//boost::asio::async_write(*(ec_socket.at(i)), boost::asio::buffer(data[i], blocksize), boost::bind(&encoder::gather_ack, this, err_code));
			}
			else
			{
				boost::asio::write(*(ec_socket.at(i)), boost::asio::buffer(coding[i - k], blocksize), err_code);
				//boost::asio::async_write(*(ec_socket.at(i)), boost::asio::buffer(coding[i - k], blocksize), err_code, boost::bind(&encoder::gather_ack, this, err_code));
			}
		}

		n++;
		totalsec += (t4.QuadPart - t3.QuadPart) * 1.0 / tc.QuadPart;
	}
#pragma endregion while_loop

	fclose(fp);
	temp_lock.unlock();

	/* Free allocated memory */
	free(s2);
	free(s1);
	free(fname);
	free(block);
	free(curdir);
// 	free(coding);
// 	free(source_path);
	/* Calculate rate in MB/sec and print */
	//gettimeofday(&t2, &tz);
	QueryPerformanceCounter(&t2);
	tsec = (t2.QuadPart - t1.QuadPart) * 1.0 / tc.QuadPart;

// 	printf("Encoding (MB/sec): %0.10f\n", (size/1024/1024)/(totalsec));
// 	printf("En_Total (MB/sec): %0.10f\n", (size/1024/1024)/(tsec));
	/* recorder: readins, buffersize*/
	recorder[0] = readins;
	recorder[1] = buffersize;
	return recorder;
}

void encoder::handle_write(const boost::system::error_code& e)
{
	if (!e)
	{
		cout << e.message() << endl;
	}
	else
	{
		cout << "Nothing !" << endl;
	}

	//Initiate graceful connection closure
	boost::system::error_code ignored_ec;
// 	file_.close();
// 	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
// 	socket_.close();
	ack_received ++ ;

	cout << "***************************************************************" << endl;
	cout << "*************************    " << ack_received << "      ***********"<< endl;
	cout << "***************************************************************" << endl;
}