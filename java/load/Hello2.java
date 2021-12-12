public class Hello2 {
    void invoke() {
        System.out.println( "call invoke:"+ getClass().getClassLoader().toString() + ":Hello2");
    }
    @Override
    protected void finalize() throws Throwable {
        System.out.println("call finalize:" + this);
    }
}

