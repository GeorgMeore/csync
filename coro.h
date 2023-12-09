#ifndef CORO_INCLUDED
#define CORO_INCLUDED

#include <pthread.h>
#include <stdlib.h>

/* Some dirty macro tricks */

#define DEFINE_CORO1(name, t1, n1) \
struct name##args { t1 n1; };               \
void name(t1 n1);                           \
void *name##wrap(struct name##args *args) { \
	name(args->n1);                         \
	free(args);                             \
	return NULL;                            \
}                                           \
void name(t1 n1)

#define DEFINE_CORO2(name, t1, n1, t2, n2) \
struct name##args { t1 n1; t2 n2; };        \
void name(t1 n1, t2 n2);                    \
void *name##wrap(struct name##args *args) { \
	name(args->n1, args->n2);               \
	free(args);                             \
	return NULL;                            \
}                                           \
void name(t1 n1, t2 n2)

#define RUN_CORO(name, argl...) { \
	pthread_t handle;                                                   \
	struct name##args *args = malloc(sizeof(*args));                    \
	*args = (struct name##args){argl};                                  \
	pthread_create(&handle, NULL, (void *(*)(void *))name##wrap, args); \
	pthread_detach(handle);                                             \
}

#endif /* CORO_INCLUDED */
