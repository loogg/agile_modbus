/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-16     Bernard      the first version
 * 2006-09-07     Bernard      move the kservice APIs to rtthread.h
 * 2007-06-27     Bernard      fix the rt_list_remove bug
 * 2012-03-22     Bernard      rename kservice.h to rtservice.h
 * 2017-11-15     JasonJia     Modify rt_slist_foreach to rt_slist_for_each_entry.
 *                             Make code cleanup.
 * 2021-01-26     Loogg        Move to Linux
 */

#ifndef __RT_SERVICE_H__
#define __RT_SERVICE_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup KernelService
 */

/**@{*/

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define RT_ALIGN(size, align) (((size) + (align)-1) & ~((align)-1))

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define RT_ALIGN_DOWN(size, align) ((size) & ~((align)-1))

/**
 * Double List structure
 */
struct rt_list_node {
    struct rt_list_node *next; /**< point to next node. */
    struct rt_list_node *prev; /**< point to prev node. */
};
typedef struct rt_list_node rt_list_t; /**< Type for lists. */

/**
 * Single List structure
 */
struct rt_slist_node {
    struct rt_slist_node *next; /**< point to next node. */
};
typedef struct rt_slist_node rt_slist_t; /**< Type for single list. */

/**
 * rt_container_of - return the member address of ptr, if the type of ptr is the
 * struct type.
 */
#define rt_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))

/**
 * @brief initialize a list object
 */
#define RT_LIST_OBJECT_INIT(object) \
    {                               \
        &(object), &(object)        \
    }

/**
 * @brief initialize a list
 *
 * @param l list to be initialized
 */
static __inline void rt_list_init(rt_list_t *l)
{
    l->next = l->prev = l;
}

/**
 * @brief insert a node after a list
 *
 * @param l list to insert it
 * @param n new node to be inserted
 */
static __inline void rt_list_insert_after(rt_list_t *l, rt_list_t *n)
{
    l->next->prev = n;
    n->next = l->next;

    l->next = n;
    n->prev = l;
}

/**
 * @brief insert a node before a list
 *
 * @param n new node to be inserted
 * @param l list to insert it
 */
static __inline void rt_list_insert_before(rt_list_t *l, rt_list_t *n)
{
    l->prev->next = n;
    n->prev = l->prev;

    l->prev = n;
    n->next = l;
}

/**
 * @brief remove node from list.
 * @param n the node to remove from the list.
 */
static __inline void rt_list_remove(rt_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = n;
}

/**
 * @brief tests whether a list is empty
 * @param l the list to test.
 */
static __inline int rt_list_isempty(const rt_list_t *l)
{
    return l->next == l;
}

/**
 * @brief get the list length
 * @param l the list to get.
 */
static __inline unsigned int rt_list_len(const rt_list_t *l)
{
    unsigned int len = 0;
    const rt_list_t *p = l;
    while (p->next != l) {
        p = p->next;
        len++;
    }

    return len;
}

/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define rt_list_entry(node, type, member) \
    rt_container_of(node, type, member)

/**
 * rt_list_for_each - iterate over a list
 * @pos:	the rt_list_t * to use as a loop cursor.
 * @head:	the head for your list.
 */
#define rt_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * rt_list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the rt_list_t * to use as a loop cursor.
 * @n:		another rt_list_t * to use as temporary storage
 * @head:	the head for your list.
 */
#define rt_list_for_each_safe(pos, n, head)                \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

/**
 * rt_list_for_each_entry  -   iterate over list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define rt_list_for_each_entry(pos, head, member)                 \
    for (pos = rt_list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head);                                  \
         pos = rt_list_entry(pos->member.next, typeof(*pos), member))

/**
 * rt_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:    the type * to use as a loop cursor.
 * @n:      another type * to use as temporary storage
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define rt_list_for_each_entry_safe(pos, n, head, member)          \
    for (pos = rt_list_entry((head)->next, typeof(*pos), member),  \
        n = rt_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                                   \
         pos = n, n = rt_list_entry(n->member.next, typeof(*n), member))

/**
 * rt_list_first_entry - get the first element from a list
 * @ptr:    the list head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define rt_list_first_entry(ptr, type, member) \
    rt_list_entry((ptr)->next, type, member)

#define RT_SLIST_OBJECT_INIT(object) \
    {                                \
        NULL                         \
    }

/**
 * @brief initialize a single list
 *
 * @param l the single list to be initialized
 */
static __inline void rt_slist_init(rt_slist_t *l)
{
    l->next = NULL;
}

static __inline void rt_slist_append(rt_slist_t *l, rt_slist_t *n)
{
    struct rt_slist_node *node;

    node = l;
    while (node->next)
        node = node->next;

    /* append the node to the tail */
    node->next = n;
    n->next = NULL;
}

static __inline void rt_slist_insert(rt_slist_t *l, rt_slist_t *n)
{
    n->next = l->next;
    l->next = n;
}

static __inline unsigned int rt_slist_len(const rt_slist_t *l)
{
    unsigned int len = 0;
    const rt_slist_t *list = l->next;
    while (list != NULL) {
        list = list->next;
        len++;
    }

    return len;
}

static __inline rt_slist_t *rt_slist_remove(rt_slist_t *l, rt_slist_t *n)
{
    /* remove slist head */
    struct rt_slist_node *node = l;
    while (node->next && node->next != n)
        node = node->next;

    /* remove node */
    if (node->next != (rt_slist_t *)0)
        node->next = node->next->next;

    return l;
}

static __inline rt_slist_t *rt_slist_first(rt_slist_t *l)
{
    return l->next;
}

static __inline rt_slist_t *rt_slist_tail(rt_slist_t *l)
{
    while (l->next)
        l = l->next;

    return l;
}

static __inline rt_slist_t *rt_slist_next(rt_slist_t *n)
{
    return n->next;
}

static __inline int rt_slist_isempty(rt_slist_t *l)
{
    return l->next == NULL;
}

/**
 * @brief get the struct for this single list node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define rt_slist_entry(node, type, member) \
    rt_container_of(node, type, member)

/**
 * rt_slist_for_each - iterate over a single list
 * @pos:    the rt_slist_t * to use as a loop cursor.
 * @head:   the head for your single list.
 */
#define rt_slist_for_each(pos, head) \
    for (pos = (head)->next; pos != NULL; pos = pos->next)

/**
 * rt_slist_for_each_entry  -   iterate over single list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your single list.
 * @member: the name of the list_struct within the struct.
 */
#define rt_slist_for_each_entry(pos, head, member)                 \
    for (pos = rt_slist_entry((head)->next, typeof(*pos), member); \
         &pos->member != (NULL);                                   \
         pos = rt_slist_entry(pos->member.next, typeof(*pos), member))

/**
 * rt_slist_first_entry - get the first element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define rt_slist_first_entry(ptr, type, member) \
    rt_slist_entry((ptr)->next, type, member)

/**
 * rt_slist_tail_entry - get the tail element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define rt_slist_tail_entry(ptr, type, member) \
    rt_slist_entry(rt_slist_tail(ptr), type, member)

/**@}*/

#ifdef __cplusplus
}
#endif

#endif
