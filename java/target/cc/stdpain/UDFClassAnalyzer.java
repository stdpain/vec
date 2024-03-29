package cc.stdpain;

import java.lang.reflect.Method;
import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.Map;

public class UDFClassAnalyzer {
    static Map<String, String> anlyMap = new HashMap<>();
    static {
        anlyMap.put("boolean", "Z");
        anlyMap.put("byte", "B");
        anlyMap.put("char", "C");
        anlyMap.put("short", "S");
        anlyMap.put("int", "I");
        anlyMap.put("long", "J");
        anlyMap.put("float", "F");
        anlyMap.put("double", "D");
        anlyMap.put("void", "V");
    }

    private static String getSign(String typeName) {
        String prefix = "";
        if (typeName.contains("[]")) {
            prefix = "[";
            typeName = typeName.replace("[]", "");
        }
        String signStr = anlyMap.get(typeName);
        if (signStr != null) {
            return prefix + signStr;
        } else {
            return prefix + "L" + typeName.replace('.', '/') + ";";
        }
    }

    public static String getSign(String methodName, Class clazz) throws NoSuchMethodException {
        for (Method declaredMethod : clazz.getDeclaredMethods()) {
            if (declaredMethod.getName().equals(methodName)) {
                StringBuilder val = new StringBuilder("(");
                for (Type genericParameterType : declaredMethod.getGenericParameterTypes()) {
                    String typeName = genericParameterType.getTypeName();
                    val.append(getSign(typeName));
                }
                val.append(")");
                val.append(getSign(declaredMethod.getReturnType().getName()));
                return val.toString();
            }
        }
        throw new NoSuchMethodException("Not Found Method" + methodName);
    }

}
