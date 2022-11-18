#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define GET_BIT(X,Y) (X & ((unsigned long long)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF
#define UNMAPPED 0
#define PRESENT 1
#define SWAPPED 2
void stat(int pid);
int convert(int pid, unsigned long long v_address,unsigned long long * p_address);


void stat(int pid){
    long long present=0;
    long long swapped=0;
    //统计所有虚拟页的映射情况

    char pagemap_filename[4096] = { 0 };
    sprintf(pagemap_filename, "/proc/%d/pagemap", pid);
    FILE* fp_pagemap = fopen(pagemap_filename, "rb");
    if (!fp_pagemap) {
        printf("file %s can not be open \n", pagemap_filename);
        exit(0);
    }
    int each=1024; //每次读取多少页 这里进行批量读取/proc/pid/pagemap 如果每次只读取一条页信息，太慢了！
    //实测，当each=1每次读取一个页的时候，读够100M页需要4秒 而each=1024时，读够100M只需500ms
    unsigned long long cnt=0;
    unsigned long long *raw_datas=(unsigned long long * )malloc(sizeof(unsigned long long)*each);//每次读取1024个页信息，一个页信息8字节，共8k数据量
    unsigned long long raw_data=0;
	printf("start to read\n");
    unsigned long long start=clock();
    while(!feof(fp_pagemap)){
        int nums=fread((char*)raw_datas, sizeof(unsigned long long ), each, fp_pagemap);//尝试从当前偏移量处读取8k字节数据
        for(int i=0;i<nums;++i){
            memcpy(&raw_data,raw_datas+i,sizeof(unsigned long long));
            if (GET_BIT(raw_data, 63)) {
                present++;
            }
            else if (GET_BIT(raw_data, 62)) {
                swapped++;
            }
        }  
        if(feof(fp_pagemap)){
            //printf("读到文件尾 最后一次读了%d个页\n",num);
            break;
        }
        if( (cnt)%( 1024*1024*100) ==0 ){//读了100M次
            printf("have read %lld00M pages,time cost:%lldms\n",(cnt)/( 1024*1024*100), (clock()-start)/(CLOCKS_PER_SEC/1000));
        }
        cnt+=each;
    }
    printf("total_time:%lld ms\n",(clock()-start)/(CLOCKS_PER_SEC/1000));
    fclose(fp_pagemap);
    free(raw_datas);
    printf("Vitual Page usage:%lld Mapped Page:%lld Swapped Page:%lld\n",swapped+present,present,swapped);
    return ;
}
//定义了两种功能，1.转换地址:convert 进程id 虚拟地址 2.统计页信息: stat 进程id
//ps:经测试发现在一些高版本的linux上无法正常读取pagemap文件，我通过测试的版本为Ubuntu18.04
//之前我用Ubuntu20运行时出现了上述问题。
int main(int argc, char** argv) {
    long pid = -1;
    char* nextCh;
    unsigned long long v_address = 0;
    if (!((argc == 3 && strcmp(argv[1], "stat") == 0) || (argc == 4)&&(strcmp(argv[1], "convert") == 0))) {
        printf("Usage:convert [pid] [address] \nor\nUsage:stat [pid]\n");
        return 0;
    }
    //提取argv参数中的Pid
    pid = strtol(argv[2], &nextCh, 10);
    if (*nextCh != '\0' || pid <= 0) {
        printf("invalid pid %s\n", argv[2]);
        return -1;
    }
    if (argc == 3) {
        stat(pid);
    }
    else {
        //convert 虚拟地址转为物理地址
        unsigned long long  v_address = strtoll(argv[3], &nextCh, 16); //提取16进制的虚拟地址
        if (*nextCh != '\0') {
            printf("invalid address %s\n", argv[3]);
            return -1;
        }
        unsigned long long p_address=0;
        int ret=convert(pid, v_address,&p_address);
        if(ret==UNMAPPED){
            printf("page was not mapped\n");
        }
        else if(ret==PRESENT){
            printf("v_address=0x%llx\n", v_address);
            printf("p_address=0x%llx\n", p_address);
        }else if(ret==SWAPPED){
            printf("page was swapped\n");
        }
    }
    return 0;
}




//@return  -1: fail 0:not mapped 1:present 2:swapped
int convert(int pid, unsigned long long v_address,unsigned long long * p_address) {
    char pagemap_filename[4096] = { 0 };
    sprintf(pagemap_filename, "/proc/%d/pagemap", pid);
    FILE* fp_pagemap = fopen(pagemap_filename, "rb");
    if (!fp_pagemap) {
        printf("file %s can not be open \n", pagemap_filename);
        exit(0);
    }
    //找到该地址的虚拟页的映射信息在pagemap中的偏移量
    //pagemap中一条数据占8字节
    unsigned long long offset = (v_address >> 12) * 8;
    if (fseek(fp_pagemap, offset, SEEK_SET)) {
        printf("pagemap file fails to fseek %llx", offset);
        fclose(fp_pagemap);
        return -1;
    }
    unsigned long long raw_data = 0;
    fread((char*)&raw_data, sizeof(unsigned long long ), 1, fp_pagemap);//从当前偏移量处读取8字节数据
    if(feof(fp_pagemap)){
           printf("reach the end of file\n");
           return -1;
    }

    unsigned long long pfn = GET_PFN(raw_data);
    printf("8 bytes raw data is\n");
    for(int i=0;i<8;++i){
        printf("%02x ",(uint8_t) *(((char *)&raw_data)+i) );    
    }
    printf("\n");
    printf("PFN is %llx\n", pfn);


    int ret=-1;
    if (GET_BIT(raw_data, 63)) {
        *p_address=(pfn << 12) + (v_address & ((1 << 12) - 1));
        ret= PRESENT; 
    }
    else if (GET_BIT(raw_data, 62)) {
        ret= SWAPPED;
    }
    else if (pfn == 0) {
        ret=UNMAPPED;
    }else{
        ret=UNMAPPED;
    }

    fclose(fp_pagemap);
    return ret;
}