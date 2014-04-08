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
}

int encoder::jfread(void *ptr, int size, int nmembers, FILE *stream)
{
	int nd;
	int *li, i;
	if (stream != NULL)
	{
		return fread(ptr, size, nmembers, stream);
	}
	nd = size/sizeof(int);
	li = (int *) ptr;
	for (i = 0; i < nd; i++)
	{
		li[i] = my_random_long();
	}
	return size;
}

int encoder::encode_file (ec_io_service_pool& ec_io_service, vector<socket_ptr> ec_socket, request& ori_req, int server_id) 
{
	//word_size,即系将一个文件，比如一个大文本，分成了很多个word，然后每个word是多大
	//Packetsize, 必须是word的整数倍，这个encode时的单位
	//buffersize，在下面还要通过上下取整来调整到最佳
	int argc = 8;

	char* source_path = "I://VS2010_Workspace//for_all_kinds_of_servers//for_all_kinds_of_servers//test.txt";
	char* coding_path = "I://VS2010_Workspace//for_all_kinds_of_servers//for_all_kinds_of_servers//Coding";

	int k = ERASURE_CODE_K;
	int m = ERASURE_CODE_M;
	int w = ERASURE_CODE_WORD_SIZE;
	int packetsize = ERASURE_CODE_PACKETSIZE;		
	int buffersize = ERASURE_CODE_BUFFERSIZE;	

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
		if (packetsize != 0 && buffersize%(sizeof(int)*w*k*packetsize) != 0) 
		{ 
			up = buffersize;
			down = buffersize;
			while (up%(sizeof(int)*w*k*packetsize) != 0 && (down%(sizeof(int)*w*k*packetsize) != 0)) 
			{
				up++;
				if (down == 0)
				{
					down--;
				}
			}
			if (up%(sizeof(int)*w*k*packetsize) == 0) 
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
		else if (packetsize == 0 && buffersize%(sizeof(int)*w*k) != 0)
		{
			up = buffersize;
			down = buffersize;
			while (up%(sizeof(int)*w*k) != 0 && down%(sizeof(int)*w*k) != 0) 
			{
				up++;
				down--;
			}
			if (up%(sizeof(int)*w*k) == 0) 
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
	
	/* Create Coding directory */
	i = _mkdir(coding_path);
	if (i == -1 && errno != EEXIST) 
	{
		fprintf(stderr, "Unable to create Coding directory.\n");
		exit(0);
	}
	
	/* Determine original size of file */
	fseek(fp, 0, SEEK_END);
	size = (long)ftell(fp);
	fseek(fp, 0,SEEK_SET);

	newsize = size;
	
	/* Find new size by determining next closest multiple */
	if (packetsize != 0) 
	{
		if (size%(k*w*packetsize*sizeof(int)) != 0) 
		{
			while (newsize%(k*w*packetsize*sizeof(int)) != 0)
			{
				newsize++;
			}
		}
	}
	else
	{
		if (size%(k*w*sizeof(int)) != 0)
		{
			while (newsize%(k*w*sizeof(int)) != 0) 
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
		block = (char *)malloc(sizeof(char)*buffersize);
		blocksize = buffersize/k;
	}
	else 
	{
		readins = 1;
		buffersize = size;
		block = (char *)malloc(sizeof(char)*newsize);
	}
	
	/* Break inputfile name into the filename and extension */	
	s1 = (char*)malloc(sizeof(char)*(strlen(source_path)+10));
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
	s2 = (char*)malloc(sizeof(char)*(strlen(source_path)+5));
	if (fname != NULL)
	{
		strcpy(s2, fname);
	}
	
	/* Allocate for full file name */
	fname = (char*)malloc(sizeof(char)*(strlen(source_path)+strlen(curdir)+10));
	sprintf(temp, "%d", k);
	md = strlen(temp);
	
	/* Allocate data and coding */
	data = (char **)malloc(sizeof(char*)*k);
	coding = (char **)malloc(sizeof(char*)*m);

	for (i = 0; i < m; i++)
	{
		coding[i] = (char *)malloc(sizeof(char)*blocksize);
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

	for (int for_k = 1; for_k <= ERASURE_CODE_K; for_k++)
	{
		int next_target_server = (server_id + for_k) % NUMBER_OF_SERVER;

		//Connect the next targeted server and send a transmit_data request
		tcp::endpoint end_p(boost::asio::ip::address_v4::from_string("127.0.0.1"), next_target_server);
		ec_socket.at(for_k - 1)->connect(end_p);

		request transmit_data_request(ori_req);
		transmit_data_request.method = "TRANSMIT_DATA_BLOCK";
		transmit_data_request.content_length /= ERASURE_CODE_K;

		boost::asio::write(*(ec_socket.at(for_k - 1)), transmit_data_request.to_buffers());		
	}

	for (int for_m = 1; for_m <= ERASURE_CODE_M; for_m++)
	{
		int next_target_server = (server_id + ERASURE_CODE_K + for_m) % NUMBER_OF_SERVER;

		//Connect the next targeted server and send a transmit_data request
		tcp::endpoint end_p(boost::asio::ip::address_v4::from_string("127.0.0.1"), next_target_server);
		ec_socket.at(ERASURE_CODE_K + for_m - 1)->connect(end_p);

		request transmit_data_request(ori_req);
		transmit_data_request.method = "TRANSMIT_PARITY_BLOCK";
		transmit_data_request.content_length /= ERASURE_CODE_K;

		boost::asio::write(*(ec_socket.at(ERASURE_CODE_K + for_m - 1)), transmit_data_request.to_buffers());		
	}

	while (n <= readins) 
	{
		/* Check if padding is needed, if so, add appropriate 
		   number of zeros */
		if (total < size && total+buffersize <= size) 
		{
			total += jfread(block, sizeof(char), buffersize, fp);
		}
		else if (total < size && total+buffersize > size) 
		{
			extra = jfread(block, sizeof(char), buffersize, fp);
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

		*/


		/* Write data and encoded data to k+m files 
		这里是写文件的模块，需要扩展成分布式环境下往不同的server上面写文件，要用到tranmit file的函数
		*********************************************************************************/

		//data segment
		for	(i = 1; i <= k; i++) 
		{
			if (fp == NULL)
			{
				memset(data[i-1], 0, blocksize);
 			} 
			else 
			{
				int temp_length = strlen(data[i - 1]);
				boost::asio::write(*(ec_socket.at(i - 1)),boost::asio::buffer(data[i - 1], temp_length));

// 				sprintf(fname, "%s\\Coding\\%s_k%0*d%s", curdir, s1, md, i, s2);
// 				if (n == 1) 
// 				{
// 					fp2 = fopen(fname, "wb");
// 				}
// 				else 
// 				{
// 					fp2 = fopen(fname, "ab");
// 				}
// 				fwrite(data[i-1], sizeof(char), blocksize, fp2);
// 				fclose(fp2);

			}
			
		}

		//Parity segment
		for	(i = 1; i <= m; i++)
		{
			if (fp == NULL) 
			{
				memset(data[i-1], 0, blocksize);
 			}
			else 
			{
				int temp_length = strlen(coding[i - 1]);
				boost::asio::write(*(ec_socket.at(i - 1)),boost::asio::buffer(coding[i - 1], temp_length));

// 				sprintf(fname, "%s\\Coding\\%s_m%0*d%s", curdir, s1, md, i, s2);
// 				if (n == 1) 
// 				{
// 					fp2 = fopen(fname, "wb");
// 				}
// 				else 
// 				{
// 					fp2 = fopen(fname, "ab");
// 				}
// 				fwrite(coding[i-1], sizeof(char), blocksize, fp2);
// 				fclose(fp2);
			}
		}

		n++;

		/* Calculate encoding time */
		totalsec += (t4.QuadPart - t3.QuadPart) * 1.0 / tc.QuadPart;
	}

	/* Create metadata file */

	/* 这里是生成一个meta文件，记录了所有的encoding过程中所用到的信息，但是这个东西不能光存一份在master
	   可以考虑作为缓存存在proxy server端，也可以在每个相关的server上存一份，再看*/
	if (fp != NULL) 
	{
		sprintf(fname, "%s\\Coding\\%s_meta.txt", curdir, s1);
		fp2 = fopen(fname, "wb");
		fprintf(fp2, "%s\n", source_path);
		fprintf(fp2, "%d\n", size);
		fprintf(fp2, "%d %d %d %d %d %d\n", k, m, w, packetsize, buffersize, readins);
		fclose(fp2);
	}
	/* Free allocated memory */
	free(s2);
	free(s1);
	free(fname);
	free(block);
	free(curdir);
	
	/* Calculate rate in MB/sec and print */
	//gettimeofday(&t2, &tz);
	QueryPerformanceCounter(&t2);
	tsec = (t2.QuadPart - t1.QuadPart) * 1.0 / tc.QuadPart;

	printf("Encoding (MB/sec): %0.10f\n", (size/1024/1024)/(totalsec));
	printf("En_Total (MB/sec): %0.10f\n", (size/1024/1024)/(tsec));


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