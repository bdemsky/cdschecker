package edu.uci.eecs.specExtraction;

import java.io.File;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import edu.uci.eecs.specExtraction.SpecUtils.IntObj;
import edu.uci.eecs.specExtraction.SpecUtils.Primitive;

/**
 * <p>
 * This class is a subclass of Construct. It represents a complete global state
 * annotation.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class GlobalConstruct extends Construct {
	public final ArrayList<VariableDeclaration> declState;
	public final Code initState;
	public final Code copyState;
	public final Code clearState;
	public final Code finalState;
	public final Code printState;
	public final ArrayList<CommutativityRule> commutativityRules;

	// Whether the state declaration is empty
	public final boolean emptyState;
	// Whether we have auto-gen the state initialization code
	public final boolean autoGenInitial;
	// Whether we have auto-gen the state copying code
	public final boolean autoGenCopy;
	// Whether we have auto-gen the state clearing code
	public final boolean autoGenClear;
	// Whether we have auto-gen the state printing code
	public final boolean autoGenPrint;

	public GlobalConstruct(File file, int beginLineNum,
			ArrayList<String> annotations) throws WrongAnnotationException {
		super(file, beginLineNum);
		declState = new ArrayList<VariableDeclaration>();
		initState = new Code();
		copyState = new Code();
		clearState = new Code();
		finalState = new Code();
		printState = new Code();
		commutativityRules = new ArrayList<CommutativityRule>();

		processAnnotations(annotations);

		emptyState = declState.isEmpty();
		if (emptyState) {
			WrongAnnotationException.warning(file, beginLineNum,
					"The state is empty. Make sure that's what you want!");
			// Add a fake state declaration
			declState.add(new VariableDeclaration("int", "FakeState"));
		}

		autoGenInitial = initState.isEmpty();
		if (autoGenInitial) {
			Code code = generateAutoInitalFunction();
			initState.addLines(code);
		}

		autoGenCopy = copyState.isEmpty();
		if (autoGenCopy) {
			Code code = generateAutoCopyFunction();
			copyState.addLines(code);
		}

		autoGenClear = clearState.isEmpty();
		if (autoGenClear) {
			Code code = generateAutoClearFunction();
			clearState.addLines(code);
		}

		autoGenPrint = printState.isEmpty();
		if (autoGenPrint) {
			Code code = generateAutoPrintFunction();
			printState.addLines(code);
		}
	}

	/**
	 * <p>
	 * This function will automatically generate the initial statements for
	 * supported types if the user has not defined the "@Initial" primitive
	 * </p>
	 * 
	 * @return The auto-generated state intialization statements
	 * @throws WrongAnnotationException
	 */
	private Code generateAutoInitalFunction() throws WrongAnnotationException {
		Code code = new Code();
		if (emptyState) // Empty state should have empty initial function
			return code;
		for (VariableDeclaration decl : declState) {
			String type = decl.type;
			String name = decl.name;
			// Primitive types
			if (type.equals("int") || type.equals("unsigned")
					|| type.equals("unsigned int")
					|| type.equals("int unsigned") || type.equals("double")
					|| type.equals("double") || type.equals("bool")) {
				// x = 0;
				code.addLine(name + " = 0;");
			} else if (type.equals("IntList") || type.equals("IntSet")
					|| type.equals("IntMap")) {
				// Supported types
				// q = IntList();
				code.addLine(name + " = " + type + "();");
			} else if (type.equals("IntList *") || type.equals("IntSet *")
					|| type.equals("IntMap *")) {
				// Supported pointer types
				// q = new IntList;
				String originalType = SpecUtils.trimSpace(type
						.replace('*', ' '));
				code.addLine(name + " = new " + originalType + "();");
			} else {
				WrongAnnotationException
						.err(file,
								beginLineNum,
								"You have types in the state declaration that we do not support auto-gen initial function.");
			}
		}

		return code;
	}

	/**
	 * <p>
	 * This function will automatically generate the copy statements for
	 * supported types if the user has not defined the "@Copy" primitive
	 * </p>
	 * 
	 * @return The auto-generated state copy statements
	 * @throws WrongAnnotationException
	 */
	private Code generateAutoCopyFunction() throws WrongAnnotationException {
		Code code = new Code();
		if (emptyState) // Empty state should have empty copy function
			return code;
		for (VariableDeclaration decl : declState) {
			String type = decl.type;
			String name = decl.name;
			// Primitive types
			if (type.equals("int") || type.equals("unsigned")
					|| type.equals("unsigned int")
					|| type.equals("int unsigned") || type.equals("double")
					|| type.equals("double") || type.equals("bool")) {
				// NEW->x = OLD->x;
				code.addLine(SpecNaming.NewStateInst + "->" + name + " = "
						+ SpecNaming.OldStateInst + "->" + name + ";");
			} else if (type.equals("IntList") || type.equals("IntSet")
					|| type.equals("IntMap")) {
				// Supported types
				// New->q = IntList(OLD->q);
				code.addLine(SpecNaming.NewStateInst + "->" + name + " = "
						+ type + "(" + SpecNaming.OldStateInst + "->" + name
						+ ");");
			} else if (type.equals("IntList *") || type.equals("IntSet *")
					|| type.equals("IntMap *")) {
				// Supported pointer types
				// New->q = new IntList(*OLD->q);
				String originalType = SpecUtils.trimSpace(type
						.replace('*', ' '));
				code.addLine(SpecNaming.NewStateInst + "->" + name + " = new "
						+ originalType + "(*" + SpecNaming.OldStateInst + "->"
						+ name + ");");
			} else {
				WrongAnnotationException
						.err(file,
								beginLineNum,
								"You have types in the state declaration that we do not support auto-gen copy function.");
			}
		}

		return code;
	}
	
	/**
	 * <p>
	 * This function will automatically generate the clear statements for
	 * supported types if the user has not defined the "@Clear" primitive
	 * </p>
	 * 
	 * @return The auto-generated state copy statements
	 * @throws WrongAnnotationException
	 */
	private Code generateAutoClearFunction() throws WrongAnnotationException {
		Code code = new Code();
		if (emptyState) // Empty state should have empty copy function
			return code;
		
		// FIXME: Just try our best to generate recycling statements
		for (VariableDeclaration decl : declState) {
			String type = decl.type;
			String name = decl.name;
			if (type.equals("IntList *") || type.equals("IntSet *")
					|| type.equals("IntMap *")) {
				// Supported pointer types
				// if (stack) delete stack;
				code.addLine("if (" + name + ") delete " + name + ";");
			}
		}

		return code;
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
	private Code generateAutoPrintFunction() throws WrongAnnotationException {
		Code code = new Code();
		if (emptyState) // Empty state should have empty printing function
			return code;
		for (VariableDeclaration decl : declState) {
			String type = decl.type;
			String name = decl.name;
			code.addLines(SpecUtils.generatePrintStatement(type, name));
		}

		return code;
	}

	/**
	 * <p>
	 * Assert that the global state primitive is valid; if not, throws an
	 * exception.
	 * </p>
	 * 
	 * @param file
	 *            Current file
	 * @param primitive
	 *            The primitive that we have extracted earlier
	 * @throws WrongAnnotationException
	 */
	private void assertValidPrimitive(File file, Primitive primitive)
			throws WrongAnnotationException {
		int lineNum = primitive.beginLineNum;
		String name = primitive.name;
		if (!name.equals(SpecNaming.DeclareState)
				&& !name.equals(SpecNaming.InitalState)
				&& !name.equals(SpecNaming.CopyState)
				&& !name.equals(SpecNaming.ClearState)
				&& !name.equals(SpecNaming.FinalState)
				&& !name.equals(SpecNaming.Commutativity)
				&& !name.equals(SpecNaming.PrintState)) {
			WrongAnnotationException.err(file, lineNum, name
					+ " is NOT a valid CDSSpec global state primitive.");
		}
	}

	/**
	 * <p>
	 * Given a "@DeclareState" primitive that has a list of strings of
	 * declarations, we initialize our local "declState" members.
	 * 
	 * @param primitive
	 * @throws WrongAnnotationException
	 */
	private void processDeclState(Primitive primitive)
			throws WrongAnnotationException {
		for (int i = 0; i < primitive.contents.size(); i++) {
			int lineNum = i + primitive.beginLineNum;
			String declLine = primitive.contents.get(i);
			VariableDeclaration tmp = new VariableDeclaration(file, lineNum,
					declLine);
			declState.add(tmp);
		}
	}

	/**
	 * <p>
	 * Given a "@DeclareState" primitive that has a list of strings of
	 * declarations, we initialize our local "declState" members.
	 * 
	 * @param primitive
	 * @throws WrongAnnotationException
	 */
	private void processCommutativity(Primitive primitive)
			throws WrongAnnotationException {
		// Mathch commutativity rule
		Pattern regexpCommute = Pattern
				.compile("\\s*(\\w+)\\s*<->\\s*(\\w+)\\s*\\((.*)\\)\\s*$");
		Matcher matcherCommute = regexpCommute.matcher("");

		for (int i = 0; i < primitive.contents.size(); i++) {
			// FIXME: Currently we only allow a one-line commutativity rule
			int curLineNum = primitive.beginLineNum + i;
			String line = primitive.contents.get(i);
			matcherCommute.reset(line);
			if (matcherCommute.find()) {
				String method1 = matcherCommute.group(1);
				String method2 = matcherCommute.group(2);
				String code = matcherCommute.group(3);
				String rule = SpecUtils.trimSpace(SpecUtils
						.trimTrailingCommentSymbol(code));
				commutativityRules.add(new CommutativityRule(method1, method2,
						rule));
			} else {
				WrongAnnotationException
						.err(file,
								curLineNum,
								"The @Commutativity annotation should be: @Commutativity: Method1 <-> Method2 (condition)\n\t"
										+ "Problematic line: \"" + line + "\"");
			}
			// Done with processing the current commutativity
		}
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
			if (name.equals(SpecNaming.DeclareState)) {
				processDeclState(primitive);
			} else if (name.equals(SpecNaming.InitalState)) {
				initState.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.CopyState)) {
				copyState.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.ClearState)) {
				clearState.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.FinalState)) {
				finalState.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.PrintState)) {
				printState.addLines(primitive.contents);
			} else if (name.equals(SpecNaming.Commutativity)) {
				processCommutativity(primitive);
			}
		}
	}

	public String toString() {
		StringBuilder sb = new StringBuilder("");
		sb.append(super.toString() + "\n");
		sb.append("@DeclareState:\n");
		sb.append(declState);
		sb.append("\n");
		sb.append("@InitState:\n");
		sb.append(initState);
		if (!printState.isEmpty()) {
			sb.append("@Print:\n");
			sb.append(printState);
		}

		for (int i = 0; i < commutativityRules.size(); i++) {
			sb.append("@Commutativity: " + commutativityRules + "\n");
		}
		return sb.toString();
	}
}
