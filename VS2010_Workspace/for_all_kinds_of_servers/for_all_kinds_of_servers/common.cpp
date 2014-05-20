#define _CRT_SECURE_NO_DEPRECATE
#include <stdlib.h>
#include "common.hpp"

using namespace std;
using std::ofstream;
using std::ifstream;
static int caculate_ec_port(int start_num)
{
	if (ec_port_helper >= 36)
	{
		ec_port_helper = 1;
	}

	int changing_port = start_num + (start_num - STARTING_SERVER_ID + 1) * 300 + ec_port_helper;
	ec_port_helper++;
	return changing_port;
}
int get_random_ec_port(int starting_num)
{
	//Starting number is actually identical to each server's own id
	return caculate_ec_port(starting_num);

}


vector<string> split(string& str,const char* c)
{
	char *cstr, *p;
	vector<string> res;
	cstr = new char[str.size()+1];
	strcpy(cstr,str.c_str());
	p = strtok(cstr,c);
	while(p!=NULL)
	{
		res.push_back(p);
		p = strtok(NULL,c);
	}
	return res;
}

string int_to_string(unsigned int tmp_int)
{
	char tmp_char[30] = "";
	_ultoa_s(tmp_int, tmp_char, 10);
	/*	_itoa_s(tmp_int, tmp_char, 10);*/
	return tmp_char;
}

void print_info(string client_id, string method, string obj_id, string detail)
{
	if (client_id.empty())
		client_id = "null";
	if (method.empty())
		method = "null";
	if (obj_id.empty())
		obj_id = "null";
	if (detail.empty())
		detail = "null";
	cout << "\nClient: " << client_id << "\nMethod: " << method << "\nTarget: " << obj_id << "\nDetail: " << detail << endl;
}
static long get_my_random_long()
{
	static boost::mt19937 gen(static_cast<unsigned int> (std::time(0)));
	boost::random::uniform_int_distribution<> engine(1, INT_MAX);
	boost::variate_generator<boost::mt19937&,boost::random::uniform_int_distribution<> > rng(gen, engine);

	return rng();
}
long my_random_long()
{
	return get_my_random_long();
}


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
			if (w == prime55[i])
			{
				return 1;
			}
			else 
			{ 
				return 0; 
			}
		}
	}

	return 0;
}

int newer_timestamp_comparison(string t1, string t2)
{
	//If t1 is strictly newer than t2, return 1
	//If t2 is strictly older than t2, return -1
	//If t1 = t2, return 0
	//timestamp format : month_day_hour_min_second_millisecond
	char* split_char = "_";
	vector<string> temp_t1 = split(t1, split_char);
	vector<string> temp_t2 = split(t2, split_char);

	for (int i = 0; i < temp_t1.size(); i++) 
	{
		if (atoi((temp_t1[i]).c_str()) > atoi((temp_t2[i]).c_str()))
		{
			return 1;
		}
		else if (atoi((temp_t1[i]).c_str()) < atoi((temp_t2[i]).c_str()))
		{
			return -1;
		}
	}
	
	//All the same ,return 0
	return 0;
}

int update_file(string file_path, string content, unsigned int offset, unsigned int total_length)
{
	ofstream random_writer(file_path.c_str(), ios::in | ios::out | ios::binary);

	if (!random_writer.is_open())
	{
		cout << "Error occurs when open the file: " << file_path << endl;
		return 0;
	}
	long cur_position = random_writer.tellp();
	random_writer.seekp(cur_position + offset);
	random_writer.write(content.c_str(), content.length());

	random_writer.close();

	return 1;
}

string extract_pure_obj_name(string full_path)
{
	if(full_path.find_last_of("\\") < NO_SUCH_SUBSTRING)
	{
		string obj_name = full_path.substr(full_path.find_last_of("\\") + 1);
		return obj_name;
	}
	else
	{
		return full_path;
	}	
}

string return_full_path(string obj_name)
{
	char* curdir;
	curdir = (char*)malloc(sizeof(char)*1000);	
	_getcwd(curdir, 1000);
	string obj_prefix(curdir);
	string full_obj_name = obj_prefix + "\\" + obj_name;

	return full_obj_name;
}

string return_update_path(string obj_name)
{
	char* curdir;
	curdir = (char*)malloc(sizeof(char)*1000);	
	_getcwd(curdir, 1000);
	string obj_prefix(curdir);
	string full_obj_name = obj_prefix + "\\update\\" + obj_name;

	return full_obj_name;
}

bool test_existence(string file_path)
{
	ofstream if_exists(file_path.c_str(), ios::out | ios::_Nocreate);
	if (if_exists)
	{
		if_exists.close();
		return true;
	}
	else
	{
		if_exists.close();
		return false;
	}
}

int* test_pointer(int recorder[])
{
	recorder[0] = 8;
	recorder[1] = 88;
	return recorder;
}

unsigned int size_of_file(string file_path)
{
	if(test_existence(file_path))
	{
// 		string locking_file = get_locking_file_path(file_path);
// 		boost::interprocess::file_lock temp_lock(locking_file.c_str());
// 		temp_lock.lock();
		FILE* fp = fopen(file_path.c_str(), "rb");

		fseek(fp, 0, SEEK_END);
		unsigned int size = (unsigned int)ftell(fp);
		fseek(fp, 0,SEEK_SET);

		fclose(fp);
/*		temp_lock.unlock();*/

		return size;
	}
	else
	{
		cout << "File doesn't exist : " << file_path << endl;
		return 0;
	}

}

string get_locking_file_path(string full_local_path)
{
	// Using the full local path to get the locking file path by replacing the suffix
	string locking_suffix = ".lock";
	string locking_file_path = full_local_path.substr(0, full_local_path.find_last_of(".")) + locking_suffix;

	return locking_file_path;
}

void creat_locking_file(string full_local_path)
{
	string tmp_path = get_locking_file_path(full_local_path);

	ofstream locking_file(tmp_path.c_str(), ios::out | ios::binary);
	string tmp_content = "lock for " + full_local_path;
	locking_file.write(tmp_content.c_str(), tmp_content.length());

	locking_file.close();
	return;
}