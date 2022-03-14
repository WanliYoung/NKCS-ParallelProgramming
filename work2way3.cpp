#include<iostream>
#include<math.h>
using namespace std;
int main() {
	//初始化
	int count = 100;//循环次数
	int m = 10;
	int n = pow(2, m);//规模
	int* arr = new int[n];//待累加数
	while (count > 0) {
		int sum = 0;//累加结果
		//优化算法之递归，双重循环实现
		for (int i = n;i > 1;i /= 2) {
			for (int j = 0;j < i / 2;j++) {
				arr[j * 2] = j * 2;
				arr[j * 2 + 1] = j * 2 + 1;
				arr[j] = arr[j * 2] + arr[j * 2 + 1];
			}
		}
		sum = arr[0];
		count--;
	}
	return 0;
}
