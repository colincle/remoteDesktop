#pragma once
#include <iostream>
#include "../include/Macros.hpp"

struct Debug
{
	template <typename T>
	Debug& operator<<(const T& value)
	{
		if (DEBUG)
			std::cout << value;
		return *this;
	}

	Debug& operator<<(std::ostream& (*manip)(std::ostream&))
	{
		if (DEBUG)
			std::cout << manip;
		return *this;
	}
};

inline Debug DEBUG_OUT;
