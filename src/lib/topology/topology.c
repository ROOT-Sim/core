/**
* @file lib/topology/topology.c
*
* @brief Topology library
*
* A library implementing commonly-required topologies in simulation models.
*
* SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <assert.h>
#include <stdlib.h>

#include <core/core.h>
#include <datatypes/list.h>

#include <ROOT-Sim.h>

/// A node in the topology adjacency matrix
struct graph_node {
	lp_id_t	neighbor;        /**< The ID of the neighbor */
	double probability;     /**< The probability to traverse this edge */
	struct graph_node *next; /**< Next node in the adjacency list */
	struct graph_node *prev; /**< Next node in the adjacency list */
};

/// The structure describing a topology
struct topology {
	lp_id_t regions;                          /**< the number of LPs involved in the topology */
	uint32_t width;                           /**< the width of the grid */
	uint32_t height;                          /**< the height of the grid */
	enum topology_geometry geometry;          /**< the topology geometry */
	list(struct graph_node) *adjacency; /**< Adjacency list for the graph topology */
};

/// Allowed directions to reach a neighbor in a TOPOLOGY_HEXAGON
static enum topology_direction directions_hexagon[] =
		{DIRECTION_E, DIRECTION_W, DIRECTION_NE, DIRECTION_NW, DIRECTION_SE, DIRECTION_SW};
/// Allowed directions to reach a neighbor in either a TOPOLOGY_SQUARE or a TOPOLOGY_TORUS
static enum topology_direction directions_square_torus[] = {DIRECTION_E, DIRECTION_W, DIRECTION_N, DIRECTION_S};

/**
 * @brief Return a random neighbor
 *
 * This function computes a random receiver, only for TOPOLOGY_HEXAGON, TOPOLOGY_SQUARE, and
 * TOPOLOGY_TORUS.
 * The algorithm simply generates a random permutation over the list of valid directions passed
 * by the caller, and then attempts to get a receiver in every direction. The first
 * direction that is not INVALID_DIRECTION dictates the picked random neighbor.
 *
 * @param from source element of the random receiver computation
 * @param topology the topology currently being considered
 * @param n_directions the number of valid directions for the given topology
 * @param directions the number of directions (a variable array)
 *
 * @return A random neighbor according to the specified topology
 */
static lp_id_t get_random_neighbor(lp_id_t from, struct topology *topology, size_t n_directions, unsigned directions[n_directions])
{
	lp_id_t ret = INVALID_DIRECTION;

	assert(topology->geometry != TOPOLOGY_RING);
	assert(topology->geometry != TOPOLOGY_BIDRING);
	assert(topology->geometry != TOPOLOGY_STAR);
	assert(topology->geometry != TOPOLOGY_FCMESH);
	assert(topology->geometry != TOPOLOGY_GRAPH);

	if (n_directions > 1) {
		for (size_t i = 0; i < n_directions - 1; i++) {
			size_t j = RandomRange((int)i, (int)n_directions - 1);
			unsigned t = directions[j];
			directions[j] = directions[i];
			directions[i] = t;
		}
	}

	for(size_t i = 0; i < n_directions; i++) {
		ret = GetReceiver(from, topology, directions[i]);
		if(ret != INVALID_DIRECTION)
			break;
	}

	assert(ret != INVALID_DIRECTION);
	return ret;
}


