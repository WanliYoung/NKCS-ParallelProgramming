#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<vector>
#include<iostream>
#include<string>
#include<sys/time.h>
using namespace std;
vector<vector<unsigned int> > results;
int* resultSize;
FILE *fi;
FILE *fp;
struct _INDEX {
	unsigned int  len;
	unsigned int *arr;
} *idx;
struct QueryIndex {
	int size;
	int arr[6];
} *querylist;
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

int result[100000];
int temp[100000];

__global__ void search(QueryIndex * querylist, _INDEX * idx, int QueryNum,int * resultSize) {
    int index = threadIdx.x + blockIdx.x * blockDim.x;
    int stride = blockDim.x * gridDim.x;
    int result[100000];
    int temp[100000];
    for (int i = index; i < QueryNum; i+=stride) {
		resultSize[i] = idx[querylist[0].arr[0]].arr[0];
        int TermNum = querylist[i].size;    //query_list[i]表示第i次查询所包含的关键词的集合
		//先把第一个列表作为基准拷贝到临时数组
        int size = 0;
		for (int j = 0;j < idx[querylist[i].arr[0]].len;j++) {
            temp[j] = idx[querylist[i].arr[0]].arr[j];
            size++;
		}
        int tmpsize = 0;
		for (int j = 1;j < TermNum;j++) { //逐个表求交
			for (int k = 0;k < size;k++) {
                bool flag = false;
                int low = 0;
                int high = idx[querylist[i].arr[j]].len - 1;
                int mid = 0;
                while (low <= high) {
                    mid = (low + high) / 2;
                    if (temp[k] == idx[querylist[i].arr[j]].arr[mid]) {
                        flag = true;
                    }
                    else if (temp[k] < idx[querylist[i].arr[j]].arr[mid]) {
                        high = mid - 1;
                    }
                    else {
                        low = mid + 1;
                    }
                }
				if (!flag) {
					//没找到，不添加
				}
				else {
					result[tmpsize] = temp[k];
                    tmpsize++;
				}
			}
			//一次求交之后，result存储临时结果
            for(int k=0;k<tmpsize;k++){
                temp[k]=result[k];
            }
            size = tmpsize;
            tmpsize  = 0;
		}
		resultSize[i] = size;
    }
}

int main() {
struct timeval t1, t2;
double timeuse = 0;
    size_t sizeresult = 1000 * sizeof(int);
	cudaMallocManaged(&resultSize, sizeresult);
    for(int i=0;i<1000;i++){
        resultSize[i]=9999;
    }
	fi = fopen("ExpIndex", "rb");
	if (NULL == fi) {
		printf("Can not open file ExpIndex!\n");
		return 1;
	}
	//idx = (struct _INDEX *)malloc(MAXARRS * sizeof(struct _INDEX));
    size_t sizeidx = MAXARRS * sizeof(struct _INDEX);
	cudaMallocManaged(&idx, sizeidx);
	if (NULL == idx) {
		printf("Can not malloc %d bytes for idx!\n", MAXARRS * sizeof(struct _INDEX));
		return 2;
	}
	j = 0;
	while (1) {
		fread(&alen, sizeof(unsigned int), 1, fi);
		if (feof(fi)) break;
		//aarr = (unsigned int *)malloc(alen * sizeof(unsigned int));
		size_t sizeaarr = alen * sizeof(unsigned int);
        cudaMallocManaged(&aarr, sizeaarr);
		if (NULL == aarr) {
			printf("Can not malloc %d bytes for aarr!\n", alen * sizeof(unsigned short));
			return 3;
		}
		for (int i = 0;i < alen;i++) {
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
    size_t sizeqlist = 1005 * sizeof(struct QueryIndex);
	cudaMallocManaged(&querylist, sizeqlist);
	for (int i = 0;i < query_list.size();i++) {
		querylist[i].size = query_list[i].size();
		for (int j = 0;j < query_list[i].size();j++) {
			querylist[i].arr[j] = query_list[i][j];
		}
	}
	//实现按表求交的平凡算法
	int QueryNum = 100; //代表要处理的查询次数
    gettimeofday(&t1, NULL);
	int threadnum = 128;
    int blocknum = 10;
	search<<<blocknum,threadnum>>>(querylist, idx, QueryNum,resultSize); // compute interbody forces
    //test<<<blocknum,threadnum>>>(resultSize,QueryNum,idx,querylist);
	cudaDeviceSynchronize();
    //cout<<querylist[0].arr[0]<<endl;
    //cout<<"test:"<<(long long)idx[1116].arr<<endl;
    gettimeofday(&t2, NULL);
	timeuse += (t2.tv_sec-t1.tv_sec) * 1000000 + t2.tv_usec-t1.tv_usec;
	cout << "time_use=" << timeuse << endl;
    for(int i=0;i<QueryNum;i++){
        cout<<resultSize[i]<<endl;
    }
	for (j = 0;j < n;j++) free(idx[j].arr);
	cudaFree(idx);
    cudaFree(querylist);
    cudaFree(resultSize);
	cudaFree(aarr);
	return 0;
}