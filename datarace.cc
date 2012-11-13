#include "datarace.h"
#include "model.h"
#include "threads-model.h"
#include <stdio.h>
#include <cstring>
#include "mymemory.h"
#include "clockvector.h"

struct ShadowTable *root;
std::vector<struct DataRace *> unrealizedraces;

/** This function initialized the data race detector. */
void initRaceDetector() {
	root = (struct ShadowTable *)snapshot_calloc(sizeof(struct ShadowTable), 1);
}

/** This function looks up the entry in the shadow table corresponding to a
 * given address.*/
static uint64_t * lookupAddressEntry(const void * address) {
	struct ShadowTable *currtable=root;
#if BIT48
	currtable=(struct ShadowTable *) currtable->array[(((uintptr_t)address)>>32)&MASK16BIT];
	if (currtable==NULL) {
		currtable = (struct ShadowTable *)(root->array[(((uintptr_t)address)>>32)&MASK16BIT] = snapshot_calloc(sizeof(struct ShadowTable), 1));
	}
#endif

	struct ShadowBaseTable * basetable=(struct ShadowBaseTable *) currtable->array[(((uintptr_t)address)>>16)&MASK16BIT];
	if (basetable==NULL) {
		basetable = (struct ShadowBaseTable *)(currtable->array[(((uintptr_t)address)>>16)&MASK16BIT] = snapshot_calloc(sizeof(struct ShadowBaseTable), 1));
	}
	return &basetable->array[((uintptr_t)address)&MASK16BIT];
}

/**
 * Compares a current clock-vector/thread-ID pair with a clock/thread-ID pair
 * to check the potential for a data race.
 * @param clock1 The current clock vector
 * @param tid1 The current thread; paired with clock1
 * @param clock2 The clock value for the potentially-racing action
 * @param tid2 The thread ID for the potentially-racing action
 * @return true if the current clock allows a race with the event at clock2/tid2
 */
static bool clock_may_race(ClockVector *clock1, thread_id_t tid1,
                           modelclock_t clock2, thread_id_t tid2)
{
	return tid1 != tid2 && clock2 != 0 && clock1->getClock(tid2) <= clock2;
}

/**
 * Expands a record from the compact form to the full form.  This is
 * necessary for multiple readers or for very large thread ids or time
 * stamps. */
static void expandRecord(uint64_t * shadow) {
	uint64_t shadowval=*shadow;

	modelclock_t readClock = READVECTOR(shadowval);
	thread_id_t readThread = int_to_id(RDTHREADID(shadowval));
	modelclock_t writeClock = WRITEVECTOR(shadowval);
	thread_id_t writeThread = int_to_id(WRTHREADID(shadowval));

	struct RaceRecord *record = (struct RaceRecord *)snapshot_calloc(1, sizeof(struct RaceRecord));
	record->writeThread=writeThread;
	record->writeClock=writeClock;

	if (readClock!=0) {
		record->capacity=INITCAPACITY;
		record->thread = (thread_id_t *)snapshot_malloc(sizeof(thread_id_t)*record->capacity);
		record->readClock = (modelclock_t *)snapshot_malloc(sizeof(modelclock_t)*record->capacity);
		record->numReads=1;
		record->thread[0]=readThread;
		record->readClock[0]=readClock;
	}
	*shadow=(uint64_t) record;
}

/** This function is called when we detect a data race.*/
static void reportDataRace(thread_id_t oldthread, modelclock_t oldclock, bool isoldwrite, ModelAction *newaction, bool isnewwrite, const void *address) {
	struct DataRace *race = (struct DataRace *)snapshot_malloc(sizeof(struct DataRace));
	race->oldthread=oldthread;
	race->oldclock=oldclock;
	race->isoldwrite=isoldwrite;
	race->newaction=newaction;
	race->isnewwrite=isnewwrite;
	race->address=address;
	unrealizedraces.push_back(race);

	/* If the race is realized, bail out now. */
	if (checkDataRaces()) {
		model->set_assert();
		model->switch_to_master(NULL);
	}
}

