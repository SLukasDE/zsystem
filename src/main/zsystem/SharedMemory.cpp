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

#include <zsystem/SharedMemory.h>
#include <sys/mman.h>

#include <stdexcept>

namespace zsystem {
SharedMemory<void>::SharedMemory(std::size_t aSharedMemSize)
: sharedMemSize(aSharedMemSize)
{
    sharedMemPtr = mmap(nullptr, sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(sharedMemPtr == MAP_FAILED) {
    	throw std::runtime_error("Mmap failed.");
    	sharedMemPtr = nullptr;
    }
}

SharedMemory<void>::~SharedMemory() {
    if(sharedMemPtr) {
        munmap(sharedMemPtr, sharedMemSize);
        sharedMemPtr = nullptr;
    }
}

void* SharedMemory<void>::getData() {
	return sharedMemPtr;
}
} /* namespace zsystem */
