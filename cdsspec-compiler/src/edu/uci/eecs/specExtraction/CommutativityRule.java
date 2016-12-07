package edu.uci.eecs.specExtraction;

/**
 * <p> This class represents a commutativity rule in the specification.
 * @author Peizhao Ou
 *
 */
public class CommutativityRule {
	public final String method1, method2;
	
	public final String condition;
	
	public CommutativityRule(String m1, String m2, String condition) {
		this.method1 = m1;
		this.method2 = m2;
		this.condition = condition;
	}
	
	public String toString() {
		return method1 + " <-> " + method2 + ": " + condition; 
	}
}
