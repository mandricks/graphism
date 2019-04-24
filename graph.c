/*
 * This is a basic graph implementation in C with extendable adjacency lists and
 * union find to help detect the relation between 2 vertices in the graph.
 */

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<stdbool.h>

#define ADJ_GROUPS_NR 8
#define ADJ_GROUP_SZ 16

#define ALLOC_PTR(ptr) (ptr = malloc(sizeof(ptr[0])))

struct vertice {
	int             v_id;
	int             v_data;
	int             v_parent;
	int             v_nr_connections;
	struct graph   *v_graph;
	struct edge    *v_edge;
};

struct edge {
	int          e_data;
	int          e_weight;
	bool         e_back;
	struct edge *e_next;
	struct edge *e_prev;
};

struct adjGroup {
	struct graph   *adj_graph;
	struct vertice *adj_list[ADJ_GROUP_SZ];
};

struct graph {
	int              g_nr_vertices;
	int              g_nr_adj_list_groups;
	bool             g_directed;
	struct adjGroup *g_adj_list_groups[ADJ_GROUPS_NR];
};


int _edge_add(struct vertice *v, int dst, int weight);
struct vertice *vertice_create(struct adjGroup *ag, int data);
void vertice_destroy(struct vertice *v);
struct vertice *vertice_find(struct adjGroup *ag, int vertex);
void set_parentof(struct adjGroup *ag, int dst, int src);
void unionOf(struct adjGroup *ag, int a, int b);

int graph_create(struct graph *g, int vertices, bool directed)
{
	int i;
	int j;
	int nr_adj_groups;
	struct adjGroup *ag;

	if (g == NULL)
		return -EINVAL;

	g->g_directed = directed;
	g->g_nr_vertices = vertices;
	g->g_nr_adj_list_groups = vertices % ADJ_GROUP_SZ > 0 ?
				  vertices / ADJ_GROUP_SZ + 1 :
				  vertices / ADJ_GROUP_SZ; 

	for (i = 0; i < g->g_nr_adj_list_groups; ++i) {
		ALLOC_PTR(g->g_adj_list_groups[i]);
		if (g->g_adj_list_groups[i] == NULL)
			return -ENOMEM;
		
	}

	for (i = 0; i < g->g_nr_adj_list_groups; ++i) {
		ag = g->g_adj_list_groups[i];
		for (j = 0; j < ADJ_GROUP_SZ; ++j) {
			ag->adj_list[i] = NULL;
		}
	}	

	return 0;
}

void graph_destroy(struct graph *g)
{
	int i = 0;
	int j = 0;
	struct vertice *v;
	struct adjGroup *ag;

	if (g == NULL)
		return;

	while (i < g->g_nr_adj_list_groups) {
		ag = g->g_adj_list_groups[i];
		if (ag == NULL) {
			++i;
			continue;
		}
		while (j < ADJ_GROUP_SZ) {
			v = ag->adj_list[j];
			if (v != NULL)
				vertice_destroy(v);
			++j;
		}
		g->g_adj_list_groups[i] = NULL;
		free(ag);
		++i;
	}
	g->g_nr_vertices = 0;
}

int edge_add(struct graph *g, int src, int dst, int weight)
{
	int              rc;
	struct vertice  *v;
	struct edge     *e;
	struct adjGroup *ag;

	if (g == NULL)
		return -EINVAL;

	ag = g->g_adj_list_groups[src / ADJ_GROUP_SZ];
	v = vertice_find(ag, src);
	if (v == NULL)
		return -ENOMEM;
	rc = _edge_add(v, dst, weight);
	if (rc == 0) {
		v = vertice_find(ag, dst);
		if (v == NULL)
			return -ENOMEM;
		unionOf(ag, src, dst);
	}

	return rc;
}

struct vertice *vertice_find(struct adjGroup *ag, int vertex)
{
	struct vertice *v;

	if (ag == NULL)
		return NULL;
	v = ag->adj_list[vertex];
	if (v == NULL) {
		v = vertice_create(ag, vertex);
		if (v == NULL) {
			printf("\nERROR, unable to create vertice, rc=%d", -ENOMEM);
			return NULL;
		}
		ag->adj_list[vertex] = v;
	}

	return v;
}

void unionOf(struct adjGroup *ag, int a, int b)
{
	int parent;
	int parenta;
	int parentb;
	int parentof;
	int max_connections = 0;

	if (ag->adj_list[a]->v_parent == a && ag->adj_list[b]->v_parent == b) {
		parent = a < b ? a : b;
		parentof = a < b ? b : a; 
		ag->adj_list[parentof]->v_parent = parent;
		++ag->adj_list[parent]->v_nr_connections;
	} else {
		if (ag->adj_list[a]->v_parent == ag->adj_list[b]->v_parent) {
			printf("\ncycle detected");
			return;
		}
		if (ag->adj_list[a]->v_parent == a)
			ag->adj_list[a]->v_parent = ag->adj_list[b]->v_parent;
		else
			ag->adj_list[b]->v_parent = ag->adj_list[a]->v_parent;
	}
}

