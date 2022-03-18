#include"ObjectPool.h"

#include<vector>
#include<time.h>

using namespace std;

struct TreeNode 
{ 
	int _val; 
	TreeNode* _left;  
	TreeNode* _right;   
	TreeNode() :_val(0), _left(nullptr), _right(nullptr) {}
};

int main()
{
	size_t N = 1000000;
	vector<TreeNode*>vet;
	vet.resize(N);
	size_t begin = clock();
	for (int i = 0; i < vet.size(); i++) {
		vet[i]=new TreeNode;
	}
	for (int i = 0; i < vet.size(); i++) {
		delete vet[i];
	}

	size_t end = clock();
	cout << "new TreeNode Time£º" << end - begin << endl;

	ObjectPool<TreeNode>pool;
	size_t begin2 = clock();
	for (int i = 0; i < vet.size(); i++) {
		vet[i]=pool.New();
	}
	for (int i = 0; i < vet.size(); i++) {
		pool.Delete(vet[i]);
	}
	size_t end2 = clock();
	cout << "ObjectPool New Time£º" << end2 - begin2 << endl;
	return 0;
}