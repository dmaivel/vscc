#include <util/list.h>
#include <stdio.h>

struct __attribute__((__packed__)) stuff {
    int number;
    struct stuff *next;
};

static bool matcher(void *element_ptr, void *data) 
{
    struct stuff *element = element_ptr;
    int to_match = *(int*)data;

    return (element->number == to_match);
}

int main()
{
    struct stuff *list = NULL;

    struct stuff *elem_1 = vscc_list_alloc((void**)&list, 4, sizeof(struct stuff));
    struct stuff *elem_2 = vscc_list_alloc((void**)&list, 4, sizeof(struct stuff));
    struct stuff *elem_3 = vscc_list_alloc((void**)&list, 4, sizeof(struct stuff));
    
    elem_1->number = 123;
    elem_2->number = 456;
    elem_3->number = 987;

    printf("before: ");
    struct stuff *indexed = list;
    for (int i = 0; i < 3; i++) {
        printf("[%d]", indexed->number);
        indexed = indexed->next;
    }
    puts("");

    int to_remove = 123;
    vscc_list_free_element((void**)&list, 4, matcher, &to_remove);
    fflush(stdout);

    printf("\nafter: ");
    indexed = list;
    for (int i = 0; i < 2; i++) {
        printf("[%d]", indexed->number);
        indexed = indexed->next;
    }
    puts("");

    vscc_list_free((void**)&list, 4);
}