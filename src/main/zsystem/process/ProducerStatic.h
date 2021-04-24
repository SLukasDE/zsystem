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

#ifndef ZSYSTEM_PROCESS_PRODUCERSTATIC_H_
#define ZSYSTEM_PROCESS_PRODUCERSTATIC_H_

#include <zsystem/process/Producer.h>
#include <zsystem/process/FileDescriptor.h>

#include <string>

namespace zsystem {
namespace process {

class ProducerStatic : public Producer {
public:
	ProducerStatic(const char* data, std::size_t size);

	std::size_t produce(process::FileDescriptor& fileDescriptor) override;

	const char* getData() const noexcept;
	std::size_t getSize() const noexcept;

private:
	const char* data;
	std::size_t size;
	std::size_t currentPos = 0;
};

} /* namespace process */
} /* namespace zsystem */

#endif /* ZSYSTEM_PROCESS_PRODUCERSTATIC_H_ */
