#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string.h>
#include <direct.h>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
using namespace std;

void print(const boost::system::error_code& err_code, boost::asio::deadline_timer* t, int* count)
{
	if(*count < 5)
	{
		time_t tt = time(0);
		cout<<"Hello Allen!\n";
		cout<<ctime(&tt)<<endl;
		(*count)++;

		t->expires_at(t->expires_at() + boost::posix_time::seconds(2));
		t->async_wait(boost::bind(print, boost::asio::placeholders::error, t, count));
	}
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

int main()
{
	int forcin=0;
	string append_file_string = "I:/test.txt";

	const int transmite_file_size = 64;
	char temp_append1[transmite_file_size] = "";
	char temp_append2[transmite_file_size] = "";
	char temp_append3[transmite_file_size] = "";
	char temp_append4[transmite_file_size] = "";
	char temp_append5[transmite_file_size] = "";
	char temp_append6[transmite_file_size] = "";

	//0's acsii decimal number is : 48 ,and 109 is 'm'
	for (int i =0 ; i < transmite_file_size - 2; i++)
		temp_append1[i] = '1';

	temp_append1[62] = '\r';
	temp_append1[63] = '\n';

	for (int i =0 ; i < transmite_file_size - 2; i++)
		temp_append2[i] = '2';

	temp_append2[62] = '\r';
	temp_append2[63] = '\n';

	for (int i =0 ; i < transmite_file_size - 2; i++)
		temp_append3[i] = '3';

	temp_append3[62] = '\r';
	temp_append3[63] = '\n';

	for (int i =0 ; i < transmite_file_size - 2; i++)
		temp_append4[i] = '4';

	temp_append4[62] = '\r';
	temp_append4[63] = '\n';

	for (int i =0 ; i < transmite_file_size - 2; i++)
		temp_append5[i] = '5';

	temp_append5[62] = '\r';
	temp_append5[63] = '\n';

	for (int i =0 ; i < transmite_file_size - 2; i++)
		temp_append6[i] = '6';

	temp_append6[62] = '\r';
	temp_append6[63] = '\n';

	ofstream append_file(append_file_string.c_str(), ios::app | ios::binary);
	for (int i = 0; i < 1; i++)
	{
		//192M
		int k = 0 ;
		int size_control = 32*1024/64;
		//Make the size of the file be 32 kb
		for (k = 0; k < size_control; k++)
		{
			append_file.write(temp_append1, 64);
		}
		for (k = 0; k < size_control; k++)
		{
			append_file.write(temp_append2, 64);
		}
		for (k = 0; k < size_control; k++)
		{
			append_file.write(temp_append3, 64);
		}
		for (k = 0; k < size_control; k++)
		{
			append_file.write(temp_append4, 64);
		}
		for (k = 0; k < size_control; k++)
		{
			append_file.write(temp_append5, 64);
		}
		for (k = 0; k < size_control; k++)
		{
			append_file.write(temp_append6, 64);
		}
	}
	append_file.flush();
	append_file.close();
	cin >> forcin;



	string random_access_file = "I:/test_random.txt";

	ofstream random_write_file(random_access_file.c_str(), ios::in | ios::out | ios::binary);

	long cur_point = random_write_file.tellp();

	random_write_file.seekp(cur_point + 3);
	random_write_file.write("prick", 5);

	random_write_file.close();

	cin >> forcin;

	unsigned long tmp_long = 333333;
	char tmp_char[30] = "";
	_itoa_s(tmp_long, tmp_char, 10);
	string tmp_string1 = tmp_char;
	cout << tmp_string1 << endl;
	cout << tmp_string1.size() << endl;

	cin >> forcin;



	string test_split = "www\r\nhost: 12.223.233\r\ncontent-type: text\r\nConnection: close\r\n\r\nAllen is now on his way and make the full advantage of his perseverance!\r\n";
	string dev = "Connection: close\r\n\r\n";
	time_t temp_time = time(0); 
	cout << ctime(&temp_time) <<endl;
	cin >> forcin;
	cout << test_split.find("\r\n\r\n") << endl;



	vector<string> result_string = split(test_split, dev.c_str());
	cout << "size of result:" << result_string.size() <<endl;

	cin >> forcin;
	for(int ll = 0; ll < result_string.size(); ll++ )
	{
		cout << result_string.at(ll) << endl;
	}

	cin >> forcin;


	const size_t buf_size = 3;
	boost::array<char, buf_size> buf_;
	buf_.assign('a');
	cout << buf_[1] << endl;
	string tmp_test = "allen go!\r\n";
	cout << tmp_test.length() << endl << tmp_test.find_first_of("\r\n") << endl << tmp_test.find_first_of("a") << endl;
	cout << tmp_test.substr(1, 3) << endl;
	cin >> forcin;

	string input_file_path = "I:/test2.txt";
	string output_file_path = "I:/test3.txt";
	char input_buf[3] = "";
	char content_buf[600] = "sssssssssssssssssss\r\nsssssssssssssssssssssaaaaaa\r\naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	ifstream file_input(input_file_path.c_str(), ios::in | ios::binary);
	ofstream file_output(output_file_path.c_str(), ios::out | ios::binary);

	while(file_input.read(input_buf, sizeof(input_buf)).gcount() > 0)
	{
		cout << "Another time for the input_buf" <<endl;
		cout << input_buf[0] << input_buf[1] << input_buf[2] << endl;
		file_output.write(input_buf, sizeof(input_buf));
	}
	file_input.close();
	file_output.close();

	cin >> forcin;




	cin >> forcin;

	int count = 0;
	boost::asio::io_service io_service;
	//boost::asio::deadline_timer temp_timer(io_service, boost::posix_time::seconds(30));

	boost::asio::deadline_timer temp_timer(io_service, boost::posix_time::seconds(15));
	temp_timer.async_wait(boost::bind(print, boost::asio::placeholders::error, &temp_timer, &count));

	for(int k = 0; k < 10; k++)
	{	
		temp_time = time(0);
		cout<<ctime(&temp_time)<<endl;
		if(k != 9)
			Sleep(2000);
	}
	io_service.run();
	cout<<ctime(&temp_time)<<endl;

	cin>>forcin;


	//io_service.run();
	temp_time = time(0);
	cout<<ctime(&temp_time)<<endl;


	cin>>forcin;

	char* for_file = (char*)malloc(sizeof(char)*500);
	getcwd(for_file, 80);

	printf("%s\n", for_file);

	string tmp_string = for_file;

	tmp_string = "Change It!";
	cout<<tmp_string<<endl;

	Encoder tmp_encoder;
	tmp_encoder.file_id = 18;
	cout<<tmp_encoder.get_id()<<endl;
	tmp_encoder.print_info("haha");

	cout<<tmp_encoder.encode("www", 1, 1,1)<<endl;



	cin>>forcin;

	FILE* tmpfile = fopen("I://Microsoft Visual Studio 2010//Workspace//j_lib_test//j_lib_test//Coding//test_meta.txt","rb");
	for(int line = 0;line < 6;line++)
	{
		int count = 0;
		fgets(for_file,80,tmpfile);

		/*	fread(for_file,1,20,tmpfile);*/
		while(for_file[count] != '\n')
		{
			printf("%c", for_file[count]);
			count++;
		}
		printf("\n");
	}

	// 	count++;
	// 	
	// 	while(for_file[count] != '\n')
	// 	{
	// 		cout<<for_file[count];
	// 		count++;
	// 	}

	/*	fseek(tmpfile,count+1,SEEK_SET);*/
	// 	count = 0;
	// 	fread(for_file,1,20,tmpfile);
	// 	count = 0;
	// 	while(for_file[count] != '\n')
	// 	{
	// 		cout<<for_file[count];
	// 		count++;
	// 	}

	cin>>forcin;



	char* tmp_dir = getcwd((char*)malloc(sizeof(char)*1000),1000);
	printf("%s\n", tmp_dir);

	cin>>forcin;

	FILE* tempfile = fopen("I://test.txt","rb");
	fseek(tempfile, 0, SEEK_END);
	long size = ftell(tempfile);
	fseek(tempfile, 0,SEEK_SET);

	cout<<ftell(tempfile)<<endl;
	char content[500];

	while (fgets(content,100,tempfile))
	{
		cout<<content<<endl;
	}

	forcin =  mkdir("I://testdirectory2");
	int for_pause = 0;
	cin>>for_pause;

	LARGE_INTEGER t1, t2, tc;
	QueryPerformanceFrequency(&tc);
	QueryPerformanceCounter(&t1);
	for(int i = 0;i < 10000;i++)
	{
		cout<<i<<endl;
	}
	QueryPerformanceCounter(&t2);
	cout<<(t2.QuadPart - t1.QuadPart) * 1.0 / tc.QuadPart<<endl;

	int just_for_pause = 0;
	cin>>just_for_pause;
	return 0;
}