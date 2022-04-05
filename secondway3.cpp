//接下来开始实现倒排索引求交技术
	//实现按表求交的位图存储方法
	int QueryNum = 100;//查询次数
	for (int i = 0;i < QueryNum;i++) {
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
		//第一层的位向量已经存储完毕，接下来建立索引
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

		for (int i = 1;i < TermNum;i++) {
			second[0] &= second[i];  //直接按位与
			for (int j = 0;j < 196992;j++) {
				if (second[0][j] == 1) {  //第j位是1，需要底层求交
					long addrtemp1 = (long)&lists[0];
					int* ptrtemp1 = (int *)(addrtemp1 + j * 16 );
					long addrtemp2 = (long)&lists[i];
					int* ptrtemp2 = (int *)(addrtemp2 + j * 16);
					int32x4_t temp1 = vld1q_s32(ptrtemp1);
					int32x4_t temp2 = vld1q_s32(ptrtemp2);
					temp1 = vandq_s32(temp1, temp2);
					vst1q_s32(ptrtemp1, temp1);
				}
				else {
					long addr = (long)&lists[0];
					bitset<128> * setptr = (bitset<128>*)(addr + 16 * j );
					*setptr = 0;//全部置零
				}
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