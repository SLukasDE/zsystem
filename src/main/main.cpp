#include <zsystem/SharedMemory.h>
#include <zsystem/Process.h>
#include <zsystem/process/Arguments.h>
#include <zsystem/process/Environment.h>
#include <zsystem/process/Consumer.h>
#include <zsystem/process/ConsumerFile.h>
#include <zsystem/process/ProducerStatic.h>
#include <zsystem/process/ProducerFile.h>
#include <zsystem/process/FileDescriptor.h>

#include <iostream>

using namespace zsystem;
using namespace zsystem::process;

std::string produceStr = "Hello\n"
		"World!\n";

class MyConsumer : public Consumer {
public:
	MyConsumer() = default;

	std::size_t read(FileDescriptor& fileDescriptor) override {
		char buffer[4096];
		std::size_t count = fileDescriptor.read(buffer, sizeof(buffer));
		if(count != FileDescriptor::npos) {
			std::string str(buffer, count);
			std::cout << "CONSUMED (" << count << " bytes)\n"
					<< "*** CONSUMED BEGIN ***\n"
					<< str << "\n"
					<< "*** CONSUMED END ***\n"
					<< "\n";
		}
		return count;
	}
};

void printTestcase_1() {
	std::cout <<
			"  1  Execute \"/usr/bin/kwrite\".\n"
			"     - Redirect stdout to OWN CONSUMER (kwrite only writes to stderr).\n"
			"     - Not closing stderr.\n"
			"     - Produce nothing to stdin.\n"
			"     Result:\n"
			"     - Consumer should displayed nothing, because kwrite does not write to stdout.\n"
			"     - But kwrite writes to stderr. This should be displayed directly.\n"
			"\n";
}

void printTestcase_2() {
	std::cout <<
			"  2  Execute \"/usr/bin/cat ../datarunner-backup/src/main/tinyxml/tinyxml.h\".\n"
			"     - Redirect stdout to nothing.\n"
			"     - Redirect nothing to stdin.\n"
			"     - Not closing stderr.\n"
			"     Result:\n"
			"     - Nothing should be displayed.\n"
			"\n";
}

void printTestcase_3() {
	std::cout <<
			"  3  Execute \"/usr/bin/cat ../datarunner-backup/src/main/tinyxml/tinyxml.h\".\n"
			"     - Redirect stdout to OWN CONSUMER.\n"
			"     - Redirect nothing to stdin.\n"
			"     - Not closing stderr.\n"
			"     Result:\n"
			"     - Consumer should display stdout in 4096 bytes chunks.\n"
			"\n";
}

void printTestcase_4() {
	std::cout <<
			"  4  Execute \"/usr/bin/cat ../datarunner-backup/src/main/tinyxml/tinyxml.h\".\n"
			"     - Redirect stdout to \"/tmp/result.txt\"\n"
			"     - Redirect nothing to stdin.\n"
			"     Result:\n"
			"     - \"/tmp/result.txt\" should be a copy of \"../datarunner-backup/src/main/tinyxml/tinyxml.h\".\n"
			"\n";
}

void printTestcase_5() {
	std::cout <<
			"  5  Execute \"/usr/bin/sed -n w\\ /tmp/result2.txt\".\n"
			"     - Redirect stdout to OWN CONSUMER.\n"
			"     - Redirect nothing to stdin.\n"
			"     Result:\n"
			"     - No input is redirected to \"sed\", so \"sed\" writes nothing to \"/tmp/result2.txt\".\n"
			"     - \"sed\" writes as well nothing to stdout, so consumer should display nothing.\n"
			"     - \"/tmp/result2.txt\" should be empty.\n"
			"\n";
}

void printTestcase_6() {
	std::cout <<
			"  6  Execute \"/usr/bin/sed -n w\\ /tmp/result2.txt\".\n"
			"     - Redirect stdout to OWN CONSUMER.\n"
			"     - Redirect OWN PRODUCER to stdin.\n"
			"     Result:\n"
			"     - Producer writes \"Hello\\nWorld!\\n\" to \"sed\".\n"
			"     - \"sed\" writes \"Hello\\nWorld!\\n\" to \"/tmp/result2.txt\" and nothing to stdout.\n"
			"     - \"/tmp/result2.txt\" should contain \"Hello\\nWorld!\\n\".\n"
			"\n";
}

void printTestcase_7() {
	std::cout <<
			"  7  Execute \"/usr/bin/sed -n w\\ /dev/stdout\".\n"
			"     - Redirect stdout to OWN CONSUMER.\n"
			"     - Redirect OWN PRODUCER to stdin.\n"
			"     Result:\n"
			"     - Producer writes \"Hello\\nWorld!\\n\" to \"sed\".\n"
			"     - \"sed\" writes \"Hello\\nWorld!\\n\" to stdout.\n"
			"     - Consumer should display \"Hello\\nWorld!\\n\".\n"
			"\n";
}

void printTestcase_8() {
	std::cout <<
			"  8  Execute \"/usr/bin/sed -n w\\ /dev/stdout\".\n"
			"     - Redirect stdout to OWN CONSUMER.\n"
			"     - Redirect \"../datarunner-backup/src/main/tinyxml/tinyxml.h\" to stdin.\n"
			"     Result:\n"
			"     - FD of \"../datarunner-backup/src/main/tinyxml/tinyxml.h\" is set to stdin of \"sed\".\n"
			"     - \"sed\" writes content of \"../datarunner-backup/src/main/tinyxml/tinyxml.h\" to stdout.\n"
			"     - Consumer should display content of \"../datarunner-backup/src/main/tinyxml/tinyxml.h\" in 4096 bytes chunks..\n"
			"\n";
}

