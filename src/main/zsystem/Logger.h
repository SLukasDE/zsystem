/*
MIT License
Copyright (c) 2019-2021 Sven Lukas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef ZSYSTEM_LOGGER_H_
#define ZSYSTEM_LOGGER_H_

#include <ostream>

//#define ZSYSTEM_LOGGING_LEVEL_DEBUG

#ifdef ZSYSTEM_LOGGING_LEVEL_DEBUG
#include <iostream>
#endif

namespace zsystem {

class Logger {
public:
	template<typename T>
	inline Logger& operator<<(const T& t) {
#ifdef ZSYSTEM_LOGGING_LEVEL_DEBUG
		std::cout << t;
#endif
		return *this;
	}

    inline Logger& operator<<(std::ostream& (*pf)(std::ostream&)) {
#ifdef ZSYSTEM_LOGGING_LEVEL_DEBUG
		std::cout << pf;
#endif
    	return *this;
    }

    inline constexpr explicit operator bool() const noexcept {
#ifdef ZSYSTEM_LOGGING_LEVEL_DEBUG
    	return true;
#else
    	return true;
#endif
    }
};

} /* namespace zsystem */

#endif /* ZSYSTEM_LOGGER_H_ */
