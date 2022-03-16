#include"ConcurrentAlloc.h"

//ntimes：一轮申请与释放内存的此时 rounds：一共测试几轮 nworks线程数

void TestBenchMarkMalloc(size_t ntimes, size_t rounds, size_t nworks) {
	std::cout << "Malloc Test" << std::endl;
	std::vector<std::thread>ThreadPool(nworks);

	//计算每个线程申请释放的时间再相加，使用的是原子性的相加，C++11语法
	std::atomic<size_t>malloc_time = 0;
	std::atomic<size_t>free_time = 0;

	for (size_t i = 0; i < nworks; i++) {
		ThreadPool[i] = std::thread(
			[&, i]() {//表示引用传递捕捉所有父作用域中的变量(包括this)
				std::vector<void*>Pool;//保存申请的空间地址
				Pool.reserve(ntimes);
				for (size_t j = 0; j < rounds; j++) {
					size_t begin = clock();
					for (size_t k = 0; k < ntimes; k++) {
						Pool.push_back(malloc(8 + k));
					}
					size_t end = clock();

					size_t begin2 = clock();
					for (size_t k = 0; k < ntimes; k++) {
						free(Pool[k]);
					}
					size_t end2 = clock();
					Pool.clear();
					malloc_time += (end - begin);
					free_time += (end2 - begin2);
				}
			}
		);//C++11 lambda表达式
	}

	for (auto& e : ThreadPool) {
		e.join();
	}
	std::cout << nworks << "个线程 单个线程测试"<<ntimes<<"轮 每轮" << rounds << "次申请与释放内存" << std::endl << 
		"申请时间" << malloc_time.load() << "ms 释放时间" << free_time.load() << "ms\n总时间" << free_time.load() + malloc_time.load() <<"ms" << std::endl;
	std::cout << "########################################" << std::endl;

}

void TestConcurrentAlloc(size_t ntimes, size_t rounds, size_t nworks) {
	std::cout << "ConcurrentAlloc Test" << std::endl;
	std::vector<std::thread>ThreadPool(nworks);
	std::atomic<size_t>Alloc_time = 0;
	std::atomic<size_t>Delete_time = 0; 

	for (size_t i = 0; i < nworks; i++) {
		ThreadPool[i] = std::thread(
			[&, i]() {
				std::vector<void*>Pool;
				Pool.reserve(ntimes);
				for (size_t j = 0; j < rounds; j++) {
					size_t begin = clock();
					for (size_t k = 0; k < ntimes; k++) {
						Pool.push_back(ConcurrentAlloc(8 ));
					}
					size_t end = clock();

					size_t begin2 = clock();
					for (size_t k = 0; k < ntimes; k++) {
						ConcurrentFree(Pool[k]);
					}
					size_t end2 = clock();
					Pool.clear();
					Alloc_time += (end - begin);
					Delete_time += (end2 - begin2);
				}
			}
		);//C++11 lambda表达式
	}

	for (auto& e : ThreadPool) {
		e.join();
	}
	std::cout << nworks << "个线程 单个线程测试" << ntimes << "轮 每轮" << rounds << "次申请与释放内存" << std::endl <<
		"申请时间" << Alloc_time.load() << "ms 释放时间" << Delete_time.load() << "ms\n总时间" << Alloc_time.load() + Delete_time.load() << "ms" << std::endl;
	std::cout << "########################################" << std::endl;

}

int main() {
	TestBenchMarkMalloc(10000,4,10);
	TestConcurrentAlloc(10000, 4, 10);
	return 0;
}