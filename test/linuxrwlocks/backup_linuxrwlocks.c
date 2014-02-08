#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"

#define RW_LOCK_BIAS            0x00100000
#define WRITE_LOCK_CMP          RW_LOCK_BIAS

typedef union {
	atomic_int lock;
} rwlock_t;

/** Example implementation of linux rw lock along with 2 thread test
 *  driver... */

/**
	Properties to check:
	1. At most 1 thread can acquire the write lock, and at the same time, no
		other threads can acquire any lock (including read/write lock).
	2. At most RW_LOCK_BIAS threads can successfully acquire the read lock.
	3. A read_unlock release 1 read lock, and a write_unlock release the write
	lock. They can not release a lock that they don't acquire.
	###
	4. Read_lock and write_lock can not be grabbed at the same time.
	5. Happpens-before relationship should be checked and guaranteed, which
	should be as the following:
		a. read_unlock hb-> write_lock
		b. write_unlock hb-> write_lock
		c. write_unlock hb-> read_lock
*/

/**
	Interesting point for locks:
	a. If the users unlock() before any lock(), then the model checker will fail.
	For this case, we can not say that the data structure is buggy, how can we
	tell them from a real data structure bug???
	b. We should specify that for a specific thread, successful locks and
	unlocks should always come in pairs. We could make this check as an
	auxiliary check since it is an extra rule for how the interfaces should called.
*/

/**
	@Begin
	@Global_define:
		@DeclareVar:
			bool writer_lock_acquired;
			int reader_lock_cnt;
		@InitVar:
			writer_lock_acquired = false;
			reader_lock_cnt = 0;
	@Happens_before:
		# Since commit_point_set has no ID attached, A -> B means that for any B,
		# the previous A happens before B.
		Read_Unlock -> Write_Lock
		Read_Unlock -> Write_Trylock(HB_Write_Trylock_Succ)
		
		Write_Unlock -> Write_Lock
		Write_Unlock -> Write_Trylock(HB_Write_Trylock_Succ)

		Write_Unlock -> Read_Lock
		Write_Unlock -> Read_Trylock(HB_Read_Trylock_Succ)
	@End
*/

static inline int read_can_lock(rwlock_t *lock)
{
	return atomic_load_explicit(&lock->lock, memory_order_relaxed) > 0;
}

static inline int write_can_lock(rwlock_t *lock)
{
	return atomic_load_explicit(&lock->lock, memory_order_relaxed) == RW_LOCK_BIAS;
}


/**
	@Begin
	@Interface_decl_define: Read_Lock
	@Commit_point_set:
		Read_Lock_Success_1 | Read_Lock_Success_2
	@Check:
		!writer_lock_acquired
	@Action:
		reader_lock_cnt++;
	@End
*/
static inline void read_lock(rwlock_t *rw)
{
	int priorvalue = atomic_fetch_sub_explicit(&rw->lock, 1, memory_order_acquire);
	/**
		@Begin
		@Commit_point_define_check: priorvalue > 0
		@Label:Read_Lock_Success_1
		@End
	*/
	while (priorvalue <= 0) {
		atomic_fetch_add_explicit(&rw->lock, 1, memory_order_relaxed);
		while (atomic_load_explicit(&rw->lock, memory_order_relaxed) <= 0) {
			thrd_yield();
		}
		priorvalue = atomic_fetch_sub_explicit(&rw->lock, 1, memory_order_acquire);
		/**
			@Begin
			@Commit_point_define_check: priorvalue > 0
			@Label:Read_Lock_Success_2
			@End
		*/
	}
}


