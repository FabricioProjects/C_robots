/*
 * This file is part of stoker.
 *
 * Copyright 2010 Kleto Zan, Maurizio Ruzzi
 *
 * All rights reserved.
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "list.h"
#include "err.h"

struct listt *
listt_create(void)
{
	struct listt *listt = err_malloc (sizeof (*listt));

	listt->list = NULL;
	err_pthread_mutex_init(&listt->mutex, NULL);

	return listt;
}

void listt_destroy(struct listt *listt, void (*func_free)(void *))
{
	struct list *list, *next = NULL;

	if (listt != NULL )
	{
		err_pthread_mutex_lock(&listt->mutex);

		list = listt->list;

		while (list != NULL )
		{
			next = list->next;

			if (func_free != NULL )
				func_free(list->data);

			free(list);
			list = next;
		}

		err_pthread_mutex_unlock(&listt->mutex);

		err_pthread_mutex_destroy(&listt->mutex);
		free(listt);
	}
}

void listt_insert_end(struct listt *listt, void *data)
{
	struct list *list, *last, *new = err_malloc (sizeof (*new));

	new->data = data;
	new->next = NULL;

	err_pthread_mutex_lock(&listt->mutex);

	list = listt->list;

	if (list != NULL )
	{
		last = list;

		while (last->next != NULL )
			last = last->next;

		last->next = new;
	}
	else
		list = new;

	listt->list = list;

	err_pthread_mutex_unlock(&listt->mutex);
}

void listt_insert_begin(struct listt *listt, void *data)
{
	struct list *new = err_malloc (sizeof (*new));

	new->data = data;

	err_pthread_mutex_lock(&listt->mutex);

	new->next = listt->list;
	listt->list = new;

	err_pthread_mutex_unlock(&listt->mutex);
}

void listt_foreach(struct listt *listt, void *data,
		bool (*func_foreach)(void *, void *))
{
	struct list *list;

	while (pthread_mutex_trylock(&listt->mutex) == EBUSY)
		/* empty */;

	list = listt->list;

	while (list != NULL )
	{
		if (func_foreach(list->data, data) == true)
			list = list->next;
		else
			list = NULL;
	}

	err_pthread_mutex_unlock(&listt->mutex);
}

void listt_remove(struct listt *listt, void *data,
		int (*func_cmp)(const void *, const void *), void (*func_free)(void *))
{
	struct list *temp, *prev = NULL;

	err_pthread_mutex_lock(&listt->mutex);

	temp = listt->list;

	while (temp != NULL )
	{
		if (func_cmp(temp->data, data) == 0)
		{
			if (prev != NULL )
				prev->next = temp->next;
			else
				listt->list = temp->next;

			if (func_free != NULL )
				func_free(temp->data);

			free(temp);
			temp = NULL;
		}
		else
		{
			prev = temp;
			temp = prev->next;
		}
	}

	err_pthread_mutex_unlock(&listt->mutex);
}

void listt_remove_all(struct listt *listt, void *data,
		int (*func_cmp)(const void *, const void *), void (*func_free)(void *))
{
	struct list *temp, *prev = NULL, *next = NULL;

	err_pthread_mutex_lock(&listt->mutex);

	temp = listt->list;

	while (temp != NULL )
	{
		if (func_cmp(temp->data, data) == 0)
		{
			next = temp->next;

			if (prev != NULL )
				prev->next = temp->next;
			else
				listt->list = temp->next;

			if (func_free != NULL )
				func_free(temp->data);

			free(temp);
			temp = next;
		}
		else
		{
			prev = temp;
			temp = prev->next;
		}
	}

	err_pthread_mutex_unlock(&listt->mutex);
}

struct list *
listt_search(struct listt *listt, const void *data,
		int (*func_cmp)(const void *, const void *))
{
	struct list *list = NULL;

	while (pthread_mutex_trylock(&listt->mutex) == EBUSY)
		/* empty */;

	list = listt->list;

	while (list != NULL )
	{
		if (func_cmp(list->data, data) == 0)
			break;

		list = list->next;
	}

	err_pthread_mutex_unlock(&listt->mutex);

	return list;
}

int list_pointer_cmp(const void *list_data, const void *user_data)
{
	return list_data != user_data;
}

struct list *
list_insert_begin(struct list *list, void *data)
{
	struct list *new;

	new = err_malloc (sizeof (*new));
	new->data = data;
	new->next = list;

	return new;
}

