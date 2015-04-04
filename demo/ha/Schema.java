import clientApi.*;

/**
 * The Schema class (should be automatically generated by the java marshaling tool)
 *
 * @author Jeong-Hyon Hwang (jhhwang@cs.brown.edu)
 * @version 1.0 03/07/2005
 */
public class Schema
    extends Tuple {

  public int clientnum;
  public long timestamp;
  public int x;
  public int y;
  public int z;
  public int seq_no;

  public Schema() {
    this(0, 0, 0, 0, 0, 0);
  }

  public Schema(int clientnum, long timestamp, int x, int y, int z, int seq_no) {
    this.clientnum = clientnum;
    this.timestamp = timestamp;
    this.x = x;
    this.y = y;
    this.z = z;
    this.seq_no = seq_no;
  }

  public String toString() {
    String s = "(clientnum = ";
    s += clientnum;
    s += ", timestamp = ";
    s += timestamp;
    s += ", x = ";
    s += x;
    s += ", y = ";
    s += y;
    s += ", z = ";
    s += z;
    s += ", seq_no = ";
    s += seq_no;
    return s + ")";
  }
}

