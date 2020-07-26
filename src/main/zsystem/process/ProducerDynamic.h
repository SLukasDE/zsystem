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

#ifndef ZSYSTEM_PROCESS_PRODUCERDYNAMIC_H_
#define ZSYSTEM_PROCESS_PRODUCERDYNAMIC_H_

#include <zsystem/process/Producer.h>
#include <zsystem/process/FileDescriptor.h>

#include <string>
#include <functional>

namespace zsystem {
namespace process {

class ProducerDynamic : public Producer {
public:
	ProducerDynamic(std::function<std::size_t(char*, std::size_t)> getDataFunction);
	ProducerDynamic(std::string content);

	std::size_t write(FileDescriptor& fileDescriptor) override;

private:
	std::function<std::size_t(char*, std::size_t)> getDataFunction;
	std::string data;

	char buffer[4096];
	const char *bufferRead;
	std::size_t currentPos = 0;
	std::size_t currentSize = 0;
};

} /* namespace process */
} /* namespace zsystem */

#endif /* ZSYSTEM_PROCESS_PRODUCERDYNAMIC_H_ */
