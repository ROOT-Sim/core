#pragma once

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define visible __attribute__ ((dllexport))
#else
#define visible __declspec(dllexport)
#endif

#ifdef __GNUC__
#define want_visible __attribute__ ((dllimport))
#else
#define want_visible __declspec(dllimport)
#endif
#else
#define want_visible
#if __GNUC__ >= 4
#define visible __attribute__ ((visibility ("default")))
#else
#define visible
#endif
#endif
