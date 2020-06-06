#include "../include/topologic.h"

int start_set(struct graph *graph, int *id, int num_vertices)
{
    if (!graph)
        return -1;
    if (!id)
        return -1;
    if (num_vertices < 0 || (graph->context == SINGLE && num_vertices > 1))
        return -1;

    int i = 0;
    for (; i < num_vertices; i++)
    {
        struct vertex *v = find(graph->vertices, id[i]);
        if (!v || push(graph->start, v) < 0)
        {
            /** Handle errors **/
            /**Given vertx failed, so at this point, free the vertices and leave**/
            while (pop(graph->start) != NULL) {}
            //destroy_graph(graph);
            return -1;
        }
    }
    return 0;
}

void run(struct graph *graph, void **init_vertex_args[])
{
    if (!graph->start)
    {
        //destroy_graph(graph);
        return;
    }
    int success = 0, v_index = 0;
    struct vertex *v = NULL;
    void *argv = malloc(FIRE_ARGV_SIZE);
    while ((v = (struct vertex *)pop(graph->start)))
    {
        if (!success)
            success = 1;
        /** TODO: Handle pthread options **/
        if (graph->context == NONE || graph->context == SWITCH)
        {
            //TODO Set up arguments in void* buffer
            int counter = 0;
            enum STATES color = RED;
            memset(argv, 0, FIRE_ARGV_SIZE);
            memcpy((argv + counter), graph, sizeof(struct graph));
            counter += sizeof(struct graph) + 1;
            memcpy((argv + counter), v, sizeof(struct vertex));
            counter += sizeof(struct vertex) + 1;
            memcpy((argv + counter), &(v->argc), sizeof(int));
            counter += sizeof(int) + 1;
            memcpy((argv + counter), init_vertex_args[v_index], sizeof(void *));
            counter += sizeof(void *) + 1;
            memcpy((argv + counter), &(color), sizeof(enum STATES));
            pthread_create(&graph->thread, NULL, fire_pthread, argv);
            ++v_index;
        }
        //fire(graph, v, v->argc, init_vertex_args[v_index], RED);
        //++v_index;
    }
    free(argv);
    graph->state_count = 1;

    if (!success)
    {
        /** TODO: HANDLE ERRORS **/
        /**This should be enough... right? **/
        fprintf(stderr, "Failure in run: success = %d\n", success);
        int i = 0;
        for (i = 0; i < v_index; i++)
        {
            int j = 0;
            for (; init_vertex_args[i][j] != NULL; j++)
            {
                free(init_vertex_args[i][j]);
            }
        }
        pthread_exit(NULL);

        destroy_graph(graph);
        return;
    }

    print(graph);

    pthread_cond_signal(&graph->red_cond);
    while (1)
    {
        switch (graph->state)
        {
        case RED:
            if (graph->red_vertex_count == 0)
            {
                /** TODO: REAP RED **/
                process_requests(graph);
                graph->state = PRINT;
                graph->previous_color = RED;
                pthread_cond_signal(&graph->print_cond);
                graph->print_flag = 1;
            }
            break;
        case BLACK:
            if (graph->black_vertex_count == 0)
            {
                /** TODO: REAP BLACK **/
                process_requests(graph);
                graph->state = PRINT;
                graph->previous_color = BLACK;
                pthread_cond_signal(&graph->print_cond);
                graph->print_flag = 1;
            }
            break;
        case PRINT:
            if (graph->print_flag == 0)
            {
                print(graph);
                graph->state_count++;
                if (graph->previous_color == RED)
                {
                    pthread_cond_signal(&graph->black_cond);
                }
                else
                {
                    pthread_cond_signal(&graph->red_cond);
                }
            }
            break;
        default:
            pthread_exit(NULL);
            return;
        }
    }
}

