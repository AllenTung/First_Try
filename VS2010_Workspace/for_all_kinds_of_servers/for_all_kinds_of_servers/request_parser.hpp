#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <boost/asio.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/array.hpp>
#include "common.hpp"

using namespace std;

class request;

//Parse for incoming requests
class request_parser
{
public:
	request_parser();

	boost::tribool simple_parse(request& req, boost::array<char, BUFFER_SIZE>& buf);
};

#endif