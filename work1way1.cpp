#include<iostream>
using namespace std;
int main() {
	int n = 1000;//问题规模，任意指定
	//初始化
	int** arr;
	arr = new int*[n];
	for (int i = 0;i < n;i++) {
		arr[i] = new int[n];
	}
	for (int i = 0;i < n;i++) {
		for (int j = 0;j < n;j++) {
			arr[i][j] = i + j;
		}
	}
	int* test;
	test = new int[n];
	for (int i = 0;i < n;i++) {
		test[i] = i;
	}
	int* sum;
	sum = new int[n];
	//计算，按列访问
	int count = 10;//循环次数
	while (count > 0) {
		for (int i = 0;i < n;i++) {
			sum[i] = 0;
		}
		for (int i = 0;i < n;i++) {
			for (int j = 0;j < n;j++) {
				sum[i] += test[j] * arr[j][i];
			}
		}
		count--;
	}
	return 0;
}
