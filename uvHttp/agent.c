#include "public_def.h"
#include "private_def.h"




/** ������ӳ�ʱ�Ķ�ʱ�� */
static void timer_cb(uv_timer_t* handle) {

}

void string_map_compare(const void* cpv_first, const void* cpv_second, void* pv_output) {
    *(bool_t*)pv_output = string_less(*(const string_t**)cpv_first, *(const string_t**)cpv_second) /*? true : false*/;
    //const string_t* cpstr_first = *(const string_t**)cpv_first;
    //const string_t* cpstr_second = *(const string_t**)cpv_second;
    //bool_t ret = string_less(cpstr_first, cpstr_second);
    //*(bool_t*)pv_output = ret;
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
    agent_t* ret;
    map_iterator_t it_pos;
    uv_mutex_lock(&h->uv_mutex_h);
    it_pos = map_find(h->agents, addr);
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
        ret = new_agent;
    } else {
        pair_t* pt_pair = (pair_t*)iterator_get_pointer(it_pos);
        agent_t* agent = *(agent_t**)pair_second(pt_pair);
        string_destroy(addr);
        ret = agent;
    }
    uv_mutex_unlock(&h->uv_mutex_h);
    return ret;
}

/** ��agent�з���һ������ */
int agent_request(request_p_t* req) {
    agent_t* agent = NULL;
    bool_t can_run = false;
    socket_t* socket = NULL;
	//������Ҫ�������ȡһ��agent
	string_t* addr = create_string();   //��ַ��map�ͷ�ʱ�ͷ�
	string_init(addr);
	string_connect(addr, req->str_addr);
	string_connect_char(addr, ':');
	string_connect(addr, req->str_port);
	agent = get_agent(req->handle, addr);

	//�������߻�ȡһ���Ѿ��������ӣ�����������ﵽ���ֵ����Ҫ������ŵ�������
    uv_mutex_lock(&agent->uv_mutex_h);
    do {
    int sockets_num = set_size(agent->sockets);
    if (sockets_num >= agent->handle->conf.max_sockets){
        //������ŵ�������
		if (agent->requests == NULL) {
			agent->requests = create_list(void*);  //list<request_t*>
			list_init_elem(agent->requests, 1, req);
		} else {
			list_push_back(agent->requests, req);
		}
        break;
    } else if (agent->free_sockets == NULL || set_empty(agent->free_sockets)) {
        //�½�һ����������������
		socket = create_socket(agent);
		socket->req = req;
		set_insert(agent->sockets, socket);
        can_run = true;
    } else {
        //�ӿ���������ȡ��һ������������
		set_iterator_t it_socket = set_begin(agent->free_sockets);
		socket = (socket_t*)iterator_get_pointer(it_socket);
		set_erase(agent->free_sockets, it_socket);
		socket->req = req;
		set_insert(agent->sockets, socket);
        can_run = true;
    }
    } while (0);
    uv_mutex_lock(&agent->uv_mutex_h);

    if(can_run)
        socket_run(socket);
	return uv_http_ok;
}

void agent_request_finish(bool_t ok, socket_t* socket) {
    agent_t* agent = (agent_t*)socket->agent;
    bool_t can_run = false;
    socket_t* next_run;
    uv_mutex_lock(&agent->uv_mutex_h);
    destory_request(socket->req);
    if (ok && agent->requests != NULL && list_size(agent->requests) > 0) {
        //�������Ӧ����ɣ��Ҵ���δִ�е����󣬼���ִ������
        request_p_t* req = (request_p_t*)list_front(agent->requests);
        printf("finish a request, and run next. request num:%d,sockets num:%d,free_socket_num:%d"
            , list_size(agent->requests), set_size(agent->sockets), set_size(agent->free_sockets));
        list_pop_front(agent->requests);
        socket->req = req;
        can_run = true;
        next_run = socket;
    } else if(ok) {
        //�������Ӧ�����, ��δִ�е����󣬽�socket�ƶ������ж���
        int e_num = set_erase(agent->sockets, socket);
        int free_sockets_num = set_size(agent->free_sockets);
        printf("finish a request, and move to free list. sockets num:%d,free_socket_num:%d"
            , set_size(agent->sockets), set_size(agent->free_sockets));
        if (free_sockets_num >= agent->handle->conf.max_free_sockets){
            //���ж�������������socket
            destory_socket(socket);
        } else {
            set_insert(agent->free_sockets, socket);
        }
    } else {
        //����ִ��ʧ�ܣ�����socket
        int e_num = set_erase(agent->sockets, socket);
        destory_socket(socket);
        if(agent->requests != NULL && list_size(agent->requests) > 0) {
            //����δִ�е����󣬴�����socketִ������
            request_p_t* req = (request_p_t*)list_front(agent->requests);
            printf("failed a request, and run next. request num:%d,sockets num:%d,free_socket_num:%d"
                , list_size(agent->requests), set_size(agent->sockets), set_size(agent->free_sockets));
            list_pop_front(agent->requests);
            next_run = create_socket(agent);
            next_run->req = req;
            set_insert(agent->sockets, next_run);
            can_run = true;
        } else {
            printf("failed a request. request num:%d,sockets num:%d,free_socket_num:%d"
                , set_size(agent->sockets), set_size(agent->free_sockets));
        }
    }
    uv_mutex_unlock(&agent->uv_mutex_h);

    if(can_run)
        socket_run(socket);
}