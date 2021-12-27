#include <benchmark/benchmark.h>

#include <cstring>
#include <ctime>
#include <iostream>
#include <thread>
#include <vector>

#include "jni.h"

// some define
constexpr int batch_size = 4096;
// constexpr int batch_size = 1024;

using Container = std::vector<int32_t>;

template <class T>
struct AlwaysZeroGenerator {
    static T next_data() { return 0; }
};

template <class T>
struct AlwaysOneGenerator {
    static T next_data() { return 1; }
};

template <class T>
struct RandomGenerator {
    static T next_data() { return rand(); }
};

template <class DataGenerator>
struct ContainerIniter {
    static void container_init(Container& container) {
        container.resize(batch_size);
        for (int i = 0; i < container.size(); ++i) {
            container[i] = DataGenerator::next_data();
        }
    }
};

using CurrentContainerIniter = ContainerIniter<RandomGenerator<int32_t>>;

// some bench code

inline int inline_func_all(int a, int b) {
    return a + b;
}

static void NativeAddCall(benchmark::State& state) {
    Container c_a;
    Container c_b;
    Container c_c;
    CurrentContainerIniter::container_init(c_a);
    CurrentContainerIniter::container_init(c_b);
    CurrentContainerIniter::container_init(c_c);
    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            c_c[i] = inline_func_all(c_a[i], c_b[i]);
        }
    }
}

__attribute__((noinline)) int noinline_func_all(int a, int b) {
    return a + b;
}

static void NativeAddFunctionCall(benchmark::State& state) {
    Container c_a;
    Container c_b;
    Container c_c;
    CurrentContainerIniter::container_init(c_a);
    CurrentContainerIniter::container_init(c_b);
    CurrentContainerIniter::container_init(c_c);
    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            c_c[i] = noinline_func_all(c_a[i], c_b[i]);
        }
    }
}

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
    }
}

