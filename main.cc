/** @file main.cc
 *  @brief Entry point for the model checker.
 */

#include <unistd.h>

#include "libthreads.h"
#include "common.h"
#include "threads.h"

#include "datarace.h"

/* global "model" object */
#include "model.h"
#include "snapshot-interface.h"

static void param_defaults(struct model_params * params) {
	params->maxreads = 0;
	params->maxfuturedelay = 100;
	params->fairwindow = 0;
	params->enabledcount = 1;
}

static void print_usage(struct model_params *params) {
	printf(
"Usage: <program name> [MC_OPTIONS] -- [PROGRAM ARGUMENTS]\n"
"\n"
"Options:\n"
"-h                    Display this help message and exit\n"
"-m                    Maximum times a thread can read from the same write\n"
"                      while other writes exist. Default: %d\n"
"-s                    Maximum actions that the model checker will wait for\n"
"                      a write from the future past the expected number of\n"
"                      actions. Default: %d\n"
"-f                    Specify a fairness window in which actions that are\n"
"                      enabled sufficiently many times should receive\n"
"                      priority for execution. Default: %d\n"
"-e                    Enabled count. Default: %d\n"
"--                    Program arguments follow.\n\n",
params->maxreads, params->maxfuturedelay, params->fairwindow, params->enabledcount);
	exit(EXIT_SUCCESS);
}

static void parse_options(struct model_params *params, int *argc, char ***argv) {
	const char *shortopts = "hm:s:f:e:";
	int opt;
	bool error = false;
	while (!error && (opt = getopt(*argc, *argv, shortopts)) != -1) {
		switch (opt) {
		case 'h':
			print_usage(params);
			break;
		case 's':
			params->maxfuturedelay = atoi(optarg);
			break;
		case 'f':
			params->fairwindow = atoi(optarg);
			break;
		case 'e':
			params->enabledcount = atoi(optarg);
			break;
		case 'm':
			params->maxreads = atoi(optarg);
			break;
		default: /* '?' */
			error = true;
			break;
		}
	}
	(*argc) -= optind;
	(*argv) += optind;

	if (error)
		print_usage(params);
}

int main_argc;
char **main_argv;

/** The real_main function contains the main model checking loop. */
static void real_main() {
	thrd_t user_thread;
	struct model_params params;

	param_defaults(&params);

	parse_options(&params, &main_argc, &main_argv);

	//Initialize race detector
	initRaceDetector();

	//Create the singleton SnapshotStack object
	snapshotObject = new SnapshotStack();

	model = new ModelChecker(params);

	snapshotObject->snapshotStep(0);
	int c=0;
	do {
		/* Start user program */
		model->add_thread(new Thread(&user_thread, (void (*)(void *)) &user_main, NULL));

		/* Wait for all threads to complete */
		model->finish_execution();
		if (c++>4000) break;
			
	} while (model->next_execution());

	delete model;

	DEBUG("Exiting\n");
}

/**
 * Main function.  Just initializes snapshotting library and the
 * snapshotting library calls the real_main function.
 */
int main(int argc, char ** argv) {
	main_argc = argc;
	main_argv = argv;

	/* Let's jump in quickly and start running stuff */
	initSnapShotLibrary(10000, 1024, 1024, 1000, &real_main);
}
