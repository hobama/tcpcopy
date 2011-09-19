#ifndef  _LINKLIST_H_INC
#define  _LINKLIST_H_INC

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct linknode
	{
		void *data;
		struct linknode *prev;
		struct linknode *next;
	}linknode,*lnodeptr;

	typedef struct linklist{
		linknode head;
	}linklist;


	lnodeptr lnode_malloc(void *data);
	void lnode_free(lnodeptr p);

	linklist * linklist_create();
	void linklist_destory(linklist *l,int needDeleteDeep);
	void linklist_append(linklist *l,lnodeptr );
	void linklist_push(linklist *l,lnodeptr p);
	lnodeptr linklist_remove(lnodeptr node);
	lnodeptr linklist_first(linklist *l);
	lnodeptr linklist_tail(linklist *l);
	lnodeptr linklist_pop_first(linklist *l);
	lnodeptr linklist_pop_tail(linklist *l);
	lnodeptr linklist_get_next(linklist *l,lnodeptr p);
	int linklist_is_empty(linklist *l);

#ifdef __cplusplus
}
#endif
#endif   /* ----- #ifndef _LINKLIST_H_INC  ----- */
