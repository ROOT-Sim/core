#include <stdio.h>
#include <math.h>
#include <core/core.h>
#include "nelder_mead.h"
#include "ftl.h"

#define DATAPOINTS 1024

struct datapoint {
	unsigned current;
	unsigned capacity;
	struct data_point_raw *data;
};

static struct datapoint gpu_data = {0,0};
static struct datapoint cpu_data = {0,0};

static const double wall_step_s = 1.0;
static const double min_param = 0.2;
static const double max_param = 0.8;
static const int forecast_steps = 10;
static const double phi = 0.9;


static int resample(struct data_point_raw *a, int len_a, struct data_point_raw *b, int len_b, int max_wall_step,
    double *dp_out)
{
	struct data_point_raw *dp_r[2] = {a, b};
	int len[2] = {len_a, len_b};

	if(len[0] < 2 || len[1] < 2)
		return 1;

	double dp[2][max_wall_step];

	for(int i = 0; i < 2; i++) {
		int idx = 0;
		double prev_gvt = dp_r[i][0].wall_s < wall_step_s ? dp_r[i][0].gvt : 0;
		double prev_gvt_wall_s = 0;
		double prev_speed = 0;
		int prev_idx = -1;

		for(int wall_step = 0; wall_step < max_wall_step; wall_step++) {
			double wall_s = (wall_step + 1) * wall_step_s;
			for(; idx < len[i] - 1 && dp_r[i][idx + 1].wall_s < wall_s; idx++)
				;
			if(idx >= len[i] - 1 || idx == prev_idx) {
				dp[i][wall_step] = prev_speed;
				continue;
			}
			prev_idx = idx;
			double gvt = dp_r[i][idx + 1].gvt;
			// printf("%.2f, %.2f, %.2f, %.2f\n", gvt, prev_gvt, wall_s, prev_gvt_wall_s);
			double speed = (gvt - prev_gvt) / (dp_r[i][idx + 1].wall_s - prev_gvt_wall_s);
			dp[i][wall_step] = speed;

			prev_gvt = gvt;
			prev_gvt_wall_s = dp_r[i][idx + 1].wall_s;
			prev_speed = speed;
		}
	}

	for(int wall_step = 0; wall_step < max_wall_step; wall_step++)
		dp_out[wall_step] = dp[1][wall_step] - dp[0][wall_step];

	for(int wall_step = 0; wall_step < max_wall_step; wall_step++) {
		for(int i = 0; i < 2; i++) {
			printf("%.2f: %.2f, %.2f; %.2f\n", (wall_step + 1) * wall_step_s, dp[0][wall_step],
			    dp[1][wall_step], dp_out[wall_step]);
		}
	}
	return 0;
}

static double clamp(double p)
{
	return min_param + fabs(fmod(p, 1)) * (max_param - min_param);
}

static double compute_stddev(int len, double *samples)
{
	double sum = 0.0;
	for(int i = 0; i < len; i++)
		sum += samples[i];
	double mean = sum / len;

	sum = 0.0;
	for(int i = 0; i < len; ++i)
		sum += pow(samples[i] - mean, 2);

	double variance = sum / len;
	return sqrt(variance);
}

static double norm_cdf(double x, double mean, double stddev)
{
	return 0.5 * (1 + erf((x - mean) / (stddev * sqrt(2.0))));
}

static double forecast(double alpha, double beta, int max_wall_step, double dp[max_wall_step], int train)
{
	alpha = clamp(alpha);
	beta = clamp(beta);

	double ss[max_wall_step], bs[max_wall_step];

	ss[0] = dp[0];
	bs[0] = dp[1] - dp[0];

	if(!train)
		printf("wall_step,ss,dp\n");

	for(int wall_step = 1; wall_step < max_wall_step; wall_step++) {
		ss[wall_step] = alpha * dp[wall_step] + (1 - alpha) * (ss[wall_step - 1] + phi * bs[wall_step - 1]);
		bs[wall_step] = beta * (ss[wall_step] - ss[wall_step - 1]) + (1 - beta) * phi * bs[wall_step - 1];
		if(!train)
			printf("%d,%.2f,%.2f\n", wall_step, ss[wall_step], dp[wall_step]);
	}

	double err = 0.0;
	double errs[max_wall_step - forecast_steps];

	for(int last_data_step = 0; last_data_step < max_wall_step - forecast_steps - 1; last_data_step++) {
		double phi_sum = 0;
		double curr_step_err = 0;
		for(int h = 1; h < forecast_steps + 1; h++) {
			phi_sum += pow(phi, h);
			double forecast = ss[last_data_step] + phi_sum * bs[last_data_step];
			double curr_h_err = dp[last_data_step + h] - forecast;
			err += curr_h_err;
			// printf("error at %d + %d: %.2f - %.2f = %.2f\n", last_data_step, h, dp[last_data_step + h],
			// forecast, fabs(dp[last_data_step + h] - forecast));
			curr_step_err += curr_h_err;
		}
		errs[last_data_step] = curr_step_err;
	}

	printf("total err: %.2f, per window: %.2f\n", err, err / (max_wall_step - forecast_steps - 1));

	if(train)
		return -fabs(err); // nm implementation maximizes

	double err_stddev = compute_stddev(max_wall_step - forecast_steps, errs);

	int last_data_step = max_wall_step - 1;
	double phi_sum = 0;
	double sum_forecast = 0;
	for(int h = 1; h < forecast_steps + 1; h++) {
		phi_sum += pow(phi, h);
		sum_forecast += ss[last_data_step] + phi_sum * bs[last_data_step];
		// printf("forecast for step %d is %.2f\n", last_data_step, ss[last_data_step] + phi_sum *
		// bs[last_data_step]);
	}

	double scale = wall_step_s / forecast_steps;
	printf(
	    "forecast over the next %.2f wall seconds is a difference of %.2f gvt units per wall second with stddev %.2f\n",
	    wall_step_s * forecast_steps, sum_forecast * scale, err_stddev * scale);

	double a_faster_prob = 1 - norm_cdf(sum_forecast, 0.0, err_stddev);
	// printf("a is faster with a probability of %.2f\n", a_faster_prob);

	return a_faster_prob;
}

