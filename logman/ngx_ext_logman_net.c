#include "ngx_ext_logman_net.h"
/*最多log连接数*/

#define LOG_MAX_LINK_NUM 32
#define IP_LEN 32
#define NGX_EXT_LOGMAN_LINK_TRY_TIMES 3
#define CONNECT_TIMEOUT 2

struct ngx_ext_logman_link
{
    char ip[IP_LEN];
    int port;
    int sock; 
};
typedef struct ngx_ext_logman_link ngx_ext_logman_link_t;

static ngx_array_t * s_log_link             = NULL;

static 
int connect_nonb(int sockfd, struct sockaddr*saptr, socklen_t salen, int nsec) {
	int flags, n, error;
	fd_set rset, wset;
	socklen_t len;
	struct timeval tval;
	struct timeval *t_ ;
	flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	error = 0;
	if ((n = connect(sockfd, saptr, salen)) < 0){
		if (errno != EINPROGRESS){
			goto CONNECT_UNDONE;
		}		
	}
	/* Do whatever we want while the connect is taking place. */
	if (n == 0){	/* connect completed immediately */
		goto CONNECT_DONE;
	}
	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;
	t_ =  (nsec > 0) ? &tval : NULL;
	tval.tv_usec = 0;
	if ((n = select(sockfd + 1, &rset, &wset, NULL,t_)) == 0) { /* timeout */
		close(sockfd);
		errno = ETIMEDOUT;
		goto CONNECT_UNDONE;
	}
	if(FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)){
		len = sizeof(error);
		if(getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len) < 0){
			goto CONNECT_UNDONE;
		}
		if(error ){
			close(sockfd);
			goto CONNECT_UNDONE;
		}
	}
	else{
		goto CONNECT_UNDONE;	
	}	
CONNECT_DONE: 
	fcntl(sockfd, F_SETFL, flags); /* restore file status flags */
	return 0;
CONNECT_UNDONE:
	fcntl(sockfd, F_SETFL, flags);
	return -1;
}

static ngx_ext_logman_link_t* ngx_ext_logman_net_init(ngx_str_t* ip,ngx_str_t*port){
    ngx_ext_logman_link_t*link = ngx_array_push(s_log_link);
    if (link == NULL){
        return NULL;
    }
    memset(link->ip,0,IP_LEN);

    int conn_fd;
	strncpy(link->ip,(char*)ip->data,ip->len);
	link->port                              =ngx_atoi(port->data,port->len);
    
	struct sockaddr_in serv_addr_in;
	
    /*必须清0*/
    memset(&serv_addr_in, 0, sizeof(struct sockaddr_in));
    serv_addr_in.sin_family = AF_INET;
    
    /*字节转换*/
    serv_addr_in.sin_port = htons(link->port);
    inet_aton(link->ip, &serv_addr_in.sin_addr);
    
    conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn_fd > 0) {
        link->sock = conn_fd;
    }
    
    if (connect_nonb(conn_fd, (struct sockaddr *) &serv_addr_in,	sizeof(serv_addr_in),CONNECT_TIMEOUT) < 0) {
       close(conn_fd);
       link->sock = 0;
    }

    return link;
}

static ngx_uint_t s_cur_link_index = 0;
static ngx_ext_logman_link_t* ngx_ext_logman_net_getlink(ngx_http_request_t *r,ngx_array_t*ipport_arr){
    /*检测是否已经初始化过logman的连接信息*/
    if(s_log_link ==NULL){
        s_log_link = ngx_array_create(ngx_cycle->pool, LOG_MAX_LINK_NUM,
                                          sizeof(ngx_ext_logman_link_t));
        ngx_uint_t i = 0;
        for(;i < ipport_arr->nelts;i++){
            ngx_keyval_t* pkv               = (ngx_keyval_t* )(ipport_arr->elts)+i;
            ngx_ext_logman_net_init(&pkv->key,&pkv->value);
            ngx_log_error(NGX_LOG_NOTICE,r->connection->log,0,"logman connect param > ip %V port %V \n",&pkv->key,&pkv->value);
        }
    }
    ngx_ext_logman_link_t*link = NULL;
    ngx_uint_t loop =0;
    while(loop++ <= NGX_EXT_LOGMAN_LINK_TRY_TIMES){
        ++s_cur_link_index;
        if(s_cur_link_index >= s_log_link->nelts){
            s_cur_link_index = 0;
        }
        link = (ngx_ext_logman_link_t*)(s_log_link->elts)+s_cur_link_index;
        if(link && link->sock > 0){
            break;
        }
        else{
            link = NULL;
        }
    }
    return link;
}

void ngx_ext_logman_net_report(ngx_http_request_t *r,ngx_array_t*ipport_arr,char* msg){
    ngx_ext_logman_link_t *link = ngx_ext_logman_net_getlink(r,ipport_arr);
    if(link == NULL){
        return;
    }
    
    char buffer[1024];
	snprintf(buffer,1024,"{\"mark\":\"log\",\"type\":\"log\",\"info\":{\"source\":\"%s\",\"level\":\"STAT\",\"type\":\"%s\",\"content\":\"%s\",\"expand\":\"%s\"}}","nginx","s",msg,"");
    
    /*这里不做错误检测*/
    send(link->sock,buffer,strlen(buffer)+1,0);
    return;
}

void ngx_ext_logman_net_initenv(){
    printf("ngx_ext_logman_net_initenv \n");
    /*这里存在socket泄露*/
    s_log_link = NULL;
}
