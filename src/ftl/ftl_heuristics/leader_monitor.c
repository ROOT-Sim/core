#include <pthread.h>
#include <stdbool.h>
#include <ROOT-Sim.h>
#include <arch/timer.h>
#include <math.h>


pthread_spinlock_t fastmutex;











static volatile bool result = false;
static volatile simtime_t last_gvt = -1;
static volatile timer_uint ftl_ml_wall_clock_timer = 0;

static double ema_speed = -1;
static double ref_speed = -1;
static volatile bool computed = false;

static unsigned global_round = 0;

#define LEADER_MONITOR_TH 0.2

void init_leader_monitor(void){
	pthread_spin_init(&fastmutex, PTHREAD_PROCESS_PRIVATE);
}

void reset_leader_monitor(double current_speed){
	result = false;
	ref_speed = current_speed;
	ema_speed = -1;
	ftl_ml_wall_clock_timer = 0;
}



bool leader_monitor(simtime_t current_gvt){
	__sync_synchronize();
	
	
	if(current_gvt <= 0) goto end; // should never be here

	if(ftl_ml_wall_clock_timer == 0){
		pthread_spin_lock(&fastmutex);
		if(!ftl_ml_wall_clock_timer){
			printf("FIRST ENTER GVT %f\n", current_gvt);
			ftl_ml_wall_clock_timer = timer_new();
			last_gvt = current_gvt;
		}
		pthread_spin_unlock(&fastmutex);
	}

	if(current_gvt <= last_gvt) goto end;
	


	timer_uint current_timer = timer_new();
	double wct_elapsed = ((double)(current_timer - ftl_ml_wall_clock_timer))/1000000;
	double gvt_elapsed = current_gvt - last_gvt;
	double cur_speed = gvt_elapsed/wct_elapsed;

	pthread_spin_lock(&fastmutex);

	if(current_gvt == last_gvt) goto out;

	printf("CUR %f GVT %f - ref %f ema %f cur %f abs diff %.2f th %f \n", last_gvt, current_gvt, ref_speed, ema_speed, cur_speed, fabs(ema_speed - cur_speed)/ema_speed, LEADER_MONITOR_TH);

	if(ema_speed < 0){
		if(cur_speed > 0) ema_speed = cur_speed;
		goto update_initial;
	}

	ema_speed = 0.75*ema_speed + 0.25*cur_speed;
	result = (ema_speed*LEADER_MONITOR_TH) < fabs(ema_speed - ref_speed);
	if(result) printf("Its time to start a challenge ref %f ema %f cur %f abs diff %.2f th %.2f \n", ema_speed, ref_speed, cur_speed, fabs(ema_speed - cur_speed)/ema_speed, LEADER_MONITOR_TH);
	
update_initial:
	ftl_ml_wall_clock_timer = current_timer;
	last_gvt = current_gvt;

out:
	pthread_spin_unlock(&fastmutex);

end:
	return result;
}