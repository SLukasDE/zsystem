#include <zsystem/SharedMemory.h>
#include <zsystem/Process.h>
#include <iostream>

int testProcessSilent() {
	zsystem::Process p;

	p.setStdOut(nullptr);
	p.setStdErr(nullptr);
	p.execute("/usr/bin/lsmem", {});

	return p.wait();
}

int testProcessFile() {
	zsystem::Process p;
	zsystem::Process::File pOut("/tmp/foo.txt");

	p.setStdOut(&pOut);
	p.setStdErr(&pOut);
	p.execute("/usr/bin/lsmem", {});

	return p.wait();
}

int testProcessDefault() {
	zsystem::Process p;
	zsystem::Process::Default pOut;

	p.setStdOut(&pOut);
	p.setStdErr(&pOut);
	p.execute("/usr/bin/lsmem", {});

	return p.wait();
}

int testProcessPipe() {
	zsystem::Process p;
	zsystem::Process::Pipe pOut;

	p.setStdOut(&pOut);
	p.setStdErr(&pOut);
	p.execute("/usr/bin/lsmem", {});

	pOut.setBlocking(true);
	while(true) {
		char b[100];
		std::size_t s = pOut.read(b, 100);
		if(s == 0 && p.isRunning() == false) {
			break;
		}
//		p.getStdOut()->setBlocking(s == 0);
		std::cout << std::string(b, s) << std::flush;
	}

	return p.wait();
}

struct Foo {
	char name[100];
};

int main(int argc, const char** argv) {
	int rc = 0;

//	zsystem::SharedMemory<void> sm1(100);
//	zsystem::SharedMemory<Foo> sm2;

	rc = testProcessPipe();

	return rc;
}

