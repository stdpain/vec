package cc.stdpain;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;

public class UDFClassLoader extends ClassLoader {
    private String UDFPath;
    private int oid;

    private static int inner_var = 0;

    public UDFClassLoader(String UDFPath) {
        this.UDFPath = UDFPath;
        this.oid = inner_var++;
    }

    protected Class<?> findClass(String name) throws ClassNotFoundException {
        Class clazz = null;
        String classFilename = UDFPath +"/" + name + ".class";
        System.out.println(classFilename);
        File classFile = new File(classFilename);
        if (classFile.exists()) {
            try (FileInputStream inputStream = new FileInputStream(classFile)) {
                byte[] bytes = new byte[(int)classFile.length()];
                inputStream.read(bytes);
                clazz = defineClass(name, bytes, 0, bytes.length);
            } catch (IOException e) {
                throw new ClassNotFoundException(name, e);
            }
        }
        if (clazz == null) {
            throw new ClassNotFoundException(name);
        }
        return clazz;
    }

    @Override
    public String toString() {
        return "UDFClassLoader{" +
                "UDFPath='" + UDFPath + '\'' +
                ", oid=" + oid +
                '}';
    }
    
    @Override
    protected void finalize() throws Throwable {
        System.out.println("call finalize:" + this);
    }
}
