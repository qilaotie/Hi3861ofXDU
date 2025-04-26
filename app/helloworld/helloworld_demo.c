#include <stdio.h>    // c语言的标准库文件 
#include "ohos_init.h" // 提供用于openharmony初始化和启动服务 
void HelloWorld(void) 
{ // 打印“helloworld!”字符串 
printf("helloworld! \r\n"); 
} 
// APP_FEATURE_INIT()是openharmony封装好的函数入口，实现main函数的功能。 
APP_FEATURE_INIT(HelloWorld);