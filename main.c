#include <stdio.h>
#include <unistd.h>

#include "chan.h"
#include "coro.h"

CORO2(foo, CHAN_OF(int) *in, CHAN_OF(int) *out)
	int x, y;
	SETARGS2(in, out);
	printf("starting a foo loop\n");
	for (;;) {
		if (chtryrecv(in, &x)) {
			printf("foo received: %d\n", x);
			y = x + 10;
			chsend(out, &y);
		} else {
			printf("foo sleeping\n");
			sleep(1);
		}
	}
	printf("exited a foo loop\n");
}

int main(void)
{
	chan in, out;
	int x, y;
	if (!CHAN_INIT(in, int) || !CHAN_INIT(out, int)) {
		return 1;
	}
	START(foo, &in, &out);
	printf("starting the main loop\n");
	while (scanf(" %d", &x) == 1) {
		chsend(&in, &x);
		chrecv(&out, &y);
		printf("%d\n", y);
	}
	printf("exited the main loop\n");
	return 0;
}
