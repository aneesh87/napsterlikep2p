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

int insertrfc (struct rfclist ** rfcdb, struct rfclist * newrfc) {
	if (*rfcdb == NULL) {
		*rfcdb = newrfc;
		return 0;
	}
	struct rfclist * tmp = *rfcdb;
	while (tmp) {
		if (tmp->rfcnum == newrfc->rfcnum) {
			return -1;
		}
		tmp = tmp->next;
	}
	newrfc->next = *rfcdb;
	*rfcdb = newrfc;
	return 0;
}