/** This function goes through the list of unrealized data races,
 *	removes the impossible ones, and print the realized ones. */

bool checkDataRaces() {
	if (model->isfeasibleprefix()) {
		/* Prune the non-racing unrealized dataraces */
		unsigned int i,newloc=0;
		for(i=0;i<unrealizedraces.size();i++) {
			struct DataRace * race=unrealizedraces[i];
			if (clock_may_race(race->newaction->get_cv(), race->newaction->get_tid(), race->oldclock, race->oldthread)) {
				unrealizedraces[newloc++]=race;
			}
		}
		if (newloc!=i)
			unrealizedraces.resize(newloc);

		if (unrealizedraces.size()!=0) {
			/* We have an actual realized race. */
			for(i=0;i<unrealizedraces.size();i++) {
				struct DataRace * race=unrealizedraces[i];
				printRace(race);
			}
			return true;
		}
	}
	return false;
}

void printRace(struct DataRace *race)
{
	printf("Datarace detected @ address %p:\n", race->address);
	printf("    Access 1: %5s in thread %2d @ clock %3u\n",
			race->isoldwrite ? "write" : "read",
			id_to_int(race->oldthread), race->oldclock);
	printf("    Access 2: %5s in thread %2d @ clock %3u\n",
			race->isnewwrite ? "write" : "read",
			id_to_int(race->newaction->get_tid()), race->newaction->get_seq_number());
}

/** This function does race detection for a write on an expanded record. */
void fullRaceCheckWrite(thread_id_t thread, void *location, uint64_t * shadow, ClockVector *currClock) {
	struct RaceRecord * record=(struct RaceRecord *) (*shadow);

	/* Check for datarace against last read. */

	for(int i=0;i<record->numReads;i++) {
		modelclock_t readClock = record->readClock[i];
		thread_id_t readThread = record->thread[i];

		/* Note that readClock can't actuall be zero here, so it could be
			 optimized. */

		if (clock_may_race(currClock, thread, readClock, readThread)) {
			/* We have a datarace */
			reportDataRace(readThread, readClock, false, model->get_parent_action(thread), true, location);
		}
	}

	/* Check for datarace against last write. */

	modelclock_t writeClock = record->writeClock;
	thread_id_t writeThread = record->writeThread;

	if (clock_may_race(currClock, thread, writeClock, writeThread)) {
		/* We have a datarace */
		reportDataRace(writeThread, writeClock, true, model->get_parent_action(thread), true, location);
	}

	record->numReads=0;
	record->writeThread=thread;
	modelclock_t ourClock = currClock->getClock(thread);
	record->writeClock=ourClock;
}

/** This function does race detection on a write. */
void raceCheckWrite(thread_id_t thread, void *location, ClockVector *currClock) {
	uint64_t * shadow=lookupAddressEntry(location);
	uint64_t shadowval=*shadow;

	/* Do full record */
	if (shadowval!=0&&!ISSHORTRECORD(shadowval)) {
		fullRaceCheckWrite(thread, location, shadow, currClock);
		return;
	}

	int threadid = id_to_int(thread);
	modelclock_t ourClock = currClock->getClock(thread);

	/* Thread ID is too large or clock is too large. */
	if (threadid > MAXTHREADID || ourClock > MAXWRITEVECTOR) {
		expandRecord(shadow);
		fullRaceCheckWrite(thread, location, shadow, currClock);
		return;
	}

	/* Check for datarace against last read. */

	modelclock_t readClock = READVECTOR(shadowval);
	thread_id_t readThread = int_to_id(RDTHREADID(shadowval));

	if (clock_may_race(currClock, thread, readClock, readThread)) {
		/* We have a datarace */
		reportDataRace(readThread, readClock, false, model->get_parent_action(thread), true, location);
	}

	/* Check for datarace against last write. */

	modelclock_t writeClock = WRITEVECTOR(shadowval);
	thread_id_t writeThread = int_to_id(WRTHREADID(shadowval));

	if (clock_may_race(currClock, thread, writeClock, writeThread)) {
		/* We have a datarace */
		reportDataRace(writeThread, writeClock, true, model->get_parent_action(thread), true, location);
	}
	*shadow = ENCODEOP(0, 0, threadid, ourClock);
}

