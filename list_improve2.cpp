#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
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

bool find(unsigned int e, _INDEX list) { //进行二分查找
	int low = 0;
	int high = list.len - 1;
	int mid = 0;
	while (low <= high) {
		mid = (low + high) / 2;
		if (e == list.arr[mid]) {
			return true;
		}
		else if (e < list.arr[mid]) {
			high = mid - 1;
		}
		else {
			low = mid + 1;
		}
	}
	return false;
}

int main() {
	fi = fopen("ExpIndex", "rb");
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

	fp = fopen("ExpQuery", "r");
	vector<vector<int> > query_list;

	vector<int> arr;
	char* line = new char[100];
	while ((fgets(line, 100, fp)) != NULL)
	{
		arr = strtoints(line);
		query_list.push_back(arr);
	}
	fclose(fp);

	//实现按表求交的平凡算法
	vector<vector<unsigned int> > results;
	int QueryNum = 100; //代表要处理的查询次数
	for (int i = 0;i < QueryNum;i++) {  //逐个处理查询请求
		int TermNum = query_list[i].size();    //query_list[i]表示第i次查询所包含的关键词的集合
		vector<unsigned int> result;  //结果数组
		vector<unsigned int> temp;  //临时数组
		//先把第一个列表作为基准拷贝到临时数组
		for (int j = 0;j < idx[query_list[i][0]].len;j++) {
			temp.push_back(idx[query_list[i][0]].arr[j]);
		}
		for (int j = 1;j < TermNum;j++) { //逐个表求交
			for (int k = 0;k < temp.size();k++) {
				if (!find(temp[k], idx[query_list[i][j]])) {
					//没找到，不添加
				}
				else {
					result.push_back(temp[k]);
				}
			}
			//一次求交之后，result存储临时结果
			temp = result;
			result.clear();
		}
		result = temp;
		cout << result.size() << endl;
		results.push_back(result);
	}
	for (j = 0;j < n;j++) free(idx[j].arr);
	free(idx);
	return 0;
}