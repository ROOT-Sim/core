#include <lib/topology/topology.h>

#include <lib/lib_internal.h>

#include <math.h>

struct _topology_global_t topology_global;

/**
 * Utility function which returns the expected number of neighbours
 * depending on the geometry of the topology.
 */
static unsigned directions_count(void)
{
	switch (topology_global.geometry) {
		case TOPOLOGY_MESH:
			return topology_global.regions_cnt - 1;
		case TOPOLOGY_HEXAGON:
			return 6;
		case TOPOLOGY_TORUS:
		case TOPOLOGY_SQUARE:
			return 4;
		case TOPOLOGY_STAR:
			return 2;
		case TOPOLOGY_RING:
			return 1;
		case TOPOLOGY_BIDRING:
			return 2;
	}
	return UINT_MAX;
}

/**
 * This pre-computes the edge of the topology;
 * the <sqrt>"()" is expensive and so we cache its value
 */
static void compute_edge(void)
{
	unsigned edge;
	const unsigned lp_cnt = topology_global.regions_cnt;
	switch (topology_global.geometry) {
		case TOPOLOGY_SQUARE:
		case TOPOLOGY_HEXAGON:
		case TOPOLOGY_TORUS:
			edge = sqrt(lp_cnt);
			// we make sure there are no "lonely" LPs
			if(edge * edge != lp_cnt)
				rootsim_error(true, "Invalid number of regions for this topology geometry (must be a square number)\n");
			break;
		default:
			// the edge value is actually unused
			edge = 0;
			break;
	}
	// set the edge value
	topology_global.edge = edge;
}

/**
 * Initialize the topology module for each LP hosted on the machine.
 * This needs to be called right after LP basic initialization before starting to process events.
 */
void topology_global_init(uint64_t lp_cnt)
{
	if(!&topology_settings)
		// the weak symbol isn't defined: we aren't needed
		return;

	// set default values
	topology_global.regions_cnt = lp_cnt - topology_settings.out_of_topology;
	topology_global.geometry = topology_settings.default_geometry;
	topology_global.directions = directions_count();
	// compute the edge value for topologies it makes sense for
	compute_edge();
}

unsigned int RegionsCount(void)
{
	return topology_global.regions_cnt;
}

unsigned int DirectionsCount(void)
{
	return topology_global.directions;
}

unsigned int GetReceiver(unsigned int from, direction_t direction)
{
	const unsigned sender = from;
	const unsigned edge = topology_global.edge;
	const unsigned regions_cnt = topology_global.regions_cnt;
	unsigned x, y;

	if(unlikely(regions_cnt <= from))
		return DIRECTION_INVALID;

	switch (topology_global.geometry) {

		case TOPOLOGY_HEXAGON:
			y = sender / edge;
			x = sender - y * edge;

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
				default:
					return DIRECTION_INVALID;
			}
			return (x < edge && y < edge) ? y * edge + x : DIRECTION_INVALID;

		case TOPOLOGY_SQUARE:
			y = sender / edge;
			x = sender - y * edge;

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
				default:
					return DIRECTION_INVALID;
			}
			return (x < edge && y < edge) ? y * edge + x : DIRECTION_INVALID;

		case TOPOLOGY_TORUS:
			y = sender / edge;
			x = sender - y * edge;

			switch (direction) {
				case DIRECTION_N:
					y += edge - 1;
					y %= edge;
					break;
				case DIRECTION_S:
					y += 1;
					y %= edge;
					break;
				case DIRECTION_E:
					x += 1;
					x %= edge;
					break;
				case DIRECTION_W:
					x += edge - 1;
					x %= edge;
					break;
				default:
					return DIRECTION_INVALID;
			}
			return y * edge + x;

		case TOPOLOGY_MESH:
			return likely(direction < regions_cnt) ? direction : DIRECTION_INVALID;

		case TOPOLOGY_BIDRING:
			switch (direction) {
				case DIRECTION_E:
					return (sender + 1) % regions_cnt;
				case DIRECTION_W:
					return (sender + regions_cnt - 1) % regions_cnt;
				default:
			}
			return DIRECTION_INVALID;

		case TOPOLOGY_RING:
			return likely(direction == DIRECTION_E) < regions_cnt ? direction : DIRECTION_INVALID;

		case TOPOLOGY_STAR:
			if(sender) {
				if(!direction)
					return 0;
			} else {
				if(direction + 1 < regions_cnt)
					return direction + 1;
			}
			return DIRECTION_INVALID;

		default:
	}
	return DIRECTION_INVALID;
}

unsigned FindReceiver(void)
{

	return receiver;
}
