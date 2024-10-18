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

#include <zsystem/Process.h>
#include <zsystem/process/ConsumerFile.h>
#include <zsystem/process/ProducerFile.h>
#include <zsystem/process/FeatureProcess.h>
#include <zsystem/SharedMemory.h>
#include <zsystem/Logger.h>

#include <unistd.h>
#include <dirent.h>
#include <poll.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>

#include <stdexcept>
#include <cstring>
#include <map>
#include <set>

#include <thread>
#include <chrono>

namespace zsystem {

namespace {
Logger logger;
}

const Process::Handle Process::noHandle = -1;

Process::Process(process::Arguments aArguments)
: arguments(std::move(aArguments))
{ }

void Process::setWorkingDir(std::string aWorkingDir) {
	workingDir = std::move(aWorkingDir);
}

void Process::setEnvironment(std::unique_ptr<process::Environment> aEnvironment) {
	environment = std::move(aEnvironment);
}

const process::Environment* Process::getEnvironment() const {
	return environment.get();
}

int Process::execute() {
	ParameterFeatures parameterFeatures;

	return execute(ParameterStreams(), parameterFeatures);
}

int Process::execute(process::FileDescriptor::Handle handle) {
	ParameterStreams parameterStream;
	ParameterFeatures parameterFeatures;

	addParameterStream(parameterStream, handle, nullptr, nullptr);
	return execute(parameterStream, parameterFeatures);
}

int Process::execute(process::Producer& producer, process::FileDescriptor::Handle handle) {
	ParameterStreams parameterStream;
	ParameterFeatures parameterFeatures;

	addParameterStream(parameterStream, handle, &producer, nullptr);
	return execute(parameterStream, parameterFeatures);
}

int Process::execute(process::Consumer& consumer, process::FileDescriptor::Handle handle) {
	ParameterStreams parameterStream;
	ParameterFeatures parameterFeatures;

	addParameterStream(parameterStream, handle, nullptr, &consumer);
	return execute(parameterStream, parameterFeatures);
}

int Process::execute(process::Feature& feature) {
	ParameterFeatures parameterFeatures;

	parameterFeatures.emplace_back(std::ref(feature));
	return execute(ParameterStreams(), parameterFeatures);
}

int Process::execute(const ParameterStreams& parameterStreams, ParameterFeatures& parameterFeatures) {
	ChildFileDescriptors childFileDescriptors;
	ParentFileDescriptors parentFileDescriptors;

	for(auto& parameterStream : parameterStreams) {
		switch(parameterStream.first) {
		case 0:
			logger << "parameterStream 0 (stdin).\n";
			break;
		case 1:
			logger << "parameterStream 1 (stdout).\n";
			break;
		case 2:
			logger << "parameterStream 2 (stderr).\n";
			break;
		default:
			logger << "parameterStream " << parameterStream.first << ".\n";
			break;
		}

		if(parameterStream.first == process::FileDescriptor::noHandle) {
			logger << "- no handle -> close !\n";
			continue;
		}

		if(parameterStream.second.producer && parameterStream.second.consumer) {
			std::pair<process::FileDescriptor, process::FileDescriptor> tmp = process::FileDescriptor::openBidirectional();

			logger << "- producer & consumer: child-fd=" << tmp.second.getHandle() << " , parent-fd=" << tmp.first.getHandle() << "\n";

			childFileDescriptors[parameterStream.first] = std::move(tmp.second);
			parentFileDescriptors.emplace_back(std::move(tmp.first), parameterStream.second.producer, parameterStream.second.consumer);
		}
		else if(parameterStream.second.producer && parameterStream.second.consumer == nullptr) {
			process::ProducerFile* producerFile = dynamic_cast<process::ProducerFile*>(parameterStream.second.producer);
			if(producerFile && producerFile->getFileDescriptor()) {
				logger << "- producer FILE: child-fd=" << producerFile->getFileDescriptor().getHandle() << "\n";
				childFileDescriptors[parameterStream.first] = std::move(producerFile->getFileDescriptor());
			}
			else {
				std::pair<process::FileDescriptor, process::FileDescriptor> tmp = process::FileDescriptor::openUnidirectional();

				logger << "- producer: child-fd=" << tmp.first.getHandle() << " , parent-fd=" << tmp.second.getHandle() << "\n";

				childFileDescriptors[parameterStream.first] = std::move(tmp.first);
				parentFileDescriptors.emplace_back(std::move(tmp.second), parameterStream.second.producer, parameterStream.second.consumer);
			}
		}
		else if(parameterStream.second.producer == nullptr && parameterStream.second.consumer) {
			process::ConsumerFile* consumerFile = dynamic_cast<process::ConsumerFile*>(parameterStream.second.consumer);
			if(consumerFile && consumerFile->getFileDescriptor()) {
				logger << "- consumer FILE: child-fd=" << consumerFile->getFileDescriptor().getHandle() << "\n";
				childFileDescriptors[parameterStream.first] = std::move(consumerFile->getFileDescriptor());
			}
			else {
				std::pair<process::FileDescriptor, process::FileDescriptor> tmp = process::FileDescriptor::openUnidirectional();

				logger << "- consumer: child-fd=" << tmp.second.getHandle() << " , parent-fd=" << tmp.first.getHandle() << "\n";

				childFileDescriptors[parameterStream.first] = std::move(tmp.second);
				parentFileDescriptors.emplace_back(std::move(tmp.first), parameterStream.second.producer, parameterStream.second.consumer);
			}
		}
		else {
			logger << "- don't close\n";
			childFileDescriptors[parameterStream.first];
		}
	}

	std::unique_ptr<SharedMemory<process::FeatureTime::TimeData>> timeData;
	for(auto& parameterFeature : parameterFeatures) {
		process::FeatureTime* featureTime = dynamic_cast<process::FeatureTime*>(&parameterFeature.get());
		if(featureTime) {
			if(!timeData) {
				timeData.reset(new SharedMemory<process::FeatureTime::TimeData>);
			}
			featureTime->setTimeDataPtr(timeData->getData());
			continue;
		}
	}

	pid = childRun(std::move(childFileDescriptors), parameterFeatures, (timeData ? timeData.get()->getData() : nullptr));
	logger << "PID = " << pid << "\n";

	int rc = parentRun(pid, std::move(parentFileDescriptors), parameterFeatures);
	logger << "rc = " << rc << "\n";

	pid = noHandle;

	return rc;
}

Process::Handle Process::getHandle() const {
	return pid;
}

Process::Handle Process::childRun(ChildFileDescriptors fileDescriptors, ParameterFeatures& parameterFeatures, process::FeatureTime::TimeData* timeData) {
	char* const* argv = arguments.getArgv();

	char* const* envp = nullptr;
	if(environment) {
		envp = environment->getEnvp();
	}

	const char* chdirStr = workingDir.empty() ? nullptr : workingDir.c_str();

	pid_t pid = fork();
	if(pid == 0) {
		/* child */
		pid = getpid();

		/* Terminate child, if parent killed, use once only !!!! */
		prctl(PR_SET_PDEATHSIG, SIGTERM);

		/* ******************* *
		 * set FileDescriptos  *
		 * ******************* */

		for(const auto& fileDescriptor : fileDescriptors) {
			if(fileDescriptor.first == process::FileDescriptor::noHandle) {
				continue;
			}
			if(fileDescriptor.second) {
				while(close(fileDescriptor.first) == EINTR) { }
				while(dup2(fileDescriptor.second.getHandle(), fileDescriptor.first) == EINTR) { }
			}
		}


		/* ********************************* *
		 * close all opened file descriptors *
		 * ********************************* */
		char procDirFd[16];
		std::sprintf(procDirFd, "/proc/%d/fd/", pid);

		DIR *dir;
		struct dirent *ent;

		if((dir = opendir(procDirFd)) == nullptr) {
			write(STDERR_FILENO, "Cannot open directory: \"", std::strlen("Cannot open directory: \""));
			write(STDERR_FILENO, procDirFd, std::strlen(procDirFd));
			write(STDERR_FILENO, "\"\n", std::strlen("\"\n"));
			_exit(EXIT_FAILURE);
		}

		// get files and directories within directory
		while((ent = readdir(dir)) != nullptr) {
			if(ent->d_type == DT_DIR) {
				continue;
			}

			// convert file name to int
			char* end;
			int fd = strtol(ent->d_name, &end, 32);
			if(!*end && fileDescriptors.find(fd) == std::end(fileDescriptors)) {
				// close valid file descriptor
				while(close(fd) == EINTR) { }
			}
		}
		closedir(dir);

		if(chdirStr && chdir(chdirStr) == -1) {
			write(STDERR_FILENO, "Unable to change to directory \"", std::strlen("Unable to change to directory \""));
			write(STDERR_FILENO, chdirStr, std::strlen(chdirStr));
			write(STDERR_FILENO, "\"\n", std::strlen("\"\n"));
			_exit(EXIT_FAILURE);
		}

		pid_t timerPid = 0;
		suseconds_t realTimeStartUsec;
		struct tms startTms;

		if(timeData) {
			timeval tv;
			gettimeofday(&tv, nullptr);
			realTimeStartUsec = tv.tv_sec * 1000000 + tv.tv_usec;
			times(&startTms);

			timerPid = fork();
			if (timerPid < 0) { /* error */
				write(STDERR_FILENO, "Inner fork failed.\n", std::strlen("Inner fork failed.\n"));
				_exit(-2);
			}
		}

		if(timerPid == 0) {
			/* Check if there has been another fork */
			if(timeData) {
				/* Terminate child, if parent killed, use once only !!!! */
				prctl(PR_SET_PDEATHSIG, SIGTERM);
			}

			if(envp) {
				execvpe(argv[0], argv, envp);
			}
			else {
				execvp(argv[0], argv);
			}

			write(STDERR_FILENO, "Unable to execute \"", std::strlen("Unable to execute \""));
			write(STDERR_FILENO, argv[0], std::strlen(argv[0]));
			write(STDERR_FILENO, "\"\n", std::strlen("\"\n"));
			_exit(EXIT_FAILURE);
		}

		/* we only run to this part if there is an time measurement enabled */
		long double clktck = static_cast<long double>(sysconf(_SC_CLK_TCK)) / 1000;
		int rc;
		while(true) {
			pid_t rcWaitPid = waitpid(timerPid, &rc, 0);

			timeval tv;
            gettimeofday(&tv, nullptr);

            double realMs = (tv.tv_sec * 1000000 + tv.tv_usec) - realTimeStartUsec;
            realMs /= 1000;
            timeData->realMs = realMs;

    		struct tms endTms;
            times(&endTms);

            double cuser = endTms.tms_cutime - startTms.tms_cutime;
            cuser /= clktck;
            timeData->userMs = cuser;

            double csystem = endTms.tms_cstime - startTms.tms_cstime;
            csystem /= clktck;
            timeData->sysMs = csystem;

			if(rcWaitPid == -1) {
				if (errno == EINTR) {
					continue;
				}
				/* on error */
				rc = EXIT_FAILURE;
				break;
			}

			if(WIFEXITED(rc)) {
				rc = WEXITSTATUS(rc);
				break;
			}

			if(WIFSIGNALED(rc)) {
				// follow the same convention as bash of returning signal values in return codes by adding 128 to them
				rc = 128 + WTERMSIG(rc);
				break;
			}
		}
		_exit(rc);
	}

	/* fork failed */
	if(pid < 0) {
		throw std::runtime_error(std::string("fork() failed: ") + std::strerror(errno));
	}

	return pid;
}


int Process::parentRun(Handle pid, ParentFileDescriptors fileDescriptors, ParameterFeatures& parameterFeatures) {
	logger << "parentRun:\n";
	logger << "----------\n\n";
	int rc = EXIT_FAILURE;

	for(auto& parameterFeature : parameterFeatures) {
		process::FeatureProcess* featureProcess = dynamic_cast<process::FeatureProcess*>(&parameterFeature.get());
		if(featureProcess) {
			featureProcess->setProcessHandle(pid);
			continue;
		}
	}

	while(true) {
		bool processed = parentProcess(parentPoll(fileDescriptors));

		if(processed) {
			continue;
		}

		// in case we are reading from the child, we have to return from waitpid
		// otherwise it might lead to a deadlock in case the child does not terminate
		// because its output is not being consumed.
		pid_t rcWaitPid = waitpid(pid, &rc, 0);

		if(rcWaitPid == -1) {
			if (errno == EINTR) {
				continue;
			}
			/* on error */
			rc = EXIT_FAILURE;
			break;
		}

		if(WIFEXITED(rc)) {
			rc = WEXITSTATUS(rc);
			break;
		}

		if(WIFSIGNALED(rc)) {
			// follow the same convention as bash of returning signal values in return codes by adding 128 to them
			rc = 128 + WTERMSIG(rc);
			break;
		}
	}

	for(auto& parameterFeature : parameterFeatures) {
		process::FeatureProcess* featureProcess = dynamic_cast<process::FeatureProcess*>(&parameterFeature.get());
		if(featureProcess) {
			featureProcess->setProcessHandle(noHandle);
			continue;
		}

		process::FeatureTime* featureTime = dynamic_cast<process::FeatureTime*>(&parameterFeature.get());
		if(featureTime) {
			featureTime->setTimeDataPtr(nullptr);
			continue;
		}
	}

	return rc;
}

Process::PollResults Process::parentPoll(ParentFileDescriptors& fileDescriptors) {
	PollResults pollResults;

	PollResults polledFileHandles;
	std::vector<struct pollfd> pollFileHandles;

	logger << "parentPoll:\n";
	logger << "-----------\n\n";

	for(auto& fileDescriptor : fileDescriptors) {
		if(!std::get<0>(fileDescriptor)) {
			logger << "- fd = 'no-handle' (closed handle)\n";
			continue;
		}

		struct pollfd tmpPollFd;
		tmpPollFd.fd = std::get<0>(fileDescriptor).getHandle();
		tmpPollFd.events = 0;
		if(std::get<1>(fileDescriptor)) {
			logger << "- producer available for fd = " << std::get<0>(fileDescriptor).getHandle() << " -> check for POLLOUT\n";
			tmpPollFd.events |= POLLOUT;
		}
		if(std::get<2>(fileDescriptor)) {
			logger << "- consumer available for fd = " << std::get<0>(fileDescriptor).getHandle() << " -> check for POLLIN\n";
			tmpPollFd.events |= POLLIN;
		}
		if(tmpPollFd.events != 0) {
			pollFileHandles.push_back(tmpPollFd);
			polledFileHandles.push_back(std::make_tuple(std::ref(std::get<0>(fileDescriptor)), std::get<1>(fileDescriptor), std::get<2>(fileDescriptor)));
		}
		else {
			logger << "- no producer & no consumer for fd = " << std::get<0>(fileDescriptor).getHandle() << " -> no check\n";
		}
	}

	if(pollFileHandles.empty() == false) {
		logger << "Poll (blocking call) ...\n";
		while(poll(&pollFileHandles[0], pollFileHandles.size(), -1) == -1 && errno == EINTR) { }
		logger << "Poll returned\n";

		for(std::size_t i = 0; i < pollFileHandles.size(); ++i) {
			process::Producer* producer = std::get<1>(polledFileHandles[i]);
			process::Consumer* consumer = std::get<2>(polledFileHandles[i]);

			if((pollFileHandles[i].revents & POLLOUT) == 0) {
				producer = nullptr;
			}
			else {
				logger << "  - writing possible to fd=" << std::get<0>(polledFileHandles[i]).get().getHandle() << "\n";
			}

			if((pollFileHandles[i].revents & POLLIN) == 0) {
				consumer = nullptr;
			}
			else {
				logger << "  - reading possible from fd=" << std::get<0>(polledFileHandles[i]).get().getHandle() << "\n";
			}

			if(producer || consumer) {
				pollResults.push_back(std::make_tuple(std::get<0>(polledFileHandles[i]), producer, consumer));
			}
		}
	}

	return pollResults;
}

bool Process::parentProcess(PollResults pollResults) {
	logger << "parentProcess:\n";
	logger << "--------------\n\n";

	bool processed = false;
	for(auto& pollResult: pollResults) {
		process::FileDescriptor& fileDescriptor = std::get<0>(pollResult).get();
		if(!fileDescriptor) {
			continue;
		}

		process::Producer* producer = std::get<1>(pollResult);
		process::Consumer* consumer = std::get<2>(pollResult);

		bool allNpos = true;

		logger << "Process fd " << fileDescriptor.getHandle() << ".\n";

		/* check if it is possible to send something to the process */
		if(producer) {
			/* send content to the process */
			logger << "- produce...\n";
			std::size_t count = producer->produce(fileDescriptor);

			if(count != process::FileDescriptor::npos) {
				logger << "  - " << count << " bytes produced\n";
				allNpos = false;
				processed = (count > 0);
			}
			/* check if there is NO MORE content to send to the CGI script */
			else {
				logger << "  - produce: no more data available\n";
				processed = true;
				/* close write file descriptor */
				std::get<1>(pollResult) = nullptr; // producer = nullptr;
			}
		}


		/* check if it is possible to receive something from the process */
		if(consumer) {
			/* reveive content from the process */
			logger << "- consume...\n";
			bool success = consumer->consume(fileDescriptor);

			if(success) {
				logger << "- consume: successful\n";
				allNpos = false;
				processed = true;
			}
			/* check if no more data to read desired. drop responseHandler */
			else {
				logger << "- consume: no more data desired\n";
				/* close write file descriptor */
				std::get<2>(pollResult) = nullptr; // consumer = nullptr;
			}
		}

		if(allNpos) {
			logger << "- close fd " << fileDescriptor.getHandle() << "\n";
			fileDescriptor.close();
		}
	}

	if(processed) {
		logger << "- something processed\n";
	}
	else {
		logger << "- nothing processed\n";
	}

	return processed;
}

void Process::addParameterStream(ParameterStreams& parameterStreams, process::FileDescriptor::Handle handle, process::Producer* producer, process::Consumer* consumer) {
	ParameterStream& parameterStream = parameterStreams[handle];

	if(producer == nullptr && consumer == nullptr) {
    	if(parameterStream.producer && parameterStream.consumer) {
    		throw std::runtime_error("Conflicting parameters: Flag defined to close handle " + std::to_string(handle) + " for child process, but there is already a producer and a consumer defined for this handle.");
    	}
    	else if(parameterStream.producer && parameterStream.consumer == nullptr) {
    		throw std::runtime_error("Conflicting parameters: Flag defined to close handle " + std::to_string(handle) + " for child process, but there is already a producer defined for this handle.");
    	}
    	else if(parameterStream.producer == nullptr && parameterStream.consumer) {
    		throw std::runtime_error("Conflicting parameters: Flag defined to close handle " + std::to_string(handle) + " for child process, but there is already a consumer defined for this handle.");
    	}
	}
	else {
    	if(producer) {
        	if(parameterStream.producer) {
        		throw std::runtime_error("Conflicting parameters: Multiple producer defined for handle " + std::to_string(handle) + " for child process.");
        	}
        	parameterStream.producer = producer;
    	}

    	if(consumer) {
        	if(parameterStream.consumer) {
        		throw std::runtime_error("Conflicting parameters: Multiple producer defined for handle " + std::to_string(handle) + " for child process.");
        	}
        	parameterStream.consumer = consumer;
    	}
	}
}


} /* namespace zsystem */
