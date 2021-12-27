public class Hello3 {
    void invoke() {
        System.out.println( "call invoke:"+ getClass().getClassLoader().toString() + ":Hello2");
    }

    String evaluate(int input) {   
        return input*102 + "";
    }

    @Override
    protected void finalize() throws Throwable {
        System.out.println("call finalize:" + this);
    }
}

