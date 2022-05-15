#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include<vector>
#include<iostream>
#include<string>
#include<bitset>
#include<sys/time.h>
#include<arm_neon.h>
#ifdef _OPENMP
#include<omp.h>
#endif // _OPENMP
#pragma warning( disable : 4996) 

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

int main() {
	struct timeval t1, t2;
	double timeuse = 0;
	//打开文档读入数据集
	
	fi = fopen("/home/s2013774/OpenMP/ExpIndex", "rb");
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
	fp = fopen("/home/s2013774/OpenMP/ExpQuery", "r");
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
		bitset<25214976> * lists;//25214976=128*196992
		lists = new bitset<25214976>[TermNum];
		bitset<196992> *  second;
		second = new bitset<196992>[TermNum];
#pragma omp parallel for num_threads(TermNum)
		for (int j = 0;j < TermNum;j++) {
			for (int k = 0;k < idx[query_list[i][j]].len;k++) {
				lists[j].set(idx[query_list[i][j]].arr[k]);//括号内是文档编号，把文档对应的二进制位置为1
			}
		//第一层的位向量已经存储完毕，接下来建立索引,建立索引已经得到了优化
			long addrtemp1 = (long)&lists[j];
			for (int k = 0;k < 196992;k++) {
				bitset<128> * setptr = (bitset<128>*)(addrtemp1 + 16 * k);
				if (*setptr == 0) {
					;
				}
				else {
					second[j].set(k);
				}
			}
		}
		for (int i = 1;i < TermNum;i++) {
			second[0] &= second[i];  //二级索引层直接按位与，底层SIMD
#pragma omp parallel for num_threads(8)
			for (int j = 0;j < 196992;j++) {
				if (second[0][j] == 1) {  //第j位是1，需要底层求交
					long addrtemp1 = (long)&lists[0];
					int* ptrtemp1 = (int *)(addrtemp1 + j * 16);
					long addrtemp2 = (long)&lists[i];
					int* ptrtemp2 = (int *)(addrtemp2 + j * 16);
					int32x4_t temp1 = vld1q_s32(ptrtemp1);
					int32x4_t temp2 = vld1q_s32(ptrtemp2);
					temp1 = vandq_s32(temp1, temp2);
					vst1q_s32(ptrtemp1, temp1);
				}
				else {
					long addr = (long)&lists[0];
					bitset<128> * setptr = (bitset<128>*)(addr + 16 * j);
					*setptr = 0;//全部置零
				}
			}
		}
		vector<unsigned int> result;
		vector<unsigned int> myresult[8];
		//结果转换优化，list[0]就是结果位向量，256为单位判断
		gettimeofday(&t1, NULL);
		long address = (long)&lists[0];
#pragma omp parallel for num_threads(8)
		for (int j = 0;j < 98496;j++) {
			bitset<256> * setptr = (bitset<256>*)(address + 32 * j);
			if (*setptr == 0) {
				;
			}
			//如果不为0再去底层判断
			else {
				for (int k = 0;k < 256;k++) {
					if (lists[0][j * 256 + k] == 1) {
						myresult[j/12312].push_back(j * 256 + k);
					}
				}
			}
		}
		//结果合并
		for (int j = 0;j < 8;j++) {
			for (int k = 0;k < myresult[j].size();k++) {
				result.push_back(myresult[j][k]);
			}
		}
		gettimeofday(&t2, NULL);
		timeuse += (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec;
		cout << result.size() << endl;
	}
	for (j = 0;j < n;j++) free(idx[j].arr);
	free(idx);

	cout << "time_use=" << timeuse << endl;

	return 0;
}
