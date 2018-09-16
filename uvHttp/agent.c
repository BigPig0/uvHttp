#include "public_type.h"
#include "typedef.h"

extern socket_t* create_socket(agent_t* agent);
extern void socket_run(socket_t* socket);
extern void destory_request(request_p_t* req);


/** ������ӳ�ʱ�Ķ�ʱ�� */
static void timer_cb(uv_timer_t* handle) {

}

void string_map_compare(const void* cpv_first, const void* cpv_second, void* pv_output) {
    *(bool_t*)pv_output = string_less(*(const string_t**)cpv_first, *(const string_t**)cpv_second) ? true : false;
}

/** ģ���ʼ�� */
void agents_init(http_t* h) {
    int ret;
    h->agents = create_map(void*, void*); //map<string_t*, agent_t*>
    map_init_ex(h->agents, string_map_compare);
    ret = uv_timer_init(h->uv, &h->timeout_timer);
	h->timeout_timer.data = h;
    ret = uv_timer_start(&h->timeout_timer, timer_cb, 10000, 10000);
}

/** ģ������ */
void agents_destory(http_t* h) {
	uv_timer_stop(&h->timeout_timer);
    map_destroy(h->agents);
}

/** ����ip�Ͷ˿ڻ�ȡһ��agent */
agent_t* get_agent(http_t* h, string_t* addr) {
    map_iterator_t it_pos = map_find(h->agents, addr);
    if(iterator_equal(it_pos, map_end(h->agents))) {
        pair_t* pt_pair;
        agent_t* new_agent = (agent_t*)malloc(sizeof(agent_t));
        memset(new_agent, 0, sizeof(agent_t));
        new_agent->handle = h;
        new_agent->req_list = create_list(void*);   //list<request_t*>
        list_init(new_agent->req_list);
        new_agent->sockets = create_set(void*);     //set<socket_t*>
        set_init(new_agent->sockets);
        new_agent->free_sockets = create_set(void*); //set<socket_t*>
        set_init(new_agent->free_sockets);
        new_agent->keep_alive = true;
        pt_pair = create_pair(void*, void*); //pair<string_t*, agent_t*>
        pair_init_elem(pt_pair, addr, new_agent);
        map_insert(h->agents, pt_pair);
        pair_destroy(pt_pair);
        return new_agent;
    } else {
        pair_t* pt_pair = (pair_t*)iterator_get_pointer(it_pos);
        agent_t* agent = *(agent_t**)pair_second(pt_pair);
        return agent;
    }
}

/** ��agent�з���һ������ */
int agents_request(request_p_t* req) {
    agent_t* agent;
    int sockets_num;
	//������Ҫ�������ȡһ��agent
	string_t* addr = create_string();
	string_init(addr);
	string_connect(addr, req->str_addr);
	string_connect_char(addr, ':');
	string_connect(addr, req->str_port);
	agent = get_agent(req->handle, addr);
	string_destroy(addr);

	//�������߻�ȡһ���Ѿ��������ӣ�����������ﵽ���ֵ����Ҫ������ŵ�������
    sockets_num = set_size(agent->sockets);
    if (sockets_num >= agent->handle->conf.max_sockets){
        //������ŵ�������
		if (agent->requests == NULL) {
			agent->requests = create_list(void*);  //list<request_t*>
			list_init_elem(agent->requests, 1, req);
		} else {
			list_push_back(agent->requests, req);
		}
    } else if (agent->free_sockets == NULL || set_empty(agent->free_sockets)) {
        //�½�һ����������������
		socket_t* socket = create_socket(agent);
		socket->req = req;
		socket_run(socket);
		set_insert(agent->sockets, socket);
    } else {
        //�ӿ���������ȡ��һ������������
		set_iterator_t it_socket = set_begin(agent->free_sockets);
		socket_t* socket = (socket_t*)iterator_get_pointer(it_socket);
		set_erase(agent->free_sockets, it_socket);
		socket->req = req;
		socket_run(socket);
		set_insert(agent->sockets, socket);
    }
	return uv_http_ok;
}

/** ��socketʵ����socket�б����Ƶ�freesocket */
void agent_free_socket(socket_t* socket) {
    agent_t* agent;
    int e_num,free_sockets_num;
    destory_request(socket->req);
    agent = (agent_t*)socket->agent;
    e_num = set_erase(agent->sockets, socket);
    free_sockets_num = set_size(agent->free_sockets);
    if (free_sockets_num >= agent->handle->conf.max_free_sockets){
    } else {
        set_insert(agent->sockets, socket);
    }
}

void agent_destory_socket(socket_t* socket) {
    agent_t* agent;
	destory_request(socket->req);
	agent = (agent_t*)socket->agent;
}