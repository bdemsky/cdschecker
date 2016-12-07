package edu.uci.eecs.specExtraction;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.LineNumberReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import edu.uci.eecs.codeGenerator.CodeGeneratorUtils;
import edu.uci.eecs.codeGenerator.Environment;
import edu.uci.eecs.utilParser.ParseException;

/**
 * <p>
 * This class represents the specification extractor of the specification. The
 * main function of this class is to read C/C++11 source files and extract the
 * corresponding specifications, and record corresponding information such as
 * location, e.g., the file name and the line number, to help the code
 * generation process.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class SpecExtractor {
	public final HashMap<File, ArrayList<DefineConstruct>> defineListMap;
	public final HashMap<File, ArrayList<InterfaceConstruct>> interfaceListMap;
	public final HashMap<File, ArrayList<OPConstruct>> OPListMap;
	public final HashSet<String> OPLabelSet;
	// Note that we only allow one entry per file at most
	public final HashMap<File, EntryConstruct> entryMap;

	public final HashSet<String> headerFiles;

	// In the generated header file, we need to forward the user-defined
	public final HashSet<String> forwardClass;

	private GlobalConstruct globalConstruct;

	public SpecExtractor() {
		defineListMap = new HashMap<File, ArrayList<DefineConstruct>>();
		interfaceListMap = new HashMap<File, ArrayList<InterfaceConstruct>>();
		OPListMap = new HashMap<File, ArrayList<OPConstruct>>();
		OPLabelSet = new HashSet<String>();
		entryMap = new HashMap<File, EntryConstruct>();
		headerFiles = new HashSet<String>();
		forwardClass = new HashSet<String>();
		globalConstruct = null;
	}

	private void addDefineConstruct(DefineConstruct construct) {
		ArrayList<DefineConstruct> list = defineListMap.get(construct.file);
		if (list == null) {
			list = new ArrayList<DefineConstruct>();
			defineListMap.put(construct.file, list);
		}
		list.add(construct);
	}

	private void addInterfaceConstruct(InterfaceConstruct construct) {
		ArrayList<InterfaceConstruct> list = interfaceListMap
				.get(construct.file);
		if (list == null) {
			list = new ArrayList<InterfaceConstruct>();
			interfaceListMap.put(construct.file, list);
		}
		list.add(construct);
	}

	private void addOPConstruct(OPConstruct construct) {
		ArrayList<OPConstruct> list = OPListMap.get(construct.file);
		if (list == null) {
			list = new ArrayList<OPConstruct>();
			OPListMap.put(construct.file, list);
		}
		list.add(construct);
	}

	private void addEntryConstruct(File file, EntryConstruct construct)
			throws WrongAnnotationException {
		EntryConstruct old = entryMap.get(file);
		if (old == null)
			entryMap.put(file, construct);
		else { // Error processing
			String errMsg = "Multiple @Entry annotations in the same file.\n\t Other @Entry at Line "
					+ old.beginLineNum + ".";
			WrongAnnotationException.err(file, construct.beginLineNum, errMsg);
		}
	}

	public GlobalConstruct getGlobalConstruct() {
		return this.globalConstruct;
	}

	/**
	 * <p>
	 * A print out function for the purpose of debugging. Note that we better
	 * call this function after having called the checkSemantics() function to
	 * check annotation consistency.
	 * </p>
	 */
	public void printAnnotations() {
		System.out
				.println("/**********    Print out of specification extraction    **********/");
		System.out.println("// Extracted header files");
		for (String header : headerFiles)
			System.out.println(header);

		System.out.println("// Global State Construct");
		if (globalConstruct != null)
			System.out.println(globalConstruct);

		for (File file : interfaceListMap.keySet()) {
			ArrayList<InterfaceConstruct> list = interfaceListMap.get(file);
			System.out.println("// Interface in file: " + file.getName());
			for (InterfaceConstruct construct : list) {
				System.out.println(construct);
				System.out.println("EndLineNumFunc: "
						+ construct.getEndLineNumFunction());
			}
		}

		for (File file : OPListMap.keySet()) {
			System.out.println("// Ordering points in file: " + file.getName());
			ArrayList<OPConstruct> list = OPListMap.get(file);
			for (OPConstruct construct : list)
				System.out.println(construct);
		}

		for (File file : entryMap.keySet()) {
			System.out.println("// Entry in file: " + file.getName());
			System.out.println(entryMap.get(file));
		}
	}

	/**
	 * <p>
	 * Perform basic semantics checking of the extracted specification.
	 * </p>
	 * 
	 * @return
	 * @throws WrongAnnotationException
	 */
	public void checkSemantics() throws WrongAnnotationException {
		String errMsg = null;

		// Assert that we have defined and only defined one global state
		// annotation
		if (globalConstruct == null) {
			errMsg = "Spec error: There should be one global state annotation.\n";
			throw new WrongAnnotationException(errMsg);
		}

		// Assert that the interface constructs have unique label name
		HashMap<String, InterfaceConstruct> interfaceMap = new HashMap<String, InterfaceConstruct>();
		for (File f : interfaceListMap.keySet()) {
			ArrayList<InterfaceConstruct> list = interfaceListMap.get(f);
			if (list != null) {
				for (InterfaceConstruct construct : list) {
					InterfaceConstruct existingConstruct = interfaceMap
							.get(construct.getName());
					if (existingConstruct != null) { // Error
						errMsg = "Interface labels duplication with: \""
								+ construct.getName() + "\" in File \""
								+ existingConstruct.file.getName()
								+ "\", Line " + existingConstruct.beginLineNum
								+ ".";
						WrongAnnotationException.err(construct.file,
								construct.beginLineNum, errMsg);
					} else {
						interfaceMap.put(construct.getName(), construct);
					}
				}
			}
		}

		// Process ordering point labels
		for (File file : OPListMap.keySet()) {
			ArrayList<OPConstruct> list = OPListMap.get(file);
			for (OPConstruct construct : list) {
				if (construct.type == OPType.OPCheck
						|| construct.type == OPType.PotentialOP) {
					String label = construct.label;
					OPLabelSet.add(label);
				}
			}
		}

	}

	/**
	 * <p>
	 * This function applies on a String (a plain line of text) to check whether
	 * the current line is a C/C++ header include statement. If it is, it
	 * extracts the header file name and store it, and returns true; otherwise,
	 * it returns false.
	 * </p>
	 * 
	 * @param line
	 *            The line of text to be processed
	 * @return Returns true if the current line is a C/C++ header include
	 *         statement
	 */
	public boolean extractHeaders(String line) {
		// "^( |\t)*#include( |\t)+("|<)([a-zA-Z_0-9\-\.])+("|>)"
		Pattern regexp = Pattern
				.compile("^( |\\t)*(#include)( |\\t)+(\"|<)([a-zA-Z_0-9\\-\\.]+)(\"|>)");
		Matcher matcher = regexp.matcher(line);

		// process the line.
		if (matcher.find()) {
			String header = null;
			String braceSymbol = matcher.group(4);
			if (braceSymbol.equals("<"))
				header = "<" + matcher.group(5) + ">";
			else
				header = "\"" + matcher.group(5) + "\"";
			if (!SpecNaming.isPreIncludedHeader(header)) {
				headerFiles.add(header);
			}
			return true;
		} else
			return false;
	}

	/**
	 * <p>
	 * A sub-routine to extract the construct from beginning till end. When
	 * called, we have already match the beginning of the construct. We will
	 * call this sub-routine when we extract the interface construct and the
	 * global state construct.
	 * </p>
	 * 
	 * <p>
	 * The side effect of this function is that the lineReader has just read the
	 * end of the construct, meaning that the caller can get the end line number
	 * by calling lineReader.getLineNumber().
	 * </p>
	 * 
	 * @param file
	 *            The file that we are processing
	 * @param lineReader
	 *            The LineNumberReader that we are using when processing the
	 *            current file.
	 * @param file
	 *            The file that we are processing
	 * @param curLine
	 *            The current line that we are processing. It should be the
	 *            beginning line of the annotation construct.
	 * @param beginLineNum
	 *            The beginning line number of the interface construct
	 *            annotation
	 * @return Returns the annotation string list of the current construct
	 * @throws WrongAnnotationException
	 */
	private ArrayList<String> extractTillConstructEnd(File file,
			LineNumberReader lineReader, String curLine, int beginLineNum)
			throws WrongAnnotationException {
		ArrayList<String> annotations = new ArrayList<String>();
		annotations.add(curLine);
		// System.out.println(curLine);
		// Initial settings for matching lines
		// "\*/\s*$"
		Pattern regexpEnd = Pattern.compile("\\*/\\s*$");
		Matcher matcher = regexpEnd.matcher(curLine);
		if (matcher.find()) {
			// The beginning line is also the end line
			// In this case, we have already add the curLine
			return annotations;
		} else {
			try {
				String line;
				while ((line = lineReader.readLine()) != null) {
					// process the line.
					// System.out.println(line);

					matcher.reset(line); // reset the input
					annotations.add(line);
					if (matcher.find())
						return annotations;
				}
				WrongAnnotationException
						.err(file,
								beginLineNum,
								"The interface annotation should have the matching closing symbol closing \"*/\"");
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		return null;
	}

	/**
	 * <p>
	 * A sub-routine to extract the global construct. When called, we have
	 * already match the beginning of the construct.
	 * </p>
	 * 
	 * @param file
	 *            The file that we are processing
	 * @param lineReader
	 *            The LineNumberReader that we are using when processing the
	 *            current file.
	 * @param curLine
	 *            The current line that we are processing. It should be the
	 *            beginning line of the annotation construct.
	 * @param beginLineNum
	 *            The beginning line number of the interface construct
	 *            annotation
	 * @throws WrongAnnotationException
	 */
	private void extractGlobalConstruct(File file, LineNumberReader lineReader,
			String curLine, int beginLineNum) throws WrongAnnotationException {
		ArrayList<String> annotations = extractTillConstructEnd(file,
				lineReader, curLine, beginLineNum);
		GlobalConstruct construct = new GlobalConstruct(file, beginLineNum,
				annotations);
		if (globalConstruct != null) { // Check if we have seen a global state
										// construct earlier
			File otherDefinitionFile = globalConstruct.file;
			int otherDefinitionLine = globalConstruct.beginLineNum;
			String errMsg = "Multiple definition of global state.\n"
					+ "\tAnother definition is in File \""
					+ otherDefinitionFile.getName() + "\" (Line "
					+ otherDefinitionLine + ").";
			WrongAnnotationException.err(file, beginLineNum, errMsg);
		}
		globalConstruct = construct;
	}

	/**
	 * @param file
	 *            The current file we are processing
	 * @param lineReader
	 *            Call this function when the lineReader will read the beginning
	 *            of the definition right away
	 * @param startingLine
	 *            The line that we should start processing
	 * @return The line number of the ending line of the interfae definition. If
	 *         returning -1, it means the curl symbols in the interface do not
	 *         match
	 * @throws WrongAnnotationException
	 */
	private int findEndLineNumFunction(File file, LineNumberReader lineReader,
			String startingLine) throws WrongAnnotationException {
		String line = startingLine;
		// FIXME: We assume that in the string of the code, there does not exist
		// the symbol '{' & '{'
		try {
			boolean foundFirstCurl = false;
			int unmatchedCnt = 0;
			do {
				// process the line.
				// System.out.println(line);

				// Extract the one-liner construct first
				extractOneLineConstruct(file, lineReader.getLineNumber(), line);

				for (int i = 0; i < line.length(); i++) {
					char ch = line.charAt(i);
					if (ch == '{') {
						foundFirstCurl = true;
						unmatchedCnt++;
					} else if (ch == '}') {
						unmatchedCnt--;
					}
					// The current line is the end of the function
					if (foundFirstCurl && unmatchedCnt == 0) {
						int endLineNumFunction = lineReader.getLineNumber();
						return endLineNumFunction;
					}
				}
			} while ((line = lineReader.readLine()) != null);
		} catch (IOException e) {
			e.printStackTrace();
		}
		// -1 means the curl symbols in the interface do not match
		return -1;
	}

	/**
	 * <p>
	 * A sub-routine to extract the define construct. When called, we have
	 * already match the beginning of the construct, and we also need to find
	 * the ending line number of the anntotation.
	 * </p>
	 * 
	 * @param file
	 *            The file that we are processing
	 * @param lineReader
	 *            The LineNumberReader that we are using when processing the
	 *            current file.
	 * @param curLine
	 *            The current line that we are processing. It should be the
	 *            beginning line of the annotation construct.
	 * @param beginLineNum
	 *            The beginning line number of the interface construct
	 *            annotation
	 * @throws WrongAnnotationException
	 * @throws IOException
	 * @throws ParseException
	 */
	private void extractDefineConstruct(File file, LineNumberReader lineReader,
			String curLine, int beginLineNum) throws WrongAnnotationException,
			IOException, ParseException {
		ArrayList<String> annotations = extractTillConstructEnd(file,
				lineReader, curLine, beginLineNum);
		int endLineNum = lineReader.getLineNumber();
		DefineConstruct construct = new DefineConstruct(file, beginLineNum,
				endLineNum, annotations);
		addDefineConstruct(construct);
	}

	/**
	 * <p>
	 * A sub-routine to extract the interface construct. When called, we have
	 * already match the beginning of the construct, and we also need to find
	 * the ending line number of the closing brace of the corresponding
	 * function.
	 * </p>
	 * 
	 * @param file
	 *            The file that we are processing
	 * @param lineReader
	 *            The LineNumberReader that we are using when processing the
	 *            current file.
	 * @param curLine
	 *            The current line that we are processing. It should be the
	 *            beginning line of the annotation construct.
	 * @param beginLineNum
	 *            The beginning line number of the interface construct
	 *            annotation
	 * @throws WrongAnnotationException
	 * @throws IOException
	 * @throws ParseException
	 */
	private void extractInterfaceConstruct(File file,
			LineNumberReader lineReader, String curLine, int beginLineNum)
			throws WrongAnnotationException, IOException, ParseException {
		ArrayList<String> annotations = extractTillConstructEnd(file,
				lineReader, curLine, beginLineNum);
		int endLineNum = lineReader.getLineNumber();
		InterfaceConstruct construct = new InterfaceConstruct(file,
				beginLineNum, endLineNum, annotations);
		addInterfaceConstruct(construct);

		// Process the corresponding interface function declaration header
		String line = null;
		int lineNum = -1;
		String errMsg;
		try {
			line = lineReader.readLine();
			lineNum = lineReader.getLineNumber();
			construct.processFunctionDeclaration(line);

			// Record those user-defined struct
			// RET
			String returnType = construct.getFunctionHeader().returnType;
			if (SpecUtils.isUserDefinedStruct(returnType))
				forwardClass.add(SpecUtils.getPlainType(returnType));
			// Arguments
			for (VariableDeclaration decl : construct.getFunctionHeader().args) {
				if (SpecUtils.isUserDefinedStruct(decl.type))
					forwardClass.add(SpecUtils.getPlainType(decl.type));
			}

		} catch (IOException e) {
			errMsg = "Spec error in file \""
					+ file.getName()
					+ "\", Line "
					+ lineNum
					+ " :\n\tThe function declaration should take only one line and have the correct syntax (follow the annotations immediately)\n";
			System.out.println(errMsg);
			throw e;
		} catch (ParseException e) {
			errMsg = "Spec error in file \""
					+ file.getName()
					+ "\", Line "
					+ lineNum
					+ " :\n\tThe function declaration should take only one line and have the correct syntax (follow the annotations immediately)\n";
			System.out.println(errMsg);
			throw e;
		}

		// Now we find the end of the interface definition
		int endLineNumFunction = findEndLineNumFunction(file, lineReader, line);
		construct.setEndLineNumFunction(endLineNumFunction);
		if (endLineNumFunction == -1) {
			WrongAnnotationException
					.err(file, beginLineNum,
							"The interface definition does NOT have matching curls '}'");
		}
	}

	/**
	 * <p>
	 * A sub-routine to extract the ordering point construct. When called, we
	 * have already match the beginning of the construct.
	 * </p>
	 * 
	 * @param file
	 *            The file that we are processing
	 * @param beginLineNum
	 *            The beginning line number of the interface construct
	 *            annotation
	 * @param curLine
	 *            The current line that we are processing. It should be the
	 *            beginning line of the annotation construct.
	 * @param type
	 *            The type of ordering point construct we are processing
	 * @throws WrongAnnotationException
	 */
	private void extractOPConstruct(File file, int beginLineNum,
			String curLine, OPType type) throws WrongAnnotationException {
		String condition = null;
		String label = null;

		// "(\(\s?(\w+)\s?\))?\s:\s?(.+)\*/\s?$"
		Pattern regexp = Pattern
				.compile("(\\(\\s*(\\w+)\\s*\\))?\\s*:\\s*(.+)\\*/\\s*$");
		Matcher matcher = regexp.matcher(curLine);
		if (matcher.find()) {
			label = matcher.group(2);
			condition = matcher.group(3);
		} else {
			WrongAnnotationException
					.err(file,
							beginLineNum,
							"Wrong syntax for the ordering point construct. You might need a colon before the condition.");
		}
		OPConstruct op = new OPConstruct(file, beginLineNum, type, label,
				condition, curLine);
		addOPConstruct(op);
	}

	/**
	 * <p>
	 * A sub-routine to extract the entry construct. When called, we have
	 * already match the beginning of the construct.
	 * </p>
	 * 
	 * @param file
	 *            The file that we are processing
	 * @param beginLineNum
	 *            The beginning line number of the interface construct
	 *            annotation
	 * @param curLine
	 *            Current line being processed
	 * @throws WrongAnnotationException
	 */
	public void extractEntryConstruct(File file, int beginLineNum,
			String curLine) throws WrongAnnotationException {
		addEntryConstruct(file, new EntryConstruct(file, beginLineNum, curLine));
	}

	/**
	 * <p>
	 * A sub-routine to extract those annotation constructs that take only one
	 * line --- Entry, OPDefine, PotentialOP, OPCheck, OPClear and OPClearDefin.
	 * </p>
	 * 
	 * @param file
	 *            The file that we are processing
	 * @param beginLineNum
	 *            The beginning line number of the interface construct
	 *            annotation
	 * @param curLine
	 *            The current line that we are processing. It should be the
	 *            beginning line of the annotation construct.
	 * @throws WrongAnnotationException
	 */
	private void extractOneLineConstruct(File file, int beginLineNum,
			String curLine) throws WrongAnnotationException {
		// "/\*\*\s*@(Entry|OPDefine|PotentialOP|OPCheck|OPClear|OPClearDefine)"
		Pattern regexpBegin = Pattern.compile("/\\*\\*\\s*@(\\w+)");
		Matcher matcher = regexpBegin.matcher(curLine);
		matcher.reset(curLine);
		if (matcher.find()) {
			String name = matcher.group(1);
			if (name.equals("Entry"))
				extractEntryConstruct(file, beginLineNum, curLine);
			else if (name.equals("OPDefine") || name.equals("PotentialOP")
					|| name.equals("OPCheck") || name.equals("OPClear")
					|| name.equals("OPClearDefine")
					|| name.equals("OPDefineUnattached")
					|| name.equals("OPClearDefineUnattached"))
				extractOPConstruct(file, beginLineNum, curLine,
						OPType.valueOf(name));
		}
	}

	/**
	 * <p>
	 * This function will process a given C/C++ file ( .h, .c or .cc). It will
	 * extract all the headers included in that file, and all the annotation
	 * constructs specified in that file. We then will store the information in
	 * the corresponding containers.
	 * </p>
	 * 
	 * <p>
	 * The basic idea is to read the file line by line, and then use regular
	 * expression to match the specific annotations or the header files.
	 * </p>
	 * 
	 * @param file
	 *            The file object of the corresponding file to be processed
	 * @throws WrongAnnotationException
	 * @throws ParseException
	 */
	public void extractConstruct(File file) throws WrongAnnotationException,
			ParseException {
		BufferedReader br = null;
		LineNumberReader lineReader = null;
		try {
			// Initial settings for processing the lines
			br = new BufferedReader(new FileReader(file));
			lineReader = new LineNumberReader(br);
			// "/\*\*\s*@(DeclareState|Interface)"
			Pattern regexpBegin = Pattern
					.compile("/\\*\\*\\s*@(DeclareState|Interface|PreCondition|JustifyingPrecondition|Transition|JustifyingPostcondition|PostCondition|Define)");
			Matcher matcher = regexpBegin.matcher("");

			String line;
			while ((line = lineReader.readLine()) != null) {
				// Start to process the line

				// First try to process the line to see if it's a header file
				// include
				boolean succ = extractHeaders(line);
				if (succ) // It's a header line and we successfully extract it
					continue;

				int beginLineNum = lineReader.getLineNumber();
				// Extract the one-liner construct first
				extractOneLineConstruct(file, beginLineNum, line);

				// Now we process the line to see if it's an annotation (State
				// or Interface)
				matcher.reset(line); // reset the input
				if (matcher.find()) { // Found the beginning line
					// The matching annotation name
					String constructName = matcher.group(1);

					// Process each annotation accordingly
					if (constructName.equals(SpecNaming.DeclareState)) {
						extractGlobalConstruct(file, lineReader, line,
								beginLineNum);
					} else if (constructName.equals(SpecNaming.Interface)
							|| constructName.equals(SpecNaming.PreCondition)
							|| constructName.equals(SpecNaming.JustifyingPrecondition)
							|| constructName.equals(SpecNaming.Transition)
							|| constructName.equals(SpecNaming.JustifyingPostcondition)
							|| constructName.equals(SpecNaming.PostCondition)) {
						extractInterfaceConstruct(file, lineReader, line,
								beginLineNum);
					} else if (constructName.equals(SpecNaming.Define)) {
						extractDefineConstruct(file, lineReader, line,
								beginLineNum);
					} else {
						WrongAnnotationException.err(file, beginLineNum,
								constructName
										+ " is not a supported annotation.");
					}

				}
			}
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			try {
				lineReader.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	/**
	 * <p>
	 * Given a list of files, it scans each file and add found SpecConstrcut to
	 * the _constructs list.
	 * </p>
	 * 
	 * @param files
	 *            The list of files that needs to be processed. In general, this
	 *            list only need to contain those that have specification
	 *            annotations
	 * @throws WrongAnnotationException
	 * @throws ParseException
	 */
	public void extract(File[] files) throws WrongAnnotationException,
			ParseException {
		for (int i = 0; i < files.length; i++)
			extract(files[i]);

		// Check basic specification semantics
		checkSemantics();
	}

	public void extract(ArrayList<File> files) throws WrongAnnotationException,
			ParseException {
		for (int i = 0; i < files.size(); i++)
			extract(files.get(i));

		// Check basic specification semantics
		checkSemantics();
	}

	/**
	 * <p>
	 * Extract the specification annotations and header files in the current
	 * file. This function should generally be called by extractFiles.
	 * </p>
	 * 
	 * @param files
	 *            The list of files that needs to be processed. In general, this
	 *            list only need to contain those that have specification
	 *            annotations
	 * @throws WrongAnnotationException
	 * @throws ParseException
	 */
	public void extract(File file) throws WrongAnnotationException,
			ParseException {
		extractConstruct(file);
	}
}
