#ifndef BSTRLIB_HELPER_INCLUDE
#define BSTRLIB_HELPER_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif


int bstrListAdd(struct bstrList * sl, bstring str);
int bstrListAddChar(struct bstrList * sl, char* str);

int bstrListDel(struct bstrList * sl, int idx);

bstring bstrListGet(struct bstrList * sl, int idx);

void bstrListPrint(struct bstrList * sl);

#ifdef __cplusplus
}
#endif

#endif /* BSTRLIB_HELPER_INCLUDE */