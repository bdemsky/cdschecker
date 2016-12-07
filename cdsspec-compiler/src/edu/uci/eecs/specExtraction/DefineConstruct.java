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
 * This class is a subclass of Construct. It represents user-defined code that
 * we allow in the header file. Note that the code here basically are the same
 * as writing code right in place. We require function declaration/definition to
 * be inline. Users should be responsible for the correctness of their code.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class DefineConstruct extends Construct {
	public final Code code;

	// The ending line number of the specification annotation construct
	public final int endLineNum;

	public DefineConstruct(File file, int beginLineNum, int endLineNum,
			ArrayList<String> annotations) throws WrongAnnotationException {
		super(file, beginLineNum);
		code = new Code();
		this.endLineNum = endLineNum;
		Primitive define = SpecUtils.extractPrimitive(file, beginLineNum,
				annotations, new IntObj(0));
		code.addLines(define.contents);
	}

	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append(super.toString() + "\n");
		sb.append("@Define:\n");
		if (!code.isEmpty()) {
			sb.append(code);
			sb.append("\n");
		}
		return sb.toString();
	}
}