void printTestcase_9() {
	std::cout <<
			"  9  Execute \"/usr/bin/sed -n w\\ /dev/stdout\".\n"
			"     - Redirect stdout to \"/tmp/result.txt\".\n"
			"     - Redirect \"../datarunner-backup/src/main/tinyxml/tinyxml.h\" to stdin.\n"
			"     Result:\n"
			"     - FD of \"../datarunner-backup/src/main/tinyxml/tinyxml.h\" is set as stdin of \"sed\".\n"
			"     - FD of \"/tmp/result.txt\" is set as stdout of \"sed\".\n"
			"     - \"sed\" writes content of \"../datarunner-backup/src/main/tinyxml/tinyxml.h\" to \"/tmp/result.txt\".\n"
			"\n";
}

void printUsage() {
	std::cout <<
			"zprocess testcase\n"
			"\n";
	printTestcase_1();
	printTestcase_2();
	printTestcase_3();
	printTestcase_4();
	printTestcase_5();
	printTestcase_6();
	printTestcase_7();
	printTestcase_8();
	printTestcase_9();
}

int main(int argc, char* argv[]) {
	if(argc != 2) {
		printUsage();
	}
	else {
		std::string testcase = argv[1];

		std::cout << "ZProcess Testcase!\n";

		if(testcase == "1") {
			MyConsumer myConsumer;

			Process process(Arguments("/usr/bin/kwrite"));
			process.execute(myConsumer, FileDescriptor::stdOutHandle, FileDescriptor::stdErrHandle);

			std::cout << "\n\nExecuted testcase:\n";
			printTestcase_1();
		}
		else if(testcase == "2") {
			Process process(Arguments("/usr/bin/cat ../datarunner-backup/src/main/tinyxml/tinyxml.h"));
			process.execute(FileDescriptor::stdErrHandle);

			std::cout << "\n\nExecuted testcase:\n";
			printTestcase_2();
		}
		else if(testcase == "3") {
			MyConsumer myConsumer;

			Process process(Arguments("/usr/bin/cat ../datarunner-backup/src/main/tinyxml/tinyxml.h"));
			process.execute(myConsumer, FileDescriptor::stdOutHandle, FileDescriptor::stdErrHandle);

			std::cout << "\n\nExecuted testcase:\n";
			printTestcase_3();
		}
		else if(testcase == "4") {
			ConsumerFile myConsumer(FileDescriptor::openFile("/tmp/result.txt", false, true, true));

			Process process(Arguments("/usr/bin/cat ../datarunner-backup/src/main/tinyxml/tinyxml.h"));
			process.execute(myConsumer, FileDescriptor::stdOutHandle);

			std::cout << "\n\nExecuted testcase:\n";
			printTestcase_4();
		}
		else if(testcase == "5") {
			MyConsumer myConsumer;

			Process process(Arguments("/usr/bin/sed -n w\\ /tmp/result2.txt"));
			process.execute(myConsumer, FileDescriptor::stdOutHandle);

			std::cout << "\n\nExecuted testcase:\n";
			printTestcase_5();
		}
		else if(testcase == "6") {
			MyConsumer myConsumer;
			ProducerStatic myProducer(produceStr.data(), produceStr.size());

			Process process(Arguments("/usr/bin/sed -n w\\ /tmp/result2.txt"));
			process.execute(myProducer, FileDescriptor::stdInHandle, myConsumer, FileDescriptor::stdOutHandle);

			std::cout << "\n\nExecuted testcase:\n";
			printTestcase_6();
		}
		else if(testcase == "7") {
			MyConsumer myConsumer;
			ProducerStatic myProducer(produceStr.data(), produceStr.size());

			Process process(Arguments("/usr/bin/sed -n w\\ /dev/stdout"));
			process.execute(myProducer, FileDescriptor::stdInHandle, myConsumer, FileDescriptor::stdOutHandle);

			std::cout << "\n\nExecuted testcase:\n";
			printTestcase_7();
		}
		else if(testcase == "8") {
			MyConsumer myConsumer;
			ProducerFile myProducer(FileDescriptor::openFile("../datarunner-backup/src/main/tinyxml/tinyxml.h", true, false, false));

			Process process(Arguments("/usr/bin/sed -n w\\ /dev/stdout"));
			process.execute(myProducer, FileDescriptor::stdInHandle, myConsumer, FileDescriptor::stdOutHandle);

			std::cout << "\n\nExecuted testcase:\n";
			printTestcase_8();
		}
		else if(testcase == "9") {
			ConsumerFile myConsumer(FileDescriptor::openFile("/tmp/result.txt", false, true, true));
			ProducerFile myProducer(FileDescriptor::openFile("../datarunner-backup/src/main/tinyxml/tinyxml.h", true, false, false));

			Process process(Arguments("/usr/bin/sed -n w\\ /dev/stdout"));
			process.execute(myConsumer, FileDescriptor::stdOutHandle, myProducer, FileDescriptor::stdInHandle);

			std::cout << "\n\nExecuted testcase:\n";
			printTestcase_9();
		}
		else {
			printUsage();
		}
	}

	return 0;
}
