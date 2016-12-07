package edu.uci.eecs.specExtraction;

import java.util.ArrayList;

/**
 * <p>
 * This represents a function declaration header. For example,
 * "void myFunction(int arg1, bool arg2)" is a function declaration header.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class FunctionHeader {
	private ArrayList<VariableDeclaration> templateList;

	// The return type of the function
	public final String returnType;
	// The function name
	public final QualifiedName funcName;
	// The argument list (with formal parameter names)
	public final ArrayList<VariableDeclaration> args;

	// The actually function definition line in plain text.
	// E.g. "void myFunction(int arg1, bool arg2) {"
	private String headerLine;

	public FunctionHeader(String returnType, QualifiedName qualifiedName,
			ArrayList<VariableDeclaration> args) {
		this.templateList = null;
		this.returnType = returnType;
		this.funcName = qualifiedName;
		this.args = args;
	}

	/**
	 * 
	 * @return Whether the return type is void
	 */
	public boolean isReturnVoid() {
		return returnType.equals("void");
	}

	public void setTemplateList(ArrayList<VariableDeclaration> templateList) {
		this.templateList = templateList;
	}

	public ArrayList<VariableDeclaration> getTemplateList() {
		return this.templateList;
	}

	public String getTemplateFullStr() {
		String templateStr = "";
		if (templateList == null)
			return templateStr;
		VariableDeclaration decl;
		decl = templateList.get(0);
		templateStr = "<" + decl.type + " " + decl.name;
		for (int i = 1; i < templateList.size(); i++) {
			decl = templateList.get(i);
			templateStr = templateStr + ", " + decl.type + " " + decl.name;
		}
		templateStr = templateStr + ">";
		return templateStr;
	}

	public String getTemplateArgStr() {
		String templateStr = null;
		if (templateList.size() == 0)
			return templateStr;
		templateStr = "<" + templateList.get(0).name;
		for (int i = 1; i < templateList.size(); i++) {
			templateStr = templateStr + ", " + templateList.get(i);
		}
		templateStr = templateStr + ">";
		return templateStr;
	}

	public String getFuncStr() {
		String res = returnType + " " + funcName.fullName + "(";
		if (args.size() >= 1) {
			res = res + args.get(0);
		}
		for (int i = 1; i < args.size(); i++) {
			res = res + ", " + args.get(i);
		}
		res = res + ")";
		return res;
	}

	public String toString() {
		String res = returnType + " " + funcName.fullName + "(";
		if (args.size() >= 1) {
			res = res + args.get(0);
		}
		for (int i = 1; i < args.size(); i++) {
			res = res + ", " + args.get(i);
		}
		res = res + ")";
		return res;
	}

	public String getRenamedFuncName() {
		return funcName.qualifiedName + SpecNaming.WrapperPrefix + "_"
				+ funcName.bareName;
	}

	public FunctionHeader getRenamedHeader(String prefix) {
		String newFullName = getRenamedFuncName();
		FunctionHeader newHeader = new FunctionHeader(returnType,
				new QualifiedName(newFullName), args);
		return newHeader;
	}

	public FunctionHeader getRenamedHeader() {
		return getRenamedHeader(SpecNaming.WrapperPrefix);
	}

	/**
	 * 
	 * @return The string that represents the renamed function header line. For
	 *         example, we would return
	 *         <code>"bool Wrapper_myFunc(int x, int y)"</code> for the fucntion
	 *         <code>"bool myFunc(int x, int y) {"</code>
	 */
	public String getRenamedFuncLine() {
		String bareName = this.funcName.bareName;
		String newName = this.getRenamedFuncName();
		return this.headerLine.replaceFirst(bareName, newName);
	}

	/**
	 * 
	 * @return The string that represents the renamed function call. For
	 *         example, we would return <code>"bool RET = myFunc(x, y)"</code>
	 *         for the fucntion <code>"bool myFunc(int x, int y)"</code>
	 */
	public String getRenamedCall() {
		return getRenamedCall(SpecNaming.WrapperPrefix);
	}

	/**
	 * 
	 * @return The original plain text line of the function header
	 */
	public String getHeaderLine() {
		return headerLine;
	}
	
	
	// No support for template right now
	public String getDeclaration() {
		String res = returnType + " " + funcName.fullName + "(";
		if (args.size() >= 1) {
			res = res + args.get(0).type + " " + args.get(0).name;
		}
		for (int i = 1; i < args.size(); i++) {
			res = res + ", " + args.get(i).type + " " + args.get(i).name;
		}
		res = res + ")";
		return res;
	}

	public String getRenamedCall(String prefix) {
		String res = "";
		if (!isReturnVoid()) {
			res = res + returnType + " " + SpecNaming.C_RET + " = ";
		}
		res = res + prefix + "_" + funcName.fullName + "(";
		if (args.size() >= 1) {
			res = res + args.get(0).name;
		}
		for (int i = 1; i < args.size(); i++) {
			res = res + ", " + args.get(i).name;
		}
		res = res + ")";
		return res;
	}

	public void setHeaderLine(String headerLine) {
		this.headerLine = headerLine;
	}
}