int _edge_add(struct vertice *v, int dst, int weight)
{
	struct edge *p = NULL;
	struct edge *e;

	e = v->v_edge;
	while (e != NULL) {
		if (e->e_data == dst)
			return 0;
		p = e;
		e = e->e_next;
	}

	ALLOC_PTR(e);
	if (e == NULL) {
		printf("ERROR, edge creation fail, rc=%d", -ENOMEM);
		return -ENOMEM;
	}

	e->e_data = dst;
	e->e_next = NULL;
	e->e_prev = p;
	e->e_weight = weight;
	if (v->v_edge == NULL) {
		v->v_edge = e;
	} else {
		p->e_next = e;
	}

	return 0;
}

struct vertice *vertice_create(struct adjGroup *ag, int data)
{
	struct vertice *v;

	ALLOC_PTR(v);
	if (v == NULL)
		return NULL;
	v->v_id = data;
	v->v_data = data;
	v->v_graph = ag->adj_graph;
	v->v_edge = NULL;
	v->v_parent = data;
	v->v_nr_connections = 0;

	return v;
}

void vertice_destroy(struct vertice *v)
{
	struct edge *e;

	while (v->v_edge != NULL) {
		e = v->v_edge;
		v->v_edge = e->e_next;;
		free(e);
	}

	free(v);
}

void print_graph_bfs(struct graph *g)
{
	int i;
	int j;
	struct vertice  *v;
	struct edge     *e;
	struct adjGroup *ag;

	for (i = 0; i < g->g_nr_adj_list_groups; ++i) {	
		ag = g->g_adj_list_groups[i];
		printf("\n");
		for (j = 0; j < ADJ_GROUP_SZ; ++j) {
			v = ag->adj_list[j];
			if (v == NULL)
				continue;
			if (v->v_edge == NULL)
				printf("(%d, %d, %p %d) ", v->v_parent, v->v_data, v->v_edge, v->v_nr_connections);
			e = v->v_edge;
			while (e != NULL) {
				printf(" (%d, %d, %d %d) ", v->v_parent, v->v_data, e->e_data, v->v_nr_connections);
				e = e->e_next;
			}
		}
	}
}

bool is_related(struct graph *g, int a, int b)
{
	struct adjGroup *ag;

	ag = g->g_adj_list_groups[a / ADJ_GROUP_SZ];
	return ag->adj_list[a]->v_parent == ag->adj_list[b]->v_parent;
}

int main(void)
{
	struct graph g;
	int          rc;

	rc = graph_create(&g, 12, false);
	if (rc != 0) {
		graph_destroy(&g);
		printf("Error, cannot create graph with rc: %d", rc);
	}
/*
	edge_add(&g, 0, 1);
	edge_add(&g, 0, 2);
	edge_add(&g, 1, 2);
	edge_add(&g, 2, 0);
	edge_add(&g, 2, 3);
	edge_add(&g, 3, 3);
*/
/*
	edge_add(&g, 0, 1);
	edge_add(&g, 1, 2);
	edge_add(&g, 3, 2);
	edge_add(&g, 4, 5);
*/
/*
	edge_add(&g, 0, 1);
	edge_add(&g, 1, 2);
	edge_add(&g, 2, 3);
	edge_add(&g, 3, 2);
	edge_add(&g, 4, 5);
*/
	edge_add(&g, 0, 1, 4);
	edge_add(&g, 1, 2, 8);
	edge_add(&g, 2, 3, 7);
	edge_add(&g, 3, 4, 9);
	edge_add(&g, 3, 5, 14);
	edge_add(&g, 4, 5, 10);
	edge_add(&g, 5, 6, 2);
	edge_add(&g, 6, 7, 1);
	edge_add(&g, 7, 0, 8);
	edge_add(&g, 7, 8, 7);
	edge_add(&g, 8, 6, 6);
	edge_add(&g, 8, 2, 2);
	edge_add(&g, 2, 5, 4);
	edge_add(&g, 9, 10, 4);
	//edge_add(&g, 3, 0);
	//edge_add(&g, 3, 4);
	//edge_add(&g, 4, 5);

	if (is_related(&g, 9, 1))
		printf("\n Yes \n");
	else
		printf("\n No \n");

	print_graph_bfs(&g);

	graph_destroy(&g);

	return 0;
}