/** This function does race detection on a read for an expanded record. */
void fullRaceCheckRead(thread_id_t thread, const void *location, uint64_t * shadow, ClockVector *currClock) {
	struct RaceRecord * record=(struct RaceRecord *) (*shadow);

	/* Check for datarace against last write. */

	modelclock_t writeClock = record->writeClock;
	thread_id_t writeThread = record->writeThread;

	if (clock_may_race(currClock, thread, writeClock, writeThread)) {
		/* We have a datarace */
		reportDataRace(writeThread, writeClock, true, model->get_parent_action(thread), false, location);
	}

	/* Shorten vector when possible */

	int copytoindex=0;

	for(int i=0;i<record->numReads;i++) {
		modelclock_t readClock = record->readClock[i];
		thread_id_t readThread = record->thread[i];

		/*  Note that is not really a datarace check as reads cannott
				actually race.  It is just determining that this read subsumes
				another in the sense that either this read races or neither
				read races. Note that readClock can't actually be zero, so it
				could be optimized.  */

		if (clock_may_race(currClock, thread, readClock, readThread)) {
			/* Still need this read in vector */
			if (copytoindex!=i) {
				record->readClock[copytoindex]=record->readClock[i];
				record->thread[copytoindex]=record->thread[i];
			}
			copytoindex++;
		}
	}

	if (copytoindex>=record->capacity) {
		int newCapacity=record->capacity*2;
		thread_id_t *newthread = (thread_id_t *)snapshot_malloc(sizeof(thread_id_t)*newCapacity);
		modelclock_t *newreadClock =( modelclock_t *)snapshot_malloc(sizeof(modelclock_t)*newCapacity);
		std::memcpy(newthread, record->thread, record->capacity*sizeof(thread_id_t));
		std::memcpy(newreadClock, record->readClock, record->capacity*sizeof(modelclock_t));
		snapshot_free(record->readClock);
		snapshot_free(record->thread);
		record->readClock=newreadClock;
		record->thread=newthread;
		record->capacity=newCapacity;
	}

	modelclock_t ourClock = currClock->getClock(thread);

	record->thread[copytoindex]=thread;
	record->readClock[copytoindex]=ourClock;
	record->numReads=copytoindex+1;
}

/** This function does race detection on a read. */
void raceCheckRead(thread_id_t thread, const void *location, ClockVector *currClock) {
	uint64_t * shadow=lookupAddressEntry(location);
	uint64_t shadowval=*shadow;

	/* Do full record */
	if (shadowval!=0&&!ISSHORTRECORD(shadowval)) {
		fullRaceCheckRead(thread, location, shadow, currClock);
		return;
	}

	int threadid = id_to_int(thread);
	modelclock_t ourClock = currClock->getClock(thread);

	/* Thread ID is too large or clock is too large. */
	if (threadid > MAXTHREADID || ourClock > MAXWRITEVECTOR) {
		expandRecord(shadow);
		fullRaceCheckRead(thread, location, shadow, currClock);
		return;
	}

	/* Check for datarace against last write. */

	modelclock_t writeClock = WRITEVECTOR(shadowval);
	thread_id_t writeThread = int_to_id(WRTHREADID(shadowval));

	if (clock_may_race(currClock, thread, writeClock, writeThread)) {
		/* We have a datarace */
		reportDataRace(writeThread, writeClock, true, model->get_parent_action(thread), false, location);
	}

	modelclock_t readClock = READVECTOR(shadowval);
	thread_id_t readThread = int_to_id(RDTHREADID(shadowval));

	if (clock_may_race(currClock, thread, readClock, readThread)) {
		/* We don't subsume this read... Have to expand record. */
		expandRecord(shadow);
		fullRaceCheckRead(thread, location, shadow, currClock);
		return;
	}

	*shadow = ENCODEOP(threadid, ourClock, id_to_int(writeThread), writeClock);
}
