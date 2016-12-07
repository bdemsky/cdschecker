package edu.uci.eecs.codeGenerator;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import edu.uci.eecs.specExtraction.Code;

/**
 * <p>
 * This class represents all the code additions that should be added to a
 * specific file.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class CodeAdditions {

	/**
	 * <p>
	 * This class represents the addition of code for a specific file. It
	 * records a list of lines to be inserted for a specific file, and the line
	 * after which the code should be inserted to the file.
	 * </p>
	 * 
	 * @author Peizhao Ou
	 * 
	 */
	public static class CodeAddition {
		// The line after which the code should be inserted to the file
		// E.g. insertingLine == 0 => insert the lines ine very beginning.
		public final int insertingLine;

		// The code to be added to the specified place
		public final Code code;

		public CodeAddition(int insertingLine, Code code) {
			this.insertingLine = insertingLine;
			this.code = code;
		}

		public static Comparator<CodeAddition> lineNumComparator = new Comparator<CodeAddition>() {
			public int compare(CodeAddition addition1, CodeAddition addition2) {
				return addition1.insertingLine - addition2.insertingLine;
			}
		};
	}

	// A list of code addition for the same file
	public final ArrayList<CodeAddition> codeAdditions;

	// The file that the list of additions belong to
	public final File file;

	public CodeAdditions(File file) {
		this.file = file;
		codeAdditions = new ArrayList<CodeAddition>();
	}

	public void addCodeAddition(CodeAddition a) {
		this.codeAdditions.add(a);
	}

	/**
	 * <p>
	 * Whether the addition list is empty
	 * </p>
	 * 
	 * @return
	 */
	public boolean isEmpty() {
		return this.codeAdditions.size() == 0;
	}

	/**
	 * <p>
	 * Sort the list of code additions to the same file in an increasing order
	 * by the inserting line number of the code additions. We will call this
	 * function so that we can process code addition more conveniently.
	 * </p>
	 */
	public void sort() {
		Collections.sort(codeAdditions, CodeAddition.lineNumComparator);
	}
}
