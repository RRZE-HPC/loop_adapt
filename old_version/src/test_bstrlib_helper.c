#include <stdlib.h>
#include <stdio.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>



int main(int argc, char* argv[])
{
	struct bstrList* sl = bstrListCreate();

	
	bstrListAddChar(sl, "Hello");
	bstrListAddChar(sl, "World");

	bstrListPrint(sl);
	bstrListDel(sl, 0);
	bstrListPrint(sl);
	bstrListDel(sl, 0);
	bstrListPrint(sl);
	bstrListAddChar(sl, "Hello");
	bstrListAddChar(sl, "World");
	bstrListPrint(sl);

	bstrListDestroy(sl);

	return 0;
}