package edu.uci.eecs.specExtraction;

import java.io.File;

/**
 * <p>
 * An abstract class for all different specification constructs, including
 * global construct, interface construct and ordering point construct.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
abstract public class Construct {
	// The file that this construct is in
	public final File file;
	// The beginning line number of this construct (the plain text line number)
	public final int beginLineNum;

	public Construct(File file, int beginLineNum) {
		this.file = file;
		this.beginLineNum = beginLineNum;
	}

	public String toString() {
		return file.getName() + ": Line " + Integer.toString(beginLineNum);
	}
}
