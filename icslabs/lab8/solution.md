# Part A
## 1. 获取命令行参数
### 1.1 getopt()
- 声明：int main( int argc, char *argv[] );
- 参数数组：如果使用参数 -v bar www.ibm.com 运行一个名为 foo 程序：
    - argv[0] - foo
    - argv[1] - -v
    - argv[2] - bar
    - argv[3] - www.ibm.com
- getopt()函数：
  - 位置：unistd.h
  - 原型：int getopt( int argc, char *const argv[], const char *optstring ); (参数optstring 则代表欲处理的选项字符串。)
  - 返回值：如果找到符合的参数则返回此参数字母, 如果参数不包含在参数optstring 的选项字母则返回"?"字符,分析结束则返回-1.
### 1.2 使用getopt()函数：
#### 1.2.1 include头文件
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
```
#### 1.2.2 使用全局变量存储命令行选项：
```
static int v_flag = 0;
static int s_num = 0;
static int e_num = 0;
static int b_num = 0;
static char *t_name = 0;
```
#### 1.2.3 使用getopt()获取命令行参数：
```
while ((ch = getopt(argc, argv, "hvs:E:b:t:"))!= -1) {
    switch (ch)
        {
            case 'h':
            //help string here:
                print_help();   //自定义的输出help的函数
                break;
            case 'v':
                v_flag = 1;
                break;
            case 's':
                s_num = atoi(optarg);
                break;
            case 'E':
                e_num = atoi(optarg);
                break;
            case 'b':
                b_num = atoi(optarg);
                break;
            case 't':
                t_name = optarg;
                break;
            default:
                print_help();  //自定义的输出help的函数
                break;
        }
}
```

- 后面带有':'的字母表示字母后面参数必填，没有则表示字母后面没有参数（如果是'::'，表示参数可选）
- optarg则为getopt()读进来的参数，atoi()将参数转成int格式

### 1.3 检查参数获取的结果
详情查看代码。
## 2. 逻辑
参考知乎大佬的答案
[CSAPP Lab -- Cache Lab](https://zhuanlan.zhihu.com/p/36384449)

维护一个二维数组，然后照着书上说的流程走一遍。

# Part B 矩阵转置
## 1. 要求描述（详情见lab-instructions/lab8)
- 转置过程中Cache Miss尽可能地小
- 最多使用12个本地变量
- 不能使用recurssion
- 不能直接对A进行修改
- 不能使用arrays和malloc
- 只考查$32\times 32$，$64\times 64$，$61 \times 67$的情况。（推荐对每种情况给出特定方案）
- $32\times 32$: miss < 300;
- $64 \times 64$: miss < 1300;
- $61 \times 67$: miss < 2000;

## 2. 开始工作！
- 先安装valgrind 

- Cache部分不建议看知乎老哥的，推荐这个：[Cache lab 解题报告](http://zxcpyp.com/csapp/2017/11/20/Cache-Lab)
- 按知乎老哥61*67的方法，虽然展开了，其实miss会更多。