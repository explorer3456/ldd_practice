#include <stdio.h>

struct some_data {
	int a;
	char b;
	char c;
	int d;
};

void * get_my_container(void *member_addr)
{
	long *ptr;
	long diff;
	long *base_addr;

	ptr = &(((struct some_data *)0)->c);
	diff = (long)ptr;

	base_addr = (long)member_addr - diff;

	return base_addr;
}

#define my_container_of( ptr, name ,type) ({	\
		size_t offset = (size_t)&(((type *)0)->name); \
		(type *)((void *)ptr - offset); \
		})

int main(void)
{
	struct some_data _data;

	struct some_data temp_data;
	long diff;
	long *ptr;

	struct some_data *temp_ptr;

	_data.a = 2;
	_data.b = '5';
	_data.c = 1;
	_data.d = 99;

	diff = ((long)&temp_data.c - (long)&temp_data);

	printf("diff: %ld\n", diff);

	printf("_data.c address : %p\n", &_data.c);

	ptr = (long)&_data.c - (long)diff;

	printf("ptr address: %p\n", ptr);

	printf("a is : %d\n", *(int *)ptr);

//	temp_ptr = get_my_container( & _data.c);
	temp_ptr = my_container_of( &_data.c, c, struct some_data );

	printf("new a is : %d\n", temp_ptr->a);
	
	return 0;
}
