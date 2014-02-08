// mcs on stack

#include <stdatomic.h>
#include <unrelacy.h>




struct mcs_node {
	std::atomic<mcs_node *> next;
	std::atomic<int> gate;

	mcs_node() {
		next.store(0);
		gate.store(0);
	}
};


struct mcs_mutex {
public:
	// tail is null when lock is not held
	std::atomic<mcs_node *> m_tail;

	mcs_mutex() {
		/**
			@Begin
	        @Entry_point
			@End
		*/
		m_tail.store( NULL );
	}
	~mcs_mutex() {
		ASSERT( m_tail.load() == NULL );
	}

	// Each thread will have their own guard.
	class guard {
	public:
		mcs_mutex * m_t;
		mcs_node    m_node; // node held on the stack

		guard(mcs_mutex * t) : m_t(t) { t->lock(this); }
		~guard() { m_t->unlock(this); }
	};

	/**
		@Begin
		@Options:
			LANG = CPP;
			CLASS = mcs_mutex;
		@Global_define:
			@DeclareVar:
				bool _lock_acquired;
			@InitVar:
				_lock_acquired = false;
		@Happens_before:
			Unlock -> Lock
		@End
	*/

	/**
		@Begin
		@Interface: Lock
		@Commit_point_set: Lock_Enqueue_Point1 | Lock_Enqueue_Point2
		@Check:
			_lock_acquired == false;
		@Action:
			_lock_acquired = true;
		@End
	*/
	void lock(guard * I) {
		mcs_node * me = &(I->m_node);

		// set up my node :
		// not published yet so relaxed :
		me->next.store(NULL, std::mo_relaxed );
		me->gate.store(1, std::mo_relaxed );

		// publish my node as the new tail :
		mcs_node * pred = m_tail.exchange(me, std::mo_acq_rel);
		/**
			@Begin
			@Commit_point_define_check: pred == NULL
			@Label: Lock_Enqueue_Point1
			@End
		*/
		if ( pred != NULL ) {
			// (*1) race here
			// unlock of pred can see me in the tail before I fill next

			// publish me to previous lock-holder :
			pred->next.store(me, std::mo_release );

			// (*2) pred not touched any more       

			// now this is the spin -
			// wait on predecessor setting my flag -
			rl::linear_backoff bo;
			int my_gate = 1;
			while (my_gate ) {
				my_gate = me->gate.load(std::mo_acquire);
				/**
					@Begin
					@Commit_point_define_check: my_gate == 0
					@Label: Lock_Enqueue_Point2
					@End
				*/
				thrd_yield();
			}
		}
	}

	/**
		@Begin
		@Interface: Unlock
		@Commit_point_set:
			Unlock_Point_Success_1 | Unlock_Point_Success_2
		@Check:
			_lock_acquired == true
		@Action:
			_lock_acquired = false;
		@End
	*/
	void unlock(guard * I) {
		mcs_node * me = &(I->m_node);

		mcs_node * next = me->next.load(std::mo_acquire);
		if ( next == NULL )
		{
			mcs_node * tail_was_me = me;
			bool success;
			success = m_tail.compare_exchange_strong(
				tail_was_me,NULL,std::mo_acq_rel);
			/**
				@Begin
				@Commit_point_define_check: success == true
				@Label: Unlock_Point_Success_1
				@End
			*/
			if (success) {
				
				// got null in tail, mutex is unlocked
				return;
			}

			// (*1) catch the race :
			rl::linear_backoff bo;
			for(;;) {
				next = me->next.load(std::mo_acquire);
				if ( next != NULL )
					break;
				thrd_yield();
			}
		}

		// (*2) - store to next must be done,
		//  so no locker can be viewing my node any more        

		// let next guy in :
		next->gate.store( 0, std::mo_release );
		/**
			@Begin
			@Commit_point_define_check: true
			@Label: Unlock_Point_Success_2
			@End
		*/
	}
};
/**
	@Begin
	@Class_end
	@End
*/
