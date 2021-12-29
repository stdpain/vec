import java.nio.ByteBuffer;

public class PassBuffer {
    static void invoke(ByteBuffer buffer) {
        byte[] bytes = new byte[buffer.capacity()];
        System.out.println("buffer.remaining() = " + buffer.remaining());
        buffer.get(bytes);
        System.out.println("buffer.remaining() = " + buffer.remaining());
        System.out.println("new String(bytes) = " + new String(bytes));
    }
}