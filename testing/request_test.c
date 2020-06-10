#include "../include/topologic.h"
#include "../include/test.h"

void init(struct graph**);
void setup_vertices(struct graph*);
void setup_edges(struct graph*);

void test_create_request(struct graph*);
void test_submit_request(struct graph*);
void test_process_requests(struct graph*);

void test_destroy_request(struct graph*);
void cleanup(struct graph*);


#define MAX_VERTICES 100
#define MAX_EDGES MAX_VERTICES
#define DEFAULT_BUFFER 32

int edgeFunction(void* args){

	int x = *(int*)(args);
	int y = *(int*)(args+4);
	return x*y;

}

struct vertex_result* vertexFunction(void* args){
	struct vertex_result* res = malloc(sizeof(struct vertex_result));
	if(res==NULL) return NULL;

	int counter = 0;

	int vertex_args = *(int*)(args+counter); counter+=sizeof(int);
	memcpy(res->vertex_argv, args+counter, sizeof(void*)*vertex_args); counter+=(sizeof(void*)*vertex_args);
	int edge_args = *(int*)(args+counter); counter+=sizeof(int);
	memcpy(res->edge_argv, (args+counter), sizeof(void*)*edge_args);
	return res;
	
	
}

int main(){

	//Setup graph
	struct graph* graph;
	init(&graph);
	assert(graph!=NULL);
	
	fprintf(stderr, "PREPARING TO TEST REQUEST MECHANISM\n");
	test_create_request(graph);
	test_submit_request(graph);
	test_process_requests(graph);
	test_destroy_request(graph);

	//cleanup(graph);
	destroy_graph(graph);
	graph=NULL;
	fprintf(stderr, "REQUEST TESTS PASSED\n");


	return 0;
}

void cleanup(struct graph* graph){
	assert(graph!=NULL);
	int i = 0;
	for(i =0; i< MAX_VERTICES; i++){
		struct vertex* v = (struct vertex*)find(graph->vertices, i);
		struct vertex* v2 = (struct vertex*)find(graph->vertices, ((i+1)>=MAX_VERTICES ? 0 : i+1)); 
		assert(v!=NULL);
		assert(v2!=NULL);
		struct edge* e = (struct edge *) find(v->edge_tree, v2->id);
		if(e->glbl) {free(e->glbl); e->glbl = NULL;}
		assert(remove_edge(graph,v, v2)==0);
	}
	destroy_graph(graph);
	graph=NULL;
}	

	

void init(struct graph** graph){
	*graph = GRAPH_INIT();
	assert(*graph!=NULL);
}

void setup_vertices(struct graph* graph){
	assert(graph!=NULL);
	int i = 0;
	for(i=0; i<MAX_VERTICES; i++){
		int id = i;
		struct vertex_result*(*f)(void*) = &vertexFunction;
		void* glbl = malloc(DEFAULT_BUFFER);
		assert(create_vertex(graph, f, id, glbl)!=NULL);
	}
}

void setup_edges(struct graph* graph){
	assert(graph!=NULL);
	int i = 0;
	int (*f)(void*) = &edgeFunction;
	for(i=0; i<MAX_EDGES; i++){
		void* glbl = malloc(DEFAULT_BUFFER);
		struct vertex* a = find(graph->vertices, (i));
		assert(a!=NULL);
		struct vertex* b;
		if(i+1==MAX_VERTICES){
				b = find(graph->vertices, (0));
		}else{
				b = find(graph->vertices, (i+1));
		}
		assert(b!=NULL);
		struct edge* edge;
		assert((edge=create_edge(graph,a, b, f, glbl))!=NULL);
	}
}


void test_create_request(struct graph* graph){
	assert(graph!=NULL);

	int i = 0;
	for(i=0; i<MAX_VERTICES; i++){
		int id = i;
		struct vertex_result*(*f)(void*) = &vertexFunction;
		void* glbl=malloc(DEFAULT_BUFFER);
		assert(glbl!=NULL);
		void* args = malloc(sizeof(struct graph)+sizeof(struct vertext_result*(void*))+sizeof(int)+DEFAULT_BUFFER);
		int counter = 0;
		memcpy(args+counter, graph, sizeof(struct graph)); counter+=sizeof(graph);
		memcpy(args+counter, f, sizeof(struct vertex_result*(void*))); counter+=sizeof(struct vertex_result*(void*));
		memcpy(args+counter, &id, sizeof(int)); counter+=sizeof(int);
		memcpy(args+counter, glbl, DEFAULT_BUFFER);
		assert(CREATE_REQUEST(CREAT_VERTEX, args)!=NULL);
	}
	fprintf(stderr, "REQUEST CREATION PASSED\n");

}

void test_submit_request(struct graph* graph){
	assert(graph!=NULL);
}

void test_process_requests(struct graph* graph){
	assert(graph!=NULL);
}

void test_destroy_request(struct graph* graph){
	assert(graph!=NULL);
}

