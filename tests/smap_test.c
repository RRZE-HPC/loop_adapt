#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <map.h>

int main(int argc, char* argv)
{
    Map_t m;
    //int ret = init_map(&m, MAP_KEY_TYPE_STR, 0, NULL);
    //int ret = init_map(&m, MAP_KEY_TYPE_BSTR, 0, NULL);
    int ret = init_smap(&m, NULL);
    if (ret == 0)
    {
        int hidx = add_smap(m, "Hello", (void*)"World");
        int fidx = add_smap(m, "foo", (void*)"bar");
        char* s = NULL;
        get_smap_by_key(m, "Hello", (void**)&s);
        printf("%s\n", s);
        del_smap(m, "Hello");
        s = NULL;
        get_smap_by_key(m, "foo", (void**)&s);
        printf("%s\n", s);
        s = NULL;
        get_smap_by_idx(m, fidx, (void**)&s);
        printf("%s\n", s);
        hidx = add_smap(m, "Hello", (void*)"World");
        get_smap_by_key(m, "Hello", (void**)&s);
        printf("%s\n", s);
        int sidx = add_smap(m, "schnipp", (void*)"schnapp");
        destroy_smap(m);
    }
}