/**
 * @brief Given a linear id in a TOPOLOGY_HEXAGON map, get the linear id of a neighbor in a given direction (if any).
 *
 * We use only one possible representation of a hexagonal map, namely a "pointy" "odd-r" horizontal layout,
 * with an arbitrary width and height. In this representation, odd rows are showed right:
 *
 *   / \ / \ / \ / \
 *  |0,0|1,0|2,0|3,0|
 *   \ / \ / \ / \ / \
 *    |0,1|1,1|2,1|3,1|
 *   / \ / \ / \ / \ /
 *  |0,2|1,2|2,2|3,2|
 *   \ / \ / \ / \ /
 *
 *  This is a map of size 3x4, in which 12 different cells are represented. In each hexagon, the (x,y)
 *  coordinates are represented. Nevertheless, to support simulation, we typically linearize the cells of
 *  the map. Indeed, the cells are linearly mapped as:
 *  0 → (0,0); 1 → (1,0); 2 → (2,0); 3 → (3,0); 4 → (0,1); 5 → (1,1); 6 → (2,1); 7 → (3,1); etc.
 *
 *  The general mapping from linear (L) to hex (x,y) is therefore implemented as (in integer arithmetic):
 *  y = L / width
 *  x = L % width
 *
 *  while the mapping from (x,y) to L is implemented as:
 *
 *  L = y * width + x
 *
 *  When checking for a neighbor, we have to consider the row in which we are starting from. Indeed, considering
 *  that it's an "odd-r" map, if we are in an odd row and we move "up left" or "down left", we retain the same x
 *  value. Conversely, if we move from an odd row "up right" or "down right", we have to increment x. Moving from
 *  an even row is the other way round (retain x if you move up/down right, decrement if you move up/down left).
 *  In this implementation, we use bitwise and with 1 to check if we are in an odd row.
 *  Moving up/down always decrements/increments y. Moving right/left only increments/decrements x.
 *
 *  At the end, to check if a move is valid, we compare new coordinates (x,y) with the size of the grid.
 *
 *
 * @param from      The linear representation of the source element
 * @param topology  The structure keeping the information about the topology
 * @param direction The direction to move towards, to find a linear id
 * @return The linear id of the neighbor, INVALID_DIRECTION if such neighbor does not exist in the topology.
 */
static lp_id_t get_neighbor_hexagon(lp_id_t from, struct topology *topology, enum topology_direction direction)
{
	uint32_t x, y;

	assert(topology->geometry == TOPOLOGY_HEXAGON);

	y = from / topology->width;
	x = from - y * topology->width;

	switch (direction) {
		case DIRECTION_NW:
			x += (y & 1U) - 1;
			y -= 1;
			break;
		case DIRECTION_NE:
			x += (y & 1U);
			y -= 1;
			break;
		case DIRECTION_SW:
			x += (y & 1U) - 1;
			y += 1;
			break;
		case DIRECTION_SE:
			x += (y & 1U);
			y += 1;
			break;
		case DIRECTION_E:
			x += 1;
			break;
		case DIRECTION_W:
			x -= 1;
			break;
		case DIRECTION_RANDOM:
			return get_random_neighbor(from, topology, sizeof(directions_hexagon) / sizeof(enum topology_direction),
						   directions_hexagon);
		default:
			return INVALID_DIRECTION;
	}
	return (x < topology->width && y < topology->height) ? y * topology->width + x : INVALID_DIRECTION;
}


/**
 * @brief Given a linear id in a TOPOLOGY_SQUARE map, get the linear id of a neighbor in a given direction (if any).
 *
 *  The general mapping from linear (L) to hex (x,y) is therefore implemented as (in integer arithmetic):
 *  y = L / width
 *  x = L % width
 *
 *  while the mapping from (x,y) to L is implemented as:
 *  L = y * width + x
 *
 *  Getting a neighbor simply entails incrementing/decrementing x or y, depending on the direction, and
 *  then checking if such an element exists in the current map.
 *
 * @param from      The linear representation of the source element
 * @param topology  The structure keeping the information about the topology
 * @param direction The direction to move towards, to find a linear id
 * @return The linear id of the neighbor, INVALID_DIRECTION if such neighbor does not exist in the topology.
 */
static lp_id_t get_neighbor_square(lp_id_t from, struct topology *topology, enum topology_direction direction)
{
	unsigned x, y;

	assert(topology->geometry == TOPOLOGY_SQUARE);

	y = from / topology->width;
	x = from - y * topology->width;

	switch (direction) {
		case DIRECTION_N:
			y -= 1;
			break;
		case DIRECTION_S:
			y += 1;
			break;
		case DIRECTION_E:
			x += 1;
			break;
		case DIRECTION_W:
			x -= 1;
			break;
		case DIRECTION_RANDOM:
			return get_random_neighbor(from, topology, sizeof(directions_square_torus) / sizeof(enum topology_direction),
						   directions_square_torus);
		default:
			return INVALID_DIRECTION;
	}
	return (x < topology->width && y < topology->height) ? y * topology->width + x : INVALID_DIRECTION;
}


