#ifndef COMMON_H
#define COMMON_H

/* Macros to ease some of the boilerplate */
VALUE generic_new(VALUE klass, void *ptr, VALUE conn,
                  RUBY_DATA_FUNC free_func);

#define generic_get(kind, v)                                            \
    do {                                                                \
        vir##kind##Ptr ptr;                                             \
        Data_Get_Struct(v, vir##kind, ptr);                             \
        if (!ptr) {                                                     \
            rb_raise(rb_eArgError, #kind " has been freed");            \
        }                                                               \
        return ptr;                                                     \
    } while (0);

#define generic_free(kind, p)                                           \
    do {                                                                \
        int r;                                                          \
        r = vir##kind##Free((vir##kind##Ptr) p);                        \
        if (r < 0) {                                                    \
            rb_raise(rb_eSystemCallError, # kind " free failed");       \
        }                                                               \
    } while(0);

VALUE create_error(VALUE error, const char* method, virConnectPtr conn);

/*
 * Code generating macros.
 *
 * We only generate function bodies, not the whole function
 * declaration.
 */

/* Generate a call to a function FUNC which returns a string. The Ruby
 * function will return the string on success and throw an exception on
 * error. The string returned by FUNC is freed if dealloc is true.
 */
#define gen_call_string(func, conn, dealloc, args...)                   \
    do {                                                                \
        const char *str;                                                \
        VALUE result;                                                   \
        int exception;                                                  \
                                                                        \
        str = func(args);                                               \
        _E(str == NULL, create_error(e_Error, # func, conn));           \
                                                                        \
        if (dealloc) {                                                  \
            result = rb_protect(rb_str_new2_wrap, (VALUE)&str, &exception); \
            xfree((void *) str);                                        \
            if (exception) {                                            \
                rb_jump_tag(exception);                                 \
            }                                                           \
        }                                                               \
        else {                                                          \
            result = rb_str_new2(str);                                  \
        }                                                               \
        return result;                                                  \
    } while(0)

/* Generate a call to vir##KIND##Free and return Qnil. Set the the embedded
 * vir##KIND##Ptr to NULL. If that pointer is already NULL, do nothing.
 */
#define gen_call_free(kind, s)                                          \
    do {                                                                \
        vir##kind##Ptr ptr;                                             \
        Data_Get_Struct(s, vir##kind, ptr);                             \
        if (ptr != NULL) {                                              \
            int r = vir##kind##Free(ptr);                               \
            _E(r < 0, create_error(e_Error, "vir" #kind "Free", connect_get(s))); \
            DATA_PTR(s) = NULL;                                         \
        }                                                               \
        return Qnil;                                                    \
    } while (0)

/* Generate a call to a function FUNC which returns an int error, where -1
 * indicates error and 0 success. The Ruby function will return Qnil on
 * success and throw an exception on error.
 */
#define gen_call_void(func, conn, args...)                              \
    do {                                                                \
        int _r_##func;                                                  \
        _r_##func = func(args);                                         \
        _E(_r_##func < 0, create_error(e_Error, #func, conn));          \
        return Qnil;                                                    \
    } while(0)

/* Generate a call to a function FUNC which returns an int; -1 indicates
 * error, 0 indicates Qfalse, and 1 indicates Qtrue.
 */
#define gen_call_truefalse(func, conn, args...)                         \
    do {                                                                \
        int _r_##func;                                                  \
        _r_##func = func(args);                                         \
        _E(_r_##func < 0, create_error(e_Error, #func, conn));          \
        return _r_##func ? Qtrue : Qfalse;                              \
    } while(0)

/* Generate a call to a function FUNC which returns an int error, where -1
 * indicates error and >= 0 success. The Ruby function will return the integer
 * success and throw an exception on error.
 */
#define gen_call_int(func, conn, args...)                               \
    do {                                                                \
        int _r_##func;                                                  \
        _r_##func = func(args);                                         \
        _E(_r_##func < 0, create_error(e_RetrieveError, #func, conn));  \
        return INT2NUM(_r_##func);                                      \
    } while(0)

#define gen_list_all(type, argc, argv, listfunc, firstarg, val, newfunc, freefunc) \
    do {                                                                \
        VALUE flags;                                                    \
        type *list;                                                     \
        size_t i;                                                       \
        int ret;                                                        \
        VALUE result;                                                   \
        int exception = 0;                                              \
        struct rb_ary_push_arg arg;                                     \
                                                                        \
        rb_scan_args(argc, argv, "01", &flags);                         \
        flags = integer_default_if_nil(flags, 0);                       \
        ret = listfunc(firstarg, &list, flags); \
        _E(ret < 0, create_error(e_RetrieveError, #listfunc, connect_get(val))); \
        result = rb_protect(rb_ary_new2_wrap, (VALUE)&ret, &exception); \
        if (exception) {                                                \
            goto exception;                                             \
        }                                                               \
        for (i = 0; i < ret; i++) {                                     \
            arg.arr = result;                                           \
            arg.value = newfunc(list[i], val);                            \
            rb_protect(rb_ary_push_wrap, (VALUE)&arg, &exception);      \
            if (exception) {                                            \
                goto exception;                                         \
            }                                                           \
        }                                                               \
                                                                        \
        free(list);                                                     \
                                                                        \
        return result;                                                  \
                                                                        \
    exception:                                                          \
        for (i = 0; i < ret; i++) {                                     \
            freefunc(list[i]);                                          \
        }                                                               \
        free(list);                                                     \
        rb_jump_tag(exception);                                         \
                                                                        \
        /* not needed, but here to shut the compiler up */              \
        return Qnil;                                                    \
    } while(0)

/* Error handling */
#define _E(cond, excep) \
    do { if (cond) rb_exc_raise(excep); } while(0)

int is_symbol_or_proc(VALUE handle);

extern VALUE e_RetrieveError;
extern VALUE e_Error;
extern VALUE e_DefinitionError;
extern VALUE e_NoSupportError;

extern VALUE m_libvirt;

char *get_string_or_nil(VALUE arg);

VALUE gen_list(int num, char **list);

VALUE get_parameters(int argc, VALUE *argv, VALUE d, virConnectPtr conn,
                     int (*nparams_cb)(VALUE d, unsigned int flags),
                     char *(*get_cb)(VALUE d, unsigned int flags,
                                     virTypedParameterPtr params, int *nparams));
VALUE set_parameters(VALUE d, VALUE in, virConnectPtr conn,
                     int (*nparams_cb)(VALUE d, unsigned int flags),
                     char *(*get_cb)(VALUE d, unsigned int flags,
                                     virTypedParameterPtr params, int *nparams),
                     char *(*set_cb)(VALUE d, unsigned int flags,
                                     virTypedParameterPtr params, int nparams));

VALUE integer_default_if_nil(VALUE in, int def);

struct rb_ary_entry_arg {
    VALUE arr;
    int elem;
};
VALUE rb_ary_entry_wrap(VALUE arg);
VALUE rb_ary_new_wrap(VALUE arg);
struct rb_ary_push_arg {
    VALUE arr;
    VALUE value;
};
VALUE rb_ary_push_wrap(VALUE arg);
VALUE rb_ary_new2_wrap(VALUE arg);

VALUE rb_str_new2_wrap(VALUE arg);
struct rb_str_new_arg {
    char *val;
    size_t size;
};
VALUE rb_str_new_wrap(VALUE arg);
VALUE rb_string_value_cstr_wrap(VALUE arg);

struct rb_iv_set_arg {
    VALUE klass;
    char *member;
    VALUE value;
};
VALUE rb_iv_set_wrap(VALUE arg);

struct rb_class_new_instance_arg {
    int argc;
    VALUE *argv;
    VALUE klass;
};
VALUE rb_class_new_instance_wrap(VALUE arg);

#ifndef RARRAY_LEN
#define RARRAY_LEN(ar) (RARRAY(ar)->len)
#endif

#ifndef RSTRING_PTR
#define RSTRING_PTR(str) (RSTRING(str)->ptr)
#endif

#ifndef RSTRING_LEN
#define RSTRING_LEN(str) (RSTRING(str)->len)
#endif

#endif
