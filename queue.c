#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *new = malloc(sizeof(struct list_head));
    if (!new)
        return NULL;

    INIT_LIST_HEAD(new);
    return new;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    element_t *e, *s;
    if (!head)
        return;

    list_for_each_entry_safe (e, s, head, list) {
        q_release_element(e);
    }

    free(head);
}


/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    element_t *new = malloc(sizeof(element_t));
    if (!new)
        return false;

    new->value = strdup(s);
    if (!new->value) {
        free(new);
        return false;
    }

    list_add(&new->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    return q_insert_head(head->prev, s);
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    struct list_head *rm = head->next;
    list_del(rm);

    if (sp) {
        strncpy(sp, list_entry(rm, element_t, list)->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }

    return list_entry(rm, element_t, list);
}
/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    return q_remove_head(head->prev->prev, sp, bufsize);
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;

    list_for_each (li, head)
        len++;
    return len;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;

    struct list_head *slow, *fast;

    fast = head;
    list_for_each (slow, head) {
        if (fast->next == head || fast->next->next == head)
            break;
        fast = fast->next->next;
    }

    list_del(slow);
    q_release_element(list_entry(slow, element_t, list));
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    element_t *e, *s;
    struct list_head garbage;

    INIT_LIST_HEAD(&garbage);
    bool is_this_dup = 0;
    list_for_each_entry_safe (e, s, head, list) {
        bool is_next_dup = s != list_entry(head, element_t, list) &&
                           strcmp(e->value, s->value) == 0;
        if (is_next_dup || is_this_dup) {
            list_del(&e->list);
            list_add(&e->list, &garbage);
        }
        is_this_dup = is_next_dup;
    }

    list_for_each_entry_safe (e, s, &garbage, list) {
        q_release_element(e);
    }

    return true;
}


/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    q_reverseK(head, 2);
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    struct list_head *li, *sa;
    list_for_each_safe (li, sa, head)
        list_move(li, head);
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    if (k <= 0)
        return;
    // https://leetcode.com/problems/reverse-nodes-in-k-group/

    struct list_head result, tmp;
    INIT_LIST_HEAD(&result);
    INIT_LIST_HEAD(&tmp);


    while (!list_empty(head)) {
        // CHECK: if k == 0;

        struct list_head *p = head->next;
        for (int i = 1; i < k && p != head; i++) {
            p = p->next;
        }

        list_cut_position(&tmp, head, (p == head) ? p->prev : p);
        if (p != head)
            q_reverse(&tmp);

        list_splice_tail(&tmp, &result);
    }

    list_splice(&result, head);
}

static int element_t_cmp(struct list_head *a, struct list_head *b, bool descend)
{
    char *a_value = list_entry(a, element_t, list)->value;
    char *b_value = list_entry(b, element_t, list)->value;

    return -(strcmp(a_value, b_value) ^ (descend << 31));
}

static inline size_t run_size(struct list_head *run)
{
    if (!run)
        return 0;
    if (!run->next)
        return 1;

    return (size_t) run->next->prev;
}

static struct list_head *find_run(struct list_head **list, bool descend)
{
    struct list_head *next = (*list)->next, *l = *list;

    if (!next) {
        *list = NULL;
        return l;
    }

    struct list_head tmp;
    INIT_LIST_HEAD(&tmp);
    list_add(l, &tmp);
    size_t len = 0;

    if (element_t_cmp(l, next, descend) > 0) {
        /* don't need to reverse */
        do {
            l = next;
            next = next->next;
            list_add_tail(l, &tmp);
            len++;
        } while (next && element_t_cmp(l, next, descend) > 0);
    } else {
        do {
            l = next;
            next = next->next;
            list_add(l, &tmp);
            len++;
        } while (next && element_t_cmp(l, next, descend) <= 0);
    }

    tmp.next->prev = tmp.prev->next = NULL;
    tmp.next->next->prev = (struct list_head *) len;
    *list = next;
    return tmp.next;
}

static struct list_head *merge_at(struct list_head *list, bool descend)
{
    size_t len = run_size(list) + run_size(list->prev);
    struct list_head *prev = list->prev->prev;

    struct list_head *a = list, *b = list->prev;
    struct list_head **tail = &list;

    for (;;) {
        if (element_t_cmp(a, b, descend) > 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }

    list->prev = prev;
    list->next->prev = (struct list_head *) len;

    return list;
}

static struct list_head *merge_force_collapse(struct list_head *stk,
                                              bool descend)
{
    while (stk && stk->prev) {
        struct list_head **iptr = &stk;
        while ((*iptr) && (*iptr)->prev) {
            (*iptr) = merge_at(*iptr, descend);
            iptr = &(*iptr)->prev;
        }
    }
    return stk;
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    /* Timsort
     * refer to 2024q1 test2 in: https://hackmd.io/@sysprog/linux2024-quiz1
     */
    if (!head || head == head->next)
        return;

    /* convert circular-lined list into singly-linked list */
    struct list_head *list = head->next, *stk_ptr = NULL, *tmp;
    list_del_init(head);
    list->prev = list->prev->next = NULL;

    int stk_size = 0;
    while (list) {
        tmp = find_run(&list, descend);
        stk_size++;
        tmp->prev = stk_ptr;
        stk_ptr = tmp;
        // stk_ptr = merge_collapse(stk_ptr, &stk_size, descend);
    }

    list = merge_force_collapse(stk_ptr, descend);

    for (struct list_head *safe = list->next;;) {
        list_add_tail(list, head);
        list = safe;
        if (!list)
            break;
        safe = safe->next;
    }
}


/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    struct list_head *n, *safe, *min = head->prev;
    int size = 1;
    for (n = min->prev, safe = n->prev; n != head;
         n = safe, safe = safe->prev) {
        if (strcmp(list_entry(n, element_t, list)->value,
                   list_entry(min, element_t, list)->value) < 0) {
            size++;
            min = n;
        } else {
            list_del(n);
            q_release_element(list_entry(n, element_t, list));
        }
    }
    return size;
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    struct list_head *n, *safe, *max = head->prev;
    int size = 1;
    for (n = max->prev, safe = n->prev; n != head;
         n = safe, safe = safe->prev) {
        if (strcmp(list_entry(n, element_t, list)->value,
                   list_entry(max, element_t, list)->value) > 0) {
            size++;
            max = n;
        } else {
            list_del(n);
            q_release_element(list_entry(n, element_t, list));
        }
    }
    return size;
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
static void merge_list(struct list_head *to,
                       struct list_head *from,
                       bool descend)
{
    struct list_head *list, *safe, *cmp_ptr = to->next;
    if (list_empty(from))
        return;

    list_for_each_safe (list, safe, from) {
        while (cmp_ptr != to && element_t_cmp(cmp_ptr, list, descend) > 0) {
            cmp_ptr = cmp_ptr->next;
        }

        if (cmp_ptr == to)
            break;

        list_del(list);
        list_add_tail(list, cmp_ptr);
    }

    list_splice_tail_init(from, to);
}

int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    struct list_head *f = head->next, *m = f->next;

    for (; f != head && m != head; m = m->next) {
        queue_contex_t *to = list_entry(f, queue_contex_t, chain);
        queue_contex_t *from = list_entry(m, queue_contex_t, chain);
        merge_list(to->q, from->q, descend);
    }

    return q_size(list_entry(f, queue_contex_t, chain)->q);
}
