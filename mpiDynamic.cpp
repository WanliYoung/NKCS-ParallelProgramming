#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include<vector>
#include<iostream>
#include<string>
#include<bitset>
#include<mpi.h>
#include<sys/time.h>
#include<algorithm>
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

vector<vector<int> > query_list;

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

int solution(int taskid) {
	int TermNum = query_list[taskid].size();
	bitset<25214976> * lists;//25214976=128*196992
	lists = new bitset<25214976>[TermNum];
	bitset<196992> *  second;
	second = new bitset<196992>[TermNum];
	for (int j = 0;j < TermNum;j++) {
		for (int k = 0;k < idx[query_list[i][j]].len;k++) {
			lists[j].set(idx[query_list[i][j]].arr[k]);//括号内是文档编号，把文档对应的二进制位置为1
		}
	}
	//第一层的位向量已经存储完毕，接下来建立索引,建立索引已经得到了优化
	for (int j = 0;j < TermNum;j++) {
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

	long addrtemp1 = (long)&lists[0];
	for (int j = 1;j < TermNum;j++) {
		second[0] &= second[j];  //二级索引层直接按位与，底层SIMD
		for (int k = 0;k < 196992;k++) {
			if (second[0][k] == 1) {  //第j位是1，需要底层求交
				bitset<128>* ptrtemp1 = (bitset<128>*)(addrtemp1 + k * 16);
				long addrtemp2 = (long)&lists[j];
				bitset<128>* ptrtemp2 = (bitset<128>*)(addrtemp2 + k * 16);
				*ptrtemp1 &= *ptrtemp2;
			}
			else {
				bitset<128> * setptr = (bitset<128>*)(addrtemp1 + 16 * k);
				*setptr = 0;//全部置零
			}
		}
	}
	vector<unsigned int> result;
	//结果转换优化，list[0]就是结果位向量，256为单位判断
	for (int j = 0;j < 98496;j++) {
		bitset<256> * setptr = (bitset<256>*)(addrtemp1 + 32 * j);
		if (*setptr == 0) {
			;
		}
		//如果不为0再去底层判断
		else {
			for (int k = 0;k < 256;k++) {
				if (lists[0][j * 256 + k] == 1) {
					result.push_back(j * 256 + k);
				}
			}
		}
	}
	return result.size();
}

int main(int argc, char* argv[]) {
	struct timeval t1, t2;
	double timeuse = 0;
	int myid, numprocs;
	int namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Get_processor_name(processor_name, &namelen);
	MPI_Status status;
	//打开文档读入数据集
	fi = fopen("/home/s2013774/MPI/ExpIndex", "rb");
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
	fp = fopen("/home/s2013774/MPI/ExpQuery", "r");
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
	if (myid == 0) { //主进程负责任务的分发
		int QueryNum = 1000;//查询次数
		gettimeofday(&t1, NULL);
		int i;
		int size = 1; //任务划分粒度
		int taskid = 0;
		int tmpResultSize;
		int recvid = 0;
		for (i = 1;i < numprocs;i++) { //为每个进程发送一个任务，只要告诉查询编号即可
			MPI_Send(&taskid, 1, MPI_INT, i, i*size, MPI_COMM_WORLD);
			taskid+=size;
		}
		while (recvid < QueryNum) {
			MPI_Recv(&tmpResultSize, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //接收结果
			cout << "result_size = " << tmpResultSize << endl;
			recvid+=size;
			if (taskid < QueryNum) { //还有任务就需要继续发送
				MPI_Send(&taskid, 1, MPI_INT, status.MPI_SOURCE, i*size, MPI_COMM_WORLD);
				i++;
				taskid+=size;
			}
			else { //向从进程发送信号，表示任务都发完了
				int temp = -1;
				MPI_Send(&temp, 1, MPI_INT, status.MPI_SOURCE, QueryNum + i, MPI_COMM_WORLD);
				i++;
			}
		}
		gettimeofday(&t2, NULL);
		cout << "done!" << endl;
		timeuse += (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec;
		cout << "time_use = " << timeuse <<  endl; //主进程记录时间即可
	}
	else { //从进程
		int taskid;
		int tmpResultSize;
		while (1) {
			MPI_Recv(&taskid, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if (taskid == -1) {//任务结束信号
				break; //结束
			}
			else { //进行处理
				tmpResultSize = solution(taskid);
				MPI_Send(&tmpResultSize, 1, MPI_INT, 0, status.MPI_TAG, MPI_COMM_WORLD);
			}
		}
	}
	for (j = 0;j < n;j++) free(idx[j].arr);
	free(idx);
	MPI_Finalize();
	return 0;
}

