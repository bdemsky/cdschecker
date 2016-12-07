package edu.uci.eecs.codeGenerator;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.LineNumberReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;

import edu.uci.eecs.codeGenerator.CodeAdditions.CodeAddition;
import edu.uci.eecs.specExtraction.Code;
import edu.uci.eecs.specExtraction.Construct;
import edu.uci.eecs.specExtraction.DefineConstruct;
import edu.uci.eecs.specExtraction.EntryConstruct;
import edu.uci.eecs.specExtraction.InterfaceConstruct;
import edu.uci.eecs.specExtraction.OPConstruct;
import edu.uci.eecs.specExtraction.SpecExtractor;
import edu.uci.eecs.specExtraction.SpecNaming;
import edu.uci.eecs.specExtraction.WrongAnnotationException;
import edu.uci.eecs.utilParser.ParseException;

/**
 * <p>
 * This class represents the engine to generate instrumented code. To construct
 * an object of this file, users need provide a string that represents the
 * sub-directory under the benchmarks directory, then the engine will explore
 * all the C/C++ files that ends with ".cc/.cpp/.c/.h" and extract specification
 * annotations and generate instrumented code in the generated directory.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class CodeGenerator {
	// Files that we need to process
	private ArrayList<File> files;

	// Code addition list --- per file
	private ArrayList<CodeAdditions> allAdditions;
	// Line change map list --- per file; Each map represents the
	// line->InterfaceConstruct mapping that will rename the interface
	// declaration line.
	private ArrayList<HashMap<Integer, InterfaceConstruct>> renamedLinesMapList;

	// The string that users provide as a sub-directory in the benchmarks
	// directory: e.g. ms-queue
	public final String dirName;

	// The original directory --- the benchmarks directory: e.g.
	// ~/model-checker/benchmarks/
	public final String originalDir;
	// The directory for generated files: e.g. ~/model-checker/test-cdsspec/
	public final String generatedDir;

	// The specification annotation extractor
	private SpecExtractor extractor;

	public CodeGenerator(String originalDir, String generatedDir, String dirName) {
		this.dirName = dirName;
		this.originalDir = originalDir + "/" + dirName + "/";
		this.generatedDir = generatedDir + "/" + dirName + "/";
		
		//this.originalDir = Environment.BenchmarksDir + dirName + "/";
		//this.generatedDir = Environment.GeneratedFilesDir + dirName + "/";
		
		try {
			files = this.getSrcFiles(this.originalDir);
			System.out.println("Original benchmarks directory: " + this.originalDir);
			System.out.println("Generated benchmark directory: " + this.generatedDir);
			System.out.println("The specific benchmark directory: " + this.dirName);
			for (int i = 0; i < files.size(); i++) {
				System.out.println("    Processing: " + files.get(i));
			}
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}
		extractor = new SpecExtractor();
		try {
			extractor.extract(files);
		} catch (WrongAnnotationException e) {
			e.printStackTrace();
		} catch (ParseException e) {
			e.printStackTrace();
		}
	}

	/**
	 * <p>
	 * This function initializes the list of code additions and line changes for
	 * all the files. For the code additions of a file, we sort them in an
	 * increasing order by the inserting line number.
	 * </p>
	 * 
	 */
	private void getAllCodeChanges() {
		allAdditions = new ArrayList<CodeAdditions>();
		renamedLinesMapList = new ArrayList<HashMap<Integer, InterfaceConstruct>>();
		for (int i = 0; i < files.size(); i++) {
			File file = files.get(i);
			// One CodeAdditions per file
			CodeAdditions additions = new CodeAdditions(file);
			// Add the additions for this file to the list
			allAdditions.add(additions);

			// One CodeChange per file
			HashMap<Integer, InterfaceConstruct> renamedLinesMap = new HashMap<Integer, InterfaceConstruct>();
			// Add it the the list
			renamedLinesMapList.add(renamedLinesMap);

			// Extract all the additions
			ArrayList<OPConstruct> OPList = extractor.OPListMap.get(file);
			EntryConstruct entry = extractor.entryMap.get(file);
			ArrayList<DefineConstruct> defineList = extractor.defineListMap
					.get(file);
			ArrayList<InterfaceConstruct> interfaceList = extractor.interfaceListMap
					.get(file);
			Code code = null;
			CodeAddition addition = null;
			// For ordering point constructs
			if (OPList != null) {
				for (OPConstruct con : OPList) {
					code = CodeGeneratorUtils.Generate4OPConstruct(con);
					addition = new CodeAddition(con.beginLineNum, code);
					additions.addCodeAddition(addition);
				}
			}
			// For entry constructs
			if (entry != null) {
				code = CodeGeneratorUtils.Generate4Entry(entry);
				addition = new CodeAddition(entry.beginLineNum, code);
				additions.addCodeAddition(addition);
			}
			// For define constructs
			if (defineList != null) {
				for (DefineConstruct con : defineList) {
					code = CodeGeneratorUtils.Generate4Define(con);
					addition = new CodeAddition(con.endLineNum, code);
					additions.addCodeAddition(addition);
				}
			}
			// For interface constructs
			if (interfaceList != null) {
				for (InterfaceConstruct con : interfaceList) {
					code = CodeGeneratorUtils.GenerateInterfaceWrapper(con);
					addition = new CodeAddition(con.getEndLineNumFunction(),
							code);
					additions.addCodeAddition(addition);
					// Record the line to be changed
					renamedLinesMap.put(con.endLineNum + 1, con);
				}
			}

			// Sort additions by line number increasingly
			additions.sort();
		}
	}

	/**
	 * <p>
	 * For a specific file, given the code additions and line changes mapping
	 * for that file, this function will generate the new code for that file and
	 * write it back to the destination directory.
	 * </p>
	 * 
	 * @param file
	 *            The file to be processed
	 * @param additions
	 *            The sorted code additions for the file
	 * @param renamedLinesMap
	 *            The line change mapping for the file
	 */
	private void writeCodeChange(File file, CodeAdditions additions,
			HashMap<Integer, InterfaceConstruct> renamedLinesMap) {
		Code newCode = new Code();
		BufferedReader br = null;
		LineNumberReader lineReader = null;
		int lineNum = 0;
		String curLine = null;

		String dest = generatedDir + file.getName();
		CodeAddition curAddition = null;
		int additionIdx = -1;
		if (!additions.isEmpty()) {
			additionIdx = 0;
			curAddition = additions.codeAdditions.get(0);
		}

		// Include the header for C/C++ files (.c/.cc/.cpp)
		String name = file.getName();
		if (name.endsWith(".c") || name.endsWith(".cc")
				|| name.endsWith(".cpp")) {
			newCode.addLine(CodeGeneratorUtils.Comment("Add the"
					+ SpecNaming.CDSSpecGeneratedHeader + " header file"));
			newCode.addLine(CodeGeneratorUtils
					.IncludeHeader(SpecNaming.CDSSpecGeneratedHeader));
			newCode.addLine("");
		}

		try {
			br = new BufferedReader(new FileReader(file));
			lineReader = new LineNumberReader(br);
			while ((curLine = lineReader.readLine()) != null) {
				lineNum = lineReader.getLineNumber();
				InterfaceConstruct construct = null;
				if ((construct = renamedLinesMap.get(lineNum)) != null) {
					// Rename line
					String newLine = construct.getFunctionHeader()
							.getRenamedFuncLine();
					newCode.addLine(newLine);
				} else {
					// First add the current line
					newCode.addLine(curLine);
					// Then check if we need to add code
					if (curAddition != null
							&& lineNum == curAddition.insertingLine) {
						// Need to insert code
						newCode.addLines(curAddition.code);
						// Increase to the next code addition
						additionIdx++;
						curAddition = additionIdx == additions.codeAdditions
								.size() ? null : additions.codeAdditions
								.get(additionIdx);
					}
				}
			}
			// Write new code change to destination
			CodeGeneratorUtils.write2File(dest, newCode.lines);
			// System.out.println("/*************** " + file.getName()
			// + "  *************/");
			// System.out.println(newCode);
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * <p>
	 * This function is the main interface for the CodeGenerator class. After
	 * constructing a CodeGenerator object, users can call this function to
	 * complete the code generation and file writing process.
	 * </p>
	 */
	public void generateCode() {
		// Extract all the code additions and line change
		getAllCodeChanges();

		// Generate the header file and CPP file
		Code generatedHeader = CodeGeneratorUtils
				.GenerateCDSSpecHeaderFile(extractor);
		Code generatedCPP = CodeGeneratorUtils
				.GenerateCDSSpecCPPFile(extractor);
		CodeGeneratorUtils
				.write2File(generatedDir + SpecNaming.CDSSpecGeneratedName
						+ ".h", generatedHeader.lines);
		CodeGeneratorUtils.write2File(generatedDir
				+ SpecNaming.CDSSpecGeneratedCPP, generatedCPP.lines);

		// Iterate over each file
		for (int i = 0; i < files.size(); i++) {
			File file = files.get(i);
			CodeAdditions additions = allAdditions.get(i);
			HashMap<Integer, InterfaceConstruct> renamedLinesMap = renamedLinesMapList
					.get(i);
			writeCodeChange(file, additions, renamedLinesMap);
		}
	}

	/**
	 * <p>
	 * This is just a testing function that outputs the generated code, but not
	 * actually write them to the disk.
	 * </p>
	 */
	private void testGenerator() {
		// Test code generation
		Code generatedHeader = CodeGeneratorUtils
				.GenerateCDSSpecHeaderFile(extractor);
		Code generatedCPP = CodeGeneratorUtils
				.GenerateCDSSpecCPPFile(extractor);

		System.out.println("/***** Generated header file *****/");
		System.out.println(generatedHeader);
		System.out.println("/***** Generated cpp file *****/");
		System.out.println(generatedCPP);

		for (File file : extractor.OPListMap.keySet()) {
			ArrayList<OPConstruct> list = extractor.OPListMap.get(file);
			for (OPConstruct con : list) {
				Code code = CodeGeneratorUtils.Generate4OPConstruct(con);
				System.out.println("/*****  *****/");
				System.out.println(con.annotation);
				System.out.println(code);
			}
		}

		for (File f : extractor.entryMap.keySet()) {
			EntryConstruct con = extractor.entryMap.get(f);
			System.out.println("/*****  *****/");
			System.out.println(con.annotation);
			System.out.println(CodeGeneratorUtils.Generate4Entry(con));
		}

		for (File file : extractor.interfaceListMap.keySet()) {
			ArrayList<InterfaceConstruct> list = extractor.interfaceListMap
					.get(file);
			for (InterfaceConstruct con : list) {
				Code code = CodeGeneratorUtils.GenerateInterfaceWrapper(con);
				System.out.println("/***** Interface wrapper *****/");
				System.out.println(con.getFunctionHeader().getHeaderLine());
				System.out
						.println(con.getFunctionHeader().getRenamedFuncLine());
				System.out.println(code);
			}
		}
	}

	public ArrayList<File> getSrcFiles(String dirName)
			throws FileNotFoundException {
		ArrayList<File> res = new ArrayList<File>();
		File dir = new File(dirName);
		if (dir.isDirectory() && dir.exists()) {
			for (String file : dir.list()) {
				if (file.endsWith(".h") || file.endsWith(".c")
						|| file.endsWith(".cc") || file.endsWith(".cpp")) {
					res.add(new File(dir.getAbsolutePath() + "/" + file));
				}
			}
		} else {
			throw new FileNotFoundException(dirName
					+ " is not a valid directory!");
		}
		return res;
	}

	static public void main(String[] args) {
		if (args.length < 3) {
			System.out
					.println("Usage: CodeGenerator <Benchmarks-directory> <Directory-for-generated-files> <specific-benchmark-lists...>");
			return;
		}
		
		// String[] dirNames = args;
		
		String[] dirNames = new String[args.length - 2];
		for (int i = 0; i < args.length - 2; i++) {
			dirNames[i] = args[i + 2];
		}
//		String[] dirNames = Environment.Benchmarks;

		for (int i = 0; i < dirNames.length; i++) {
			String dirName = dirNames[i];
			System.out.println("/**********   Generating CDSSpec files for "
					+ dirName + "    **********/");
//			CodeGenerator generator = new CodeGenerator(Environment.BenchmarksDir, Environment.GeneratedFilesDir, dirName);
			CodeGenerator generator = new CodeGenerator(args[0], args[1], dirName);
			generator.generateCode();
		}
	}
}
