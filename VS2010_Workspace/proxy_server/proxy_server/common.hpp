#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <time.h>
#include <boost/random.hpp>
#include <Windows.h>

#if defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)

using boost::asio::ip::tcp;
using boost::asio::windows::overlapped_ptr;
using boost::asio::windows::random_access_handle;
using namespace std;

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

#define talloc(type, num) (type *) malloc(sizeof(type)*(num))

#define BUFFER_SIZE 8192
#define THREAD_NUMBER 5

#define RECEIVE_BUFFER_SIZE 8192
#define RECEIVE_BUFFER_SIZE_TINY 256

//Request Type between client and server
#define GET_REQUEST 1 
#define POST_REQUEST 2
#define UPDATE_REQUEST 3

//Request Type among servers, and distinguish the data parity blocks
#define TRANSMIT_DATA_BLOCK 4
#define TRANSMIT_PARITY_BLOCK 5

#define IO_POOL_FETCH_MODE_PLUS 1
#define IO_POOL_FETCH_MODE_MINUS -1
#define IO_POOL_FETCH_MODE_STAY 0

#define METHOND_COUNT 10

#define NO_SUCH_SUBSTRING 1000000

//Parameters for endoer and decoder
#define ERASURE_CODE_K 4
#define ERASURE_CODE_M 2
#define ERASURE_CODE_W 8
#define ERASURE_CODE_PACKETSIZE 1024
#define ERASURE_CODE_BUFFERSIZE 1024

//Max number of servers
#define NUMBER_OF_SERVER 256
#define STARTING_SERVER_ID 20888

//For Hashing
#define HASHING_SEED 131
#define HASHING_FOR_RANGE 249997

extern vector<string> split(string& str,const char* c);

void print_info(string client_id, string method, string uri, string detail);

int is_prime(int w);

long my_random_long();

int newer_timestamp_comparison(string t1, string t2);

string int_to_string(unsigned int tmp_int);

unsigned int get_hash_value(char*str);   

template <typename Handler>
void transmit_file(tcp::socket& socket, random_access_handle& file, Handler handler)
{
	//construct an overlapped-derived object to contain the handler
	overlapped_ptr overlapped(socket.get_io_service(), handler);
	BOOL ok = ::TransmitFile(socket.native_handle(), file.native_handle(), 0, 0, overlapped.get(), 0, 0);
	DWORD last_error = ::GetLastError();

	//Check if the operation completed immediately
	if (!ok && last_error != ERROR_IO_PENDING)
	{
		// The operation completed immediately, so a completion notification needs
		// to be posted. When complete() is called, ownership of the OVERLAPPED-
		// derived object passes to the io_service.
		boost::system::error_code ec(last_error, boost::asio::error::get_system_category());
		overlapped.complete(ec, 0);
	}
	else
	{
		// The operation was successfully initiated, so ownership of the
		// OVERLAPPED-derived object has passed to the io_service.
		overlapped.release();
	}
}



#else // defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)
# error Overlapped I/O not available on this platform
#endif // defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)