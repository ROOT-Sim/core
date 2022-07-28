/**
* @file test/tests/lib/topology.c
*
* @brief Test: topology library
*
* SPDX-FileCopyrightText: 2008-2022 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#include <time.h>

#include <test.h>
#include <ROOT-Sim.h>
#include <lp/lp.h>

const enum topology_geometry LAST_TOPOLOGY_WITH_TWO_PARAMETERS = TOPOLOGY_TORUS;
const enum topology_geometry LAST_TOPOLOGY_VALID_VALUE = TOPOLOGY_GRAPH;
const enum topology_direction LAST_DIRECTION_VALID_VALUE = DIRECTION_SE;

#define RANDOM_TRIALS 100

int test_hexagon(_unused void *_)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_HEXAGON, 5, 4);

	// Test odd row
	test_assert(GetReceiver(5, topology, DIRECTION_W) == 4);
	test_assert(GetReceiver(5, topology, DIRECTION_E) == 6);
	test_assert(GetReceiver(5, topology, DIRECTION_NE) == 2);
	test_assert(GetReceiver(5, topology, DIRECTION_NW) == 1);
	test_assert(GetReceiver(5, topology, DIRECTION_SE) == 10);
	test_assert(GetReceiver(5, topology, DIRECTION_SW) == 9);

	// Test even row
	test_assert(GetReceiver(10, topology, DIRECTION_W) == 9);
	test_assert(GetReceiver(10, topology, DIRECTION_E) == 11);
	test_assert(GetReceiver(10, topology, DIRECTION_NE) == 6);
	test_assert(GetReceiver(10, topology, DIRECTION_NW) == 5);
	test_assert(GetReceiver(10, topology, DIRECTION_SE) == 14);
	test_assert(GetReceiver(10, topology, DIRECTION_SW) == 13);

	// Test boundaries
	test_assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(1, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(1, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(2, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(2, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(3, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(3, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(3, topology, DIRECTION_E) == INVALID_DIRECTION);
	test_assert(GetReceiver(7, topology, DIRECTION_E) == INVALID_DIRECTION);
	test_assert(GetReceiver(7, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(7, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(11, topology, DIRECTION_E) == INVALID_DIRECTION);
	test_assert(GetReceiver(15, topology, DIRECTION_E) == INVALID_DIRECTION);
	test_assert(GetReceiver(15, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(15, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(19, topology, DIRECTION_E) == INVALID_DIRECTION);
	test_assert(GetReceiver(19, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(19, topology, DIRECTION_SW) == INVALID_DIRECTION);
	test_assert(GetReceiver(18, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(18, topology, DIRECTION_SW) == INVALID_DIRECTION);
	test_assert(GetReceiver(17, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(17, topology, DIRECTION_SW) == INVALID_DIRECTION);
	test_assert(GetReceiver(16, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(16, topology, DIRECTION_SW) == INVALID_DIRECTION);
	test_assert(GetReceiver(16, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(16, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(12, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(8, topology, DIRECTION_SW) == INVALID_DIRECTION);
	test_assert(GetReceiver(8, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(8, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(4, topology, DIRECTION_W) == INVALID_DIRECTION);

	// Sanity checks
	test_assert(GetReceiver(5, topology, DIRECTION_N) == INVALID_DIRECTION);
	test_assert(GetReceiver(5, topology, DIRECTION_S) == INVALID_DIRECTION);

	// Test neighbors count
	test_assert(CountDirections(0, topology) == 2);
	test_assert(CountDirections(1, topology) == 4);
	test_assert(CountDirections(2, topology) == 4);
	test_assert(CountDirections(3, topology) == 3);
	test_assert(CountDirections(4, topology) == 5);
	test_assert(CountDirections(7, topology) == 3);
	test_assert(CountDirections(8, topology) == 3);
	test_assert(CountDirections(11, topology) == 5);
	test_assert(CountDirections(12, topology) == 5);
	test_assert(CountDirections(15, topology) == 3);
	test_assert(CountDirections(16, topology) == 2);
	test_assert(CountDirections(17, topology) == 4);
	test_assert(CountDirections(18, topology) == 4);
	test_assert(CountDirections(19, topology) == 3);
	test_assert(CountDirections(5, topology) == 6);
	test_assert(CountDirections(10, topology) == 6);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 20; j++) // 20 is the number of regions in this test
			test_assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	test_assert(IsNeighbor(0, 1, topology) == true);
	test_assert(IsNeighbor(0, 4, topology) == true);
	test_assert(IsNeighbor(5, 10, topology) == true);
	test_assert(IsNeighbor(17, 8, topology) == false);
	test_assert(IsNeighbor(7, 4, topology) == false);
	test_assert(IsNeighbor(3, 1, topology) == false);

	ReleaseTopology(topology);

	return 0;
}

int test_square(_unused void *_)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_SQUARE, 3, 4);

	// Test all directions
	test_assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_S) == 4);
	test_assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_E) == 1);
	test_assert(GetReceiver(1, topology, DIRECTION_N) == INVALID_DIRECTION);
	test_assert(GetReceiver(1, topology, DIRECTION_S) == 5);
	test_assert(GetReceiver(1, topology, DIRECTION_W) == 0);
	test_assert(GetReceiver(1, topology, DIRECTION_E) == 2);
	test_assert(GetReceiver(2, topology, DIRECTION_N) == INVALID_DIRECTION);
	test_assert(GetReceiver(2, topology, DIRECTION_S) == 6);
	test_assert(GetReceiver(2, topology, DIRECTION_W) == 1);
	test_assert(GetReceiver(2, topology, DIRECTION_E) == 3);
	test_assert(GetReceiver(3, topology, DIRECTION_N) == INVALID_DIRECTION);
	test_assert(GetReceiver(3, topology, DIRECTION_S) == 7);
	test_assert(GetReceiver(3, topology, DIRECTION_W) == 2);
	test_assert(GetReceiver(3, topology, DIRECTION_E) == INVALID_DIRECTION);
	test_assert(GetReceiver(4, topology, DIRECTION_N) == 0);
	test_assert(GetReceiver(4, topology, DIRECTION_S) == 8);
	test_assert(GetReceiver(4, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(4, topology, DIRECTION_E) == 5);
	test_assert(GetReceiver(5, topology, DIRECTION_N) == 1);
	test_assert(GetReceiver(5, topology, DIRECTION_S) == 9);
	test_assert(GetReceiver(5, topology, DIRECTION_W) == 4);
	test_assert(GetReceiver(5, topology, DIRECTION_E) == 6);
	test_assert(GetReceiver(6, topology, DIRECTION_N) == 2);
	test_assert(GetReceiver(6, topology, DIRECTION_S) == 10);
	test_assert(GetReceiver(6, topology, DIRECTION_W) == 5);
	test_assert(GetReceiver(6, topology, DIRECTION_E) == 7);
	test_assert(GetReceiver(7, topology, DIRECTION_N) == 3);
	test_assert(GetReceiver(7, topology, DIRECTION_S) == 11);
	test_assert(GetReceiver(7, topology, DIRECTION_W) == 6);
	test_assert(GetReceiver(7, topology, DIRECTION_E) == INVALID_DIRECTION);
	test_assert(GetReceiver(8, topology, DIRECTION_N) == 4);
	test_assert(GetReceiver(8, topology, DIRECTION_S) == INVALID_DIRECTION);
	test_assert(GetReceiver(8, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(8, topology, DIRECTION_E) == 9);
	test_assert(GetReceiver(9, topology, DIRECTION_N) == 5);
	test_assert(GetReceiver(9, topology, DIRECTION_S) == INVALID_DIRECTION);
	test_assert(GetReceiver(9, topology, DIRECTION_W) == 8);
	test_assert(GetReceiver(9, topology, DIRECTION_E) == 10);
	test_assert(GetReceiver(10, topology, DIRECTION_N) == 6);
	test_assert(GetReceiver(10, topology, DIRECTION_S) == INVALID_DIRECTION);
	test_assert(GetReceiver(10, topology, DIRECTION_W) == 9);
	test_assert(GetReceiver(10, topology, DIRECTION_E) == 11);
	test_assert(GetReceiver(11, topology, DIRECTION_N) == 7);
	test_assert(GetReceiver(11, topology, DIRECTION_S) == INVALID_DIRECTION);
	test_assert(GetReceiver(11, topology, DIRECTION_W) == 10);
	test_assert(GetReceiver(11, topology, DIRECTION_E) == INVALID_DIRECTION);

	// Test neighbors count
	test_assert(CountDirections(0, topology) == 2);
	test_assert(CountDirections(1, topology) == 3);
	test_assert(CountDirections(2, topology) == 3);
	test_assert(CountDirections(3, topology) == 2);
	test_assert(CountDirections(4, topology) == 3);
	test_assert(CountDirections(5, topology) == 4);
	test_assert(CountDirections(6, topology) == 4);
	test_assert(CountDirections(7, topology) == 3);
	test_assert(CountDirections(8, topology) == 2);
	test_assert(CountDirections(9, topology) == 3);
	test_assert(CountDirections(10, topology) == 3);
	test_assert(CountDirections(11, topology) == 2);

	// Sanity check
	test_assert(GetReceiver(5, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(5, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(5, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(5, topology, DIRECTION_SW) == INVALID_DIRECTION);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 12; j++) // 12 is the number of regions in this test
			test_assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	test_assert(IsNeighbor(0, 1, topology) == true);
	test_assert(IsNeighbor(0, 4, topology) == true);
	test_assert(IsNeighbor(5, 10, topology) == false);
	test_assert(IsNeighbor(11, 8, topology) == false);
	test_assert(IsNeighbor(7, 4, topology) == false);
	test_assert(IsNeighbor(3, 1, topology) == false);

	ReleaseTopology(topology);

	return 0;
}

int test_ring(_unused void *_)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_RING, 5);

	// Sanity check
	test_assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_S) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);

	// Check valid directions
	test_assert(GetReceiver(0, topology, DIRECTION_E) == 1);
	test_assert(GetReceiver(1, topology, DIRECTION_E) == 2);
	test_assert(GetReceiver(2, topology, DIRECTION_E) == 3);
	test_assert(GetReceiver(3, topology, DIRECTION_E) == 4);
	test_assert(GetReceiver(4, topology, DIRECTION_E) == 0);

	// Test neighbors count
	test_assert(CountDirections(0, topology) == 1);
	test_assert(CountDirections(1, topology) == 1);
	test_assert(CountDirections(2, topology) == 1);
	test_assert(CountDirections(3, topology) == 1);
	test_assert(CountDirections(4, topology) == 1);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 5; j++) // 5 is the number of regions in this test
			test_assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	test_assert(IsNeighbor(0, 1, topology) == true);
	test_assert(IsNeighbor(1, 2, topology) == true);
	test_assert(IsNeighbor(2, 4, topology) == false);
	test_assert(IsNeighbor(1, 0, topology) == false);
	test_assert(IsNeighbor(2, 1, topology) == false);
	test_assert(IsNeighbor(4, 2, topology) == false);

	ReleaseTopology(topology);

	return 0;
}

int test_bidring(_unused void *_)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_BIDRING, 5);

	// Sanity check
	test_assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_S) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);

	// Check valid directions
	test_assert(GetReceiver(0, topology, DIRECTION_E) == 1);
	test_assert(GetReceiver(1, topology, DIRECTION_E) == 2);
	test_assert(GetReceiver(2, topology, DIRECTION_E) == 3);
	test_assert(GetReceiver(3, topology, DIRECTION_E) == 4);
	test_assert(GetReceiver(4, topology, DIRECTION_E) == 0);
	test_assert(GetReceiver(0, topology, DIRECTION_W) == 4);
	test_assert(GetReceiver(1, topology, DIRECTION_W) == 0);
	test_assert(GetReceiver(2, topology, DIRECTION_W) == 1);
	test_assert(GetReceiver(3, topology, DIRECTION_W) == 2);
	test_assert(GetReceiver(4, topology, DIRECTION_W) == 3);

	// Test neighbors count
	test_assert(CountDirections(0, topology) == 2);
	test_assert(CountDirections(1, topology) == 2);
	test_assert(CountDirections(2, topology) == 2);
	test_assert(CountDirections(3, topology) == 2);
	test_assert(CountDirections(4, topology) == 2);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 5; j++) // 5 is the number of regions in this test
			test_assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	test_assert(IsNeighbor(0, 1, topology) == true);
	test_assert(IsNeighbor(1, 2, topology) == true);
	test_assert(IsNeighbor(2, 4, topology) == false);
	test_assert(IsNeighbor(1, 0, topology) == true);
	test_assert(IsNeighbor(2, 1, topology) == true);
	test_assert(IsNeighbor(4, 2, topology) == false);

	ReleaseTopology(topology);

	return 0;
}

int test_torus(_unused void *_)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_TORUS, 3, 4);

	// Test all directions
	test_assert(GetReceiver(0, topology, DIRECTION_N) == 8);
	test_assert(GetReceiver(0, topology, DIRECTION_S) == 4);
	test_assert(GetReceiver(0, topology, DIRECTION_W) == 3);
	test_assert(GetReceiver(0, topology, DIRECTION_E) == 1);
	test_assert(GetReceiver(1, topology, DIRECTION_N) == 9);
	test_assert(GetReceiver(1, topology, DIRECTION_S) == 5);
	test_assert(GetReceiver(1, topology, DIRECTION_W) == 0);
	test_assert(GetReceiver(1, topology, DIRECTION_E) == 2);
	test_assert(GetReceiver(2, topology, DIRECTION_N) == 10);
	test_assert(GetReceiver(2, topology, DIRECTION_S) == 6);
	test_assert(GetReceiver(2, topology, DIRECTION_W) == 1);
	test_assert(GetReceiver(2, topology, DIRECTION_E) == 3);
	test_assert(GetReceiver(3, topology, DIRECTION_N) == 11);
	test_assert(GetReceiver(3, topology, DIRECTION_S) == 7);
	test_assert(GetReceiver(3, topology, DIRECTION_W) == 2);
	test_assert(GetReceiver(3, topology, DIRECTION_E) == 0);
	test_assert(GetReceiver(4, topology, DIRECTION_N) == 0);
	test_assert(GetReceiver(4, topology, DIRECTION_S) == 8);
	test_assert(GetReceiver(4, topology, DIRECTION_W) == 7);
	test_assert(GetReceiver(4, topology, DIRECTION_E) == 5);
	test_assert(GetReceiver(5, topology, DIRECTION_N) == 1);
	test_assert(GetReceiver(5, topology, DIRECTION_S) == 9);
	test_assert(GetReceiver(5, topology, DIRECTION_W) == 4);
	test_assert(GetReceiver(5, topology, DIRECTION_E) == 6);
	test_assert(GetReceiver(6, topology, DIRECTION_N) == 2);
	test_assert(GetReceiver(6, topology, DIRECTION_S) == 10);
	test_assert(GetReceiver(6, topology, DIRECTION_W) == 5);
	test_assert(GetReceiver(6, topology, DIRECTION_E) == 7);
	test_assert(GetReceiver(7, topology, DIRECTION_N) == 3);
	test_assert(GetReceiver(7, topology, DIRECTION_S) == 11);
	test_assert(GetReceiver(7, topology, DIRECTION_W) == 6);
	test_assert(GetReceiver(7, topology, DIRECTION_E) == 4);
	test_assert(GetReceiver(8, topology, DIRECTION_N) == 4);
	test_assert(GetReceiver(8, topology, DIRECTION_S) == 0);
	test_assert(GetReceiver(8, topology, DIRECTION_W) == 11);
	test_assert(GetReceiver(8, topology, DIRECTION_E) == 9);
	test_assert(GetReceiver(9, topology, DIRECTION_N) == 5);
	test_assert(GetReceiver(9, topology, DIRECTION_S) == 1);
	test_assert(GetReceiver(9, topology, DIRECTION_W) == 8);
	test_assert(GetReceiver(9, topology, DIRECTION_E) == 10);
	test_assert(GetReceiver(10, topology, DIRECTION_N) == 6);
	test_assert(GetReceiver(10, topology, DIRECTION_S) == 2);
	test_assert(GetReceiver(10, topology, DIRECTION_W) == 9);
	test_assert(GetReceiver(10, topology, DIRECTION_E) == 11);
	test_assert(GetReceiver(11, topology, DIRECTION_N) == 7);
	test_assert(GetReceiver(11, topology, DIRECTION_S) == 3);
	test_assert(GetReceiver(11, topology, DIRECTION_W) == 10);
	test_assert(GetReceiver(11, topology, DIRECTION_E) == 8);

	// Test neighbors count
	test_assert(CountDirections(0, topology) == 4);
	test_assert(CountDirections(1, topology) == 4);
	test_assert(CountDirections(2, topology) == 4);
	test_assert(CountDirections(3, topology) == 4);
	test_assert(CountDirections(4, topology) == 4);
	test_assert(CountDirections(5, topology) == 4);
	test_assert(CountDirections(6, topology) == 4);
	test_assert(CountDirections(7, topology) == 4);
	test_assert(CountDirections(8, topology) == 4);
	test_assert(CountDirections(9, topology) == 4);
	test_assert(CountDirections(10, topology) == 4);
	test_assert(CountDirections(11, topology) == 4);

	// Sanity check
	test_assert(GetReceiver(5, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(5, topology, DIRECTION_NW) == INVALID_DIRECTION);
	test_assert(GetReceiver(5, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(5, topology, DIRECTION_SW) == INVALID_DIRECTION);

	// Test random receiver
	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 0; j < 12; j++) // 12 is the number of regions in this test
			test_assert(GetReceiver(j, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Test neighbor check
	test_assert(IsNeighbor(0, 1, topology) == true);
	test_assert(IsNeighbor(0, 4, topology) == true);
	test_assert(IsNeighbor(5, 10, topology) == false);
	test_assert(IsNeighbor(11, 8, topology) == true);
	test_assert(IsNeighbor(3, 11, topology) == true);
	test_assert(IsNeighbor(3, 1, topology) == false);

	ReleaseTopology(topology);

	return 0;
}

int test_star(_unused void *_)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_STAR, 5);

	// Sanity check
	test_assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_S) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_E) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);

	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 1; j < 5; j++) // 5 is the number of regions in this test
			assert(GetReceiver(j, topology, DIRECTION_RANDOM) == 0);
		assert(GetReceiver(0, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Check directions count
	test_assert(CountDirections(0, topology) > 1);
	test_assert(CountDirections(0, topology) < CountRegions(topology));
	test_assert(CountDirections(1, topology) == 1);
	test_assert(CountDirections(2, topology) == 1);
	test_assert(CountDirections(3, topology) == 1);
	test_assert(CountDirections(4, topology) == 1);

	// Test neighbor check
	test_assert(IsNeighbor(0, 1, topology) == true);
	test_assert(IsNeighbor(0, 2, topology) == true);
	test_assert(IsNeighbor(0, 3, topology) == true);
	test_assert(IsNeighbor(0, 4, topology) == true);
	test_assert(IsNeighbor(1, 0, topology) == true);
	test_assert(IsNeighbor(2, 0, topology) == true);
	test_assert(IsNeighbor(3, 0, topology) == true);
	test_assert(IsNeighbor(4, 0, topology) == true);
	test_assert(IsNeighbor(1, 2, topology) == false);
	test_assert(IsNeighbor(2, 1, topology) == false);
	test_assert(IsNeighbor(1, 3, topology) == false);
	test_assert(IsNeighbor(3, 1, topology) == false);
	test_assert(IsNeighbor(2, 4, topology) == false);
	test_assert(IsNeighbor(4, 2, topology) == false);

	ReleaseTopology(topology);

	return 0;
}

int test_mesh(_unused void *_)
{
	struct topology *topology = InitializeTopology(TOPOLOGY_FCMESH, 5);

	// Sanity check
	test_assert(GetReceiver(0, topology, DIRECTION_N) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_S) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_W) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_E) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_SE) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_SW) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NE) == INVALID_DIRECTION);
	test_assert(GetReceiver(0, topology, DIRECTION_NW) == INVALID_DIRECTION);

	for(unsigned i = 0; i < RANDOM_TRIALS; i++) {
		for(unsigned j = 1; j < 5; j++) // 5 is the number of regions in this test
			test_assert(GetReceiver(0, topology, DIRECTION_RANDOM) < CountRegions(topology));
	}

	// Check directions count
	test_assert(CountDirections(0, topology) == CountRegions(topology) - 1);
	test_assert(CountDirections(1, topology) == CountRegions(topology) - 1);
	test_assert(CountDirections(2, topology) == CountRegions(topology) - 1);
	test_assert(CountDirections(3, topology) == CountRegions(topology) - 1);
	test_assert(CountDirections(4, topology) == CountRegions(topology) - 1);

	// Test neighbor check
	test_assert(IsNeighbor(0, 1, topology) == true);
	test_assert(IsNeighbor(0, 2, topology) == true);
	test_assert(IsNeighbor(0, 3, topology) == true);
	test_assert(IsNeighbor(0, 4, topology) == true);
	test_assert(IsNeighbor(1, 0, topology) == true);
	test_assert(IsNeighbor(2, 0, topology) == true);
	test_assert(IsNeighbor(3, 0, topology) == true);
	test_assert(IsNeighbor(4, 0, topology) == true);
	test_assert(IsNeighbor(1, 2, topology) == true);
	test_assert(IsNeighbor(2, 1, topology) == true);
	test_assert(IsNeighbor(1, 3, topology) == true);
	test_assert(IsNeighbor(3, 1, topology) == true);
	test_assert(IsNeighbor(2, 4, topology) == true);
	test_assert(IsNeighbor(4, 2, topology) == true);

	ReleaseTopology(topology);

	return 0;
}

#define MAX_NODES_TEST 20
#define NUM_QUERIES 50
int test_graph(_unused void *_)
{
	struct topology *topology;
	unsigned num_edges;
	lp_id_t from, to;

	for(unsigned nodes = 1; nodes < MAX_NODES_TEST; nodes++) {
		topology = InitializeTopology(TOPOLOGY_GRAPH, nodes);

		num_edges = (unsigned)(nodes + (double)test_random_u() / ((double)ULLONG_MAX / (9 * nodes + 1) + 1));
		for(unsigned edge = 0; edge < num_edges; edge++) {
			from = test_random_range(nodes);
			to = test_random_range(nodes);
			test_assert(AddTopologyLink(topology, from, to, test_random_double()));
			test_assert(AddTopologyLink(topology, from, to, 2.0) == false);
			test_assert(AddTopologyLink(topology, from, to, -1.0) == false);
		}

		for(unsigned i = 0; i < NUM_QUERIES; i++) {
			from = test_random_range(nodes);
			to = GetReceiver(from, topology, DIRECTION_RANDOM);

			// We get an invalid direction only if there is a node with no
			// outgoing edges. In this case, we check this condition.
			if(to == INVALID_DIRECTION) {
				test_assert(CountDirections(from, topology) == 0);
				continue;
			}

			test_assert(to < nodes);
			test_assert(IsNeighbor(from, to, topology));
		}

		ReleaseTopology(topology);
	}

	// Test sanity checks on graphs
	topology = InitializeTopology(TOPOLOGY_GRAPH, 1);
	for(enum topology_direction i = 0; i <= LAST_DIRECTION_VALID_VALUE; i++)
		test_assert(GetReceiver(0, topology, i) == INVALID_DIRECTION);
	ReleaseTopology(topology);

	return 0;
}
#undef NUM_QUERIES
#undef MAX_NODES_TEST

int test_init_fini(_unused void *_)
{
	struct topology *topology;
	unsigned par1, par2; // Testing a variadic function, these are the two parameters

	for(enum topology_geometry i = 1; i <= LAST_TOPOLOGY_WITH_TWO_PARAMETERS; i++) {
		test_assert(InitializeTopology(i, 0, 0) == NULL);
		test_assert(InitializeTopology(i, 1, 0) == NULL);
		test_assert(InitializeTopology(i, 0, 1) == NULL);

		par1 = test_random_range(100) + 1;
		par2 = test_random_range(100) + 1;
		topology = InitializeTopology(i, par1, par2);
		test_assert(CountRegions(topology) == par1 * par2);
		test_assert(GetReceiver(par1 * par2 + 1, topology, DIRECTION_E) == INVALID_DIRECTION);
		test_assert(AddTopologyLink(topology, par1, par2, test_random_double()) == false);
		ReleaseTopology(topology);
	}

	for(enum topology_geometry i = LAST_TOPOLOGY_WITH_TWO_PARAMETERS + 1; i <= LAST_TOPOLOGY_VALID_VALUE; i++) {
		test_assert(InitializeTopology(i, 0) == NULL);

		par1 = test_random_range(100) + 1;
		topology = InitializeTopology(i, par1);
		test_assert(CountRegions(topology) == par1);
		test_assert(GetReceiver(par1 + 1, topology, DIRECTION_E) == INVALID_DIRECTION);
		ReleaseTopology(topology);
	}

	// Test if the variadic function detects a wrong number of parameters
	test_assert(InitializeTopology(TOPOLOGY_HEXAGON, 1) == NULL);
	test_assert(InitializeTopology(TOPOLOGY_HEXAGON, 1, 1, 1) == NULL);
	test_assert(InitializeTopology(TOPOLOGY_HEXAGON, 1, 1, 1, 1) == NULL);
	test_assert(InitializeTopology(TOPOLOGY_GRAPH, 1, 1) == NULL);
	test_assert(InitializeTopology(TOPOLOGY_GRAPH, 1, 1, 1) == NULL);
	test_assert(InitializeTopology(TOPOLOGY_GRAPH, 1, 1, 1, 1) == NULL);

	// Test what happens if wrong geometry is passed to InitializeTopology()
	test_assert(InitializeTopology(10 * TOPOLOGY_GRAPH, 1) == NULL);

	return 0;
}

int main(void)
{
	current_lp = test_lp_mock_get();

	test("Topology initialization and release", test_init_fini, NULL);
	test("Hexagon topology", test_hexagon, NULL);
	test("Square topology", test_square, NULL);
	test("Ring topology", test_ring, NULL);
	test("BidRing topology", test_bidring, NULL);
	test("Torus topology", test_torus, NULL);
	test("Star topology", test_star, NULL);
	test("Fully Connected Mesh topology", test_mesh, NULL);
	test("Graph topology", test_graph, NULL);
}
