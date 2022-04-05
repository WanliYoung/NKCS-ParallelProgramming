#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include<vector>
#include<iostream>
#include<string>
#include<bitset>
#include<arm_neon.h>
using namespace std;
FILE *fi;
FILE *fp;
struct _INDEX {
	unsigned int  len;
	unsigned int *arr;
} *idx;
int MAXARRS = 2000;
unsigned int i, alen;
unsigned int *aarr;
int j, n;

vector<int> strtoints(char* line) {
	vector<int> arr;
	int i = 0;
	int num = 0;
	while (line[i] == ' ' || (line[i] >= 48 && line[i] <= 57)) {
		num = 0;
		while (line[i] != ' ') {
			num *= 10;
			int tmp = line[i] - 48;
			num += tmp;
			i++;
		}
		i++;
		arr.push_back(num);
	}
	return arr;
}

bool find(unsigned int e, _INDEX list) {
	for (int i = 0;i < list.len;i++) {
		if (e == list.arr[i]) {
			return true;
		}
	}
	return false;
}

void bittoint(bitset<25205184> bits, int * arr) {
	bitset<32> tmp;
	int i = 0;
	int count = 0;
	int times = 0;
	while (count < 25205184) {
		i = 0;
		while (i < 32) {
			tmp[i] = bits[times * 32 + i];
			i++;
			count++;
		}
		arr[times] = tmp.to_ulong();
		times += 1;
	}
}

void inttobitset(bitset<25205184> bits, int * arr) {
	for (int i = 0;i < 787622;i++) {
		bitset<32> tmp = arr[i];
		for (int j = 0;j < 32;j++) {
			bits[i * 32 + j] = tmp[j];
		}
	}
}

void dividebit(bitset<25205248> bits, bitset<128>* arr) {
}

int main() {
	//打开文档读入数据集
	fi = fopen("/home/s2013774/SIMD/ExpIndex", "rb");
	if (NULL == fi) {
		printf("Can not open file ExpIndex!\n");
		return 1;
	}
	idx = (struct _INDEX *)malloc(MAXARRS * sizeof(struct _INDEX));
	if (NULL == idx) {
		printf("Can not malloc %d bytes for idx!\n", MAXARRS * sizeof(struct _INDEX));
		return 2;
	}
	j = 0;
	while (1) {
		fread(&alen, sizeof(unsigned int), 1, fi);
		if (feof(fi)) break;
		aarr = (unsigned int *)malloc(alen * sizeof(unsigned int));
		if (NULL == aarr) {
			printf("Can not malloc %d bytes for aarr!\n", alen * sizeof(unsigned short));
			return 3;
		}
		for (i = 0;i < alen;i++) {
			fread(&aarr[i], sizeof(unsigned int), 1, fi);
			if (feof(fi)) break;
		}
		if (feof(fi)) break;
		idx[j].len = alen;
		idx[j].arr = aarr;
		j++;
		if (j >= MAXARRS) {
			printf("Too many arrays(>=%d)!\n", MAXARRS);
			break;
		}
	}
	fclose(fi);
	//现在已经有一个idx数组存储了这个倒排索引文件，idx[i].arr表示第i个关键词的倒排索引链表
	//下面是query_list代表查询的二维数组，大概能到2000个关键词，所以上面的max可以设置为2000
	int numIndex = j;
	fp = fopen("/home/s2013774/SIMD/ExpQuery", "r");
	vector<vector<int> > query_list;

	vector<int> arr;
	char* line = new char[100];
	while ((fgets(line, 100, fp)) != NULL)
	{
		arr = strtoints(line);
		query_list.push_back(arr);
	}
	fclose(fp);

	//接下来开始实现倒排索引求交技术
	//实现按表求交的位图存储方法
	int QueryNum = 100;//查询次数
	for (int i = 0;i < QueryNum;i++) {
		int TermNum = query_list[i].size();
		bitset<25205248> * lists;//25205184=128的倍数
		lists = new bitset<25205248>[TermNum];
		for (int j = 0;j < TermNum;j++) {
			for (int k = 0;k < idx[query_list[i][j]].len;k++) {
				lists[j].set(idx[query_list[i][j]].arr[k]);//括号内是文档编号，把文档对应的二进制位置为1
			}
		}
		for (int i = 1;i < TermNum;i++) {
			for (int j = 0;j < 196916;j++) {
				long addrtmp1 = (long)&lists[0];
				int* ptrtmp1 = (int *)(addrtmp1 + 16 * j);
				long addrtmp2 = (long)&lists[i];
				int* ptrtmp2 = (int *)(addrtmp2 + 16 * j);
				int32x4_t tmp1 = vld1q_s32(ptrtmp1);
				int32x4_t tmp2 = vld1q_s32(ptrtmp2);
				tmp1 = vandq_s32(tmp1, tmp2);
				vst1q_s32(ptrtmp1, tmp1);
			}
		}

		//输出结果
		vector<unsigned int> result;
		for (int i = 0;i < 25205174;i++) {
			if (lists[0][i] == 1) {
				result.push_back(i);
			}
		}
		cout << result.size() << endl;
	}
	for (j = 0;j < n;j++) free(idx[j].arr);
	free(idx);

	return 0;
}
