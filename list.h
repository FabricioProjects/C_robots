/*
 * This file is part of stoker.
 *
 * Copyright 2010 Kleto Zan, Maurizio Ruzzi
 *
 * All rights reserved.
 */
#ifndef __LIST_H__
#define __LIST_H__

#ifndef _STDBOOL_H
# include <stdbool.h>
#endif

#ifndef _PTHREAD_H
# include <pthread.h>
#endif

struct list
{
	void *data;
	struct list *next;
};

struct listt
{
	struct list *list;
	pthread_mutex_t mutex;
};

extern struct listt *listt_create(void);
extern void listt_destroy(struct listt *, void (*)(void *));
extern void listt_insert_end(struct listt *, void *);
extern void listt_insert_begin(struct listt *, void *);
extern void listt_foreach(struct listt *, void *, bool (*)(void *, void *));
extern void listt_remove(struct listt *, void *,
		int (*)(const void *, const void *), void (*)(void *));
extern void listt_remove_all(struct listt *, void *,
		int (*)(const void *, const void *), void (*)(void *));
extern struct list *listt_search(struct listt *, const void *,
		int (*)(const void *, const void *));

extern int list_pointer_cmp(const void *, const void *);
extern struct list *list_insert_begin(struct list *, void *);
extern struct list *list_insert_end(struct list *, void *, int *);
extern struct list *list_insert_sorted(struct list *, void *, void *,
		int (*)(const void *, const void *, void *), int *);
extern struct list *list_insert_sorted_node(struct list *list, void *data,
		void *data_cmp, int (*func_cmp)(const void *, const void *, void *),
		struct list **, int *);
extern struct list *list_insert(struct list *, void *, int);
extern struct list *list_insert_begin_node(struct list *, struct list *);
extern struct list *list_insert_end_node(struct list *, struct list *);
extern struct list *list_insert_node(struct list *, struct list *, int);
extern struct list *list_remove_before(struct list *, int);
extern struct list *list_remove_index(struct list *, int, void (*)(void *));
extern struct list *list_remove_all(struct list *, void *,
		int (*)(const void *, const void *), void (*)(void *));
extern struct list *list_remove(struct list *, void *,
		int (*)(const void *, const void *), void (*)(void *), int *);
extern struct list *list_remove_node(struct list *, void *,
		int (*)(const void *, const void *), struct list **, int *);
extern struct list *list_remove_begin_node(struct list *, struct list **);
extern struct list *list_remove_index_node(struct list *, int, struct list **);
extern void list_destroy(struct list *, void (*)(void *));
extern struct list *list_search(struct list *, const void *,
		int (*)(const void *, const void *), int *);
extern void * list_search_data(struct list *, const void *,
		int (*)(const void *, const void *), int *);
extern int list_search_index(struct list *, const void *,
		int (*)(const void *, const void *));
extern struct list *list_nth(struct list *, int);
extern void list_foreach(struct list *, void *, bool (*)(void *, void *));
extern struct list *list_invert(struct list *);
extern int list_count(struct list *);

#define LIST_NEXT(list) ((list) ? (((struct list *)(list))->next) : NULL)
#define LISTT_FOREACH_BEGIN(listt) pthread_mutex_lock (&(listt)->mutex)
#define LISTT_FOREACH_END(listt) pthread_mutex_unlock (&(listt)->mutex)
#define LISTT_LIST(listt) ((listt)->list)

#endif /* __LIST_H__ */