/**
 * @brief Given a linear id in a TOPOLOGY_TORUS map, get the linear id of a neighbor in a given direction (if any).
 *
 *  The general mapping from linear (L) to hex (x,y) is therefore implemented as (in integer arithmetic):
 *  y = L / width
 *  x = L % width
 *
 *  while the mapping from (x,y) to L is implemented as:
 *  L = y * width + x
 *
 *  Getting a neighbor simply entails incrementing/decrementing x or y, depending on the direction, and
 *  computing the modulus on the map size.
 *
 * @param from      The linear representation of the source element
 * @param topology  The structure keeping the information about the topology
 * @param direction The direction to move towards, to find a linear id
 * @return The linear id of the neighbor, which always exists.
 */
static lp_id_t get_neighbor_torus(lp_id_t from, struct topology *topology, enum topology_direction direction)
{
	uint32_t x, y;

	assert(topology->geometry == TOPOLOGY_TORUS);

	y = from / topology->width;
	x = from - y * topology->width;

	switch (direction) {
		case DIRECTION_N:
			y += topology->height - 1;
			y %= topology->height;
			break;
		case DIRECTION_S:
			y += 1;
			y %= topology->height;
			break;
		case DIRECTION_E:
			x += 1;
			x %= topology->width;
			break;
		case DIRECTION_W:
			x += topology->width - 1;
			x %= topology->width;
			break;
		case DIRECTION_RANDOM:
			return get_random_neighbor(from, topology, sizeof(directions_square_torus) / sizeof(enum topology_direction),
						   directions_square_torus);

		default:
			return INVALID_DIRECTION;
	}
	return y * topology->width + x;
}


/**
 * @brief Given an id in a TOPOLOGY_FCMESH map, get the id of a neighbor.
 *
 *  The algorithm is simple: return a random element in the topology, different from from.
 *  The only corner case is if we have only one element, in which case we return INVALID_DIRECTION.
 *
 * @param from      The linear representation of the source element
 * @param topology  The structure keeping the information about the topology
 * @param direction Can be only set to DIRECTION_RANDOM
 * @return The linear id of the neighbor, INVALID_DIRECTION if such neighbor does not exist in the topology.
 */
static lp_id_t get_neighbor_mesh(lp_id_t from, struct topology *topology, enum topology_direction direction)
{
	lp_id_t ret;

	assert(topology->geometry == TOPOLOGY_FCMESH);

	if(unlikely(direction != DIRECTION_RANDOM)) {
		fprintf(stderr, "[ERROR] Asking for a non-random direction in a graph.\n");
		return INVALID_DIRECTION;
	}

	// Corner case
	if(topology->regions == 1)
		return INVALID_DIRECTION;

	do {
		ret = ((double)topology->regions * Random());
	} while(ret == from);

	return ret;
}


static lp_id_t get_neighbor_bidring(lp_id_t from, struct topology *topology, enum topology_direction direction)
{
	assert(topology->geometry == TOPOLOGY_BIDRING);

	if(direction == DIRECTION_RANDOM) {
		if(Random() < 0.5)
			direction = DIRECTION_E;
		else
			direction = DIRECTION_W;
	}

	if(direction == DIRECTION_E)
		return (from + 1) % topology->regions;
	if(direction == DIRECTION_W)
		return (from + topology->regions - 1) % topology->regions;
	return INVALID_DIRECTION;
}


static lp_id_t get_neighbor_ring(lp_id_t from, struct topology *topology, enum topology_direction direction)
{
	assert(topology->geometry == TOPOLOGY_RING);

	if(direction == DIRECTION_E || direction == DIRECTION_RANDOM)
		return (from + 1) % topology->regions;
	return INVALID_DIRECTION;
}


