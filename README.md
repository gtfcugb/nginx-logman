nginx-logman
============

logman是一个日志中心，nginx http模块实现了logman的http客户端


nginx-1.7.2 测试可以使用。

使用方法：
在nginx/src/下创建ext目录，将logman目录copy至该目录下。
生成configure文件，./configure --with-debug --add-module=/root/soft/nginx-1.4.4/src/ext/logman/ 
make install

nginx配置文件：

location /stat {
            logman;
            mylog_ipport 192.168.3.155 8400;
            mylog_ipport 192.168.3.155 8402;
            mylog_ipport 192.168.3.155 8404;
        }
        
http访问：
http://192.168.3.188/stat?key,1,2,3

key 表示统计关键字
1，表示统计对象id
2，表示统计数量
3，表示统计目标id