static double cmp_speeds(struct data_point_raw *a, int len_a, struct data_point_raw *b, int len_b)
{
	int max_wall_step = max(a[len_a - 1].wall_s, b[len_b - 1].wall_s) / wall_step_s;
	double dp[max_wall_step];

	if(resample(a, len_a, b, len_b, max_wall_step, dp))
		return 0.0;

	nm_init(forecast, max_wall_step, dp);
	double alpha, beta;
	nm_optimize(&alpha, &beta);

	return forecast(alpha, beta, max_wall_step, dp, 0);
}

bool is_cpu_faster(void)
{
	if(cpu_data.current == 0) {
		printf("Not enough CPU data!\n");
		return false;
	}
	if(gpu_data.current == 0) {
		printf("Not enough GPU data!\n");
		return true;
	}
	return 0.5 <= cmp_speeds(cpu_data.data, cpu_data.current, gpu_data.data, gpu_data.current);
}


void register_cpu_data(double wall_s, double gvt)
{
	
	if(cpu_data.capacity == cpu_data.current){
		unsigned size = !cpu_data.capacity ? DATAPOINTS : cpu_data.capacity*2;
		cpu_data.capacity = size;
		cpu_data.data = realloc(cpu_data.data, sizeof(struct data_point_raw)*size);
	}
	

	
	printf("\nRegistering CPU data: wall %f gvt %f", wall_s, gvt);
    cpu_data.data[cpu_data.current].wall_s = wall_s;
    cpu_data.data[cpu_data.current].gvt = gvt;
    cpu_data.current++;
}

void register_gpu_data(double wall_s, double gvt)
{
	if(gpu_data.capacity == gpu_data.current){
		unsigned size = !gpu_data.capacity ? DATAPOINTS : gpu_data.capacity*2;
		gpu_data.capacity = size;
		gpu_data.data = realloc(gpu_data.data, sizeof(struct data_point_raw)*size);
	}
	
	
	printf("\nRegistering GPU data: wall %f gvt %f", wall_s, gvt);

    gpu_data.data[gpu_data.current].wall_s = wall_s;
    gpu_data.data[gpu_data.current].gvt = gvt;
    gpu_data.current++;
}

void reset_ftl_series(void)
{
	gpu_data.current = 0;
	cpu_data.current = 0;
}

/*
int main()
{
  const int num_data_points = 100;
  struct data_point_raw dp_a[num_data_points];
  struct data_point_raw dp_b[num_data_points];
  double t_a = 0, t_b = 0;
  double gvt_a = 0, gvt_b = 0;
  srand(time(NULL));
  for (int i = 0; i < num_data_points; i++) {
    double delta_t_a = 1.0 + 2 * (double)rand() / RAND_MAX;
    double delta_t_b = 1.0 + 10 * (double)rand() / RAND_MAX;
    t_a += delta_t_a;
    t_b += delta_t_b;
    gvt_a += delta_t_a * (2.0 + 0.1 * (double)rand() / RAND_MAX + 5e-1 * sin(i / 3));
    gvt_b += delta_t_b * (2.2 + 0.1 * (double)rand() / RAND_MAX);
    dp_a[i].wall_s = t_a;
    dp_a[i].gvt = gvt_a;

    dp_b[i].wall_s = t_b;
    dp_b[i].gvt = gvt_b;
  }

  double a_faster_prob = cmp_speeds(dp_a, num_data_points, dp_b, num_data_points);
  printf("a is faster with probability %.2f\n", a_faster_prob);

  return 0;
}
*/
