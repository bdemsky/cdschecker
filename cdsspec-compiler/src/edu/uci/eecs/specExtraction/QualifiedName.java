package edu.uci.eecs.specExtraction;

/**
 * <p>
 * This class represents a qualified variable name in C++, e.g.
 * Base::Mine::func. We use this class in the FunctionHeader class to represent
 * the name of the function. However, for the sake of simplicity in usage, we
 * only use it as a plain string with the bareName.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class QualifiedName {
	// The full name --- Base::Mine::func
	public final String fullName;
	// The bare name --- func
	public final String bareName;
	// The qualified name --- Base::Mine
	public final String qualifiedName;

	public QualifiedName(String fullName) {
		this.fullName = fullName;
		this.bareName = getBareName();
		this.qualifiedName = getQualifiedName();
	}

	private String getBareName() {
		int beginIdx;
		beginIdx = fullName.lastIndexOf(':');
		if (beginIdx == -1)
			return fullName;
		else
			return fullName.substring(beginIdx + 1);
	}

	private String getQualifiedName() {
		int endIdx = fullName.lastIndexOf(bareName);
		if (endIdx == 0)
			return "";
		return fullName.substring(0, endIdx);
	}

	public String toString() {
		return fullName + "\n" + bareName;
	}
}
