package edu.uci.eecs.specExtraction;

import java.util.ArrayList;

import edu.uci.eecs.specExtraction.SpecUtils.IntObj;

/**
 * <p>
 * This class represents a piece of code --- a list of strings (lines). Such a
 * wrapper makes code extraction and generation easier.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class Code {

	// The list that represents all the lines of this code snippet
	public final ArrayList<String> lines;

	public Code() {
		lines = new ArrayList<String>();
	}

	/**
	 * <p>
	 * It adds a line, which is very likely to be a simple C/C++ statement,
	 * which is a string lines of statement.
	 * </p>
	 * 
	 * @param line
	 *            A C/C++ line of statement
	 */
	public void addLine(String line) {
		lines.add(line);
	}

	/**
	 * <p>
	 * It adds a list of lines, which are very likely to be simple C/C++
	 * statements.
	 * </p>
	 * 
	 * @param line
	 *            A list of C/C++ lines of statement
	 */
	public void addLines(ArrayList<String> code) {
		for (int i = 0; i < code.size(); i++)
			lines.add(code.get(i));
	}

	/**
	 * <p>
	 * It adds a list of lines, which are very likely to be simple C/C++
	 * statements.
	 * </p>
	 * 
	 * @param line
	 *            A Code object --- list of C/C++ lines of statement
	 */
	public void addLines(Code code) {
		addLines(code.lines);
	}

	/**
	 * @return Whether this code snippet is empty
	 */
	public boolean isEmpty() {
		return lines.size() == 0;
	}

	/**
	 * <p>
	 * Align the set of code with an initial number of tabs. This basically
	 * tries to make the generated code more readable.
	 * 
	 * @param initialTabsCnt
	 *            The number of tabs that we want to put before the code
	 *            initially.
	 */
	public void align(int initialTabsCnt) {
		int tabLevel = initialTabsCnt;
		IntObj idx = new IntObj(0);
		alignHelper(idx, tabLevel, false);
	}

	/**
	 * <p>
	 * This is a helper function to align a list of code. Caller should
	 * initialize an IntObj with intial value "0", and pass with the initial tab
	 * level and pass the value of "false" to the noBraceKeyword.
	 * 
	 * @param idx
	 *            The IntObj that represents the current index of the code lines
	 * @param tabLevel
	 *            The tab level we are currently at
	 * @param noBraceKeyword
	 *            Whether we just encountered a "noBraceKeyword"
	 */
	private void alignHelper(IntObj idx, int tabLevel, boolean noBraceKeyword) {
		for (; idx.getVal() < lines.size(); idx.inc()) {
			String curLine = lines.get(idx.getVal());
			String newLine = null;
			// Return to the previous recursive level
			if (closingBrace(curLine)) {
				return;
			}
			if ((noBraceKeyword && !keywordBrace(curLine) && !keywordNoBrace(curLine))) {
				// Before returning, we just need to first add the line with the
				// current tab
				newLine = makeTabs(tabLevel) + curLine;
				lines.set(idx.getVal(), newLine);
				return;
			}

			newLine = makeTabs(tabLevel) + curLine;
			lines.set(idx.getVal(), newLine);

			if (keywordBrace(curLine)) {
				idx.inc();
				alignHelper(idx, tabLevel + 1, false);
				// Add the closing line
				curLine = lines.get(idx.getVal());
				newLine = makeTabs(tabLevel) + curLine;
				lines.set(idx.getVal(), newLine);
			} else if (keywordNoBrace(curLine)) { // No brace
				idx.inc();
				alignHelper(idx, tabLevel + 1, true);
				if (noBraceKeyword)
					return;
			}
		}
	}

	/**
	 * <p>
	 * Lines that starts with a key word and ends with ";".
	 * </p>
	 * 
	 * @param curLine
	 * @return
	 */
	private boolean closingBrace(String curLine) {
		return curLine.endsWith("}");
	}

	/**
	 * <p>
	 * Lines that starts with a key word and ends with "{".
	 * </p>
	 * 
	 * @param curLine
	 * @return
	 */
	private boolean keywordBrace(String curLine) {
		return (curLine.startsWith("for") || curLine.startsWith("ForEach")
				|| curLine.startsWith("if") || curLine.startsWith("else")
				|| curLine.startsWith("while") || curLine.startsWith("do"))
				&& curLine.endsWith("{");
	}

	/**
	 * <p>
	 * Lines that starts with a key word and ends with no "{" and no ";".
	 * </p>
	 * 
	 * @param curLine
	 * @return
	 */
	private boolean keywordNoBrace(String curLine) {
		return (curLine.startsWith("for") || curLine.startsWith("ForEach")
				|| curLine.startsWith("if") || curLine.startsWith("else")
				|| curLine.startsWith("while") || curLine.startsWith("do"))
				&& !curLine.endsWith("{") && !curLine.endsWith(";");
	}

	/**
	 * @param tabCnt
	 *            The number of tabs
	 * @return Generate a string whose content is a specific number (tabCnt) of
	 *         tab symbols.
	 */
	private String makeTabs(int tabCnt) {
		String res = "";
		for (int i = 0; i < tabCnt; i++)
			res = res + "\t";
		return res;
	}

	public String toString() {
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < lines.size(); i++) {
			sb.append(lines.get(i) + "\n");
		}
		return sb.toString();
	}
}
