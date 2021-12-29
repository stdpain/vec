#include "jni.h"

#include <unistd.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

void callGC(JNIEnv* env) {
    auto system_clazz = env->FindClass("java/lang/System");
    assert(system_clazz);
    auto gc_method = env->GetStaticMethodID(system_clazz, "gc", "()V");
    assert(gc_method);
    env->CallStaticVoidMethod(system_clazz, gc_method);
}

void dumpString(JNIEnv* env, jthrowable throwable) {
    jclass clazz = env->FindClass("java/lang/Throwable");
    jmethodID method = env->GetMethodID(clazz, "toString", "()Ljava/lang/String;");
    jobject res = env->CallObjectMethod((jobject)throwable, method);
    const char* charflow = env->GetStringUTFChars((jstring)res, nullptr);
    std::cout << charflow << std::endl;
    env->ReleaseStringUTFChars((jstring)res, charflow);
    env->DeleteLocalRef(res);

    method = env->GetMethodID(clazz, "printStackTrace", "()V");
    env->CallVoidMethod((jobject)throwable, method);
}

void dumpString(JNIEnv* env, jobject object) {
    jclass clazz = env->FindClass("java/lang/Object");
    assert(clazz);
    jmethodID method = env->GetMethodID(clazz, "toString", "()Ljava/lang/String;");
    assert(method);
    jobject res = env->CallObjectMethod(object, method);
    const char* charflow = env->GetStringUTFChars((jstring)res, nullptr);
    std::cout << charflow << std::endl;
    env->ReleaseStringUTFChars((jstring)res, charflow);
    env->DeleteLocalRef(res);
}

void dumpStringIfException(JNIEnv* env) {
    jthrowable jthr = env->ExceptionOccurred();
    if (jthr) {
        env->ExceptionClear();
        dumpString(env, jthr);
    }
}

