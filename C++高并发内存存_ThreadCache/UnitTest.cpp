#include"ConcurrentAlloc.h"

void Alloc() {
	for (int i = 0; i < 10; i++) {
		void* ptr = ConcurrentAlloc(6);
	}
}

void Alloc2() {
	for (int i = 0; i < 10; i++) {
		void* ptr = ConcurrentAlloc(7);
	}
}

void TestTLS() {
	std::thread t1(Alloc);
	t1.join();
	std::thread t2(Alloc2);
	t2.join();
}

int main()
{
	TestTLS();
	return 0;
}