static lp_id_t get_neighbor_star(lp_id_t from, struct topology *topology, enum topology_direction direction)
{
	assert(topology->geometry == TOPOLOGY_STAR);

	if(unlikely(direction != DIRECTION_RANDOM)) {
		fprintf(stderr, "[ERROR] Asking for a non-random direction in a star.\n");
		return INVALID_DIRECTION;
	}

	if(from == 0)
		return RandomRange(1, (int)(topology->regions - 1));
	return 0;
}


static lp_id_t get_neighbor_graph(lp_id_t from, struct topology *topology, enum topology_direction direction)
{
	double rand, cumulative = 0.0;
	struct graph_node *adj_node;
	unsigned adj_neighbor;

	assert(topology->geometry == TOPOLOGY_GRAPH);
	assert(topology->adjacency != NULL);
	assert(from < topology->regions);

	if(topology->geometry == TOPOLOGY_GRAPH && direction != DIRECTION_RANDOM) {
		fprintf(stderr, "[ERROR] Asking for a non-random direction in a graph.\n");
		return INVALID_DIRECTION;
	}

	if(list_size(topology->adjacency[from]) == 0)
		return INVALID_DIRECTION;

	rand = Random();
	adj_node = list_head(topology->adjacency[from]);
	do {
		adj_neighbor = adj_node->neighbor;
		cumulative += adj_node->probability;
		adj_node = list_next(adj_node);
	} while(rand < cumulative && adj_node != NULL);

	return adj_neighbor;
}


lp_id_t CountRegions(struct topology *topology)
{
	return topology->regions;
}


lp_id_t CountDirections(lp_id_t from, struct topology *topology)
{
	lp_id_t neighbors;
	uint32_t x, y;

	assert(topology);

	switch (topology->geometry) {
		case TOPOLOGY_FCMESH:
			assert(topology->geometry == TOPOLOGY_FCMESH);
			return topology->regions - 1;

		case TOPOLOGY_HEXAGON:
			assert(topology->geometry == TOPOLOGY_HEXAGON);
			neighbors = 6;
			y = from / topology->width;
			x = from - y * topology->width;
			if(y == 0 || y == topology->height - 1)
				neighbors -= x == 0 ? 1 : 2;
			if(x == 0)
				neighbors -= 3 - 2 * (y & 1U);
			if(x == topology->width - 1)
				neighbors -= 3 - 2 * (1 - (y & 1U));
			return neighbors;

		case TOPOLOGY_TORUS:
			assert(topology->geometry == TOPOLOGY_TORUS);
			return 4;

		case TOPOLOGY_SQUARE:
			assert(topology->geometry == TOPOLOGY_SQUARE);
			neighbors = 4;
			y = from / topology->width;
			x = from - y * topology->width;
			if(x == 0 || x == topology->width - 1)
				neighbors--;
			if(y == 0 || y == topology->height - 1)
				neighbors--;
			return neighbors;

		case TOPOLOGY_BIDRING:
			assert(topology->geometry == TOPOLOGY_BIDRING);
			return 2;

		case TOPOLOGY_STAR:
			assert(topology->geometry == TOPOLOGY_STAR);
			if(from == 0)
				return topology->regions - 1;
			return 1;

		case TOPOLOGY_RING:
			assert(topology->geometry == TOPOLOGY_RING);
			return 1;

		case TOPOLOGY_GRAPH:
			assert(topology->geometry == TOPOLOGY_GRAPH);
			assert(topology->adjacency != NULL);
			assert(from < topology->regions);
			return list_size(topology->adjacency[from]);
	}
	return UINT_MAX;
}


