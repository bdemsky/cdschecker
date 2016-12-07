package edu.uci.eecs.specExtraction;

import java.io.File;

/**
 * <p>
 * This class is the main error processing exception in processing the
 * annotations.
 * </p>
 * 
 * @author Peizhao Ou
 * 
 */
public class WrongAnnotationException extends Exception {
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public WrongAnnotationException(String msg) {
		super(msg);
	}

	/**
	 * <p>
	 * This is a utility function for more conveniently generating specification
	 * error exceptions. We help locate the error by the line number in the
	 * processing file, and an associated error message.
	 * </p>
	 * 
	 * @param file
	 *            The file that we see the error
	 * @param line
	 *            The associated line number of the error
	 * @param msg
	 *            The error message printout
	 * @throws WrongAnnotationException
	 */
	public static void err(File file, int line, String msg)
			throws WrongAnnotationException {
		String errMsg = "Spec error in file \"" + file.getName() + "\", Line "
				+ line + " :\n\t" + msg + "\n";
		throw new WrongAnnotationException(errMsg);
	}

	/**
	 * <p>
	 * This is a utility function for more conveniently generating specification
	 * warning. We help locate the warning by the line number in the processing
	 * file, and an associated warning message.
	 * </p>
	 * 
	 * @param file
	 *            The file that we see the warning
	 * @param line
	 *            The associated line number of the warning
	 * @param msg
	 *            The warning message printout
	 * @throws WrongAnnotationException
	 */
	public static void warning(File file, int line, String msg) {
		String errMsg = "Spec WARNING in file \"" + file.getName()
				+ "\", Line " + line + " :\n\t" + msg + "\n";
		System.out.println(errMsg);
	}
}
