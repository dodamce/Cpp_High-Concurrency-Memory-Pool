#include"ConcurrentAlloc.h"

void TestConcurrentAlloc() {
	void* p1 = ConcurrentAlloc(6);
	void* p2 = ConcurrentAlloc(8);
	void* p3 = ConcurrentAlloc(1);
	void* p4 = ConcurrentAlloc(7);
	void* p5 = ConcurrentAlloc(18);

	cout << p1 << endl;
	cout << p2 << endl;
	cout << p3 << endl;
	cout << p4 << endl;
	cout << p5 << endl;

	ConcurrentFree(p1, 6);
	ConcurrentFree(p2, 8);
	ConcurrentFree(p3, 1);
	ConcurrentFree(p4, 7);
	ConcurrentFree(p5, 18);
}

void TestConcurrentAlloc2() {
	for (size_t i = 0; i < 5; i++) {
		void* p1 = ConcurrentAlloc(6);
		cout << p1 << endl;
	}
	void* p2 = ConcurrentAlloc(6);
	cout << p2 << endl;
}

void TestPageId() {
	PAGE_ID id = 2020;
	PAGE_ID id2 = 2021;//1页大小
	char* start = (char*)(id << PAGESIZE);
	char* end = (char*)(id2 << PAGESIZE);
	cout << (void*)start << " " << (void*)end << endl;
	while (start <= end) {
		cout << (void*)start << " PAGEID:" << ((PAGE_ID)start >> PAGESIZE) << endl;
		start += 8;//一块空间8字节
	}
}

void f1() {
	std::vector<void*>vet;
	for (int i = 0; i < 10; i++) {
		void* ptr = ConcurrentAlloc(6);
		vet.push_back(ptr);
	}
	for (auto e : vet) {
		ConcurrentFree(e, 6);
	}
}

void f2() {
	std::vector<void*>vet;
	for (int i = 0; i < 10; i++) {
		void* ptr = ConcurrentAlloc(7);
		vet.push_back(ptr);
	}
	for (auto e : vet) {
		ConcurrentFree(e, 7);
	}
}

void TestConcurrentAllocThread() {
	std::thread t1(f1);
	t1.join();
	std::thread t2(f2);
	t2.join();
}

int main()
{
	TestConcurrentAllocThread();
	return 0;
}