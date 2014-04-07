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
#include <signal.h>
#include <direct.h>
using namespace std; 

#define talloc(type, num) (type *) malloc(sizeof(type)*(num))
#define N 10

int readins, n;

enum Coding_Technique {Reed_Sol_Van, Reed_Sol_R6_Op, Cauchy_Orig, Cauchy_Good, Liberation, Blaum_Roth, Liber8tion, RDP, EVENODD, No_Coding};

char *Methods[N] = {"reed_sol_van", "reed_sol_r6_op", "cauchy_orig", "cauchy_good", "liberation", "blaum_roth", "liber8tion", "no_coding"};

/* Global variables for signal handler */

enum Coding_Technique method;
/* is_prime returns 1 if number if prime, 0 if not prime */
int is_prime(int w) 
{
	int prime55[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,
		73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,
		181,191,193,197,199,211,223,227,229,233,239,241,251,257};
	int i;
	for (i = 0; i < 55; i++)
	{
		if (w%prime55[i] == 0) 
		{
			if (w == prime55[i]) return 1;
			else { return 0; }
		}
	}
}

/* Handles ctrl-\ event */
void ctrl_bs_handler(int dummy)
{
	time_t mytime;
	mytime = time(0);
	fprintf(stderr, "\n%s\n", ctime(&mytime));
	fprintf(stderr, "You just typed ctrl-\\ in encoder.cpp.\n");
	fprintf(stderr, "Total number of read ins = %d\n", readins);
	fprintf(stderr, "Current read in: %d\n", n);
	fprintf(stderr, "Method: %s\n\n", Methods[method]);	
	signal(SIGINT, ctrl_bs_handler);
}

int jfread(void *ptr, int size, int nmembers, FILE *stream)
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

int main () 
{
	//word_size,即系将一个文件，比如一个大文本，分成了很多个word，然后每个word是多大
	//Packetsize, 必须是word的整数倍，这个encode时的单位
	//buffersize，在下面还要通过上下取整来调整到最佳
	int argc = 8;

	char **argv;
	char* source_path = "I://VS2010_Workspace//j_lib_test//j_lib_test//test.txt";
	char* coding_path = "I://VS2010_Workspace//j_lib_test//j_lib_test//Coding";
	char* coding_tech = "cauchy_good";

	int k = 6;
	int m = 2;
	int w = 8;
	int packetsize = 1024;		
	int buffersize = 1024;	

	int i, j;				
	int blocksize;					
	int total;
	int extra;
	
	
	FILE *fp, *fp2;				// file pointers
	char *memblock;				// reading in file
	char *block;				// padding file
	int size, newsize;			// size of file and temp size 
	struct stat status;			// finding file size
	
	enum Coding_Technique tech;		// coding technique (parameter)

	/* Jerasure Arguments */
	char **data;				
	char **coding;
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
	LARGE_INTEGER t1, t2, t3, t4, tc, start, stop;
	/* tsec and totalsec are expressed in second */
	double tsec, totalsec;

	/* Initial time counting */
	QueryPerformanceFrequency(&tc);

	/* Find buffersize */
	int up, down;
	signal(SIGINT, ctrl_bs_handler);

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

	/* Setting of coding technique and error checking */
	
	if (strcmp(coding_tech, "no_coding") == 0) 
	{
		tech = No_Coding;
	}
	else if (strcmp(coding_tech, "reed_sol_van") == 0) 
	{
		tech = Reed_Sol_Van;
	}
	else if (strcmp(coding_tech, "reed_sol_r6_op") == 0) 
	{
		tech = Reed_Sol_R6_Op;
	}
	else if (strcmp(coding_tech, "cauchy_orig") == 0) 
	{
		tech = Cauchy_Orig;
	}
	else if (strcmp(coding_tech, "cauchy_good") == 0) 
	{
		tech = Cauchy_Good;
	}
	else if (strcmp(coding_tech, "liberation") == 0) 
	{
		tech = Liberation;
	}
	else if (strcmp(coding_tech, "blaum_roth") == 0)
	{
		tech = Blaum_Roth;
	}
	else if (strcmp(coding_tech, "liber8tion") == 0) 
	{
		tech = Liber8tion;
	}

	/* Set global variable method for signal handler */
	method = tech;

	/* Get current working directory for construction of file names */
	curdir = (char*)malloc(sizeof(char)*1000);	
	getcwd(curdir, 1000);

	fp = fopen(source_path, "rb");
	if (fp == NULL) 
	{
		fprintf(stderr,  "Unable to open file.\n");
		exit(0);
	}
	
	/* Create Coding directory */
	i = mkdir(coding_path);
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
	switch(tech)
	{
		case No_Coding:
			break;
		case Reed_Sol_Van:
			matrix = reed_sol_vandermonde_coding_matrix(k, m, w);
			break;
		case Cauchy_Orig:
			matrix = cauchy_original_coding_matrix(k, m, w);
			bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, matrix);
			schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, bitmatrix);
			break;
		case Cauchy_Good:
			matrix = cauchy_good_general_coding_matrix(k, m, w);
			bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, matrix);
			schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, bitmatrix);
			break;	
		case Liberation:
			bitmatrix = liberation_coding_bitmatrix(k, w);
			schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, bitmatrix);
			break;
		case Blaum_Roth:
			bitmatrix = blaum_roth_coding_bitmatrix(k, w);
			schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, bitmatrix);
			break;
		case Liber8tion:
			bitmatrix = liber8tion_coding_bitmatrix(k);
			schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, bitmatrix);
			break;
	}
