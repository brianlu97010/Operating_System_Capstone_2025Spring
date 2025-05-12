#ifndef _LIST_H
#define _LIST_H

#include "types.h"

/*
 * Circular doubly linked list implementation.
 * Refer to : `include/linux/list.h` in linux source tree
 * 
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */


/* Circular doubly linked list structure */
struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

/**
 * INIT_LIST_HEAD - Initialize a list_head structure
 * @list: list_head structure to be initialized.
 *
 * Initializes the list_head to point to itself.  If it is a list header,
 * the result is an empty list.
 */
static inline void INIT_LIST_HEAD(struct list_head *list){
    list->next = list;
    list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new_list, struct list_head *prev, struct list_head *next)
{
    next->prev = new_list;
    new_list->next = next;
    new_list->prev = prev;
    prev->next = new_list;
}

/**
* list_add - add a new entry
* @new: new entry to be added
* @head: list head to add it after
*
* Insert a new entry after the specified head.
*/
static inline void list_add(struct list_head *new_list, struct list_head *head)
{
    __list_add(new_list, head, head->next); 
}

/**
 * list_add_tail - add a new entry to the end of the list
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head, meaning at the tail of the list.
 */
static inline void list_add_tail(struct list_head *new_list, struct list_head *head)
{
    __list_add(new_list, head->prev, head);
}


static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 */
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = (struct list_head *)NULL;
    entry->prev = (struct list_head *)NULL;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head){
    return head->next == head;
}

/**
 * Computes the byte offset of a member within a structure.
 * It pretends the structure starts at address 0, and gets the address of the member.
 * This works because we don't actually access memory—just compute the offset.
 */
#define offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)

/**
 * Given a pointer to a struct member (typically a list node),
 * this macro returns the pointer to the structure that contains it.
 * It subtracts the offset of the member from the member's address to get the structure's base address.
 */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/**
 * Returns the first entry in a linked list.
 */
#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

/**
 * list_for_each - iterate over a list
 * @pos: the &struct list_head to use as a loop cursor.
 * @head: the head for your list.
 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos: the &struct list_head to use as a loop cursor.
 * @n: another &struct list_head to use as temporary storage
 * @head: the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

#endif