bool IsNeighbor(lp_id_t from, lp_id_t to, struct topology *topology)
{
	struct graph_node *adj_node;

	switch(topology->geometry) {
		case TOPOLOGY_HEXAGON:
			assert(topology->geometry == TOPOLOGY_HEXAGON);
			for(unsigned i = 0; i < DIRECTION_RANDOM; i++)
				if(get_neighbor_hexagon(from, topology, i) == to)
					return true;
			break;

		case TOPOLOGY_TORUS:
			assert(topology->geometry == TOPOLOGY_TORUS);
			for(unsigned i = 0; i < DIRECTION_NE; i++)
				if(get_neighbor_torus(from, topology, i) == to)
					return true;
			break;

		case TOPOLOGY_SQUARE:
			assert(topology->geometry == TOPOLOGY_SQUARE);
			for(unsigned i = 0; i < DIRECTION_NE; i++)
				if(get_neighbor_square(from, topology, i) == to)
					return true;
			break;

		case TOPOLOGY_BIDRING:
			assert(topology->geometry == TOPOLOGY_BIDRING);
			if(get_neighbor_bidring(from, topology, DIRECTION_E) == to)
				return true;
			if(get_neighbor_bidring(from, topology, DIRECTION_W) == to)
				return true;
			break;

		case TOPOLOGY_RING:
			assert(topology->geometry == TOPOLOGY_RING);
			if(get_neighbor_ring(from, topology, DIRECTION_E) == to)
				return true;
			break;

		case TOPOLOGY_STAR:
			assert(topology->geometry == TOPOLOGY_STAR);
			if(from == 0 && to != 0 && to < topology->regions)
				return true;
			if(from != 0 && to == 0 && from < topology->regions)
				return true;
			break;

		case TOPOLOGY_FCMESH:
			assert(topology->geometry == TOPOLOGY_FCMESH);
			if(from < topology->regions && to < topology->regions)
				return true;
			break;

		case TOPOLOGY_GRAPH:
			assert(topology->geometry == TOPOLOGY_GRAPH);
			assert(topology->adjacency != NULL);

			if(list_size(topology->adjacency[from]) == 0)
				return false;
			adj_node = list_head(topology->adjacency[from]);
			while(adj_node && adj_node->neighbor != to)
				adj_node = list_next(adj_node);
			if(adj_node != NULL)
				return true;

			break;

		default:
			fprintf(stderr, "[ERROR] Unexpected topology type.\n");
	}

	return false;
}


lp_id_t GetReceiver(lp_id_t from, struct topology *topology, enum topology_direction direction)
{
	if(unlikely(from >= topology->regions)) {
		fprintf(stderr, "[ERROR] `from` does not belong to the topology.\n");
		return INVALID_DIRECTION;
	}

	switch (topology->geometry) {
		case TOPOLOGY_HEXAGON:
			return get_neighbor_hexagon(from, topology, direction);

		case TOPOLOGY_SQUARE:
			return get_neighbor_square(from, topology, direction);

		case TOPOLOGY_TORUS:
			return get_neighbor_torus(from, topology, direction);

		case TOPOLOGY_FCMESH:
			return get_neighbor_mesh(from, topology, direction);

		case TOPOLOGY_BIDRING:
			return get_neighbor_bidring(from, topology, direction);

		case TOPOLOGY_RING:
			return get_neighbor_ring(from, topology, direction);

		case TOPOLOGY_STAR:
			return get_neighbor_star(from, topology, direction);

		case TOPOLOGY_GRAPH:
			return get_neighbor_graph(from, topology, direction);
	}
	return INVALID_DIRECTION;
}


/**
 * @brief Initialize a topology region.
 *
 * This is a variadic function, that initializes a topology depending on its actual shape.
 * In case the topology is a grid, two unsigned parameters should be passed,
 * to specify the width and height of the grid. For all other topologies (included generic graphs)
 * only a single unsigned parameter is needed, which is the number of elements composing the topology.
 *
 * @param geometry The geometry to be used in the topology, a value from enum topology_geometry
 * @param argc This is the number of variadic arguments passed to the function, computed thanks to
 *             some preprocessor black magic. This allows to make some early sanity check and prevent
 *             users to mess with the stack or initialize wrong topologies.
 * @param ... If geometry is TOPOLOGY_HEXAGON, TOPOLOGY_SQUARE, or TOPOLOGY_TORUS, two unsigned
 *            should be passed, to specify the width and height of the topology's grid.
 *            For all the other topologies, a single unsigned, determining the number of elements
 *            that compose the topology should be passed.
 * @return A pointer to as newly-allocated opaque topology struct. Releasing the topology (and all
 *         the memory internally used to represent it) can be done by passing it to ReleaseTopology().
 */
