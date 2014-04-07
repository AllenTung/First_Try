#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <boost/random.hpp>
using namespace std; 

#define talloc(type, num) (type *) malloc(sizeof(type)*(num))

static long my_random_long()
{
	static boost::mt19937 gen(static_cast<unsigned int> (std::time(0)));
	boost::random::uniform_int_distribution<> engine(1, INT_MAX);
	boost::variate_generator<boost::mt19937&,boost::random::uniform_int_distribution<> > rng(gen, engine);

	return rng();
}