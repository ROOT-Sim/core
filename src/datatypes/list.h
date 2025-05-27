/**
* @file datatypes/list.h
*
* @brief List datatype
*
* A generic doubly-linked list
*
* SPDX-FileCopyrightText: 2008-2025 HPDCS Group <rootsim@googlegroups.com>
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#include <assert.h>
#include <memory.h>
#include <stddef.h>

/// This structure defines a generic list. Nodes of the list must have a next/prev pointer properly typed.
struct list {
	/// the size of the list
	size_t size;
	/// pointer to the first element
	void *head;
	/// pointer to the last element
	void *tail;
};

/// This macro is a slightly-different implementation of the standard offsetof macro
#define my_offsetof(st, m) ((size_t)( (unsigned char *)&((st)->m ) - (unsigned char *)(st)))

/// Declare a "typed" list. This is a pointer to type, but the variable will instead reference a struct rootsim_list!
#define list(type) type *

/** This macro allocates a struct rootsim_list object and cast it to the type pointer.
 *  It can be used to mimic the C++ syntax of templated lists, like:
 *  \code
 *   list(int) = new_list(int);
 *  \endcode
 */
#define new_list(type)	\
	__extension__({ \
		void *__lmptr; \
		__lmptr = malloc(sizeof(struct list)); \
		memset(__lmptr, 0, sizeof(struct list));\
		__lmptr;\
	})

/**
 * @brief Retrieves the size of the list.
 *
 * This macro evaluates to the size of the list, which is stored in the `size` member
 * of the `struct list`.
 *
 * @param list A pointer to a list created using the `new_list()` macro.
 * @return The size of the list as a `size_t`.
 */
#define list_sizeof(list) ((struct list *)list)->size

/**
 * This macro retrieves a pointer to the head node of a list.
 *
 * @param li a pointer to a list created using the new_list() macro.
 */
#define list_head(li) ((__typeof__(li))(((struct list *)(li))->head))

/**
 * This macro retrieves a pointer to the tail node of a list.
 *
 * @param li a pointer to a list created using the new_list() macro.
 */
#define list_tail(li) ((__typeof__(li))(((struct list *)(li))->tail))

/**
 * Given a pointer to a list node, this macro retrieves a pointer to the next node, if any.
 *
 * @param ptr a pointer to a list node.
 */
#define list_next(ptr) ((ptr)->next)

/**
 * Given a pointer to a list node, this macro retrieves a pointer to the prev node, if any.
 *
 * @param ptr a pointer to a list node.
 */
#define list_prev(ptr) ((ptr)->prev)

/// This macro retrieves the key of a payload data structure given its offset, and casts the value to double.
#define get_key(data) ({\
			char *__key_ptr = ((char *)(data) + __key_position);\
			double *__key_double_ptr = (double *)__key_ptr;\
			*__key_double_ptr;\
		      })

/**
 * Given a pointer to a list, this macro evaluates to a boolean telling whether
 * the list is empty or not.
 *
 * @param list a pointer to a list created using the new_list() macro.
 */
#define list_empty(list) (((struct list *)list)->size == 0)


/**
 * @brief Inserts a new node at the tail of the list.
 *
 * This macro appends a new node to the end of the list. If the list is empty,
 * the new node becomes both the head and the tail. The list size is incremented accordingly.
 *
 * @param li A pointer to the list created using the `new_list()` macro.
 * @param data A pointer to the node to be inserted. The node must have `next` and `prev` members.
 */
#define list_insert_tail(li, data) \
	do {	\
		__typeof__(data) __new_n = (data); /* in-block scope variable */\
		struct list *__l;\
		__new_n->next = NULL;\
		__new_n->prev = NULL;\
		do {\
			__l = (struct list *)(li);\
			assert(__l);\
			if(__l->size == 0) { /* is the list empty? */\
				__l->head = __new_n;\
				__l->tail = __new_n;\
				break; /* leave the inner do-while */\
			}\
			__new_n->next = NULL; /* Otherwise add at the end */\
			__new_n->prev = __l->tail;\
			((__typeof__(data))(__l->tail))->next = __new_n;\
			__l->tail = __new_n;\
		} while(0);\
		__l->size++;\
	} while(0)


