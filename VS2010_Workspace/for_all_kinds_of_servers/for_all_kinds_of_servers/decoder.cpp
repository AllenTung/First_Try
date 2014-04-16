#include "decoder.h"

decoder::decoder()
{
	encode_method = "cauchy_good";
	readins = 0;
	n = 1;
}

int decoder::decode_file(string obj_id)
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
	
	int k, m, w, packetsize, buffersize;
	
	int i, j;				// loop control variables
	int blocksize;			// size of individual files
	int origsize;			// size of file before padding
	int total;				// used to write data, not padding to file
	int numerased;			// number of erased files
		
	/* Used to recreate file names */
	char *temp;
	char *cs1, *cs2;
	char *fname;
	int md;
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

	/* Read in parameters from metadata file */
	sprintf(fname, "%s\\Coding\\%s_meta.txt", curdir, cs1);

	fp = fopen(fname, "rb");
	temp = (char *)malloc(sizeof(char)*(strlen(source_path)+10));
	fscanf(fp, "%s", temp);	
	
	if (fscanf(fp, "%d", &origsize) != 1)
	{
		fprintf(stderr, "Original size is not valid\n");
		exit(0);
	}
	int temp_count = fscanf(fp, "%d %d %d %d %d %d", &k, &m, &w, &packetsize, &buffersize, &readins);
	if(temp_count != 6)
	{
		fprintf(stderr, "Parameters are not correct\n");
		exit(0);
	}

	fclose(fp);	

	//这里的read in其实是一个次数，跟前面的buffersize是配合使用的，就是每次decodde的时候分配一个buffersize空间
	//然后buffersize*readins是等于original size的，然后比如用了一个（4，2）的方案，那么4个数据块和2个parity是分开来搞的
	//接下来的一层就是要分配更小的单位，blocksize，就是说每次decode的时候，给每个数据块分配一个4分之一大的内存空间，然后并行地用这4个block来搞一部分还原的数据
	//而至于要搞多少次这样的操作，那就是下面的那个很大的while来处理了，用readins来控制次数
	//所以一次while循环，就是搞了被恢复的那个数据的一部分，可以理解成，这个数据块顶部的数据，就是要其他数据块或校验快顶部的数据来恢复的，然后中部的是用其他人的中部来恢复的
	//然后底部是用底部来控制的，就这么理解可以了


	/* Allocate memory */
	erased = (int *)malloc(sizeof(int)*(k+m));
	for (i = 0; i < k+m; i++)
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
		//这里原来的metafile里面记录的readins和buffersize来分配空间是
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

	sprintf(temp, "%d", k);
	md = strlen(temp);
	QueryPerformanceCounter(&t3);

	/* Create coding matrix or bitmatrix */
	//这里的matrix就是那个encode的矩阵，这里标明了方法就要往下传，反正有对应的函数来生成，decode的时候是要用的

	matrix = cauchy_good_general_coding_matrix(k, m, w);
	bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, matrix);

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
	while (n <= readins) 
	{
		//这个readins就是说有多少个buffersize的大小，不知道为什么在encoder的时候，
		//就给定了一个大小未196608也就是192k的buffer，然后这里的话比如我现在测试的文件是1G
		//那么这个readins就是5462,乘起来就是1G
		numerased = 0;
		/* Open files, check for erasures, read in data/coding */	
		for (i = 1; i <= k; i++)
		{
			sprintf(fname, "%s\\Coding\\%s_k%0*d%s", curdir, cs1, md, i, cs2);
			fp = fopen(fname, "rb");
			if (fp == NULL) 
			{
				erased[i-1] = 1;
				erasures[numerased] = i-1;
				numerased++;
				//printf("%s failed\n", fname);
			}
			else 
			{
				if (buffersize == origsize) 
				{
					fseek(fp, 0, SEEK_END);
					blocksize = (long)ftell(fp);
					fseek(fp, 0,SEEK_SET);
// 					stat(fname, &status);
// 					blocksize = status.st_size;
					data[i-1] = (char *)malloc(sizeof(char)*blocksize);
					fread(data[i-1], sizeof(char), blocksize, fp);
				}
				else 
				{
					fseek(fp, blocksize*(n-1), SEEK_SET); 
					fread(data[i-1], sizeof(char), buffersize/k, fp);
				}
				fclose(fp);
			}
		}
		for (i = 1; i <= m; i++) 
		{
			sprintf(fname, "%s\\Coding\\%s_m%0*d%s", curdir, cs1, md, i, cs2);
			fp = fopen(fname, "rb");
			if (fp == NULL)
			{
				erased[k+(i-1)] = 1;
				erasures[numerased] = k+i-1;
				numerased++;
				//printf("%s failed\n", fname);
			}
			else
			{
				if (buffersize == origsize)
				{
					fseek(fp, 0, SEEK_END);
					blocksize = (long)ftell(fp);
					fseek(fp, 0,SEEK_SET);

					coding[i-1] = (char *)malloc(sizeof(char)*blocksize);
					fread(coding[i-1], sizeof(char), blocksize, fp);
				}
				else 
				{
					fseek(fp, blocksize*(n-1), SEEK_SET);
					fread(coding[i-1], sizeof(char), blocksize, fp);
				}	
				fclose(fp);
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
					coding[erasures[i]-k] = (char *)malloc(sizeof(char)*blocksize);
				}
			}
		}
		
		erasures[numerased] = -1;
		QueryPerformanceCounter(&t3);

		//这里的bitmatrix是上已经生成好了的，不需要自己再次调用matrix_to_bitmatrix
		//这里其实运行完这个函数之后，已经把所有的data 和 coding 都已经恢复好了，然后就可以根据相应的erasures数组里面的
		//记录，恢复出相应的东东，erasures最后一个元素是-1，然后用是否大于k来判断是data还是coding
		//然后下面的就直接写进文件即可
		i = jerasure_bitmatrix_decode(k, m, w, bitmatrix, 1, erasures, data, coding, blocksize, packetsize);

		QueryPerformanceCounter(&t4);
	
		/* Exit if decoding was unsuccessful */
		if (i == -1) 
		{
			fprintf(stderr, "Unsuccessful!\n");
			exit(0);
		}

		/* Create decoded file */
		sprintf(fname, "%s\\Coding\\%s_decoded%s", curdir, cs1, cs2);
		if (n == 1) 
		{
			fp = fopen(fname, "wb");
		}
		else
		{
			fp = fopen(fname, "ab");
		}
		for (i = 0; i < 1; i++) 
		{
			if (total+blocksize <= origsize) 
			{
				//total 应该是用来表示目前总共写入了多少，然后blocksize相当于是每次以多大的缓冲来写入，其实就是给写入的过程yy了一个文件
				//系统的block,orgsize就是那个完整的文件本来的大小了
				//rintf("%s\n", data[i]);
				fwrite(data[2], sizeof(char), blocksize, fp);
				total+= blocksize;
			}
			else
			{
				for (j = 0; j < blocksize; j++) 
				{
					if (total < origsize)
					{
						fprintf(fp, "%c", data[i][j]);
						total++;
					}
					else 
					{
						break;
					}
					
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
    QueryPerformanceCounter(&t2);
	tsec = (t2.QuadPart - t1.QuadPart) * 1.0 / tc.QuadPart;
	printf("Decoding (MB/sec): %0.10f\n", (origsize/1024/1024)/(totalsec));
	printf("De_Total (MB/sec): %0.10f\n", (origsize/1024/1024)/(tsec));

	int for_pause = 0;
	cin>>for_pause;

	return 0;
}	
