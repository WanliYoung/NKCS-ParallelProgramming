#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include<vector>
#include<iostream>
#include<string>
#include<bitset>
#include<arm_neon.h>
#include<pthread.h>
#include<semaphore.h>
using namespace std;

pthread_mutex_t amutex = PTHREAD_MUTEX_INITIALIZER;
int QueryNum = 100;//查询次数，作为全局变量

bitset<25214976>* resultbits;

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

//线程函数
void* threadFunc(void* param) {
	int t_id = ((package*)param)->id;
	for (int k = 0; k < QueryNum; k++) {
		sem_wait(&sem_workerstart[t_id]);     // 阻塞，等待主线完成求交操作（操作自己专属的信号量）
		//循环划分任务
		long addr = (long)resultbits;
		long myaddr = addr + 450272 * t_id;//起始地址
		if (t_id == 6) {//第七个线程负责剩下3601920位
			for (int j = 0;j < 14070;j++) {
				bitset<256> * setptr = (bitset<256>*)(myaddr + 32 * j);
				if (*setptr == 0) {
					;
				}
				//如果不为0再去底层判断
				else {
					for (int k = 0;k < 256;k++) {
						if (((*resultbits)[t_id * 450272 + j * 256 + k]) == 1) {
							((package*)param)->myresult.push_back(t_id * 450272 + j * 256 + k);
						}
					}
				}
			}
		}
		else {  //其他线程负责14071*256位
			for (int j = 0;j < 14071;j++) {
				bitset<256> * setptr = (bitset<256>*)(myaddr + 32 * j);
				if (*setptr == 0) {
					;
				}
				//如果不为0再去底层判断
				else {
					for (int k = 0;k < 256;k++) {
						if (((*resultbits)[t_id*450272+j*256+k]) == 1) {
							((package*)param)->myresult.push_back(t_id * 450272 + j * 256 + k);
						}
					}
				}
			}
		}
		//获取到一个自己线程的myresult，要添加到result里,需要采用同步机制，可以考虑互斥量
		//添加进入result
		pthread_mutex_lock(&amutex);
		for (int x = 0;x < ((package*)param)->myresult.size();x++) {
			result.push_back(((package*)param)->myresult[x]);
		}
		pthread_mutex_unlock(&amutex);
		
		((package*)param)->myresult.clear();//清空临时结果列表为下次做准备
		sem_post(&sem_main); //唤醒主线程
		sem_wait(&sem_workerend[t_id]); //阻塞，等待主线程唤醒进入下一轮
	}
	pthread_exit(NULL);
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
	fp = fopen("/home/s2013774/Pthread/ExpQuery", "r");
	vector<vector<int> > query_list;

	vector<int> arr;
	char* line = new char[100];
	while ((fgets(line, 100, fp)) != NULL)
	{
		arr = strtoints(line);
		query_list.push_back(arr);
	}
	fclose(fp);

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

	//接下来开始实现倒排索引求交技术
	
	for (int i = 0;i < QueryNum;i++) {
		//开始处理每次查询
		//建立位向量和二级索引
		int TermNum = query_list[i].size();
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
		for (int i = 0;i < TermNum;i++) {
			long addrtemp1 = (long)&lists[i];
			for (int j = 0;j < 196992;j++) {
				bitset<128> * setptr = (bitset<128>*)(addrtemp1 + 16 * j);
				if (*setptr == 0) {
					;
				}
				else {
					second[i].set(j);
				}
			}
		}

		//开始按位与操作
		for (int i = 1;i < TermNum;i++) {
			second[0] &= second[i];  //二级索引层直接按位与，底层SIMD
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

		resultbits = &lists[0];

		//现在已经有结果位向量了，转换成结果列表，采用静态线程的Pthread编程
		//开始唤醒工作线程
		for (int t_id = 0; t_id < 7; t_id++) {
			sem_post(&sem_workerstart[t_id]);
		}
		//主线程睡眠（等待所有的工作线程完成此轮消去任务）
		for (int t_id = 0; t_id < 7; t_id++) {
			sem_wait(&sem_main);
		}
		// 主线程再次唤醒工作线程进入下一轮次的消去任务
		for(int t_id = 0; t_id < 7; t_id++) {
			sem_post(&sem_workerend[t_id]);
		}
		cout << result.size() << endl;
		results.push_back(result);
		result.clear();
	}

	for (int t_id = 0; t_id < 7; t_id++) {
		pthread_join(handles[t_id], NULL);
	}	//销毁所有信号量
	sem_destroy(&sem_main);
	for (int i = 0;i < 7;i++) {
		sem_destroy(&sem_workerstart[i]);
		sem_destroy(&sem_workerend[i]);
	}

	for (j = 0;j < n;j++) free(idx[j].arr);
	free(idx);

	return 0;
}
