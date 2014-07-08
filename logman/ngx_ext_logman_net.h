
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGINX_HTTP_MYLOG_NET_H_INCLUDED_
#define _NGINX_HTTP_MYLOG_NET_H_INCLUDED_

#include <unistd.h>   
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <setjmp.h>
#include <netdb.h>
#include	<fcntl.h>
#include	<sys/ioctl.h>
#include	<sys/un.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define NGX_EXT_LOGMAN_MAX_LEN 256

/*
* 将log内容推送到logman
*/
void ngx_ext_logman_net_report(ngx_http_request_t *r,ngx_array_t*ipport_arr,char *args);

/*环境初始化*/
void ngx_ext_logman_net_initenv();
#endif /* _NGINX_HTTP_MYLOG_NET_H_INCLUDED_ */
