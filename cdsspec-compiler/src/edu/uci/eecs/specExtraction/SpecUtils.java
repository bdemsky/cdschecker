package edu.uci.eecs.specExtraction;

import java.io.File;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>
 * This class contains a list of static utility functions for extracting
 * specification annotations more conveniently.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class SpecUtils {

	/**
	 * <p>
	 * This inner class defines a primitive --- a sub-annotation. For example,
	 * the "@DeclareState" in the global state is one primitive. We record the
	 * beginning line number of that primitive, the name of the primitive and a
	 * list of lines that belong to that primitive.
	 * </p>
	 * 
	 * @author Peizhao Ou
	 * 
	 */
	public static class Primitive {
		public final int beginLineNum;
		public final String name;
		public final ArrayList<String> contents;

		public Primitive(String name, int beginLineNum) {
			this.name = name;
			this.beginLineNum = beginLineNum;
			contents = new ArrayList<String>();
		}

		public void addLine(String line) {
			contents.add(line);
		}

		public String toString() {
			return "@" + name + ":\n" + contents;
		}
	};

	/**
	 * <p>
	 * This is a customized wrap of integer so that we can use integer type as
	 * if they were in a C/C++ pass-by-reference fashion. It basically wraps an
	 * int primitive to allow read, write and increment its internal int value.
	 * </p>
	 * 
	 * @author Peizhao Ou
	 * 
	 */
	public static class IntObj {
		private int value;

		public IntObj(int value) {
			this.value = value;
		}

		public void setVal(int value) {
			this.value = value;
		}

		public int getVal() {
			return this.value;
		}

		public void dec() {
			this.value--;
		}

		public void inc() {
			this.value++;
		}

		public String toString() {
			return Integer.toString(value);
		}
	}

	// A regular expression pattern that matches "@WORD:"
	public static final Pattern regexpPrimitive = Pattern.compile("@(\\w+):");
	// The matcher for the primitive regular expression
	public static final Matcher matcherPrimitive = regexpPrimitive.matcher("");

	// Match code after colon: ":" + Code + "$"
	public static final Pattern regexpCode = Pattern.compile(":(.*)$");
	public static final Matcher matcherCode = regexpCode.matcher("");

	// Match a valid word: "^\w+$"
	public static final Pattern regexpWord = Pattern.compile("^\\w+$");
	public static final Matcher matcherWord = regexpWord.matcher("");

	/**
	 * <p>
	 * This function will look for the first line that contains a primitive from
	 * the "curIdx", and then extracts all the non-empty lines of that primitive
	 * until it gets to the end of the list or the beginning of the next
	 * primitive. It returns a list of the actual code for that primitive. When
	 * we are done with the function call, the curIdx either points to the next
	 * primitive or the end of the annotation list.
	 * </p>
	 * 
	 * @param file
	 *            The file begin processing
	 * @param beginLineNum
	 *            The beginning line number of the first annotation lines
	 * @param annotations
	 *            The list of annotations
	 * @param curIdx
	 *            The current index (the index of the line that we are
	 *            processing). We will update the current index after calling
	 *            this function. Keep in mind that this is a customized wrap of
	 *            integer so that we can use it as if it is a C/C++ reference
	 * @return The primitive starting from the curIdx till either the end of the
	 *         annotations or the beginning line of the next primitive
	 * @throws WrongAnnotationException
	 */
	public static Primitive extractPrimitive(File file, int beginLineNum,
			ArrayList<String> annotations, IntObj curIdx)
			throws WrongAnnotationException {
		if (curIdx.getVal() == annotations.size()) // The current index points
													// to the end
			// of the list
			return null;

		String line = null;
		int curLineNum = -1;
		Primitive primitive = null;
		// Find the first primitive
		for (; curIdx.getVal() < annotations.size(); curIdx.inc()) {
			line = annotations.get(curIdx.getVal());
			curLineNum = curIdx.getVal() + beginLineNum;
			matcherPrimitive.reset(line);
			if (matcherPrimitive.find()) {
				String name = matcherPrimitive.group(1);
				primitive = new Primitive(name, curLineNum);
				break;
			}
		}
		// Assert that we must have found one primitive
		if (primitive == null) {
			WrongAnnotationException
					.err(file, curLineNum,
							"Something is wrong! We must have found one primitve here!\n");
		}

		// Process the current "primitive"
		// Deal with the first special line. E.g. @DeclareState: int x;
		String code = null;
		matcherCode.reset(line);
		if (matcherCode.find()) {
			code = matcherCode.group(1);
			String trimmedCode = trimSpace(trimTrailingCommentSymbol(code));
			if (!trimmedCode.equals("")) {
				primitive.addLine(trimmedCode);
			}
		} else {
			WrongAnnotationException
					.err(file, curLineNum,
							"The state annotation should have correct primitive syntax (sub-annotations)");
		}

		// Deal with other normal line. E.g. y = 1;
		curIdx.inc();
		;
		for (; curIdx.getVal() < annotations.size(); curIdx.inc()) {
			curLineNum = beginLineNum + curIdx.getVal();
			line = annotations.get(curIdx.getVal());
			matcherPrimitive.reset(line);
			if (!matcherPrimitive.find()) {
				// This is another line that we should add to the result
				code = trimSpace(trimTrailingCommentSymbol(line));
				if (!code.equals(""))
					primitive.addLine(code);
			} else
				// We get to the end of the current primitive
				break;
		}

		if (primitive.contents.size() == 0) { // The content of the primitive is
												// empty
			WrongAnnotationException.warning(file, curLineNum, "Primitive "
					+ primitive.name + " is empty.");
		}
		return primitive;
	}

	/**
	 * 
	 * @param line
	 *            The line to be processed
	 * @return The string whose beginning and ending space have been trimmed
	 */
	public static String trimSpace(String line) {
		// "^\s*(.*?)\s*$"
		Pattern regexp = Pattern.compile("^\\s*(.*?)\\s*$");
		Matcher matcher = regexp.matcher(line);
		if (matcher.find()) {
			return matcher.group(1);
		} else {
			return line;
		}
	}

	/**
	 * <p>
	 * It processed the line in a way that it removes the trailing C/C++ comment
	 * symbols "STAR SLASH"
	 * </p>
	 * 
	 * @param line
	 * @return
	 */
	public static String trimTrailingCommentSymbol(String line) {
		Pattern regexp = Pattern.compile("(.*?)\\s*(\\*/)?\\s*$");
		Matcher matcher = regexp.matcher(line);
		if (matcher.find()) {
			return matcher.group(1);
		} else {
			return null;
		}
	}

	public static boolean isUserDefinedStruct(String type) {
		// FIXME: We only consider the type is either a one-level pointer or a
		// struct
		String bareType = trimSpace(type.replace('*', ' '));
		return !bareType.equals("int") && !bareType.matches("unsigned\\s+int")
				&& !bareType.equals("unsigned") && !bareType.equals("bool")
				&& !bareType.equals("double") && !bareType.equals("float")
				&& !bareType.equals("void");
	}

	public static String getPlainType(String type) {
		// FIXME: We only consider the type is either a one-level pointer or a
		// struct
		String bareType = trimSpace(type.replace('*', ' '));
		return bareType;
	}

	/**
	 * <p>
	 * This function will automatically generate the printing statements for
	 * supported types when given a type and a name of the declaration.
	 * </p>
	 * 
	 * @return The auto-generated state printing statements
	 */
	public static Code generatePrintStatement(String type, String name) {
		Code code = new Code();
		// Primitive types
		if (type.equals("int") || type.equals("unsigned")
				|| type.equals("unsigned int") || type.equals("int unsigned")
				|| type.equals("double") || type.equals("double")
				|| type.equals("bool")) {
			// PRINT("\tx=%d\n", x);
			code.addLine(SpecNaming.PRINT + "(\"\\t" + name + "=%d\\n\", "
					+ name + ");");
		} else if (type.equals("int *") || type.equals("unsigned *")
				|| type.equals("unsigned int *")
				|| type.equals("int unsigned *") || type.equals("double *")
				|| type.equals("double *") || type.equals("bool *")) {
			// Supported pointer types for primitive types
			// PRINT("\t*x=%d\n", *x);
			code.addLine(SpecNaming.PRINT + "(\"\\t*" + name + "=%d\\n\", *"
					+ name + ");");
		} else if (type.equals("IntList") || type.equals("IntSet")
				|| type.equals("IntMap")) {
			// Supported types
			// PRINT("\tq: ");
			// printContainer(&q);
			// model_print("\n");
			code.addLine(SpecNaming.PRINT + "(\"\\t" + name + ": \");");
			if (type.equals("IntMap")) {
				code.addLine(SpecNaming.PrintMap + "(&" + name + ");");
			} else {
				code.addLine(SpecNaming.PrintContainer + "(&" + name + ");");
			}
			code.addLine(SpecNaming.PRINT + "(\"\\n\");");
		} else if (type.equals("IntList *") || type.equals("IntSet *")
				|| type.equals("IntMap *")) {
			// Supported pointer types
			// PRINT("\tq: ");
			// printContainer(q);
			// model_print("\n");
			code.addLine(SpecNaming.PRINT + "(\"\\t" + name + ": \");");
			if (type.equals("IntMap *")) {
				code.addLine(SpecNaming.PrintMap + "(" + name + ");");
			} else {
				code.addLine(SpecNaming.PrintContainer + "(" + name + ");");
			}
			code.addLine(SpecNaming.PRINT + "(\"\\n\");");
		} else if (type.equals("void")) {
			// Just do nothing!
		} else {
			if (type.endsWith("*")) { // If it's an obvious pointer (with a STAR)
				// Weak support pointer types (just print out the address)
				// PRINT("\tmutex=%p\n", mutex);
				code.addLine(SpecNaming.PRINT + "(\"\\t" + name + "=%p\\n\", "
						+ name + ");");
			} else {
				code.addLine("// We do not support auto-gen print-out for type: "
						+ type + ".");
			}
			
		}

		return code;
	}

}
