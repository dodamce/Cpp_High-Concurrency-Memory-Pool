#include"ConcurrentAlloc.h"

//ntimes��һ���������ͷ��ڴ�Ĵ�ʱ rounds��һ�����Լ��� nworks�߳���

void TestBenchMarkMalloc(size_t ntimes, size_t rounds, size_t nworks) {
	std::cout << "Malloc Test" << std::endl;
	std::vector<std::thread>ThreadPool(nworks);

	//����ÿ���߳������ͷŵ�ʱ������ӣ�ʹ�õ���ԭ���Ե���ӣ�C++11�﷨
	std::atomic<size_t>malloc_time = 0;
	std::atomic<size_t>free_time = 0;

	for (size_t i = 0; i < nworks; i++) {
		ThreadPool[i] = std::thread(
			[&, i]() {//��ʾ���ô��ݲ�׽���и��������еı���(����this)
				std::vector<void*>Pool;//��������Ŀռ��ַ
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
		);//C++11 lambda���ʽ
	}

	for (auto& e : ThreadPool) {
		e.join();
	}
	std::cout << nworks << "���߳� �����̲߳���"<<ntimes<<"�� ÿ��" << rounds << "���������ͷ��ڴ�" << std::endl << 
		"����ʱ��" << malloc_time.load() << "ms �ͷ�ʱ��" << free_time.load() << "ms\n��ʱ��" << free_time.load() + malloc_time.load() <<"ms" << std::endl;
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
		);//C++11 lambda���ʽ
	}

	for (auto& e : ThreadPool) {
		e.join();
	}
	std::cout << nworks << "���߳� �����̲߳���" << ntimes << "�� ÿ��" << rounds << "���������ͷ��ڴ�" << std::endl <<
		"����ʱ��" << Alloc_time.load() << "ms �ͷ�ʱ��" << Delete_time.load() << "ms\n��ʱ��" << Alloc_time.load() + Delete_time.load() << "ms" << std::endl;
	std::cout << "########################################" << std::endl;

}

int main() {
	TestBenchMarkMalloc(10000,4,10);
	TestConcurrentAlloc(10000, 4, 10);
	return 0;
}