static void JavaAddBatchFunctionCall(benchmark::State& state) {
    Container c_a;
    Container c_b;
    Container c_c;
    CurrentContainerIniter::container_init(c_a);
    CurrentContainerIniter::container_init(c_b);
    CurrentContainerIniter::container_init(c_c);

    JavaVM* vm;
    JNIEnv* env;
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 0;
    vm_args.ignoreUnrecognized = 1;
    jint res = JNI_CreateJavaVM(&vm, (void**)&env, &vm_args);
    std::cout << "TRACE++++++ " << res << std::endl;

    std::string target_patch = "/home/disk2/fha/JNI/vec/java/load/";
    add_path(env, target_patch.c_str());

    jclass add_c = env->FindClass("HelloAdd");
    // std::cout << "Load Class:" << add_c << std::endl;
    jclass invoke_c = env->FindClass("InvokeMethod");
    jmethodID invoke_method =
            env->GetStaticMethodID(invoke_c, "batch_add", "(Ljava/lang/Class;[I[I[I)V");

    jintArray ary = env->NewIntArray(batch_size);
    jintArray bry = env->NewIntArray(batch_size);
    jintArray resy = env->NewIntArray(batch_size);
    for (auto _ : state) {
        env->SetIntArrayRegion(ary, 0, batch_size, c_a.data());
        env->SetIntArrayRegion(bry, 0, batch_size, c_b.data());

        env->CallStaticVoidMethod(invoke_c, invoke_method, (jobject)add_c, resy, ary, bry);
        int* buffer3 = env->GetIntArrayElements((jintArray)resy, NULL);
        // std::cout << "buffer3:" << *buffer3 << std::endl;
        memcpy(c_c.data(), buffer3, batch_size * sizeof(int));
        // for (int i = 0; i < batch_size; ++i) {
        //     if (c_c[i] != c_a[i] + c_b[i]) {
        //         std::cout << "expect:" << (c_a[i] + c_b[i]) << "Real:" << c_c[i] << std::endl;
        //     }
        // }
        env->ReleaseIntArrayElements((jintArray)resy, buffer3, 0);
        // env->DeleteLocalRef(resy);q
    }

    // std::cout << "Call Destroy:" << method_id << std::endl;
    // env->DeleteLocalRef(add_c);
    // vm->DestroyJavaVM();
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

void dumpStringIfException(JNIEnv* env) {
    jthrowable jthr = env->ExceptionOccurred();
    if (jthr) {
        env->ExceptionClear();
        dumpString(env, jthr);
    }
}

static void JavaAddBatchBoxFunctionCall(benchmark::State& state) {
    Container c_a;
    Container c_b;
    Container c_c;
    CurrentContainerIniter::container_init(c_a);
    CurrentContainerIniter::container_init(c_b);
    CurrentContainerIniter::container_init(c_c);

    JavaVM* vm;
    JNIEnv* env;
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 0;
    vm_args.ignoreUnrecognized = 1;
    jint res = JNI_CreateJavaVM(&vm, (void**)&env, &vm_args);

    std::string target_patch = "/home/disk2/fha/JNI/vec/java/load/";
    add_path(env, target_patch.c_str());

    jclass add_c = env->FindClass("HelloBoxAdd");
    // std::cout << "Load Class:" << add_c << std::endl;
    jclass invoke_c = env->FindClass("InvokeMethod");
    jmethodID invoke_method = env->GetStaticMethodID(
            invoke_c, "batch_add3", "(Ljava/lang/Class;[Ljava/lang/Object;[Ljava/lang/Object;)V");
    // std::cout << invoke_method << std::endl;

    jclass intc = env->FindClass("java/lang/Integer");
    // std::cout << "INTCLASS:" << intc << std::endl;
    jmethodID intm = env->GetMethodID(intc, "intValue", "()I");
    // std::cout << "INTMETHOD:" << intm << std::endl;
    jmethodID cvt = env->GetStaticMethodID(invoke_c, "convert", "([Ljava/lang/Object;)[I");
    // std::cout << "CVTMETHOD:" << cvt << std::endl;

    jintArray ary = env->NewIntArray(batch_size);
    jintArray bry = env->NewIntArray(batch_size);
    jobjectArray resy =
            env->NewObjectArray(batch_size, env->FindClass("Ljava/lang/Integer;"), nullptr);
    jobjectArray input = env->NewObjectArray(2, env->FindClass("Ljava/lang/Object;"), nullptr);
    for (auto _ : state) {
        env->SetIntArrayRegion(ary, 0, batch_size, c_a.data());
        env->SetIntArrayRegion(bry, 0, batch_size, c_b.data());
        env->SetObjectArrayElement(input, 0, ary);
        env->SetObjectArrayElement(input, 1, bry);
        env->CallStaticVoidMethod(invoke_c, invoke_method, (jobject)add_c, resy, input);
        // jthrowable jthr = env->ExceptionOccurred();
        // if (jthr) {
        //     dumpStringIfException(env);
        // }
        jobject resz = env->CallStaticObjectMethod(invoke_c, cvt, resy);
        int* buffer3 = env->GetIntArrayElements((jintArray)resz, NULL);
        // memcpy(c_c.data(), buffer3, batch_size * sizeof(int));
        jobject obj = env->GetObjectArrayElement(resy, 0);
        jint vx = env->CallIntMethod(obj, intm);

        // for (int i = 0; i < batch_size; ++i) {
        //     if (c_c[i] != c_a[i] + c_b[i]) {
        //         std::cout << "expect:" << (c_a[i] + c_b[i]) << "Real:" << c_c[i] << std::endl;
        //     }
        // }
        // env->ReleaseIntArrayElements((jintArray)resz, buffer3, 0);
    }

    // std::cout << "Call Destroy:" << method_id << std::endl;
    // env->DeleteLocalRef(add_c);
    // vm->DestroyJavaVM();
}

static void JavaAddBatchHasGCCall(benchmark::State& state) {
    Container c_a;
    Container c_b;
    Container c_c;
    CurrentContainerIniter::container_init(c_a);
    CurrentContainerIniter::container_init(c_b);
    CurrentContainerIniter::container_init(c_c);

    JavaVM* vm;
    JNIEnv* env;
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 0;
    vm_args.ignoreUnrecognized = 1;
    jint res = JNI_CreateJavaVM(&vm, (void**)&env, &vm_args);

    std::string target_patch = "/home/disk2/fha/JNI/vec/java/load/";
    add_path(env, target_patch.c_str());

    jclass add_c = env->FindClass("HelloAdd");
    // std::cout << "Load Class:" << add_c << std::endl;
    jclass invoke_c = env->FindClass("InvokeMethod");
    jmethodID invoke_method =
            env->GetStaticMethodID(invoke_c, "batch_add2", "(Ljava/lang/Class;[I[I)[I");
    jintArray ary = env->NewIntArray(batch_size);
    jintArray bry = env->NewIntArray(batch_size);
    for (auto _ : state) {
        env->SetIntArrayRegion(ary, 0, batch_size, c_a.data());
        env->SetIntArrayRegion(bry, 0, batch_size, c_b.data());

        jobject resy =
                env->CallStaticObjectMethod(invoke_c, invoke_method, (jobject)add_c, ary, bry);
        int* buffer3 = env->GetIntArrayElements((jintArray)resy, NULL);
        memcpy(c_c.data(), buffer3, batch_size * sizeof(int));
        // for (int i = 0; i < batch_size; ++i) {
        //     if (c_c[i] != c_a[i] + c_b[i]) {
        //         std::cout << "expect:" << (c_a[i] + c_b[i]) << "Real:" << c_c[i] << std::endl;
        //     }
        // }
        env->ReleaseIntArrayElements((jintArray)resy, buffer3, 0);
    }

    // std::cout << "Call Destroy:" << method_id << std::endl;
    // env->DeleteLocalRef(add_c);
    // vm->DestroyJavaVM();
}

static void JavaAddFunctionCall(benchmark::State& state) {
    Container c_a;
    Container c_b;
    Container c_c;
    CurrentContainerIniter::container_init(c_a);
    CurrentContainerIniter::container_init(c_b);
    CurrentContainerIniter::container_init(c_c);

    JavaVM* vm;
    JNIEnv* env;
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 0;
    int a = JNI_EDETACHED;
    vm_args.ignoreUnrecognized = 1;
    jint res = JNI_CreateJavaVM(&vm, (void**)&env, &vm_args);
    std::cout << "TRACE++++++ " << res << std::endl;

    std::string target_patch = "/home/disk2/fha/JNI/vec/java/load/";
    add_path(env, target_patch.c_str());

    jclass add_c = env->FindClass("HelloAdd");
    // std::cout << "Load Class:" << add_c << std::endl;
    jmethodID method_id = env->GetStaticMethodID(add_c, "add", "(II)I");
    
    std::cout << "TRACE4"  << std::endl;
    std::thread thr([&]() {
        JNIEnv* env2;
        int rc = vm->AttachCurrentThread((void**)&env2, nullptr);
        std::cout << "TRACExxxxxx " << rc << std::endl;
        std::cout << "TRACExxxxxx " << env2 << std::endl;
        // jclass add_c = env2->FindClass("HelloAdd");
        // jclass add_c = env2->FindClass("HelloAdd");
        jmethodID method_id = env2->GetStaticMethodID(add_c, "add", "(II)I");
        while (true) {
            for (int i = 0; i < batch_size; ++i) {
                c_c[i] = env2->CallStaticIntMethod(add_c, method_id, c_a[i], c_b[i]);
            }
        }
    });
    std::cout << "TRACExxxxxx 33333333" << std::endl;
    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            c_c[i] = env->CallStaticIntMethod(add_c, method_id, c_a[i], c_b[i]);
        }
    }
    thr.join();

    // env->NewDirectByteBuffer(void *address, int capacity);
    // env->directbuffer
}

BENCHMARK(NativeAddCall);
BENCHMARK(NativeAddFunctionCall);
BENCHMARK(JavaAddBatchBoxFunctionCall);
BENCHMARK(JavaAddBatchFunctionCall);
BENCHMARK(JavaAddBatchHasGCCall);
BENCHMARK(JavaAddFunctionCall);
BENCHMARK_MAIN();