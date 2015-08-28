#include <stdio.h>
int main(int argc,char *argv[])
{
	int i, j;

	while(EOF != scanf("%d%d", &i, &j))
		printf("%d -- %d\n", i, j);

	return 0;
}
