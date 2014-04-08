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
#include <signal.h>
#include <direct.h>
#include "common.hpp"

using namespace std; 
class decoder
{
public:

	int readins;
	int n; // for control
	string encode_method;
	decoder();
	int decode_file(string obj_id);

};




#endif