// https://stackoverflow.com/questions/45232522/how-to-set-classpath-of-a-running-jvm-in-cjni
void add_path(JNIEnv* env, const std::string& path) {
    const std::string urlPath = "file://" + path;
    jclass classLoaderCls = env->FindClass("java/lang/ClassLoader");
    jmethodID getSystemClassLoaderMethod = env->GetStaticMethodID(
            classLoaderCls, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    jobject classLoaderInstance =
            env->CallStaticObjectMethod(classLoaderCls, getSystemClassLoaderMethod);
    jclass urlClassLoaderCls = env->FindClass("java/net/URLClassLoader");
    jmethodID addUrlMethod = env->GetMethodID(urlClassLoaderCls, "addURL", "(Ljava/net/URL;)V");
    jclass urlCls = env->FindClass("java/net/URL");
    jmethodID urlConstructor = env->GetMethodID(urlCls, "<init>", "(Ljava/lang/String;)V");
    jobject urlInstance =
            env->NewObject(urlCls, urlConstructor, env->NewStringUTF(urlPath.c_str()));
    env->CallVoidMethod(classLoaderInstance, addUrlMethod, urlInstance);
    jthrowable jthr = env->ExceptionOccurred();
    if (jthr) {
        env->ExceptionClear();
        dumpString(env, jthr);
    }
    std::cout << "Added " << urlPath << " to the classpath." << std::endl;
}

void test_jni_basic() {
    JavaVM* vm;
    JNIEnv* env;
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_2;
    vm_args.nOptions = 0;
    vm_args.ignoreUnrecognized = 1;

    // Construct a VM
    jint res = JNI_CreateJavaVM(&vm, (void**)&env, &vm_args);
    assert(res == 0);
    // Test Upper
    {
        // Construct a String
        jstring jstr = env->NewStringUTF("Hello World");
        // First get the class that contains the method you need to call
        jclass clazz = env->FindClass("java/lang/String");

        // Get the method that you want to call
        jmethodID to_lower = env->GetMethodID(clazz, "toLowerCase", "()Ljava/lang/String;");
        // Call the method on the object
        jobject result = env->CallObjectMethod(jstr, to_lower);

        // Get a C-style string
        const char* str = env->GetStringUTFChars((jstring)result, NULL);

        printf("%s\n", str);

        // Clean up
        env->ReleaseStringUTFChars(jstr, str);
    }
    // Test subString
    {
        // Construct a String
        jstring jstr = env->NewStringUTF("Hello stdpain");
        jclass clazz = env->FindClass("java/lang/String");
        jmethodID substring = env->GetMethodID(clazz, "substring", "(II)Ljava/lang/String;");
        assert(substring != nullptr);
        jstring result = (jstring)env->CallObjectMethod(jstr, substring, 3, 10);
        const char* str = env->GetStringUTFChars((jstring)result, NULL);
        printf("%s\n", str);
        printf("%p\n", str);
        env->ReleaseStringUTFChars(result, str);
        str = env->GetStringUTFChars((jstring)result, NULL);
        env->ReleaseStringUTFChars(result, str);
        printf("%p\n", str);
        str = env->GetStringUTFChars((jstring)result, NULL);
        env->ReleaseStringUTFChars(result, NULL);
        printf("%p\n", str);
        env->ReleaseStringUTFChars(jstr, NULL);
        env->ReleaseStringUTFChars(result, str);

        jmethodID valueOf = env->GetStaticMethodID(clazz, "valueOf", "(I)Ljava/lang/String;");
        // jmethodID valueOf = env->GetMethodID(clazz, "valueOf", "(I)Ljava/lang/String;");
        assert(valueOf != nullptr);
        jobject vf = env->CallStaticObjectMethod(clazz, valueOf, 12);
        assert(vf != nullptr);
        const char* svf = env->GetStringUTFChars((jstring)vf, nullptr);
        printf("%s\n", svf);

        env->DeleteLocalRef(jstr);
        env->DeleteLocalRef(result);
    }
    // Test Class Loader
    {
        std::string target_patch = "/home/disk2/fha/JNI/vec/java/target/";
        std::string load_path = "/home/disk2/fha/JNI/vec/java/load";
        add_path(env, target_patch.c_str());
        auto hello = env->FindClass("Hello");
        assert(hello != nullptr);
        auto clazz = env->FindClass("cc/stdpain/UDFClassLoader");
        assert(clazz != nullptr);
        // create a class loader instance
        jmethodID UDFLoaderContructor = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;)V");
        assert(UDFLoaderContructor != nullptr);
        jobject UDFLoaderInstance =
                env->NewObject(clazz, UDFLoaderContructor, env->NewStringUTF(load_path.c_str()));
        dumpStringIfException(env);
        assert(UDFLoaderInstance);
        dumpString(env, UDFLoaderInstance);
        // invoke findClass
        auto method = env->GetMethodID(clazz, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        assert(method);
        auto hello2_clazz =
                env->CallObjectMethod(UDFLoaderInstance, method, env->NewStringUTF("Hello2"));
        dumpStringIfException(env);
        assert(hello2_clazz);
        jmethodID hello2_construct = env->GetMethodID((jclass)hello2_clazz, "<init>", "()V");
        assert(hello2_construct);
        auto hello2_obj = env->NewObject((jclass)hello2_clazz, hello2_construct);
        assert(hello2_obj);
        auto hello2_invoke_m = env->GetMethodID((jclass)hello2_clazz, "invoke", "()V");
        assert(hello2_invoke_m);
        env->CallVoidMethod(hello2_obj, hello2_invoke_m);
        {
            jobject UDFLoaderInstance2 = env->NewObject(clazz, UDFLoaderContructor,
                                                        env->NewStringUTF(load_path.c_str()));

            auto hello2_clazz =
                    env->CallObjectMethod(UDFLoaderInstance2, method, env->NewStringUTF("Hello2"));
            jmethodID hello2_construct = env->GetMethodID((jclass)hello2_clazz, "<init>", "()V");
            auto hello2_obj = env->NewObject((jclass)hello2_clazz, hello2_construct);
            auto hello2_invoke_m = env->GetMethodID((jclass)hello2_clazz, "invoke", "()V");

            {
                // Test Concurrency
                std::thread thr([&]() {
                    JNIEnv* env2;
                    int rc = vm->AttachCurrentThread((void**)&env2, nullptr);
                    jmethodID hello2_construct =
                            env2->GetMethodID((jclass)hello2_clazz, "<init>", "()V");
                    assert(hello2_construct != nullptr);
                    auto hello2_obj_2 = env2->NewObject((jclass)hello2_clazz, hello2_construct);
                    assert(hello2_obj_2 != nullptr);
                    // auto hello2_invoke_m = env2->GetMethodID((jclass)hello2_clazz, "invoke", "()V");

                    auto hello2_clazz = env->CallObjectMethod(UDFLoaderInstance2, method,
                                                              env->NewStringUTF("Hello2"));

                    env2->DeleteLocalRef(hello2_obj_2);
                    vm->DetachCurrentThread();
                });
                thr.join();
            }

            env->CallVoidMethod(hello2_obj, hello2_invoke_m);
            env->DeleteLocalRef(hello2_obj);
            env->DeleteLocalRef(hello2_clazz);
            env->DeleteLocalRef(UDFLoaderInstance2);

            std::cout << "call GC" << std::endl;
            callGC(env);
            // not the same thread
            sleep(1);
        }

        std::cout << "call GC" << std::endl;
        env->DeleteLocalRef(hello2_obj);
        callGC(env);
    }
    std::cout << "Trace"
              << "Test Integer" << std::endl;
    // Test Class convert
    {
        jclass intc = env->FindClass("java/lang/Integer");
        assert(intc != nullptr);
        jmethodID valueOf = env->GetStaticMethodID(intc, "valueOf", "(I)Ljava/lang/Integer;");
        assert(valueOf != nullptr);
        jvalue values[1];
        values[0].i = 12345;
        jobject int_obj = env->CallStaticObjectMethodA(intc, valueOf, values);
        assert(int_obj != nullptr);
        dumpString(env, int_obj);
    }
    std::cout << "Trace"
              << "Test bytes" << std::endl;
    // Test bytes
    {
        jclass charsets = env->FindClass("java/nio/charset/StandardCharsets");
        assert(charsets != nullptr);
        auto fieldId = env->GetStaticFieldID(charsets, "UTF_8", "Ljava/nio/charset/Charset;");
        assert(fieldId != nullptr);
        jobject utf8_charsets = env->GetStaticObjectField(charsets, fieldId);
        assert(utf8_charsets);

        jclass stringC = env->FindClass("java/lang/String");
        jmethodID stringCon =
                env->GetMethodID(stringC, "<init>", "([BLjava/nio/charset/Charset;)V");
        assert(stringCon);
        const char* bytes = "Hello";
        auto bytesArr = env->NewByteArray(5);
        env->SetByteArrayRegion(bytesArr, 0, 5, (const signed char*)bytes);
        jobject nstr = env->NewObject(stringC, stringCon, bytesArr, utf8_charsets);
        assert(nstr);
        dumpString(env, nstr);
    }
    {
        char buffer[1024];
        jclass passbuffer = env->FindClass("PassBuffer");
        assert(passbuffer);
        auto methodID = env->GetStaticMethodID(passbuffer, "invoke", "(Ljava/nio/ByteBuffer;)V");
        assert(methodID);
        auto dbuf = env->NewDirectByteBuffer(buffer, 1024);
        env->CallStaticVoidMethod(passbuffer, methodID, dbuf);
    }
    // Shutdown the VM.
    vm->DestroyJavaVM();
}

int main() {
    test_jni_basic();
    return 0;
}