static ftl_phase last_winner = INIT;

static inline unsigned recompute_ftl_periods(ftl_phase current, unsigned cur_period){
	unsigned res = 0;
		 if(last_winner == INIT)    res = CHALLENGE_PERIODS;
	else if(current == last_winner) res = cur_period + 10;
	else                            res = cur_period * 0.5;
	res = res < CHALLENGE_PERIODS ? CHALLENGE_PERIODS : res;
	last_winner = current;
	return res;
}