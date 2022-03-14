#include<iostream>
#include<math.h>
using namespace std;
int main() {
	//初始化
	int count = 5;//循环次数
	int m = 15;
	int n = pow(2, m);//问题规模，任意指定
	int* arr = new int[n];//待累加的数
	while (count > 0) {
		int sum = 0;//累加结果
		//平凡算法,逐个累加，循环展开幅度4
		for (int i = 0;i < n;i += 4) {
			arr[i] = i;
			sum += arr[i];
			arr[i + 1] = i + 1;
			sum += arr[i + 1];
			arr[i + 2] = i + 2;
			sum += arr[i + 2];
			arr[i + 3] = i + 3;
			sum += arr[i + 3];
		}
		count--;
	}
	return 0;
}
