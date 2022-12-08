#ifndef _LIST_H_
#define _LIST_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif

#endif /*  __cplusplus */


#define list_entry(ptr, type, name)\
	(type *)((char *)ptr-(char *)(&((type *)0)->name))

typedef struct list_s list_t;
struct list_s
{
	list_t *prev, *next;
};

// 后来添加
#define INIT_LIST_HEAD(ptr) do{ \
       (ptr)->next = (ptr); (ptr)->prev = (ptr); \
       }while(0)
 
#define list_for_each(pos, head)\
	for(pos = (head)->next; pos != (head); pos=pos->next)

#define list_for_each_safe(pos, n, head) \
	      for (pos = (head)->next, n = pos->next; pos != (head); \
				                pos = n, n = pos->next)

static inline void list_init(list_t *head)
{
	head->prev = head;
	head->next = head;
}
static inline void __list_add(list_t *p, list_t *pprev, list_t *pnext)
{
	p->next = pnext;
	p->prev = pprev;
	pnext->prev = p;
	pprev->next = p;
}
static inline void list_add_head(list_t *head, list_t *p)
{
	__list_add(p, head, head->next);
}
static inline void list_add_tail(list_t *head, list_t *p)
{
	__list_add(p, head->prev, head);
}
static inline void __list_del(list_t *pprev, list_t *pnext)
{
	pprev->next = pnext;
	pnext->prev = pprev;
}
static inline void list_del(list_t *p)
{
	__list_del(p->prev, p->next);
}
static inline void __list_del_entry(list_t *entry)
{
	__list_del(entry->prev, entry->next);
}
// 使用list_for_each_safe + list_del_init 删除更安全
static inline void list_del_init(list_t *entry)
{
	__list_del_entry(entry);
	INIT_LIST_HEAD(entry);
}
static inline int list_empty(list_t *head)
{
	return head->next == head->prev;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /*  __cplusplus */


#endif