struct list *
list_insert_end(struct list *list, void *data, int *i)
{
	struct list *new, *last;

	new = err_malloc (sizeof (*new));
	new->data = data;
	new->next = NULL;

	*i = 0;

	if (list != NULL )
	{
		(*i)++;
		last = list;

		while (last->next != NULL )
		{
			(*i)++;
			last = last->next;
		}

		last->next = new;
	}
	else
		list = new;

	return list;
}

struct list *
list_insert_sorted(struct list *list, void *data, void *data_cmp,
		int (*func_cmp)(const void *, const void *, void *), int *i)
{
	int cmp;
	struct list *new, *temp = list, *prev = NULL;

	new = err_malloc (sizeof (*new));
	new->data = data;

	*i = 0;

	if (list == NULL )
	{
		new->next = list;
		return new;
	}

	cmp = func_cmp(temp->data, data, data_cmp);

	while (temp->next != NULL && cmp > 0)
	{
		(*i)++;
		prev = temp;
		temp = temp->next;

		cmp = func_cmp(temp->data, data, data_cmp);
	}

	if (temp->next == NULL && cmp > 0)
	{
		(*i)++;
		temp->next = new;
		new->next = NULL;
		return list;
	}

	if (prev != NULL )
	{
		prev->next = new;
		new->next = temp;
		return list;
	}
	else
	{
		new->next = list;
		return new;
	}
}

struct list *
list_insert(struct list *list, void *data, int i)
{
	struct list *new, *temp = list, *prev = NULL;

	new = err_malloc (sizeof (*new));
	new->data = data;

	while (i-- > 0 && temp != NULL )
	{
		prev = temp;
		temp = temp->next;
	}

	new->next = temp;

	if (prev != NULL )
		prev->next = new;
	else
		list = new;

	return list;
}

struct list *
list_insert_sorted_node(struct list *list, void *data, void *data_cmp,
		int (*func_cmp)(const void *, const void *, void *), struct list **node,
		int *i)
{
	int cmp;
	struct list *temp = list, *prev = NULL;
	struct list *new = *node;

	*i = 0;

	if (list == NULL )
	{
		new->next = list;
		return new;
	}

	cmp = func_cmp(temp->data, data, data_cmp);

	while (temp->next != NULL && cmp > 0)
	{
		(*i)++;
		prev = temp;
		temp = temp->next;

		cmp = func_cmp(temp->data, data, data_cmp);
	}

	if (temp->next == NULL && cmp > 0)
	{
		(*i)++;
		temp->next = new;
		new->next = NULL;
		return list;
	}

	if (prev != NULL )
	{
		prev->next = new;
		new->next = temp;
		return list;
	}
	else
	{
		new->next = list;
		return new;
	}
}

struct list *
list_insert_begin_node(struct list *list, struct list *node)
{
	node->next = list;
	list = node;

	return list;
}

struct list *
list_insert_end_node(struct list *list, struct list *node)
{
	struct list *last;

	if (list != NULL )
	{
		last = list;

		while (last->next != NULL )
			last = last->next;

		last->next = node;
	}
	else
		list = node;

	return list;
}

struct list *
list_insert_node(struct list *list, struct list *node, int i)
{
	struct list *temp = list, *prev = NULL;

	while (i-- > 0 && temp != NULL )
	{
		prev = temp;
		temp = temp->next;
	}

	node->next = temp;

	if (prev != NULL )
		prev->next = node;
	else
		list = node;

	return list;
}

struct list *
list_remove_before(struct list *list, int i)
{
	struct list *temp;

	while (i-- >= 0 && list != NULL )
	{
		temp = list;
		list = list->next;

		free(temp->data);
		free(temp);
	}

	return list;
}

struct list *
list_remove_index(struct list *list, int i, void (*func_free)(void *))
{
	struct list *temp = list, *prev = NULL;

	while (i-- > 0 && temp != NULL )
	{
		prev = temp;
		temp = temp->next;
	}

	if (temp != NULL )
	{
		if (prev != NULL )
			prev->next = temp->next;
		else
			list = temp->next;

		if (func_free != NULL )
			func_free(temp->data);

		free(temp);
	}

	return list;
}

