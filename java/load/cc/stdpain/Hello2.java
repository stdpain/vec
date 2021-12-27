public class Hello2 {
    void invoke() {
        System.out.println( "call invoke:"+ getClass().getClassLoader().toString() + ":Hello2");
    }
    String evaluate(String key1, Integer integer, Double flt) {   
        return key1 + integer + flt;
    }
    @Override
    protected void finalize() throws Throwable {
        System.out.println("call finalize:" + this);
    }
}

