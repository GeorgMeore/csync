#ifndef CORO_INCLUDED
#define CORO_INCLUDED

#include <pthread.h>

/* Some dirty macro tricks */

#define CORO_BEGIN2(name, p1, p2) \
struct name##args { p1; p2; };      \
void *name(struct name##args *args) \
{                                   \
	p1; p2;

#define CORO_BEGIN1(name, p1) \
struct name##args { p1; };      \
void *name(struct name##args *args) \
{                                   \
	p1;

#define CORO_SETARGS1(n1) \
	n1 = args->n1;       \
	free(args);

#define CORO_SETARGS2(n1, n2) \
	n1 = args->n1;       \
	n2 = args->n2;       \
	free(args);

#define CORO_END \
	return NULL; \
}

#define CORO_START(name, argl...) { \
	pthread_t handle;                                             \
	struct name##args *args = malloc(sizeof(*args));              \
	*args = (struct name##args){argl};                            \
	pthread_create(&handle, NULL, (void *(*)(void *))name, args); \
	pthread_detach(handle);                                       \
}

#endif /* CORO_INCLUDED */
