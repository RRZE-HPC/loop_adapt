

#include <stdio.h>
#include <stdlib.h>


//#include <unistd.h>
//#include <string.h>

#include <bstrlib.h>


int bstrListAdd(struct bstrList * sl, bstring str)
{
	if (sl->qty >= sl->mlen) {
        int mlen = sl->mlen * 2;
        bstring * tbl;
    

	    while (sl->qty >= mlen) {
	        if (mlen < sl->mlen) return BSTR_ERR;
	        mlen += mlen;
	    }

	    tbl = (bstring *) realloc (sl->entry, sizeof (bstring) * mlen);
	    if (tbl == NULL) return BSTR_ERR;

	    sl->entry = tbl;
	    sl->mlen = mlen;
	}
	sl->entry[sl->qty] = bstrcpy(str);
    sl->qty++;
    return BSTR_OK;
}

int bstrListAddChar(struct bstrList * sl, char* str)
{
	if (!sl || !str) return BSTR_ERR;
	bstring tmp = bformat("%s", str);
	int err = bstrListAdd(sl, tmp);
	bdestroy(tmp);
	return err;
}

void bstrListPrint(struct bstrList * sl)
{
	int i = 0;
	if (!sl) return;
	if (sl->qty > 0)
	{
		printf("[%s", bdata(sl->entry[0]));
		for (i = 1; i < sl->qty; i++)
		{
			printf(", %s", bdata(sl->entry[i]));
		}
		printf("]\n");
	}
	else if (sl->qty == 0)
	{
		printf("[]\n");
	}
}

int bstrListDel(struct bstrList * sl, int idx)
{
	int i;

	if (!sl || idx < 0 || idx >= sl->qty) return BSTR_ERR;

	bdestroy(sl->entry[idx]);

	for (i = idx+1; i < sl->qty; i++)
	{
		sl->entry[i-1] = bstrcpy(sl->entry[i]);
	}
	sl->qty--;

	return BSTR_OK;
}

bstring bstrListGet(struct bstrList * sl, int idx)
{
	if (!sl || idx < 0 || idx >= sl->qty) return NULL;
	return sl->entry[idx];
}



