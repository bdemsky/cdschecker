#include <unrelacy.h>
#include <atomic>

#include "eventcount.h"

template<typename T>
class spsc_queue
{
	
public:

	/**
		@Begin
		@Options:
			LANG = CPP;
			CLASS = spsc_queue;
		@Global_define:
			@DeclareStruct:
			typedef struct tag_elem {
				call_id_t id;
				T data;
			} tag_elem_t;
		
		@DeclareVar:
			spec_list *__queue;
			call_id_t *tag;
		@InitVar:
			__queue = new_spec_list();
			tag = new_id_tag();
		@DefineFunc:
			tag_elem_t* new_tag_elem(call_id_t id, T data) {
	        	tag_elem_t *e = (tag_elem_t*) MODEL_MALLOC(sizeof(tag_elem_t));
				e->id = id;
				e->data = data;
				return e;
			}
		@DefineFunc:
			call_id_t get_id(void *wrapper) {
				return ((tag_elem_t*) wrapper)->id;
			}
		@DefineFunc:
			unsigned int get_data(void *wrapper) {
				return ((tag_elem_t*) wrapper)->data;
			}
		@Happens_before:
			Enqueue -> Dequeue		
		@End
	**/

	spsc_queue()
	{
		node* n = new node ();
		head = n;
		tail = n;
	}

	~spsc_queue()
	{
		RL_ASSERT(head == tail);
		delete ((node*)head($));
	}
	
	/**
		@Begin
		@Interface: Enqueue
		@Commit_point_set: Enqueue_Point
		@ID: get_and_inc(tag);
		@Action:
			tag_elem_t *elem = new_tag_elem(__ID__, data);
			push_back(__queue, elem);
		@End
	*/
	void enqueue(T data)
	{
		node* n = new node (data);
		head($)->next.store(n, std::memory_order_release);
		/**
			@Begin
			@Commit_point_define_check: true
			@Label: Enqueue_Point
			@End
		*/
		head = n;
		// #Mark delete this
		ec.signal();
	}
	/**
		@Begin
		@Interface: Dequeue
		@Commit_point_set: Dequeue_Point
		@ID: get_id(front(__queue))
		@Action:
			T _Old_Val = get_data(front(__queue));
			pop_front(__queue);
		@Post_check:
			_Old_Val == __RET__
		@End
	*/
	T dequeue()
	{
		T data = try_dequeue();
		while (0 == data)
		{
			int cmp = ec.get();
			data = try_dequeue();
			if (data)
				break;
			ec.wait(cmp);
			data = try_dequeue();
			if (data)
				break;
		}
		return data;
	}

private:
	struct node
	{
		std::atomic<node*> next;
		rl::var<T> data;

		node(T data = T())
			: data(data)
		{
			next = 0;
		}
	};

	rl::var<node*> head;
	rl::var<node*> tail;

	eventcount ec;

	T try_dequeue()
	{
		node* t = tail($);
		node* n = t->next.load(std::memory_order_acquire);
		/**
			@Begin
			@Commit_point_define_check: n != 0
			@Label: Dequeue_Point
			@End
		*/
		if (0 == n)
			return 0;
		T data = n->data($);
		delete (t);
		tail = n;
		return data;
	}
};
/**
	@Begin
	@Class_end
	@End
*/
