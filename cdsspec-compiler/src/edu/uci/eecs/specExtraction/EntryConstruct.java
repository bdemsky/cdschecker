package edu.uci.eecs.specExtraction;

import java.io.File;

/**
 * <p>
 * This class is a subclass of Construct. It represents a complete entry
 * annotation.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class EntryConstruct extends Construct {

	// The original line of text of the entry annotation
	public final String annotation;

	public EntryConstruct(File file, int beginLineNum, String annotation) {
		super(file, beginLineNum);
		this.annotation = annotation;
	}

	public String toString() {
		StringBuffer res = new StringBuffer();
		res.append(super.toString() + "\n");
		res.append("@Entry");
		return res.toString();
	}
}
