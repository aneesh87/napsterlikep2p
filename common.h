#define true 1
#define false 0

#define MAX_NAME_LEN 255
#define MAX_TITLE 1024

#define MAX_BUFFER_SIZE 2048
#define LARGE_BUFFER_SIZE 65536

#define LOCK(X) pthread_mutex_lock(&X);
#define UNLOCK(X) pthread_mutex_unlock(&X);

struct rfclist {
	int rfcnum;
	char title[MAX_TITLE];
	struct rfclist * next;
};