/**
 * @brief Inserts a new node at the head of the list.
 *
 * This macro adds a new node to the beginning of the list. If the list is empty,
 * the new node becomes both the head and the tail. The list size is incremented accordingly.
 *
 * @param li A pointer to the list created using the `new_list()` macro.
 * @param data A pointer to the node to be inserted. The node must have `next` and `prev` members.
 */
#define list_insert_head(li, data) \
	do {	\
		__typeof__(data) __new_n = (data); /* in-block scope variable */\
		struct list *__l;\
		__new_n->next = NULL;\
		__new_n->prev = NULL;\
		__l = (struct list *)(li);\
		assert(__l);\
		if(__l->size == 0) { /* is the list empty? */\
			__l->head = __new_n;\
			__l->tail = __new_n;\
		}else{\
        	__new_n->prev = NULL; /* Otherwise add at the beginning */\
        	__new_n->next = __l->head;\
        	((__typeof__(data))__l->head)->prev = __new_n;\
        	__l->head = __new_n;\
        }\
		__l->size++;\
	} while(0)


/**
 * @brief Inserts a new node into the list in a sorted order based on a key.
 *
 * This macro inserts a new node into the list while maintaining the order of the list
 * based on the key value. The key is extracted from the node using the specified key name.
 * The list is traversed from the tail to find the appropriate position for the new node.
 *
 * @param li A pointer to the list created using the `new_list()` macro.
 * @param key_name The name of the key field in the node structure.
 * @param data A pointer to the node to be inserted. The node must have `next` and `prev` members.
 *
 * @note The list must be sorted in increasing order of the key values.
 * @note The key field in the node must be of type `double`.
 *
 * @pre The list must be initialized using the `new_list()` macro.
 * @pre The node to be inserted must not already be part of another list.
 *
 * @post The list size is incremented by 1.
 * @post The new node is inserted at the correct position based on the key value.
 */
#define list_insert(li, key_name, data)\
	do {\
		__typeof__(data) __n; /* in-block scope variable */\
		__typeof__(data) __new_n = (data);\
		size_t __key_position = my_offsetof((li), key_name);\
		double __key;\
		size_t __size_before;\
		struct list *__l;\
		do {\
			__l = (struct list *)(li);\
			assert(__l);\
			__size_before = __l->size;\
			if(__l->size == 0) { /* Is the list empty? */\
				__new_n->prev = NULL;\
				__new_n->next = NULL;\
				__l->head = __new_n;\
				__l->tail = __new_n;\
				break;\
			}\
			__key = get_key(__new_n); /* Retrieve the new node's key */\
			/* Scan from the tail, as keys are ordered in an increasing order */\
			__n = __l->tail;\
			while(__n != NULL && __key < get_key(__n)) {\
				__n = __n->prev;\
			}\
			/* Insert depending on the position */\
		 	if(__n == __l->tail) { /* tail */\
				__new_n->next = NULL;\
				((__typeof__(data))__l->tail)->next = __new_n;\
				__new_n->prev = __l->tail;\
				__l->tail = __new_n;\
			} else if(__n == NULL) { /* head */\
				__new_n->prev = NULL;\
				__new_n->next = __l->head;\
				((__typeof__(data))__l->head)->prev = __new_n;\
				__l->head = __new_n;\
			} else { /* middle */\
				__new_n->prev = __n;\
				__new_n->next = __n->next;\
				__n->next->prev = __new_n;\
				__n->next = __new_n;\
			}\
		} while(0);\
		__l->size++;\
		assert(__l->size == (__size_before + 1));\
	} while(0)


/**
 * @brief Detaches a node from the list by its content.
 *
 * This macro removes a specified node from the list, updating the list's head, tail, and size
 * as necessary. The node's `next` and `prev` pointers are set to invalid values to indicate
 * that it is no longer part of the list.
 *
 * @param li A pointer to the list created using the `new_list()` macro.
 * @param node A pointer to the node to be detached. The node must have `next` and `prev` members.
 *
 * @pre The list must be initialized using the `new_list()` macro.
 * @pre The node to be detached must be part of the list.
 *
 * @post The node is removed from the list, and the list size is decremented by 1.
 * @post The `next` and `prev` pointers of the detached node are set to invalid values.
 */
