/* JNI prototype, not a production JIT. It loads packaged libtcc.so from
 * Android's application native linker namespace and compiles only to a
 * non-executable ET_REL ELF object inside the app-private sandbox.
 * Never execve app data or request executable anonymous writable memory. */
#include <jni.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

typedef void TCCState;
typedef TCCState *(*create_fn)(void);
typedef void (*delete_fn)(TCCState *);
typedef int (*output_type_fn)(TCCState *, int);
typedef int (*compile_fn)(TCCState *, const char *);
typedef int (*output_file_fn)(TCCState *, const char *);

JNIEXPORT jstring JNICALL
Java_com_qigao_tccpoc_SmokeActivity_compileObjectInSandbox(
    JNIEnv *env, jobject self, jstring directory) {
    const char *root = NULL;
    char filename[1024];
    const char *status = "FAIL:UNKNOWN";
    void *module = NULL;
    TCCState *state = NULL;
    FILE *file = NULL;
    unsigned char header[24];
    create_fn create_state = NULL;
    delete_fn delete_state = NULL;
    output_type_fn set_output_type = NULL;
    compile_fn compile_source = NULL;
    output_file_fn output_object = NULL;
    (void)self;
    if (!directory) { status = "FAIL:PATH_NULL"; goto finish; }
    root = (*env)->GetStringUTFChars(env, directory, NULL);
    if (!root) return NULL;
    if (snprintf(filename, sizeof(filename), "%s/native_tcc.o", root) >=
        (int)sizeof(filename)) { status = "FAIL:PATH_TOO_LONG"; goto finish; }
    module = dlopen("libtcc.so", RTLD_NOW | RTLD_LOCAL);
    if (!module) { status = "FAIL:LIBTCC_LOAD"; goto finish; }
    create_state = (create_fn)dlsym(module, "tcc_new");
    delete_state = (delete_fn)dlsym(module, "tcc_delete");
    set_output_type = (output_type_fn)dlsym(module, "tcc_set_output_type");
    compile_source = (compile_fn)dlsym(module, "tcc_compile_string");
    output_object = (output_file_fn)dlsym(module, "tcc_output_file");
    if (!create_state || !delete_state || !set_output_type ||
        !compile_source || !output_object) {
        status = "FAIL:ABI_SYMBOLS"; goto finish;
    }
    state = create_state();
    if (!state) { status = "FAIL:TCC_NEW"; goto finish; }
    /* TCC_OUTPUT_OBJ == 3 in the pinned upstream libtcc.h. */
    if (set_output_type(state, 3) < 0) {
        status = "FAIL:OUTPUT_TYPE"; goto finish;
    }
    if (compile_source(state,
       "int poc_android_apk_symbol(int x) { return (x ^ 0x55) + 7; }") < 0) {
        status = "FAIL:COMPILE"; goto finish;
    }
    if (output_object(state, filename) < 0) {
        status = "FAIL:OUTPUT_FILE"; goto finish;
    }
    file = fopen(filename, "rb");
    if (!file || fread(header, 1, sizeof(header), file) != sizeof(header)) {
        status = "FAIL:ELF_OUTPUT"; goto finish;
    }
    /* Little-endian ELF64 ET_REL, x86-64 or AArch64 e_machine. */
    if (memcmp(header, "\177ELF", 4) != 0 ||
        header[4] != 2 || header[5] != 1 ||
        header[16] != 1 || header[17] != 0 ||
        !((header[18] == 62 && header[19] == 0) ||
          (header[18] == 183 && header[19] == 0))) {
        status = "FAIL:ELF_HEADER"; goto finish;
    }
    status = "PASS:APP_SANDBOX_ELF_OBJECT";
finish:
    if (file) fclose(file);
    if (state && delete_state) delete_state(state);
    if (module) dlclose(module);
    if (root) (*env)->ReleaseStringUTFChars(env, directory, root);
    return (*env)->NewStringUTF(env, status);
}
