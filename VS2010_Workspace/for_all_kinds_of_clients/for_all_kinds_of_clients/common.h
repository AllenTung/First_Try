#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <ctime>
#include <time.h>
#include <direct.h>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#if defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)
using namespace std;



using boost::asio::ip::tcp;
using boost::asio::windows::overlapped_ptr;
using boost::asio::windows::random_access_handle;

#define GET_REQUEST 1
#define POST_REQUEST 2
#define UPDATE_REQUEST 3
#define POST_REQUEST_BUF_SIZE 8192

#define CLIENT_POOL_SIZE 3
#define THREAD_POOL_SIZE 3
#define NO_SUCH_SUBSTRING 1000000

#define RECEIVE_BUFFER_SIZE 65536

#define REMOTE_ADDRESS "127.0.0.1:3333"

string get_random_file_name(int thread_id);
string get_request_random_file_name(int thread_id);
string int_to_string(unsigned int tmp_int);
void print_info(string client_id, string method, string uri, string detail);

unsigned int get_random_offset(unsigned int content_length);

string get_random_update_content();

string update_request_random_file_name(int thread_id);

string extract_pure_obj_name(string full_path);

string return_full_path(string obj_name);

string get_systime_string();

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