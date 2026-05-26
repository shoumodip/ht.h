#include <stdio.h>

#define HT_IMPLEMENTATION
#include "ht.h"

int main(void) {
    HT(const char *, int) ht = {.hasheq = ht_hasheq_cstr};

    ht_set(&ht, "foo", 69);
    ht_set(&ht, "bar", 420);
    ht_set(&ht, "baz", 1337);

    ht_foreach(my, &ht) {
        printf("%s => %d\n", *my.key, *my.value);
    }

    ht_delete(&ht, "foo");
    printf("\nfoo => %p\n", ht_get(&ht, "foo"));

    ht_free(&ht);
    return 0;
}
