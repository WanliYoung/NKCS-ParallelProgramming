#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#include<vector>
#include<iostream>
#include<string>
#include<pthread.h>
#include<semaphore.h>
using namespace std;

pthread_mutex_t amutex = PTHREAD_MUTEX_INITIALIZER;
int QueryNum = 10;//查询次数，作为全局变量
int TermNum;
vector<unsigned int> result;
vector<vector <unsigned int>> results;
//信号量定义,7个线程
sem_t sem_main; //主函数的信号量
sem_t sem_workerstart[7]; // 每个线程有自己专属的信号量
sem_t sem_workerend[7];
struct package {
	int id;
	vector<unsigned int> myresult;
};

FILE *fi;
FILE *fp;
struct _INDEX {
	unsigned int  len;
	unsigned int *arr;
} *idx;

_INDEX* mylist[10];

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

void* threadFunc(void* parm) {
	int t_id = ((package*)parm)->id;
	for (int i = 0;i < QueryNum;i++) {
		sem_wait(&sem_workerstart[t_id]);     // 阻塞，等待主线完成求交操作（操作自己专属的信号量）
		int num = (*mylist[0]).len / 7;
		if (t_id == 6) {
			for (int j = num * t_id;j < (*mylist[0]).len;j++) {
				bool flag = true;
				for (int k = 1;k < TermNum;k++) {
					if (!find((*mylist[0]).arr[j], (*mylist[k]))) {  //在第k个关键词的列表当中没有找到
						flag = false;
						break;
					}
				}
				if (flag == true) {
					((package*)parm)->myresult.push_back((*mylist[0]).arr[j]);
				}
			}
		}
		else {
			for (int j = num*t_id;j < num*(t_id+1);j++) {
				bool flag = true;
				for (int k = 1;k < TermNum;k++) {
					if (!find((*mylist[0]).arr[j], (*mylist[k]))) {  //在第k个关键词的列表当中没有找到
						flag = false;
						break;
					}
				}
				if (flag == true) {
					((package*)parm)->myresult.push_back((*mylist[0]).arr[j]);
				}
			}
		}
		//获取到一个自己线程的myresult，要添加到result里,需要采用同步机制，可以考虑互斥量
		//添加进入result
		pthread_mutex_lock(&amutex);
		for (int x = 0;x < ((package*)parm)->myresult.size();x++) {
			result.push_back(((package*)parm)->myresult[x]);
		}
		pthread_mutex_unlock(&amutex);

		((package*)parm)->myresult.clear();//清空临时结果列表为下次做准备
		sem_post(&sem_main); //唤醒主线程
		sem_wait(&sem_workerend[t_id]); //阻塞，等待主线程唤醒进入下一轮
		
	}
	return NULL;
}

int main() {
	pthread_mutex_init(&amutex, NULL);
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
	//初始化信号量
	sem_init(&sem_main, 0, 0);
	for (int i = 0;i < 7;i++) {
		sem_init(&sem_workerstart[i], 0, 0);
		sem_init(&sem_workerend[i], 0, 0);
	}
	//创建线程
	pthread_t handles[7];// 创建对应的 Handle
	//创建参数包
	package packages[7];
	//threadParam_t param[NUM_THREADS];// 创建对应的线程数据结构
	for (int t_id = 0; t_id < 7; t_id++) {
		packages[t_id].id = t_id;
		pthread_create(&handles[t_id], NULL, threadFunc, (void*)&packages[t_id]);
	}
	for (int i = 0;i < QueryNum;i++) {
		TermNum = query_list[i].size();
		for (int j = 0;j < TermNum;j++) {
			mylist[j] = &idx[query_list[i][j]];
		}
	
		//开始唤醒工作线程
		for (int t_id = 0; t_id < 7; t_id++) {
			sem_post(&sem_workerstart[t_id]);
		}
		//主线程睡眠（等待所有的工作线程完成此轮消去任务）
		for (int t_id = 0; t_id < 7; t_id++) {
			sem_wait(&sem_main);
		}
		// 主线程再次唤醒工作线程进入下一轮次的消去任务
		for (int t_id = 0; t_id < 7; t_id++) {
			sem_post(&sem_workerend[t_id]);
		}
		cout << result.size() << endl;
		results.push_back(result);
		result.clear();
	}
	for (int t_id = 0; t_id < 7; t_id++) {
		pthread_join(handles[t_id], NULL);
	}	//销毁所有信号量
	sem_destroy(&sem_main);
	for (int i = 0;i < 7;i++) {
		sem_destroy(&sem_workerstart[i]);
		sem_destroy(&sem_workerend[i]);
	}
	for (j = 0;j < n;j++) free(idx[j].arr);
	free(idx);
	return 0;
}