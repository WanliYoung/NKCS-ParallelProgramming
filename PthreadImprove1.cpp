#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include<vector>
#include<iostream>
#include<string>
#include<bitset>
#include<arm_neon.h>
#include<pthread.h>
using namespace std;
FILE *fi;
FILE *fp;
struct _INDEX {
	unsigned int  len;
	unsigned int *arr;
} *idx;
struct package {
	bitset<25214976> first; //底层向量
	bitset<196992> second;  // 二级索引
	_INDEX index;  //关键词的倒排列表
};
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

//构建线程执行的处理函数
void* tobits (void* parm) {
	//package pack = *(package*)parm;  //把参数包转换成package，要把pack中的关键词列表转换成位向量和二级索引
	for (int k = 0;k < (*(package*)parm).index.len;k++) {
		(*(package*)parm).first.set((*(package*)parm).index.arr[k]);//括号内是文档编号，把文档对应的二进制位置为1
	}
	//位向量转换成功
	long addrtemp = (long)&(*(package*)parm).first;
	for (int j = 0;j < 196992;j++) {
		bitset<128> * setptr = (bitset<128>*)(addrtemp + 16 * j);
		if (*setptr == 0) {
			;
		}
		else {
			(*(package*)parm).second.set(j);
		}
	}
	//二级索引建立成功
	return NULL;
}

int main() {
	//打开文档读入数据集
	fi = fopen("/home/s2013774/Pthread/ExpIndex", "rb");
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
	int QueryNum = 1000;//查询次数
	vector<vector<unsigned int>> results;
	for (int i = 0;i < QueryNum;i++) {
		int TermNum = query_list[i].size();
		package* packages = new package[TermNum];
		for (int j = 0;j < TermNum;j++) {
			packages[j].index = idx[query_list[i][j]];
		}

		long thread;
		int thread_count = TermNum;//线程数就是关键词的数量
		pthread_t* thread_handles;
		thread_handles = new pthread_t[thread_count];
		for (thread = 0;thread < thread_count;thread++) {
			pthread_create(&thread_handles[thread], NULL, tobits, (void*)&packages[thread]);
		}
		for (thread = 0;thread < thread_count;thread++) {
			pthread_join(thread_handles[thread], NULL);
		}
		free(thread_handles);

		
		for (int i = 1;i < TermNum;i++) {
			packages[0].second &= packages[i].second;
			for (int j = 0;j < 196992;j++) {
				if (packages[0].second[j] == 1) {
					long addrtemp1 = (long)&packages[0].first;
					int* ptrtemp1 = (int *)(addrtemp1 + j * 16);
					long addrtemp2 = (long)&packages[i].first;
					int* ptrtemp2 = (int *)(addrtemp2 + j * 16);
					int32x4_t temp1 = vld1q_s32(ptrtemp1);
					int32x4_t temp2 = vld1q_s32(ptrtemp2);
					temp1 = vandq_s32(temp1, temp2);
					vst1q_s32(ptrtemp1, temp1);
				}
				else {
					long addr = (long)&packages[0];
					bitset<128> * setptr = (bitset<128>*)(addr + 16 * j);
					*setptr = 0;
				}
			}
		}


		vector<unsigned int> result;
		//结果转换优化，list[0]就是结果位向量，256为单位判断
		long address = (long)&packages[0].first;
		for (int j = 0;j < 98496;j++) {
			bitset<256> * setptr = (bitset<256>*)(address + 32 * j);
			if (*setptr == 0) {
				;
			}
			//如果不为0再去底层判断
			else {
				for (int k = 0;k < 256;k++) {
					if (packages[0].first[j * 256 + k] == 1) {
						result.push_back(j * 256 + k);
					}
				}
			}
		}
		//处理完一次查询
		//results.push_back(result);
		cout << result.size() << endl;
	}

	for (j = 0;j < n;j++) free(idx[j].arr);
	free(idx);

	return 0;
}
