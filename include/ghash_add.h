#ifndef __G_HASH_ADD_H__
#define __G_HASH_ADD_H__

GHashTable * g_hash_table_new_full (GHashFunc      hash_func,
				    GEqualFunc     key_equal_func,
				    GDestroyNotify key_destroy_func,
				    GDestroyNotify value_destroy_func);
#endif
