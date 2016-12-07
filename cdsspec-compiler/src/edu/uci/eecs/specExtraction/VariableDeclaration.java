package edu.uci.eecs.specExtraction;

import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import edu.uci.eecs.utilParser.ParseException;
import edu.uci.eecs.utilParser.UtilParser;

/**
 * <p>
 * This class represents a variable declaration in C/C++, in which there exist a
 * type and a name.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class VariableDeclaration {
	public final String type;
	public final String name;

	public VariableDeclaration(String type, String name) {
		this.type = SpecUtils.trimSpace(type);
		this.name = SpecUtils.trimSpace(name);
	}

	public VariableDeclaration(File file, int lineNum, String line)
			throws WrongAnnotationException {
		VariableDeclaration decl = null;
		try {
			decl = UtilParser.parseDeclaration(line);
		} catch (ParseException e) {
			WrongAnnotationException.err(file, lineNum, "The declaration: \""
				+ line + "\" has wrong syntax.");
		} finally {
			type = decl == null ? null : decl.type;
			name = decl == null ? null : decl.name;
		}
	}

	public String toString() {
		return type + ": " + name;
	}
}
