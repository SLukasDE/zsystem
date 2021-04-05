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

#ifndef ZSYSTEM_PROCESS_H_
#define ZSYSTEM_PROCESS_H_

#include <zsystem/process/Arguments.h>
#include <zsystem/process/Environment.h>
#include <zsystem/process/FileDescriptor.h>
#include <zsystem/process/Producer.h>
#include <zsystem/process/Consumer.h>
#include <zsystem/process/Feature.h>
#include <zsystem/process/FeatureTime.h>

#include <unistd.h>

#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <utility>
#include <functional>
#include <memory>

namespace zsystem {

class Process {
public:
	using Handle = pid_t;
    static const Handle noHandle;

	struct ParameterStream {
		process::Producer* producer = nullptr;
		process::Consumer* consumer = nullptr;
	};
	using ParameterStreams = std::map<process::FileDescriptor::Handle, ParameterStream>;

	using ParameterFeatures = std::vector<std::reference_wrapper<process::Feature>>;

	Process(process::Arguments arguments);

	void setWorkingDir(std::string workingDir);
	void setEnvironment(std::unique_ptr<process::Environment> environment);
	const process::Environment* getEnvironment() const;

	int execute();
	int execute(process::FileDescriptor::Handle handle);
	int execute(process::Producer& producer, process::FileDescriptor::Handle handle);
	int execute(process::Consumer& consumer, process::FileDescriptor::Handle handle);
	int execute(process::Feature& feature);
	int execute(const ParameterStreams& parameterStreams, ParameterFeatures& parameterFeatures);

    template<typename... Args>
	int execute(process::FileDescriptor::Handle handle, Args&... args) {
    	ParameterStreams parameterStreams;
    	ParameterFeatures parameterFeatures;

    	addParameterStream(parameterStreams, handle, nullptr, nullptr);
    	return execute(parameterStreams, parameterFeatures, args...);
    }

    template<typename... Args>
	int execute(process::Producer& producer, process::FileDescriptor::Handle handle, Args&... args) {
    	ParameterStreams parameterStreams;
    	ParameterFeatures parameterFeatures;

    	addParameterStream(parameterStreams, handle, &producer, nullptr);
    	return execute(parameterStreams, parameterFeatures, args...);
    }

	template<typename... Args>
	int execute(process::Consumer& consumer, process::FileDescriptor::Handle handle, Args&... args) {
		ParameterStreams parameterStreams;
		ParameterFeatures parameterFeatures;

		addParameterStream(parameterStreams, handle, nullptr, &consumer);
    	return execute(parameterStreams, parameterFeatures, args...);
    }

	template<typename... Args>
	int execute(process::Feature& feature, Args&... args) {
		ParameterFeatures parameterFeatures;

		parameterFeatures.emplace_back(std::ref(feature));
    	return execute(ParameterStreams(), parameterFeatures, args...);
	}

	Handle getHandle() const;

private:
	template<typename... Args>
	int execute(ParameterStreams& parameterStreams, ParameterFeatures& parameterFeatures, process::FileDescriptor::Handle handle, Args&... args) {
		addParameterStream(parameterStreams, handle, nullptr, nullptr);
    	return execute(parameterStreams, parameterFeatures, args...);
    }

    template<typename... Args>
	int execute(ParameterStreams& parameterStreams, ParameterFeatures& parameterFeatures, process::Producer& producer, process::FileDescriptor::Handle handle, Args&... args) {
    	addParameterStream(parameterStreams, handle, &producer, nullptr);
    	return execute(parameterStreams, parameterFeatures, args...);
    }

	template<typename... Args>
	int execute(ParameterStreams& parameterStreams, ParameterFeatures& parameterFeatures, process::Consumer& consumer, process::FileDescriptor::Handle handle, Args&... args) {
		addParameterStream(parameterStreams, handle, nullptr, &consumer);
    	return execute(parameterStreams, parameterFeatures, args...);
	}

	template<typename... Args>
	int execute(ParameterStreams& parameterStreams, ParameterFeatures& parameterFeatures, process::Feature& feature, Args&... args) {
		parameterFeatures.emplace_back(std::ref(feature));
    	return execute(ParameterStreams(), parameterFeatures, args...);
	}

	using ChildFileDescriptors = std::map<process::FileDescriptor::Handle, process::FileDescriptor>;
	using ParentFileDescriptors = std::vector<std::tuple<process::FileDescriptor, process::Producer*, process::Consumer*>>;
	using PollResults = std::vector<std::tuple<std::reference_wrapper<process::FileDescriptor>, process::Producer*, process::Consumer*>>;


	Handle childRun(ChildFileDescriptors fileDescriptors, ParameterFeatures& parameterFeatures, process::FeatureTime::TimeData* timeData);
	static int parentRun(Handle pid, ParentFileDescriptors fileDescriptors, ParameterFeatures& parameterFeatures);
	static PollResults parentPoll(ParentFileDescriptors& fileDescriptors);
	static bool parentProcess(PollResults pollResults);

	static void addParameterStream(ParameterStreams& parameterStreams, process::FileDescriptor::Handle handle, process::Producer* producer, process::Consumer* consumer);

	process::Arguments arguments;
	std::unique_ptr<process::Environment> environment;
	std::string workingDir;

	Handle pid = noHandle;
};

} /* namespace zsystem */

#endif /* ZSYSTEM_PROCESS_H_ */
