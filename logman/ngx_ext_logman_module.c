#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_ext_logman_net.h"

static ngx_int_t ngx_ext_logman_init_process(ngx_cycle_t*);

static void*
ngx_ext_logman_create_loc_conf(ngx_conf_t *cf);

static char *
ngx_ext_logman(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_ext_logman_handler(ngx_http_request_t *r);

typedef struct
{
     ngx_array_t*  	mylog_ipport;
} ngx_ext_logman_conf_t;

static ngx_command_t  ngx_ext_logman_commands[] =
{

    {
        ngx_string("logman"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
        ngx_ext_logman,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("mylog_ipport"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
        ngx_conf_set_keyval_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_ext_logman_conf_t, mylog_ipport),
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t  ngx_ext_logman_module_ctx =
{
    NULL,                                   /* preconfiguration */
    NULL,                  		            /* postconfiguration */
    NULL,                              /* create main configuration */
    NULL,                              /* init main configuration */

    NULL,                              /* create server configuration */
    NULL,                              /* merge server configuration */

    ngx_ext_logman_create_loc_conf,       			/* create location configuration */
    NULL         			                        /* merge location configuration */
};

ngx_module_t  ngx_ext_logman_module =
{
    NGX_MODULE_V1,
    &ngx_ext_logman_module_ctx,           /* module context */
    ngx_ext_logman_commands,              /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    ngx_ext_logman_init_process,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};
static void ngx_ext_logman_sendto_logcenter(ngx_http_request_t*,ngx_array_t*ipport_arr,ngx_str_t *args);

static ngx_int_t ngx_ext_logman_init_process(ngx_cycle_t*cycle){
    ngx_log_error(NGX_LOG_NOTICE,cycle->log,0,"ngx_ext_logman_init_process ");
    ngx_ext_logman_net_initenv();
    return NGX_OK;
}

static void* ngx_ext_logman_create_loc_conf(ngx_conf_t *cf){
    ngx_ext_logman_conf_t  *mycf;

    mycf = (ngx_ext_logman_conf_t  *)ngx_pcalloc(cf->pool, sizeof(ngx_ext_logman_conf_t));
    if (mycf == NULL)
    {
        return NULL;
    }

    mycf->mylog_ipport = NULL;
    return mycf;
}

static char *
ngx_ext_logman(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    ngx_http_core_loc_conf_t  *clcf;

    //首先找到mytest配置项所属的配置块，clcf貌似是location块内的数据
    //结构，其实不然，它可以是main、srv或者loc级别配置项，也就是说在每个
    //http{}和server{}内也都有一个ngx_http_core_loc_conf_t结构体
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    //http框架在处理用户请求进行到NGX_HTTP_CONTENT_PHASE阶段时，如果
    //请求的主机域名、URI与mytest配置项所在的配置块相匹配，就将调用我们
    //实现的ngx_ext_logman_handler方法处理这个请求
    clcf->handler = ngx_ext_logman_handler;


    return NGX_CONF_OK;
}


static ngx_int_t ngx_ext_logman_handler(ngx_http_request_t *r){
    //必须是GET或者HEAD方法，否则返回405 Not Allowed
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }    
    
    //仅需记录日志
    ngx_ext_logman_sendto_logcenter(r,((ngx_ext_logman_conf_t*)(r->loc_conf[ngx_ext_logman_module.ctx_index]))->mylog_ipport,&(r->args));
    
    ngx_http_finalize_request(r, NGX_HTTP_NO_CONTENT);
    return NGX_OK;
}


static void ngx_ext_logman_sendto_logcenter(ngx_http_request_t *r,ngx_array_t*ipport_arr,ngx_str_t *args){
    if(args->len <=0 || args->len > NGX_EXT_LOGMAN_MAX_LEN){
        return;
    }
    char msg[NGX_EXT_LOGMAN_MAX_LEN];

    /*
    * log格式为1,2,3,4, 我们需要将,替换为logman约定的#
    */
    size_t index    =0;
    for(index=0;index <args->len;index++){
        char c = args->data[index];
        if(','==c){
            msg[index] = '#';
        }
        else{
            msg[index] = c;
        }
    };
    msg[index] = '\0';
    ngx_ext_logman_net_report(r,ipport_arr,msg);
    return;
}
