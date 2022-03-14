#include<iostream>
#include<math.h>
using namespace std;
void recursion(int * arr, int n)
{
	if (n == 1) {
		return;
	}
	else
	{
		for (int i = 0; i < n / 2; i++) {
			arr[i] += arr[n - i - 1];
		}
		n = n / 2;
		recursion(arr, n);
	}
}

int main() {
	//初始化
	int count = 100;//循环次数
	int m = 20;
	int n = pow(2, m);//规模
	int* arr = new int[n];//待累加的数
	for (int i = 0;i < n;i++) {
		arr[i] = i;
	}
	while (count > 0) {
	int sum = 0;//累加结果
	//优化算法之函数递归
	recursion(arr, n);
	sum = arr[0];
	count--;
	}
	return 0;
}