#define list_detach_by_content(li, node)                                                                               \
	do {                                                                                                           \
		__typeof__(node) __n = (node); /* in-block scope variable */                                           \
		struct list *__l;                                                                                      \
		__l = (struct list *)(li);                                                                             \
		assert(__l);                                                                                           \
		/* Unchain the node */                                                                                 \
		if(__l->head == __n) {                                                                                 \
			__l->head = __n->next;                                                                         \
			if(__l->head != NULL) {                                                                        \
				((__typeof__(node))__l->head)->prev = NULL;                                            \
			}\
		}\
		if(__l->tail == __n) {\
			__l->tail = __n->prev;\
			if(__l->tail != NULL) {\
				((__typeof__(node))__l->tail)->next = NULL;\
			}\
		}\
		if(__n->next != NULL) {\
			__n->next->prev = __n->prev;\
		}\
		if(__n->prev != NULL) {\
			__n->prev->next = __n->next;\
		}\
		__n->next = (void *)0xBEEFC0DE;\
		__n->prev = (void *)0xDEADC0DE;\
		__l->size--;\
	} while(0)


/**
 * @brief Removes the head node from the list.
 *
 * This macro removes the first node (head) from the list, updating the list's head pointer,
 * size, and the `next` and `prev` pointers of the removed node. The removed node's pointers
 * are set to invalid values to indicate that it is no longer part of the list.
 *
 * @param list A pointer to the list created using the `new_list()` macro.
 *
 * @pre The list must be initialized using the `new_list()` macro.
 * @pre The list must not be NULL.
 *
 * @post The head node is removed from the list, and the list size is decremented by 1.
 * @post The `next` and `prev` pointers of the removed node are set to invalid values.
 */
#define list_pop(list)\
	do {\
		struct list *__l;\
		size_t __size_before;\
		__typeof__ (list) __n;\
		__typeof__ (list) __n_next;\
		__l = (struct list *)(list);\
		assert(__l);\
		__size_before = __l->size;\
		__n = __l->head;\
		if(__n != NULL) {\
			__l->head = __n->next;\
			if(__n->next != NULL) {\
				__n->next->prev = NULL;\
			}\
			__n_next = __n->next;\
			__n->next = (void *)0xDEFEC8ED;\
			__n->prev = (void *)0xDEFEC8ED;\
			__n = __n_next;\
			__l->size--;\
			assert(__l->size == (__size_before - 1));\
		}\
	} while(0)


/**
 * @brief Truncates a list up to a certain point, starting from the head.
 *
 * This macro removes nodes from the head of the list up to the first node
 * whose key is greater than or equal to the specified key value. The removed
 * nodes are released using the provided release function.
 *
 * @param list A pointer to the list created using the `new_list()` macro.
 * @param key_name The name of the key field in the node structure.
 * @param key_value The key value up to which nodes should be removed.
 * @param release_fn A function to release the memory or resources of the removed nodes.
 *
 * @return The number of nodes removed from the list.
 *
 * @pre The list must be initialized using the `new_list()` macro.
 * @pre The nodes in the list must have a key field of type `double`.
 * @pre The release function must be callable for each removed node.
 *
 * @post The list size is decremented by the number of removed nodes.
 * @post The `next` and `prev` pointers of the removed nodes are set to invalid values.
 */
#define list_trunc(list, key_name, key_value, release_fn) \
		({\
		struct list *__l = (struct list *)(list);\
		__typeof__(list) __n;\
		__typeof__(list) __n_adjacent;\
		unsigned int __deleted = 0;\
		size_t __key_position = my_offsetof((list), key_name);\
		assert(__l);\
		size_t __size_before = __l->size;\
		/* Attempting to truncate an empty list? */\
		if(__l->size > 0) {\
			__n = __l->head;\
			while(__n != NULL && get_key(__n) < (key_value)) {\
				__deleted++;\
	                	__n_adjacent = __n->next;\
		                __n->next = (void *)0xBAADF00D;\
	        	        __n->prev = (void *)0xBAADF00D;\
				release_fn(__n);\
				__n = __n_adjacent;\
			}\
			__l->head = __n;\
			if(__l->head != NULL)\
			((__typeof__(list))__l->head)->prev = NULL;\
		}\
		__l->size -= __deleted;\
		assert(__l->size == (__size_before - __deleted));\
		__deleted;\
		})


/**
 * @brief Retrieves the size of the list.
 *
 * This macro evaluates to the size of the list, which is stored in the `size` member
 * of the `struct list`.
 *
 * @param li A pointer to a list created using the `new_list()` macro.
 * @return The size of the list as a `size_t`.
 */
#define list_size(li) ((struct list *)(li))->size