/**
	@Begin
	@Interface_decl_define: Write_Lock
	@Commit_point_set:
		Write_Lock_Success_1 | Write_Lock_Success_2
	@Check:
		!writer_lock_acquired && reader_lock_cnt == 0
	@Action:
		writer_lock_acquired = true;
	@End
*/
static inline void write_lock(rwlock_t *rw)
{
	int priorvalue = atomic_fetch_sub_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_acquire);
	/**
		@Begin
		@Commit_point_define_check: priorvalue == RW_LOCK_BIAS
		@Label: Write_Lock_Success_1
		@End
	*/
	while (priorvalue != RW_LOCK_BIAS) {
		atomic_fetch_add_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_relaxed);
		while (atomic_load_explicit(&rw->lock, memory_order_relaxed) != RW_LOCK_BIAS) {
			thrd_yield();
		}
		priorvalue = atomic_fetch_sub_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_acquire);
		/**
			@Begin
			@Commit_point_define_check: priorvalue == RW_LOCK_BIAS
			@Label: Write_Lock_Success_2
			@End
		*/
	}
}

/**
	@Begin
	@Interface_decl_define: Read_Trylock
	@Commit_point_set:
		Read_Trylock_Point
	@Condition:
		writer_lock_acquired == false
	@HB_condition:
		HB_Read_Trylock_Succ :: __RET__ == 1
	@Action:
		if (__COND_SAT__)
			reader_lock_cnt++;
	@Post_check:
		__COND_SAT__ ? __RET__ == 1 : __RET__ == 0
	@End
*/
static inline int read_trylock(rwlock_t *rw)
{
	int priorvalue = atomic_fetch_sub_explicit(&rw->lock, 1, memory_order_acquire);
	/**
		@Begin
		@Commit_point_define_check: true
		@Label:Read_Trylock_Point
		@End
	*/
	if (priorvalue > 0)
		return 1;

	atomic_fetch_add_explicit(&rw->lock, 1, memory_order_relaxed);
	return 0;
}

/**
	@Begin
	@Interface_decl_define: Write_Trylock
	@Commit_point_set:
		Write_Trylock_Point
	@Condition:
		!writer_lock_acquired && reader_lock_cnt == 0
	@HB_condition:
		HB_Write_Trylock_Succ :: __RET__ == 1
	@Action:
		if (__COND_SAT__)
			writer_lock_acquired = true;
	@Post_check:
		__COND_SAT__ ? __RET__ == 1 : __RET__ == 0
	@End
*/
static inline int write_trylock(rwlock_t *rw)
{
	int priorvalue = atomic_fetch_sub_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_acquire);
	/**
		@Begin
		@Commit_point_define_check: true
		@Label: Write_Trylock_Point
		@End
	*/
	if (priorvalue == RW_LOCK_BIAS)
		return 1;

	atomic_fetch_add_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_relaxed);
	return 0;
}

/**
	@Begin
	@Interface_decl_define: Read_Unlock
	@Commit_point_set: Read_Unlock_Point
	@Check:
		reader_lock_cnt > 0 && !writer_lock_acquired
	@Action: 
		reader_lock_cnt--;
	@End
*/
static inline void read_unlock(rwlock_t *rw)
{
	atomic_fetch_add_explicit(&rw->lock, 1, memory_order_release);
	/**
		@Begin
		@Commit_point_define_check: true
		@Label: Read_Unlock_Point
		@End
	*/
}

/**
	@Begin
	@Interface_decl_define: Write_Unlock
	@Commit_point_set: Write_Unlock_Point
	@Check:
		reader_lock_cnt == 0 && writer_lock_acquired
	@Action: 
		writer_lock_acquired = false;
	@End
*/
static inline void write_unlock(rwlock_t *rw)
{
	atomic_fetch_add_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_release);
	/**
		@Begin
		@Commit_point_define_check: true
		@Label: Write_Unlock_Point
		@End
	*/
}

rwlock_t mylock;
int shareddata;

static void a(void *obj)
{
	int i;
	for(i = 0; i < 2; i++) {
		if ((i % 2) == 0) {
			read_lock(&mylock);
			load_32(&shareddata);
			read_unlock(&mylock);
		} else {
			write_lock(&mylock);
			store_32(&shareddata,(unsigned int)i);
			write_unlock(&mylock);
		}
	}
}

int user_main(int argc, char **argv)
{
	/**
		@Begin
		@Entry_point
		@End
	*/
	thrd_t t1, t2;
	atomic_init(&mylock.lock, RW_LOCK_BIAS);

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&a, NULL);

	thrd_join(t1);
	thrd_join(t2);

	return 0;
}
