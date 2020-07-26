/*
MIT License
Copyright (c) 2019, 2020 Sven Lukas

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

#ifndef ZSYSTEM_PROCESS_ARGUMENTS_H_
#define ZSYSTEM_PROCESS_ARGUMENTS_H_

#include <string>

namespace zsystem {
namespace process {

class Arguments {
public:
	Arguments() = default;
	Arguments(const Arguments& other);
	Arguments(Arguments&& other);
	Arguments(std::string args);
	Arguments(std::size_t argc, const char** argv);
	~Arguments();

	Arguments& operator=(const Arguments& other);
	Arguments& operator=(Arguments&& other);

	const std::string& getArgs() const noexcept;
	std::size_t getArgc() const noexcept;
	char** getArgv() const noexcept;

private:
	static const char* argumentSize(const char* src, std::size_t& length);
	static const char* argumentCopy(const char* src, char* dst);

	std::string args;
	std::size_t argc = 0;
	char** argv = nullptr;
};

} /* namespace process */
} /* namespace zsystem */

#endif /* ZSYSTEM_PROCESS_ARGUMENTS_H_ */