struct list *
list_remove_all(struct list *list, void *data,
		int (*func_cmp)(const void *, const void *), void (*func_free)(void *))
{
	struct list *temp = list, *prev = NULL, *next = NULL;

	while (temp != NULL )
	{
		if (func_cmp(temp->data, data) == 0)
		{
			next = temp->next;

			if (prev != NULL )
				prev->next = temp->next;
			else
				list = temp->next;

			if (func_free != NULL )
				func_free(temp->data);

			free(temp);
			temp = next;
		}
		else
		{
			prev = temp;
			temp = prev->next;
		}
	}

	return list;
}

struct list *
list_remove(struct list *list, void *data,
		int (*func_cmp)(const void *, const void *), void (*func_free)(void *),
		int *i)
{
	struct list *temp = list, *prev = NULL;

	*i = 0;

	while (temp != NULL )
	{
		if (func_cmp(temp->data, data) == 0)
		{
			if (prev != NULL )
				prev->next = temp->next;
			else
				list = temp->next;

			if (func_free != NULL )
				func_free(temp->data);

			free(temp);
			temp = NULL;
		}
		else
		{
			(*i)++;
			prev = temp;
			temp = prev->next;
		}
	}

	return list;
}

struct list *
list_remove_begin_node(struct list *list, struct list **node)
{
	struct list *temp = list;

	if (temp != NULL )
	{
		list = temp->next;
		temp->next = NULL;
	}

	*node = temp;

	return list;
}

struct list *
list_remove_index_node(struct list *list, int i, struct list **node)
{
	struct list *temp = list, *prev = NULL;

	while (i-- > 0 && temp != NULL )
	{
		prev = temp;
		temp = temp->next;
	}

	if (temp != NULL )
	{
		if (prev != NULL )
			prev->next = temp->next;
		else
			list = temp->next;

		temp->next = NULL;
	}

	*node = temp;

	return list;
}

struct list *
list_remove_node(struct list *list, void *data,
		int (*func_cmp)(const void *, const void *), struct list **node, int *i)
{
	struct list *temp = list, *prev = NULL;

	*i = 0;

	while (temp != NULL )
	{
		if (func_cmp(temp->data, data) == 0)
		{
			if (prev != NULL )
				prev->next = temp->next;
			else
				list = temp->next;

			temp->next = NULL;
			*node = temp;
			temp = NULL;
		}
		else
		{
			(*i)++;
			prev = temp;
			temp = prev->next;
		}
	}

	return list;
}

void list_destroy(struct list *list, void (*func_free)(void *))
{
	struct list *temp = list, *next = NULL;

	while (temp != NULL )
	{
		next = temp->next;

		if (func_free != NULL )
			func_free(temp->data);

		free(temp);
		temp = next;
	}
}

int list_search_index(struct list *list, const void *data,
		int (*func_cmp)(const void *, const void *))
{
	int i = 0;
	struct list *temp = list;

	while (temp != NULL )
	{
		if (func_cmp(temp->data, data) == 0)
			return i;

		temp = temp->next;
		i++;
	}

	return -1;
}

struct list *
list_search(struct list *list, const void *data,
		int (*func_cmp)(const void *, const void *), int *i)
{
	struct list *temp = list;

	*i = 0;

	while (temp != NULL )
	{
		if (func_cmp(temp->data, data) == 0)
			return temp;

		temp = temp->next;
		(*i)++;
	}

	return NULL ;
}

void *
list_search_data(struct list *list, const void *data,
		int (*func_cmp)(const void *, const void *), int *i)
{
	struct list *temp = list;

	*i = 0;

	while (temp != NULL )
	{
		if (func_cmp(temp->data, data) == 0)
			return temp->data;

		temp = temp->next;
		(*i)++;
	}

	return NULL ;
}

struct list *
list_nth(struct list *list, int n)
{
	while (n-- > 0 && list != NULL )
		list = list->next;

	return list;
}

void list_foreach(struct list *list, void *data,
		bool (*func_foreach)(void *, void *))
{
	struct list *temp = list;

	while (temp != NULL )
	{
		if (func_foreach(temp->data, data) == true)
			temp = temp->next;
		else
			temp = NULL;
	}
}

struct list *
list_invert(struct list *list)
{
	struct list *prev = NULL, *next;

	while (list != NULL )
	{
		next = list->next;
		list->next = prev;
		prev = list;
		list = next;
	}

	return prev;
}

int list_count(struct list *list)
{
	int num = 0;

	while (list != NULL )
	{
		num++;
		list = list->next;
	}

	return num;
}
