#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#define VMRSS_LINE 21 // VMRSS所在行, 注:根据不同的系统,位置可能有所区别.
#define pid_t int
int get_memory_by_pid(pid_t p)
{
    FILE *fd;
    char name[32], line_buff[256] = {0}, file[64] = {0};
    int i, vmrss = 0;
    sprintf(file, "/proc/%d/status", p);
    // 以R读的方式打开文件再赋给指针fd
    fd = fopen(file, "r");
    if(fd==NULL){
        return -1;
    }
    // 读取VmRSS这一行的数据
    for (i = 0; i < 40; i++){
        if (fgets(line_buff, sizeof(line_buff), fd) == NULL){
            break;
        }
        if (strstr(line_buff, "VmPeak:") != NULL){
            sscanf(line_buff, "%s %d", name, &vmrss);
            break;
        }
    }
    fclose(fd);
    return vmrss;
}