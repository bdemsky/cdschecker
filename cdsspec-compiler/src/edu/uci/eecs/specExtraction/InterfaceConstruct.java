package edu.uci.eecs.specExtraction;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import edu.uci.eecs.specExtraction.SpecUtils.IntObj;
import edu.uci.eecs.specExtraction.SpecUtils.Primitive;
import edu.uci.eecs.utilParser.ParseException;
import edu.uci.eecs.utilParser.UtilParser;

/**
 * <p>
 * This class is a subclass of Construct. It represents a complete interface
 * annotation.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class InterfaceConstruct extends Construct {
	// The interface label of the current interface; If not specified, we use
	// the actual interface name to represent this interface
	private String name;
	// Each interface interface will have an auto-generated struct that
	// represents the return value and the arguments of that interface method
	// call. We will mangle the interface label name to get this name in case we
	// have other name conflict
	private String structName;
	public final Code preCondition;
	public final Code justifyingPrecondition;
	public final Code transition;
	public final Code justifyingPostcondition;
	public final Code postCondition;
	public final Code print;

	// The ending line number of the specification annotation construct
	public final int endLineNum;

	// The ending line number of the function definition
	private int endLineNumFunction;
	// The function header of the corresponding interface --- The list of
	// variable declarations that represent the RETURN value and
	// arguments of the interface
	private FunctionHeader funcHeader;

	public final boolean autoGenPrint;

	public InterfaceConstruct(File file, int beginLineNum, int endLineNum,
			ArrayList<String> annotations) throws WrongAnnotationException {
		super(file, beginLineNum);
		this.endLineNum = endLineNum;
		this.name = null;
		this.structName = null;
		this.preCondition = new Code();
		this.justifyingPrecondition = new Code();
		this.transition = new Code();
		this.justifyingPostcondition = new Code();
		this.postCondition = new Code();
		this.print = new Code();

		processAnnotations(annotations);

		autoGenPrint = print.isEmpty();
	}

	public FunctionHeader getFunctionHeader() {
		return this.funcHeader;
	}

	public boolean equals(Object other) {
		if (!(other instanceof InterfaceConstruct)) {
			return false;
		}
		InterfaceConstruct o = (InterfaceConstruct) other;
		if (o.name.equals(this.name))
			return true;
		else
			return false;
	}

	public String getName() {
		return this.name;
	}

	/**
	 * <p>
	 * This function will automatically generate the printing statements for
	 * supported types if the user has not defined the "@Print" primitive
	 * </p>
	 * 
	 * @return The auto-generated state printing statements
	 * @throws WrongAnnotationException
	 */
	private Code generateAutoPrintFunction() {
		Code code = new Code();
		// For C_RET
		code.addLines(SpecUtils.generatePrintStatement(funcHeader.returnType,
				SpecNaming.C_RET));
		// For arguments
		for (VariableDeclaration decl : funcHeader.args) {
			String type = decl.type;
			String name = decl.name;
			code.addLines(SpecUtils.generatePrintStatement(type, name));
		}
		return code;
	}

	/**
	 * <p>
	 * Assert that the interface primitive is valid; if not, throws an exception
	 * </p>
	 * 
	 * @param file
	 *            Current file
	 * @param primitive
	 *            The primitive string that we have extracted earlier
	 * @throws WrongAnnotationException
	 */
	private void assertValidPrimitive(File file, Primitive primitive)
			throws WrongAnnotationException {
		int lineNum = primitive.beginLineNum;
		String name = primitive.name;
		if (!name.equals(SpecNaming.Interface)
				&& !name.equals(SpecNaming.Transition)
				&& !name.equals(SpecNaming.PreCondition)
				&& !name.equals(SpecNaming.JustifyingPrecondition)
				&& !name.equals(SpecNaming.SideEffect)
				&& !name.equals(SpecNaming.JustifyingPostcondition)
				&& !name.equals(SpecNaming.PostCondition)
				&& !name.equals(SpecNaming.PrintValue)) {
			WrongAnnotationException.err(file, lineNum, name
					+ " is NOT a valid CDSSpec interface primitive.");
		}
	}

	/**
	 * <p>
	 * Assert that the "@Interface" primitive has correct syntax; if not, throws
	 * an exception. If so, it basically checks whether content of the primitive
	 * is a valid word and then return interface label name.
	 * </p>
	 * 
	 * @param file
	 *            Current file
	 * @param lineNum
	 *            Current line number
	 * @param primitive
	 *            The primitive string that we have extracted earlier
	 * @throws WrongAnnotationException
	 */
	private String extractInterfaceName(File file, Primitive primitive)
			throws WrongAnnotationException {
		int lineNum = primitive.beginLineNum;
		String name = primitive.name;
		if (primitive.contents.size() != 1)
			WrongAnnotationException.err(file, lineNum,
					"The @Interface primitive: " + name + " has wrong syntax.");
		String line = primitive.contents.get(0);
		SpecUtils.matcherWord.reset(line);
		if (!SpecUtils.matcherWord.find())
			WrongAnnotationException.err(file, lineNum, name
					+ " is NOT a valid CDSSpec @Interface primitive.");
		return line;
	}

	private void processAnnotations(ArrayList<String> annotations)
			throws WrongAnnotationException {
		IntObj curIdx = new IntObj(0);
		Primitive primitive = null;

		while ((primitive = SpecUtils.extractPrimitive(file, beginLineNum,
				annotations, curIdx)) != null) {
			String name = primitive.name;
			assertValidPrimitive(file, primitive);
			if (primitive.contents.size() == 0)
				continue;
			if (name.equals(SpecNaming.Interface)) {
				String interName = extractInterfaceName(file, primitive); 
				setNames(interName);
			} else if (name.equals(SpecNaming.Transition)) {
				this.transition.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.PreCondition)) {
				this.preCondition.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.JustifyingPrecondition)) {
				this.justifyingPrecondition.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.JustifyingPostcondition)) {
				this.justifyingPostcondition.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.PostCondition)) {
				this.postCondition.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.PrintValue)) {
				this.print.addLines(primitive.contents);
			}
		}
	}

	/**
	 * <p>
	 * This function is called to extract all the declarations that should go to
	 * the corresponding value struct --- a C++ struct to be generated for this
	 * interface that contains the information of the return value and the
	 * arguments.
	 * </p>
	 * 
	 * @param line
	 *            The line that represents the interface declaration line
	 * @throws ParseException
	 */
	public void processFunctionDeclaration(String line) throws ParseException {
		// FIXME: Currently we only allow the declaration to be one-liner
		funcHeader = UtilParser.parseFuncHeader(line);
		// Record the original declaration line
		funcHeader.setHeaderLine(line);

		// If users have not defined @Interface, we should use the function name
		// as the interface label
		if (name == null) {
			setNames(funcHeader.funcName.bareName);
		}

		// Once we have the compelte function declaration, we can auto-gen the
		// print-out statements if it's not defined
		if (autoGenPrint) {
			print.addLines(generateAutoPrintFunction());
		}
	}

	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append(super.toString() + "\n");
		sb.append("@Interface: " + name + "\n");
		if (!transition.isEmpty())
			sb.append("@Transition:\n" + transition);
		if (!preCondition.isEmpty())
			sb.append("@PreCondition:\n" + preCondition);
		if (!postCondition.isEmpty())
			sb.append("@PostCondition:\n" + postCondition);
		if (!print.isEmpty())
			sb.append("@Print:\n" + print + "\n");
		sb.append(funcHeader);

		return sb.toString();
	}

	public int getEndLineNumFunction() {
		return endLineNumFunction;
	}

	public void setEndLineNumFunction(int endLineNumFunction) {
		this.endLineNumFunction = endLineNumFunction;
	}

	public String getStructName() {
		return structName;
	}

	private void setNames(String name) {
		this.name = name;
		if (name == null)
			return;
		structName = createStructName(name);
	}
	
	static public String createStructName(String labelName) {
		return "__struct_" + labelName + "__";
	}
}
