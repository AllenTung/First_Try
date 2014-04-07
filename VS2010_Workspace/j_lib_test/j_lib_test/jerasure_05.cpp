extern "C"
{
#include "jerasure.h"
#include "cauchy.h"
#include "galois.h"
#include "liberation.h"
#include "reed_sol.h"
};
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "external_tool.h"
#include <boost/random.hpp>
using namespace std; 

#define talloc(type, num) (type *) malloc(sizeof(type)*(num))

static void print_data_and_coding(int k, int m, int w, int size, 
	char **data, char **coding) 
{
	int i, j, x;
	int n, sp;
	long l;

	if(k > m) n = k;
	else n = m;
	sp = size * 2 + size/(w/8) + 8;

	printf("%-*sCoding\n", sp, "Data");
	for(i = 0; i < n; i++) {
		if(i < k) {
			printf("D%-2d:", i);
			for(j=0;j< size; j+=(w/8)) { 
				printf(" ");
				for(x=0;x < w/8;x++){
					printf("%02x", (unsigned char)data[i][j+x]);
				}
			}
			printf("    ");
		}
		else printf("%*s", sp, "");
		if(i < m) {
			printf("C%-2d:", i);
			for(j=0;j< size; j+=(w/8)) { 
				printf(" ");
				for(x=0;x < w/8;x++){
					printf("%02x", (unsigned char)coding[i][j+x]);
				}
			}
		}
		printf("\n");
	}
	printf("\n");
}

int main()
{
	long long l;
	int k, m, w, size;
	int i, j;
	int *matrix;
	char **data, **coding;
	int *erasures, *erased;
	int *decoding_matrix, *dm_ids;
	//scanf("%d %d %d %d", &k, &m, &w, &size);	
	k=3;
	m=4;
	w=8;
	size=4;

	//这里搞一个矩阵，后面decode的时候也是传这个进去。
	matrix = talloc(int, m*k);
	for (i = 0; i < m; i++) {
		for (j = 0; j < k; j++) {
			matrix[i*k+j] = galois_single_divide(1, i ^ (m + j), w);
		}
	}

	printf("The Coding Matrix (the last m rows of the Distribution Matrix):\n\n");
	jerasure_print_matrix(matrix, m, k, w);
	printf("\n");

	data = talloc(char *, k);
	for (i = 0; i < k; i++) {
		data[i] = talloc(char, size);
		for(j = 0; j < size; j+=(sizeof(long))) {
			l = my_random_long();
			memcpy(data[i] + j, &l, (sizeof(long)));
		}
	}

	coding = talloc(char *, m);
	for (i = 0; i < m; i++) {
		coding[i] = talloc(char, size);
	}

	jerasure_matrix_encode(k, m, w, matrix, data, coding, size);

	printf("Encoding Complete:\n\n");

	print_data_and_coding(k, m, w, size, data, coding);

	erasures = talloc(int, (m+1));
	erased = talloc(int, (k+m));
	for (i = 0; i < m+k; i++) erased[i] = 0;
	l = 0;
	for (i = 0; i < m; ) {
		erasures[i] = rand()%(k+m);
		if (erased[erasures[i]] == 0) {
			erased[erasures[i]] = 1;

			memset((erasures[i] < k) ? data[erasures[i]] : coding[erasures[i]-k], 0, size);
			i++;
		}
	}
	erasures[i] = -1;

	printf("Erased %d random devices:\n\n", m);
	print_data_and_coding(k, m, w, size, data, coding);

	i = jerasure_matrix_decode(k, m, w, matrix, 0, erasures, data, coding, size);

	printf("State of the system after decoding:\n\n");
	print_data_and_coding(k, m, w, size, data, coding);

	decoding_matrix = talloc(int, k*k);
	dm_ids = talloc(int, k);

	for (i = 0; i < m; i++) erased[i] = 1;
	for (; i < k+m; i++) erased[i] = 0;

	//搞个解码矩阵出来
	jerasure_make_decoding_matrix(k, m, w, matrix, erased, decoding_matrix, dm_ids);

	printf("Suppose we erase the first %d devices.  Here is the decoding matrix:\n\n", m);
	jerasure_print_matrix(decoding_matrix, k, k, w);
	printf("\n");
	printf("And dm_ids:\n\n");
	jerasure_print_matrix(dm_ids, 1, k, w);

	memset(data[0], 0, size);
	jerasure_matrix_dotprod(k, w, decoding_matrix, dm_ids, 0, data, coding, size);

	printf("\nAfter calling jerasure_matrix_dotprod, we calculate the value of device #0 to be:\n\n");
	printf("D0 :");
	for(i=0;i< size; i+=(w/8)) { //表示有多少行
		printf(" ");
		for(j=0;j < w/8;j++){ //根据wordsize的大小来搞每一行有多少个字节
			printf("%02x", (unsigned char)data[2][i+j]);
		}
	}
	printf("\n\n");

	//Just a simple pause for observation
	int for_pause = 0;
	cin>>for_pause;

	return 0;
}