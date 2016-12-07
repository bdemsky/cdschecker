package edu.uci.eecs.codeGenerator;

import java.util.ArrayList;

/**
 * <p>
 * This class contains some constant strings related to the code generation
 * process.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class Environment {
	public final static String HomeDir = System.getProperty("user.dir");
	public final static String ModelCheckerHome = System
			.getProperty("user.home")
			+ "/model-checker-priv/model-checker-priv/";
	public final static String BenchmarksDir = ModelCheckerHome
			+ "/benchmarks/";
	public final static String ModelCheckerTestDir = ModelCheckerHome
			+ "/test-cdsspec/";
	public final static String GeneratedFilesDir = ModelCheckerTestDir;

	public final static String REGISTER_ACQREL = "register-acqrel";
	public final static String REGISTER_RELAXED = "register-relaxed";
	public final static String MS_QUEUE = "ms-queue";
	public final static String LINUXRWLOCKS = "linuxrwlocks";
	public final static String MCS_LOCK = "mcs-lock";
	public final static String DEQUE = "chase-lev-deque-bugfix";
	public final static String DEQUE_BUGGY = "chase-lev-deque";
	public final static String TREIBER_STACK = "treiber-stack";
	public final static String TICKET_LOCK = "ticket-lock";
	public final static String SEQLOCK = "seqlock";
	public final static String READ_COPY_UPDATE = "read-copy-update";
	public final static String CONCURRENT_MAP = "concurrent-hashmap";
	public final static String SPSC = "spsc-bugfix";
	public final static String MPMC = "mpmc-queue";
	
	public final static String BLOCKING_QUEUE_EXAMPLE = "blocking-mpmc-example";
	
	public final static String[] Benchmarks = {
		MS_QUEUE,
		LINUXRWLOCKS,
		MCS_LOCK,
		DEQUE,
		DEQUE_BUGGY,
		TICKET_LOCK,
		SEQLOCK,
		READ_COPY_UPDATE,
		SPSC,
		CONCURRENT_MAP,
		MPMC,
		BLOCKING_QUEUE_EXAMPLE,
	}; 

}
