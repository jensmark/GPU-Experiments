#ifndef _EXCEPTION_H__
#define _EXCEPTION_H__

#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>

class SimException : public std::runtime_error {
public:
	SimException(const char* file, unsigned int line, const char* msg) : std::runtime_error(msg) {
		std::cerr << file << ":" << line << ": " << msg << std::endl;
	}

	SimException(const char* file, unsigned int line, const std::string msg) : std::runtime_error(msg) {
		std::cerr << file << ":" << line << ": " << msg << std::endl;
	}
};


#define THROW_EXCEPTION(msg) throw SimException(__FILE__, __LINE__, msg)

#endif