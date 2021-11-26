#include <stdlib.h>
#include <time.h>

#include <ROOT-Sim.h>

#include <core/core.h>
#include <lp/process.h>
#include <lp/lp.h>
#include "test.h"

const int LAST_TOPOLOGY_WITH_TWO_PARAMETERS = TOPOLOGY_TORUS;
const int LAST_TOPOLOGY_VALID_VALUE = TOPOLOGY_GRAPH;
const int LAST_DIRECTION_VALID_VALUE = DIRECTION_SE;

#define RANDOM_TRIALS 100

int test_hexagon(void)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_HEXAGON, 5, 4);

	// Test odd row
	assert(GetReceiver(5, topology, DIRECTION_W) == 4);
	assert(GetReceiver(5, topology, DIRECTION_E) == 6);
	assert(GetReceiver(5, topology, DIRECTION_NE) == 2);
	assert(GetReceiver(5, topology, DIRECTION_NW) == 1);
	assert(GetReceiver(5, topology, DIRECTION_SE) == 10);
	assert(GetReceiver(5, topology, DIRECTION_SW) == 9);

	// Test even row
	assert(GetReceiver(10, topology, DIRECTION_W) == 9);
	assert(GetReceiver(10, topology, DIRECTION_E) == 11);
	assert(GetReceiver(10, topology, DIRECTION_NE) == 6);
	assert(GetReceiver(10, topology, DIRECTION_NW) == 5);
	assert(GetReceiver(10, topology, DIRECTION_SE) == 14);
	assert(GetReceiver(10, topology, DIRECTION_SW) == 13);

	// Test boundaries
	assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(1, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(1, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(2, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(2, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(3, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(3, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(3, topology, DIRECTION_E) == INVALID_DIRECTION);
	assert(GetReceiver(7, topology, DIRECTION_E) == INVALID_DIRECTION);
	assert(GetReceiver(7, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(7, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(11, topology, DIRECTION_E) == INVALID_DIRECTION);
	assert(GetReceiver(15, topology, DIRECTION_E) == INVALID_DIRECTION);
	assert(GetReceiver(15, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(15, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(19, topology, DIRECTION_E) == INVALID_DIRECTION);
	assert(GetReceiver(19, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(19, topology, DIRECTION_SW) == INVALID_DIRECTION);
	assert(GetReceiver(18, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(18, topology, DIRECTION_SW) == INVALID_DIRECTION);
	assert(GetReceiver(17, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(17, topology, DIRECTION_SW) == INVALID_DIRECTION);
	assert(GetReceiver(16, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(16, topology, DIRECTION_SW) == INVALID_DIRECTION);
	assert(GetReceiver(16, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(16, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(12, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(8, topology, DIRECTION_SW) == INVALID_DIRECTION);
	assert(GetReceiver(8, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(8, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(4, topology, DIRECTION_W) == INVALID_DIRECTION);

	// Sanity checks
	assert(GetReceiver(5, topology, DIRECTION_N) == INVALID_DIRECTION);
	assert(GetReceiver(5, topology, DIRECTION_S) == INVALID_DIRECTION);

	// Test neighbors count
	assert(CountDirections(0, topology) == 2);
	assert(CountDirections(1, topology) == 4);
	assert(CountDirections(2, topology) == 4);
	assert(CountDirections(3, topology) == 3);
	assert(CountDirections(4, topology) == 5);
	assert(CountDirections(7, topology) == 3);
	assert(CountDirections(8, topology) == 3);
	assert(CountDirections(11, topology) == 5);
	assert(CountDirections(12, topology) == 5);
	assert(CountDirections(15, topology) == 3);
	assert(CountDirections(16, topology) == 2);
	assert(CountDirections(17, topology) == 4);
	assert(CountDirections(18, topology) == 4);
	assert(CountDirections(19, topology) == 3);
	assert(CountDirections(5, topology) == 6);
	assert(CountDirections(10, topology) == 6);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 20; j++) // 20 is the number of regions in this test
			assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	assert(IsNeighbor(0, 1, topology) == true);
	assert(IsNeighbor(0, 4, topology) == true);
	assert(IsNeighbor(5, 10, topology) == true);
	assert(IsNeighbor(17, 8, topology) == false);
	assert(IsNeighbor(7, 4, topology) == false);
	assert(IsNeighbor(3, 1, topology) == false);

	ReleaseTopology(topology);
	check_passed_asserts();
};

int test_square(void)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_SQUARE, 3, 4);

	// Test all directions
	assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_S) == 4);
	assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_E) == 1);
	assert(GetReceiver(1, topology, DIRECTION_N) == INVALID_DIRECTION);
	assert(GetReceiver(1, topology, DIRECTION_S) == 5);
	assert(GetReceiver(1, topology, DIRECTION_W) == 0);
	assert(GetReceiver(1, topology, DIRECTION_E) == 2);
	assert(GetReceiver(2, topology, DIRECTION_N) == INVALID_DIRECTION);
	assert(GetReceiver(2, topology, DIRECTION_S) == 6);
	assert(GetReceiver(2, topology, DIRECTION_W) == 1);
	assert(GetReceiver(2, topology, DIRECTION_E) == 3);
	assert(GetReceiver(3, topology, DIRECTION_N) == INVALID_DIRECTION);
	assert(GetReceiver(3, topology, DIRECTION_S) == 7);
	assert(GetReceiver(3, topology, DIRECTION_W) == 2);
	assert(GetReceiver(3, topology, DIRECTION_E) == INVALID_DIRECTION);
	assert(GetReceiver(4, topology, DIRECTION_N) == 0);
	assert(GetReceiver(4, topology, DIRECTION_S) == 8);
	assert(GetReceiver(4, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(4, topology, DIRECTION_E) == 5);
	assert(GetReceiver(5, topology, DIRECTION_N) == 1);
	assert(GetReceiver(5, topology, DIRECTION_S) == 9);
	assert(GetReceiver(5, topology, DIRECTION_W) == 4);
	assert(GetReceiver(5, topology, DIRECTION_E) == 6);
	assert(GetReceiver(6, topology, DIRECTION_N) == 2);
	assert(GetReceiver(6, topology, DIRECTION_S) == 10);
	assert(GetReceiver(6, topology, DIRECTION_W) == 5);
	assert(GetReceiver(6, topology, DIRECTION_E) == 7);
	assert(GetReceiver(7, topology, DIRECTION_N) == 3);
	assert(GetReceiver(7, topology, DIRECTION_S) == 11);
	assert(GetReceiver(7, topology, DIRECTION_W) == 6);
	assert(GetReceiver(7, topology, DIRECTION_E) == INVALID_DIRECTION);
	assert(GetReceiver(8, topology, DIRECTION_N) == 4);
	assert(GetReceiver(8, topology, DIRECTION_S) == INVALID_DIRECTION);
	assert(GetReceiver(8, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(8, topology, DIRECTION_E) == 9);
	assert(GetReceiver(9, topology, DIRECTION_N) == 5);
	assert(GetReceiver(9, topology, DIRECTION_S) == INVALID_DIRECTION);
	assert(GetReceiver(9, topology, DIRECTION_W) == 8);
	assert(GetReceiver(9, topology, DIRECTION_E) == 10);
	assert(GetReceiver(10, topology, DIRECTION_N) == 6);
	assert(GetReceiver(10, topology, DIRECTION_S) == INVALID_DIRECTION);
	assert(GetReceiver(10, topology, DIRECTION_W) == 9);
	assert(GetReceiver(10, topology, DIRECTION_E) == 11);
	assert(GetReceiver(11, topology, DIRECTION_N) == 7);
	assert(GetReceiver(11, topology, DIRECTION_S) == INVALID_DIRECTION);
	assert(GetReceiver(11, topology, DIRECTION_W) == 10);
	assert(GetReceiver(11, topology, DIRECTION_E) == INVALID_DIRECTION);

	// Test neighbors count
	assert(CountDirections(0, topology) == 2);
	assert(CountDirections(1, topology) == 3);
	assert(CountDirections(2, topology) == 3);
	assert(CountDirections(3, topology) == 2);
	assert(CountDirections(4, topology) == 3);
	assert(CountDirections(5, topology) == 4);
	assert(CountDirections(6, topology) == 4);
	assert(CountDirections(7, topology) == 3);
	assert(CountDirections(8, topology) == 2);
	assert(CountDirections(9, topology) == 3);
	assert(CountDirections(10, topology) == 3);
	assert(CountDirections(11, topology) == 2);

	// Sanity check
	assert(GetReceiver(5, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(5, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(5, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(5, topology, DIRECTION_SW) == INVALID_DIRECTION);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 12; j++) // 12 is the number of regions in this test
			assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	assert(IsNeighbor(0, 1, topology) == true);
	assert(IsNeighbor(0, 4, topology) == true);
	assert(IsNeighbor(5, 10, topology) == false);
	assert(IsNeighbor(11, 8, topology) == false);
	assert(IsNeighbor(7, 4, topology) == false);
	assert(IsNeighbor(3, 1, topology) == false);

	ReleaseTopology(topology);
	check_passed_asserts();
}

int test_ring(void)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_RING, 5);

	// Sanity check
	assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_S) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);

	// Check valid directions
	assert(GetReceiver(0, topology, DIRECTION_E) == 1);
	assert(GetReceiver(1, topology, DIRECTION_E) == 2);
	assert(GetReceiver(2, topology, DIRECTION_E) == 3);
	assert(GetReceiver(3, topology, DIRECTION_E) == 4);
	assert(GetReceiver(4, topology, DIRECTION_E) == 0);

	// Test neighbors count
	assert(CountDirections(0, topology) == 1);
	assert(CountDirections(1, topology) == 1);
	assert(CountDirections(2, topology) == 1);
	assert(CountDirections(3, topology) == 1);
	assert(CountDirections(4, topology) == 1);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 5; j++) // 5 is the number of regions in this test
			assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	assert(IsNeighbor(0, 1, topology) == true);
	assert(IsNeighbor(1, 2, topology) == true);
	assert(IsNeighbor(2, 4, topology) == false);
	assert(IsNeighbor(1, 0, topology) == false);
	assert(IsNeighbor(2, 1, topology) == false);
	assert(IsNeighbor(4, 2, topology) == false);

	ReleaseTopology(topology);
	check_passed_asserts();
}

int test_bidring(void)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_BIDRING, 5);

	// Sanity check
	assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_S) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);

	// Check valid directions
	assert(GetReceiver(0, topology, DIRECTION_E) == 1);
	assert(GetReceiver(1, topology, DIRECTION_E) == 2);
	assert(GetReceiver(2, topology, DIRECTION_E) == 3);
	assert(GetReceiver(3, topology, DIRECTION_E) == 4);
	assert(GetReceiver(4, topology, DIRECTION_E) == 0);
	assert(GetReceiver(0, topology, DIRECTION_W) == 4);
	assert(GetReceiver(1, topology, DIRECTION_W) == 0);
	assert(GetReceiver(2, topology, DIRECTION_W) == 1);
	assert(GetReceiver(3, topology, DIRECTION_W) == 2);
	assert(GetReceiver(4, topology, DIRECTION_W) == 3);

	// Test neighbors count
	assert(CountDirections(0, topology) == 2);
	assert(CountDirections(1, topology) == 2);
	assert(CountDirections(2, topology) == 2);
	assert(CountDirections(3, topology) == 2);
	assert(CountDirections(4, topology) == 2);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 5; j++) // 5 is the number of regions in this test
			assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	assert(IsNeighbor(0, 1, topology) == true);
	assert(IsNeighbor(1, 2, topology) == true);
	assert(IsNeighbor(2, 4, topology) == false);
	assert(IsNeighbor(1, 0, topology) == true);
	assert(IsNeighbor(2, 1, topology) == true);
	assert(IsNeighbor(4, 2, topology) == false);

	ReleaseTopology(topology);
	check_passed_asserts();
}

int test_torus(void)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_TORUS, 3, 4);

	// Test all directions
	assert(GetReceiver(0, topology, DIRECTION_N) == 8);
	assert(GetReceiver(0, topology, DIRECTION_S) == 4);
	assert(GetReceiver(0, topology, DIRECTION_W) == 3);
	assert(GetReceiver(0, topology, DIRECTION_E) == 1);
	assert(GetReceiver(1, topology, DIRECTION_N) == 9);
	assert(GetReceiver(1, topology, DIRECTION_S) == 5);
	assert(GetReceiver(1, topology, DIRECTION_W) == 0);
	assert(GetReceiver(1, topology, DIRECTION_E) == 2);
	assert(GetReceiver(2, topology, DIRECTION_N) == 10);
	assert(GetReceiver(2, topology, DIRECTION_S) == 6);
	assert(GetReceiver(2, topology, DIRECTION_W) == 1);
	assert(GetReceiver(2, topology, DIRECTION_E) == 3);
	assert(GetReceiver(3, topology, DIRECTION_N) == 11);
	assert(GetReceiver(3, topology, DIRECTION_S) == 7);
	assert(GetReceiver(3, topology, DIRECTION_W) == 2);
	assert(GetReceiver(3, topology, DIRECTION_E) == 0);
	assert(GetReceiver(4, topology, DIRECTION_N) == 0);
	assert(GetReceiver(4, topology, DIRECTION_S) == 8);
	assert(GetReceiver(4, topology, DIRECTION_W) == 7);
	assert(GetReceiver(4, topology, DIRECTION_E) == 5);
	assert(GetReceiver(5, topology, DIRECTION_N) == 1);
	assert(GetReceiver(5, topology, DIRECTION_S) == 9);
	assert(GetReceiver(5, topology, DIRECTION_W) == 4);
	assert(GetReceiver(5, topology, DIRECTION_E) == 6);
	assert(GetReceiver(6, topology, DIRECTION_N) == 2);
	assert(GetReceiver(6, topology, DIRECTION_S) == 10);
	assert(GetReceiver(6, topology, DIRECTION_W) == 5);
	assert(GetReceiver(6, topology, DIRECTION_E) == 7);
	assert(GetReceiver(7, topology, DIRECTION_N) == 3);
	assert(GetReceiver(7, topology, DIRECTION_S) == 11);
	assert(GetReceiver(7, topology, DIRECTION_W) == 6);
	assert(GetReceiver(7, topology, DIRECTION_E) == 4);
	assert(GetReceiver(8, topology, DIRECTION_N) == 4);
	assert(GetReceiver(8, topology, DIRECTION_S) == 0);
	assert(GetReceiver(8, topology, DIRECTION_W) == 11);
	assert(GetReceiver(8, topology, DIRECTION_E) == 9);
	assert(GetReceiver(9, topology, DIRECTION_N) == 5);
	assert(GetReceiver(9, topology, DIRECTION_S) == 1);
	assert(GetReceiver(9, topology, DIRECTION_W) == 8);
	assert(GetReceiver(9, topology, DIRECTION_E) == 10);
	assert(GetReceiver(10, topology, DIRECTION_N) == 6);
	assert(GetReceiver(10, topology, DIRECTION_S) == 2);
	assert(GetReceiver(10, topology, DIRECTION_W) == 9);
	assert(GetReceiver(10, topology, DIRECTION_E) == 11);
	assert(GetReceiver(11, topology, DIRECTION_N) == 7);
	assert(GetReceiver(11, topology, DIRECTION_S) == 3);
	assert(GetReceiver(11, topology, DIRECTION_W) == 10);
	assert(GetReceiver(11, topology, DIRECTION_E) == 8);

	// Test neighbors count
	assert(CountDirections(0, topology) == 4);
	assert(CountDirections(1, topology) == 4);
	assert(CountDirections(2, topology) == 4);
	assert(CountDirections(3, topology) == 4);
	assert(CountDirections(4, topology) == 4);
	assert(CountDirections(5, topology) == 4);
	assert(CountDirections(6, topology) == 4);
	assert(CountDirections(7, topology) == 4);
	assert(CountDirections(8, topology) == 4);
	assert(CountDirections(9, topology) == 4);
	assert(CountDirections(10, topology) == 4);
	assert(CountDirections(11, topology) == 4);

	// Sanity check
	assert(GetReceiver(5, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(5, topology, DIRECTION_NW) == INVALID_DIRECTION);
	assert(GetReceiver(5, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(5, topology, DIRECTION_SW) == INVALID_DIRECTION);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 12; j++) // 12 is the number of regions in this test
			assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	assert(IsNeighbor(0, 1, topology) == true);
	assert(IsNeighbor(0, 4, topology) == true);
	assert(IsNeighbor(5, 10, topology) == false);
	assert(IsNeighbor(11, 8, topology) == true);
	assert(IsNeighbor(3, 11, topology) == true);
	assert(IsNeighbor(3, 1, topology) == false);

	ReleaseTopology(topology);
	check_passed_asserts();
}

int test_star(void)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_STAR, 5);

	// Sanity check
	assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_S) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_E) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);

	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 1; j < 5; j++) // 5 is the number of regions in this test
			assert(GetReceiver(j, topology, DIRECTION_RANDOM) == 0);
		assert(GetReceiver(0, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Check directions count
	assert(CountDirections(0, topology) > 1);
	assert(CountDirections(0, topology) < CountRegions(topology));
	assert(CountDirections(1, topology) == 1);
	assert(CountDirections(2, topology) == 1);
	assert(CountDirections(3, topology) == 1);
	assert(CountDirections(4, topology) == 1);

	// Test neighbor check
	assert(IsNeighbor(0, 1, topology) == true);
	assert(IsNeighbor(0, 2, topology) == true);
	assert(IsNeighbor(0, 3, topology) == true);
	assert(IsNeighbor(0, 4, topology) == true);
	assert(IsNeighbor(1, 0, topology) == true);
	assert(IsNeighbor(2, 0, topology) == true);
	assert(IsNeighbor(3, 0, topology) == true);
	assert(IsNeighbor(4, 0, topology) == true);
	assert(IsNeighbor(1, 2, topology) == false);
	assert(IsNeighbor(2, 1, topology) == false);
	assert(IsNeighbor(1, 3, topology) == false);
	assert(IsNeighbor(3, 1, topology) == false);
	assert(IsNeighbor(2, 4, topology) == false);
	assert(IsNeighbor(4, 2, topology) == false);

	ReleaseTopology(topology);
	check_passed_asserts();
}

int test_mesh(void)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_FCMESH, 5);

	// Sanity check
	assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_S) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_E) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_SE) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);

	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 1; j < 5; j++) // 5 is the number of regions in this test
			assert(GetReceiver(0, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Check directions count
	assert(CountDirections(0, topology) == CountRegions(topology) - 1);
	assert(CountDirections(1, topology) == CountRegions(topology) - 1);
	assert(CountDirections(2, topology) == CountRegions(topology) - 1);
	assert(CountDirections(3, topology) == CountRegions(topology) - 1);
	assert(CountDirections(4, topology) == CountRegions(topology) - 1);

	// Test neighbor check
	assert(IsNeighbor(0, 1, topology) == true);
	assert(IsNeighbor(0, 2, topology) == true);
	assert(IsNeighbor(0, 3, topology) == true);
	assert(IsNeighbor(0, 4, topology) == true);
	assert(IsNeighbor(1, 0, topology) == true);
	assert(IsNeighbor(2, 0, topology) == true);
	assert(IsNeighbor(3, 0, topology) == true);
	assert(IsNeighbor(4, 0, topology) == true);
	assert(IsNeighbor(1, 2, topology) == true);
	assert(IsNeighbor(2, 1, topology) == true);
	assert(IsNeighbor(1, 3, topology) == true);
	assert(IsNeighbor(3, 1, topology) == true);
	assert(IsNeighbor(2, 4, topology) == true);
	assert(IsNeighbor(4, 2, topology) == true);

	ReleaseTopology(topology);
	check_passed_asserts();
}

#define MAX_NODES_TEST 20
#define NUM_QUERIES 50
int test_graph(void)
{
	struct topology *topology;
	unsigned num_edges;
	unsigned from, to;

	for(unsigned nodes = 1; nodes < MAX_NODES_TEST; nodes++) {
		topology = InitializeTopology(TOPOLOGY_GRAPH, nodes);

		num_edges = (unsigned)(nodes + (double)rand() / ((double)RAND_MAX / (9 * nodes + 1) + 1));
		for(unsigned edge = 0; edge < num_edges; edge++) {
			from = (unsigned)(nodes * (double)rand() / RAND_MAX);
			to = (unsigned)(nodes * (double)rand() / RAND_MAX);
			assert(AddTopologyLink(topology, from, to, (double)rand() / RAND_MAX));
			assert(AddTopologyLink(topology, from, to, 2.0) == false);
			assert(AddTopologyLink(topology, from, to, -1.0) == false);
		}

		for(unsigned i = 0; i < NUM_QUERIES; i++) {
			from = (unsigned)(nodes * (double)rand() / RAND_MAX);
			to = GetReceiver(from, topology, DIRECTION_RANDOM);

			// We get an invalid direction only if there is a node with no
			// outgoing edges. In this case, we check this condition.
			if(to == INVALID_DIRECTION) {
				assert(CountDirections(from, topology) == 0);
				continue;
			}

			assert(to < nodes);
			assert(IsNeighbor(from, to, topology));
		}

		ReleaseTopology(topology);
	}

	// Test sanity checks on graphs
	topology = InitializeTopology(TOPOLOGY_GRAPH, 1);
	for(int i = 0; i <= LAST_DIRECTION_VALID_VALUE; i++)
		assert(GetReceiver(0, topology, i) == INVALID_DIRECTION);

	check_passed_asserts();
}
#undef NUM_QUERIES
#undef MAX_NODES_TEST

int test_init_fini(void)
{
	struct topology *topology;
	unsigned par1, par2; // Testing a variadic function, these are the two parameters

	for(unsigned i = 1; i <= LAST_TOPOLOGY_WITH_TWO_PARAMETERS; i++) {
		assert(InitializeTopology(i, 0, 0) == NULL);
		assert(InitializeTopology(i, 1, 0) == NULL);
		assert(InitializeTopology(i, 0, 1) == NULL);

		par1 = (unsigned)(100 * (double)rand() / RAND_MAX) + 1;
		par2 = (unsigned)(100 * (double)rand() / RAND_MAX) + 1;
		topology = InitializeTopology(i, par1, par2);
		assert(CountRegions(topology) == par1 * par2);
		assert(GetReceiver(par1 * par2 + 1, topology, DIRECTION_E) == INVALID_DIRECTION);
		assert(AddTopologyLink(topology, par1, par2, (double)rand() / RAND_MAX) == false);
		ReleaseTopology(topology);
	}

	for(unsigned i = LAST_TOPOLOGY_WITH_TWO_PARAMETERS + 1; i <= LAST_TOPOLOGY_VALID_VALUE; i++) {
		assert(InitializeTopology(i, 0) == NULL);

		par1 = (unsigned)(100 * (double)rand() / RAND_MAX) + 1;
		topology = InitializeTopology(i, par1);
		assert(CountRegions(topology) == par1);
		assert(GetReceiver(par1 + 1, topology, DIRECTION_E) == INVALID_DIRECTION);
		ReleaseTopology(topology);
	}

	// Test if the variadic function detects a wrong number of parameters
	assert(InitializeTopology(TOPOLOGY_HEXAGON, 1) == NULL);
	assert(InitializeTopology(TOPOLOGY_HEXAGON, 1, 1, 1) == NULL);
	assert(InitializeTopology(TOPOLOGY_HEXAGON, 1, 1, 1, 1) == NULL);
	assert(InitializeTopology(TOPOLOGY_GRAPH, 1, 1) == NULL);
	assert(InitializeTopology(TOPOLOGY_GRAPH, 1, 1, 1) == NULL);
	assert(InitializeTopology(TOPOLOGY_GRAPH, 1, 1, 1, 1) == NULL);

	// Test what happens if wrong geometry is passed to InitializeTopology()
	assert(InitializeTopology(10 * TOPOLOGY_GRAPH, 1) == NULL);

	check_passed_asserts();
}

int main(void) {
	init();

	// Mock a fake LP
	struct lp_ctx lp = {0};
	lp.lib_ctx = malloc(sizeof(*current_lp->lib_ctx));
	lp.lib_ctx->rng_s[0] = 7319936632422683443ULL;
	lp.lib_ctx->rng_s[1] = 2268344373199366324ULL;
	lp.lib_ctx->rng_s[2] = 3443862242366399137ULL;
	lp.lib_ctx->rng_s[3] = 2366399137344386224ULL;
	current_lp = &lp;

	srand(time(NULL));

	test("Topology initialization and release", test_init_fini);
	test("Hexagon topology", test_hexagon);
	test("Square topology", test_square);
	test("Ring topology", test_ring);
	test("BidRing topology", test_bidring);
	test("Torus topology", test_torus);
	test("Star topology", test_star);
	test("Fully Connected Mesh topology", test_mesh);
	test("Graph topology", test_graph);

	finish();
}
