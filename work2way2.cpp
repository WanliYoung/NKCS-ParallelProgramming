#include<iostream>
#include<math.h>
using namespace std;
int main() {
	//初始化
	int count = 5;//循环次数
	int m = 15;
	int n = pow(2, m);//问题规模
	int* arr = new int[n];//待累加的数
	while (count > 0) {
		int sum = 0;//累加结果
		//优化算法之双路
		int sum1 = 0;
		int sum2 = 0;
		//循环展开，一步循环包含四步操作
		for (int i = 0;i < n;i += 8) {
			arr[i] = i;
			arr[i + 1] = i + 1;
			sum1 += arr[i];
			sum2 += arr[i + 1];
			arr[i + 2] = i + 2;
			arr[i + 3] = i + 3;
			sum1 += arr[i + 2];
			sum2 += arr[i + 3];
			arr[i + 4] = i + 4;
			arr[i + 5] = i + 5;
			sum1 += arr[i + 4];
			sum2 += arr[i + 5];
			arr[i + 6] = i + 6;
			arr[i + 7] = i + 7;
			sum1 += arr[i + 6];
			sum2 += arr[i + 7];

		}
		sum = sum1 + sum2;
		count--;
	}
	return 0;
}