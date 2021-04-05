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

#include <zsystem/process/Feature.h>

#ifndef ZSYSTEM_PROCESS_FEATURETIME_H_
#define ZSYSTEM_PROCESS_FEATURETIME_H_

namespace zsystem {
namespace process {

class FeatureTime : public Feature {
public:
	struct TimeData {
		unsigned int realMs = 0;
		unsigned int userMs = 0;
		unsigned int sysMs = 0;
	};

	unsigned int getRealMS() const noexcept;
	unsigned int getUserMS() const noexcept;
	unsigned int getSysMS() const noexcept;

	void setTimeDataPtr(TimeData* timeData) noexcept;

private:
	TimeData* timeDataPtr = nullptr;
	TimeData timeData;
};

} /* namespace process */
} /* namespace zsystem */

#endif /* ZSYSTEM_PROCESS_FEATURETIME_H_ */
