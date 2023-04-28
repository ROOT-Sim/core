#pragma once

#include <test.h>

extern int test_ctrl_msg(bool distributed);
extern void ProcessEvent(_unused lp_id_t me, _unused simtime_t now, _unused unsigned event_type,
    _unused const void *content, _unused unsigned size, _unused void *s);
