#include <stdio.h>

struct some_data {
	int a;
	char b;
	char c;
	int d;
};

int main(void)
{
	struct some_data _data;

	struct some_data temp_data;
	long diff;
	long *ptr;

	_data.a = 2;
	_data.b = '5';
	_data.c = 1;
	_data.d = 99;

	diff = ((long)&temp_data.c - (long)&temp_data.a);

	printf("diff: %ld\n", diff);

	printf("_data.c address : %p\n", &_data.c);

	ptr = (long)&_data.c - (long)diff;

	printf("ptr address: %p\n", ptr);

	printf("a is : %d\n", *(int *)ptr);
	
	return 0;
}