struct topology *vInitializeTopology(enum topology_geometry geometry, int argc, ...)
{
	struct topology *topology = NULL;
	unsigned regions;
	unsigned width = 0;
	unsigned height = 0;

	// Take the proper number of parameters
	va_list args;
	va_start(args, argc);

	switch(geometry) {
		case TOPOLOGY_HEXAGON:
		case TOPOLOGY_SQUARE:
		case TOPOLOGY_TORUS:
			if(argc != 2) {
				fprintf(stderr, "[ERROR] Wrong number of parameters to InitializeTopology.\n");
				goto out;
			}
			height = va_arg(args, unsigned);
			width = va_arg(args, unsigned);
			regions = width * height;
			break;
		case TOPOLOGY_RING:
		case TOPOLOGY_BIDRING:
		case TOPOLOGY_STAR:
		case TOPOLOGY_FCMESH:
		case TOPOLOGY_GRAPH:
			if(argc != 1) {
				fprintf(stderr, "[ERROR] Wrong number of parameters to InitializeTopology.\n");
				goto out;
			}
			regions = va_arg(args, unsigned);
			break;
		default:
			fprintf(stderr, "[ERROR] Unexpected topology geometry.\n");
			goto out;
	}

	if(regions == 0) {
		fprintf(stderr, "[ERROR] Cannot initialize a topology with no regions.\n");
		goto out;
	}

	topology = malloc(sizeof(*topology));
	if(topology == NULL)
		goto out;
	memset(topology, 0, sizeof(*topology));

	topology->regions = regions;
	topology->geometry = geometry;
	topology->width = width;
	topology->height = height;

	// In case of a graph, allocate empty adjacency lists for all nodes
	size_t i; // This is global for error recovery below, at label err2.
	if(topology->geometry == TOPOLOGY_GRAPH) {
		topology->adjacency = malloc(sizeof(list(struct graph_node)) * regions);
		if(topology->adjacency == NULL)
			goto err1;

		for(i = 0; i < regions; i++) {
			topology->adjacency[i] = new_list(struct graph_node);
			if(topology->adjacency[i] == NULL)
				goto err2;
		}
	}

    out:
	va_end(args);
	return topology;

    err2:
	// Free the memory that has been partially allocated
	for(size_t k = 0; k < i; ++k)
		free(topology->adjacency[k]);
	free(topology->adjacency);

    err1:
	free(topology);
	topology = NULL;
	goto out;
}


void ReleaseTopology(struct topology *topology)
{
	if(topology->geometry == TOPOLOGY_GRAPH && topology->adjacency != NULL) {
		for(size_t i = 0; i < topology->regions; i++) {
			struct graph_node *curr;
			while((curr = list_head(topology->adjacency[i])) != NULL) {
				list_detach_by_content(topology->adjacency[i], curr);
				free(curr);
			}
			free(topology->adjacency[i]);
		}
		free(topology->adjacency);
	}
	free(topology);
}


bool AddTopologyLink(struct topology *topology, lp_id_t from, lp_id_t to, double probability)
{
	if(unlikely(topology->geometry != TOPOLOGY_GRAPH)) {
		fprintf(stderr, "[ERROR] Setting a weighted link in a topology which is not a graph.");
		return false;
	}

	if(unlikely(probability < 0)) {
		fprintf(stderr, "[ERROR] Setting a link probability < 0.");
		return false;
	}

	if(unlikely(probability > 1)) {
		fprintf(stderr, "[ERROR] Setting a link probability > 1.");
		return false;
	}

	assert(topology->adjacency != NULL);
	assert(from < topology->regions);
	assert(to < topology->regions);

	// Get the correct adjacency list
	list(struct graph_node) list = topology->adjacency[from];

	// See if there is already a node representing the link
	struct graph_node *element = list_head(list);
	while(element && element->neighbor != to)
		element = list_next(element);

	if(element == NULL) {
		element = malloc(sizeof(*element));
		element->neighbor = to;
		list_insert_tail(list, element);
	}

	element->probability = probability;
	return true;
}