int fire(struct graph *graph, struct vertex *vertex, int argc, void *args, enum STATES color) {
    if (!graph || !vertex)
        return -1;
    enum STATES flip_color = BLACK;
    pthread_mutex_lock(&vertex->lock);
    vertex->is_active = 1;
    if (color == RED)
    {
        pthread_cond_wait(&graph->red_cond, &vertex->lock);

        pthread_mutex_lock(&graph->lock);
        graph->red_vertex_count++;
        pthread_mutex_unlock(&graph->lock);
    }
    else if (color == BLACK)
    {
        pthread_cond_wait(&graph->black_cond, &vertex->lock);
        flip_color = RED;
    }
    else
    {
        pthread_mutex_unlock(&vertex->lock);
        return -1;
    }
    if (argc != vertex->argc)
    {
        pthread_mutex_unlock(&vertex->lock);
        return -1;
    }

    int edge_argc = 0, vertex_argc = 0;
    void *edge_argv = NULL, *vertex_argv = NULL;
    struct vertex_result *v_res = (vertex->f)(argc, args);
    if (v_res) {
        edge_argc = v_res->edge_argc;
        edge_argv = v_res->edge_argv;
        vertex_argc = v_res->vertex_argc;
        vertex_argv = v_res->vertex_argv;
    }

    if (args) {
        free(args);
        args = NULL;
    }

    struct stack *edges = init_stack();
    preorder(vertex->edge_tree, edges);
    struct edge *edge = NULL;
    while ((edge = (struct edge *)pop(edges)) != NULL) {
        if (!edge->b)
        {
            void *data = malloc(sizeof(struct vertex) + sizeof(int) + 2);
            memset(data, 0, sizeof(struct vertex) + sizeof(int) + 2);
            memcpy(data, vertex, sizeof(struct vertex));
            memcpy(data + sizeof(struct vertex) + 1, &(edge->id), sizeof(int));
            struct request *req = create_request(DESTROY_EDGE, data, (void *)remove_edge_id, 2);
            submit_request(graph, req);
        }
        else if ((int)(edge->f)(edge_argc, edge_argv) >= 0)
        {
            if (switch_vertex(graph, edge->b, vertex_argc, vertex_argv, flip_color) < 0)
            {
                pthread_mutex_lock(&graph->lock);
                if (color == RED)
                    graph->red_vertex_count--;
                else
                    graph->black_vertex_count--;
                pthread_mutex_unlock(&graph->lock);

                pthread_mutex_unlock(&vertex->lock);
                return -1;
            }
            if (graph->context == SINGLE || graph->context == NONE)
                break;
        }
    }
    destroy_stack(edges);

    if (v_res->edge_argv)
    {
        free(v_res->edge_argv);
        v_res->edge_argv = NULL;
    }
    free(v_res);
    v_res = NULL;

    pthread_mutex_lock(&graph->lock);
    if (color == RED)
        graph->red_vertex_count--;
    else
        graph->black_vertex_count--;
    pthread_mutex_unlock(&graph->lock);

    vertex->is_active = 0;
    pthread_mutex_unlock(&vertex->lock);
    return 0;
}

void *fire_pthread(void *vargp)
{
    struct graph *graph = NULL;
    struct vertex *v = NULL;
    int argc = 0;
    void *args = NULL;
    enum STATES color = RED;

    /*Counter var*/
    int counter = 0;
    graph = (struct graph *) (vargp + counter);
    counter += sizeof(struct graph) + 1;
    v = (struct vertex *) (vargp + counter);
    counter += sizeof(struct vertex) + 1;
    argc = *((int *) (vargp + counter));
    counter += sizeof(int) + 1;
    v->argc = argc;
    args = (vargp + counter);
    counter += (sizeof(void) * argc) + 1;
    color = *((unsigned int *) (vargp + counter)) + 1;

    int ret_val = fire(graph, v, argc, args, color);
    pthread_exit((void *) (intptr_t) ret_val);
    return (void *) (intptr_t) ret_val;
}

int switch_vertex(struct graph *graph, struct vertex *vertex, int argc, void *args, enum STATES color)
{
    //HANDLE STUFF LIKE THREADS HERE
    void *argv = malloc(FIRE_ARGV_SIZE);
    if (!argv) return -1;
    int counter = 0;
    memset(argv, 0, FIRE_ARGV_SIZE);
    memcpy((argv + counter), graph, sizeof(struct graph));
    counter += sizeof(struct graph) + 1;
    memcpy((argv + counter), vertex, sizeof(struct vertex));
    counter += sizeof(struct vertex) + 1;
    memcpy((argv + counter), &(argc), sizeof(int));
    counter += sizeof(int) + 1;
    memcpy((argv + counter), args, sizeof(void *));
    counter += sizeof(void *) + 1;
    memcpy((argv + counter), &(color), sizeof(enum STATES));
    pthread_create(&graph->thread, NULL, fire_pthread, argv);
    free(argv);
    return 0;
}