#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <map.h>

const char* world = "World";

int main(int argc, char* argv)
{
    Map_t m;
    int ret = init_imap(&m, NULL);
    if (ret == 0)
    {
        int hidx = add_imap(m, 42, (void*)world);
        int fidx = add_imap(m, 14, (void*)"bar");
        char* s = NULL;
        get_imap_by_key(m, 42, (void**)&s);
        printf("%s\n", s);
        del_imap(m, 42);
        s = NULL;
        get_imap_by_key(m, 14, (void**)&s);
        printf("%s\n", s);
        s = NULL;
        get_imap_by_idx(m, fidx, (void**)&s);
        printf("%s\n", s);
        s = NULL;
        hidx = add_imap(m, 42, (void*)world);

        if (hidx >= 0)
        {
            if (get_imap_by_key(m, 42, (void**)&s) == 0)
            {
                printf("%s\n", s);
            }
        }
        int sidx = add_imap(m, 111, (void*)"schnapp");
        destroy_imap(m);
    }
}
