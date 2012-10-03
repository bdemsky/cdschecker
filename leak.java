import java.io.*;
import java.util.*;

public class leak {
	public static void main(String x[]) throws Exception {
		String file=x[0];
		BufferedReader in	= new BufferedReader(new FileReader(file));
		HashMap m=new HashMap();
		do {
			String i=in.readLine();
			if (i==null)
				break;
			String[] s=i.split(" ");
			if (s[0].equals("FR")) {
				m.remove(s[1]);
			} else if (s[0].equals("ALLOC:")) {
				m.put(s[1], i);
			}
		} while(true);
		HashMap<Object, Integer> m2=new HashMap<Object, Integer>();
		HashMap<Object, String> m3=new HashMap<Object, String>();
		for(Iterator it=m.keySet().iterator();it.hasNext();) {
			Object o=it.next();
			String k=(String)m.get(o);
			String[] xx=k.split(" ");
			String met=xx[4];
			int size=Integer.valueOf(xx[2]);
			if (!m2.containsKey(met)) {
				m2.put(met, new Integer(0));
				m3.put(met, xx[3]);
			}
			m2.put(met, m2.get(met)+size);
		}
		for(Iterator it=m2.keySet().iterator();it.hasNext();) {
			Object o=it.next();
			System.out.println(o+" "+m2.get(o)+" "+m3.get(o));
		}
	}

}