// 	gettimeofday(&start, &tz);	
// 	gettimeofday(&t4, &tz);
	QueryPerformanceCounter(&start);
	QueryPerformanceCounter(&t4);

	totalsec += (t4.QuadPart - t3.QuadPart) * 1.0 / tc.QuadPart;
	
	/* Read in data until finished */
	n = 1;
	total = 0;

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
		
		QueryPerformanceCounter(&t3);

		/* Encode according to coding method */
		switch(tech) 
		{	
			case No_Coding:
				break;
			case Reed_Sol_Van:
				jerasure_matrix_encode(k, m, w, matrix, data, coding, blocksize);
				break;
			case Reed_Sol_R6_Op:
				reed_sol_r6_encode(k, w, data, coding, blocksize);
				break;
			case Cauchy_Orig:
				jerasure_schedule_encode(k, m, w, schedule, data, coding, blocksize, packetsize);
				break;
			case Cauchy_Good:
				jerasure_schedule_encode(k, m, w, schedule, data, coding, blocksize, packetsize);
				break;
			case Liberation:
				jerasure_schedule_encode(k, m, w, schedule, data, coding, blocksize, packetsize);
				break;
			case Blaum_Roth:
				jerasure_schedule_encode(k, m, w, schedule, data, coding, blocksize, packetsize);
				break;
			case Liber8tion:
				jerasure_schedule_encode(k, m, w, schedule, data, coding, blocksize, packetsize);
				break;
		}
		//gettimeofday(&t4, &tz);
		QueryPerformanceCounter(&t4);

		/* Write data and encoded data to k+m files */
		//这里是写文件的模块，需要扩展成分布式环境下往不同的server上面写文件，要用到tranmit file的函数

		//写data segment
		for	(i = 1; i <= k; i++) 
		{
			if (fp == NULL)
			{
				memset(data[i-1], 0, blocksize);
 			} 
			else 
			{
				sprintf(fname, "%s\\Coding\\%s_k%0*d%s", curdir, s1, md, i, s2);
				if (n == 1) 
				{
					fp2 = fopen(fname, "wb");
				}
				else 
				{
					fp2 = fopen(fname, "ab");
				}
				fwrite(data[i-1], sizeof(char), blocksize, fp2);
				fclose(fp2);
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
				sprintf(fname, "%s\\Coding\\%s_m%0*d%s", curdir, s1, md, i, s2);
				if (n == 1) 
				{
					fp2 = fopen(fname, "wb");
				}
				else 
				{
					fp2 = fopen(fname, "ab");
				}
				fwrite(coding[i-1], sizeof(char), blocksize, fp2);
				fclose(fp2);
			}
		}

		n++;

		/* Calculate encoding time */
		totalsec += (t4.QuadPart - t3.QuadPart) * 1.0 / tc.QuadPart;

	}

	/* Create metadata file */
	if (fp != NULL) 
	{
		sprintf(fname, "%s\\Coding\\%s_meta.txt", curdir, s1);
		fp2 = fopen(fname, "wb");
		fprintf(fp2, "%s\n", source_path);
		fprintf(fp2, "%d\n", size);
		fprintf(fp2, "%d %d %d %d %d\n", k, m, w, packetsize, buffersize);
		fprintf(fp2, "%s\n", coding_tech);
		fprintf(fp2, "%d\n", tech);
		fprintf(fp2, "%d\n", readins);
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

