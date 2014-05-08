#include "encoder.h"

#define talloc(type, num) (type *) malloc(sizeof(type)*(num))

using namespace std;
using boost::asio::ip::tcp;

encoder::encoder()
{
	encode_tech = "cauchy_good";
	readins = 0;
	n = 1;
	ack_received = 0;

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

int encoder::encode_file (ec_io_service_pool& ec_io_service, vector<socket_ptr> ec_socket, request& ori_req, int server_id, string pure_obj_name, string full_local_path) 
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
	
	
	FILE *fp, *fp2;				// file pointers
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

	/* 通过向上向下取整来获得最佳的buffersize  */
	if (buffersize != 0) 
	{
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

	/* 这里以后要考虑并发操作的问题 ， 其他操作也在读或者有的操作在update的时候 */

	fp = fopen(source_path, "rb");
	if (fp == NULL) 
	{
		fprintf(stderr,  "Unable to open file.\n");
		exit(0);
	}
	
	/* Determine original size of file */
	fseek(fp, 0, SEEK_END);
	size = (long)ftell(fp);
	fseek(fp, 0,SEEK_SET);

	newsize = size;

	cout << "The size of the to-be-encoded file is : " << size << "B !!!!!!!!!!!!!!!!!!" << endl;
	
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
	//*************************************************************************
	//这个while就是主模块，不断从一部分一部分地从源文件里面读出数据，然后每读出一部分，就encode一部分，然后将这部分数据写入k+m个不同的文件中
	//**************************************************************************


	//Ready for the coming transmit
	//测试阶段先使得这些要接受数据块和校验快的server是一些端口递增的server进程
	//先向每个server发出一个transmit_data或transmit_parity的请求，然后在下面的while循环的时候再不断地把数据传送到对应的server上

	for (int i = 1; i <= k + m ; i++)
	{

		//Connect the next targeted server and send a transmit_data request
		int next_target_server = server_id + i;
		// Round circle calculation
		if (next_target_server > STARTING_SERVER_ID + NUMBER_OF_SERVER - 1)
		{
			next_target_server -= NUMBER_OF_SERVER;
		}

		tcp::endpoint end_p(boost::asio::ip::address_v4::from_string("127.0.0.1"), next_target_server);
		ec_socket.at(i - 1)->connect(end_p);

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

	/* 这里是生成一个meta文件，记录了所有的encoding过程中所用到的信息，但是这个东西不能光存一份在master
	   可以考虑作为缓存存在proxy server端，也可以在每个相关的server上存一份，再看*/
// 	if (fp != NULL) 
// 	{
// 		sprintf(fname, "%s\\%s_meta.txt", curdir, s1);
// 		fp2 = fopen(fname, "wb");
// 		fprintf(fp2, "%s\n", source_path);
// 		fprintf(fp2, "%d\n", size);
// 		fprintf(fp2, "%d %d %d %d %d %d\n", k, m, w, packetsize, buffersize, readins);
// 		fclose(fp2);
// 	}

	fclose(fp);


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
	return 0;
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