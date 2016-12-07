package edu.uci.eecs.specExtraction;

import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>
 * This class represents one complete ordering point annotation. We integrate
 * all ordering point annotations into this class. We use the ordering point
 * type (OPType) to differentiate each specific ordering point annotation.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class OPConstruct extends Construct {
	// The ordering point type
	public final OPType type;
	// The ordering point label --- only for OPCheck and PotentialOP; otherwise
	// null
	public final String label;
	// The condition under which the current annotation will be instrumented
	public final String condition;

	// The original line of text of the entry annotation
	public final String annotation;

	public OPConstruct(File file, int beginLineNum, OPType type, String label,
			String condition, String annotation) {
		super(file, beginLineNum);
		this.type = type;
		this.label = label;
		// Trim the preceding and trailing spaces
		this.condition = SpecUtils.trimSpace(condition);
		this.annotation = annotation;
	}

	public String toString() {
		StringBuffer res = new StringBuffer();
		res.append(super.toString() + "\n");
		res.append("@");
		res.append(type);
		if (type == OPType.PotentialOP || type == OPType.OPCheck) {
			res.append("(" + label + ")");
		}
		res.append(": ");
		res.append(condition + "\n");
		return res.toString();
	}
}
