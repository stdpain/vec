import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class InvokeMethod {
    public static void batch_add(Class clazz,int[]res, int[] x, int[] y)
            throws InvocationTargetException, IllegalAccessException {
        Method method = clazz.getDeclaredMethods()[0];
        int batch_size = x.length;
        // System.out.println("batch-size:" + batch_size);
        for (int i = 0; i < batch_size; i++) {
            res[i] = (int)method.invoke(null, x[i], y[i]);
        }
    }
    public static int[] batch_add2(Class clazz, int[] x, int[] y)
            throws InvocationTargetException, IllegalAccessException {
        Method method = clazz.getDeclaredMethods()[0];
        int batch_size = x.length;
        int[]res = new int[batch_size];
        for (int i = 0; i < batch_size; i++) {
            res[i] = (int)method.invoke(null, x[i], y[i]);
        }
        return res;
    }

    public static void batch_add3(Class clazz, Object[] res, Object ...x)
            throws InvocationTargetException, IllegalAccessException {
        Method method = clazz.getDeclaredMethods()[0];
        final int parameterSize = x.length;
        int batch_size = 0;
        Object[][] params = new Object[parameterSize][];
        for (int i = 0; i < x.length; i++) {
            if(x[i] instanceof int[]) {
                int[] raw_array = (int[])x[i];
                batch_size = raw_array.length;
                Integer[] int_array = new Integer[batch_size];
                for(int j = 0;j < batch_size; ++j) {
                    int_array[j] = raw_array[j];
                }
                params[i] = int_array;
            }
        }
        // System.out.println("trace");
        for (int i = 0; i < batch_size; i++) {
            // System.out.println("trace" + i);
            res[i] = method.invoke(null, params[0][i], params[1][i]);
        }
        // System.out.println("res 0:" + res[0]);
    }

    public static int[] convert(Object[] array) {
        int batch_size = array.length;
        int[] res = new int[batch_size];
        Integer[] arr = (Integer[])(array);
        for(int i = 0;i < arr.length; ++i) {
            res[i] = arr[i];
        }
        return res;
    }
}