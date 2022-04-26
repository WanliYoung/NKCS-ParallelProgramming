#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<vector>
#include<iostream>
#include<string>
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
		if (e < list.arr[i]) {
			return false;
		}
		if (e == list.arr[i]) {
			return true;
		}
	}
	return false;
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

	//接下来开始实现按元素求交的倒排索引求交技术
	int QueryNum = 1000;//查询次数
	vector<vector<unsigned int>> results;
	for (int i = 0;i < QueryNum;i++) {
		vector<unsigned int> result;
		int TermNum = query_list[i].size();
		for (int j = 0;j < idx[query_list[i][0]].len;j++) {
			bool flag = true;
			for (int k = 1;k < TermNum;k++) {
				if (!find(idx[query_list[i][0]].arr[j], idx[query_list[i][k]])) {  //在第k个关键词的列表当中没有找到
					flag = false;
				}
			}
			if (flag == true) {
				result.push_back(idx[query_list[i][0]].arr[j]);
			}
		}
		cout << result.size() << endl;
		results.push_back(result);
	}

	for (j = 0;j < n;j++) free(idx[j].arr);
	free(idx);
	return 0;
}