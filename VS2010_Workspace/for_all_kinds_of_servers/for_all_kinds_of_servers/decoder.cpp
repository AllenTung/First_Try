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

	//�����read in��ʵ��һ����������ǰ���buffersize�����ʹ�õģ�����ÿ��decodde��ʱ�����һ��buffersize�ռ�
	//Ȼ��buffersize*readins�ǵ���original size�ģ�Ȼ���������һ����4��2���ķ�������ô4�����ݿ��2��parity�Ƿֿ������
	//��������һ�����Ҫ�����С�ĵ�λ��blocksize������˵ÿ��decode��ʱ�򣬸�ÿ�����ݿ����һ��4��֮һ����ڴ�ռ䣬Ȼ���е�����4��block����һ���ֻ�ԭ������
	//������Ҫ����ٴ������Ĳ������Ǿ���������Ǹ��ܴ��while�������ˣ���readins�����ƴ���
	//����һ��whileѭ�������Ǹ��˱��ָ����Ǹ����ݵ�һ���֣��������ɣ�������ݿ鶥�������ݣ�����Ҫ�������ݿ��У��춥�����������ָ��ģ�Ȼ���в������������˵��в����ָ���
	//Ȼ��ײ����õײ������Ƶģ�����ô��������


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
		//����ԭ����metafile�����¼��readins��buffersize������ռ���
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
	//�����matrix�����Ǹ�encode�ľ�����������˷�����Ҫ���´��������ж�Ӧ�ĺ��������ɣ�decode��ʱ����Ҫ�õ�

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
		//���readins����˵�ж��ٸ�buffersize�Ĵ�С����֪��Ϊʲô��encoder��ʱ��
		//�͸�����һ����Сδ196608Ҳ����192k��buffer��Ȼ������Ļ����������ڲ��Ե��ļ���1G
		//��ô���readins����5462,����������1G
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

		//�����bitmatrix�����Ѿ����ɺ��˵ģ�����Ҫ�Լ��ٴε���matrix_to_bitmatrix
		//������ʵ�������������֮���Ѿ������е�data �� coding ���Ѿ��ָ����ˣ�Ȼ��Ϳ��Ը�����Ӧ��erasures���������
		//��¼���ָ�����Ӧ�Ķ�����erasures���һ��Ԫ����-1��Ȼ�����Ƿ����k���ж���data����coding
		//Ȼ������ľ�ֱ��д���ļ�����
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
				//total Ӧ����������ʾĿǰ�ܹ�д���˶��٣�Ȼ��blocksize�൱����ÿ���Զ��Ļ�����д�룬��ʵ���Ǹ�д��Ĺ���yy��һ���ļ�
				//ϵͳ��block,orgsize�����Ǹ��������ļ������Ĵ�С��
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
