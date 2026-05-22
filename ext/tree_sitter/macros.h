#ifndef _RB_TREE_SITTER_MACROS_H
#define _RB_TREE_SITTER_MACROS_H

#define DECLARE_GETTER(klass, type, field)                                     \
  rb_define_method(klass, #field, type##_get_##field, 0);

#define DECLARE_SETTER(klass, type, field)                                     \
  rb_define_method(klass, #field "=", type##_set_##field, 1);

#define DECLARE_ACCESSOR(klass, type, field)                                   \
  DECLARE_GETTER(klass, type, field)                                           \
  DECLARE_SETTER(klass, type, field)

// Plain DEFINE_GETTER/DEFINE_SETTER/etc are for TypedData structs, reaching
// their top-level fields, and are of type VALUE
//
// DATA_* are for TypeData structs, raching their data field
// which can be of an arbitraty tuype.

#define DEFINE_GETTER(type, field)                                             \
  static VALUE type##_get_##field(VALUE self) { return (unwrap(self))->field; }

#define DEFINE_SETTER(type, field)                                             \
  static VALUE type##_set_##field(VALUE self, VALUE val) {                     \
    unwrap(self)->field = val;                                                 \
    return Qnil;                                                               \
  }

#define DEFINE_ACCESSOR(type, field)                                           \
  DEFINE_GETTER(type, field)                                                   \
  DEFINE_SETTER(type, field)

#define DATA_WRAP(base, type)                                                  \
  DATA_TYPE(TS##base, type)                                                    \
  DATA_FREE(type)                                                              \
  DATA_MEMSIZE(type)                                                           \
  DATA_DECLARE_DATA_TYPE(type)                                                 \
  DATA_ALLOCATE(type)                                                          \
  DATA_UNWRAP(type)                                                            \
  DATA_NEW(c##base, TS##base, type)                                            \
  DATA_FROM_VALUE(TS##base, type)

#define DATA_PTR_WRAP(base, type)                                              \
  DATA_TYPE(TS##base *, type)                                                  \
  DATA_FREE_PTR(type)                                                          \
  DATA_MEMSIZE(type)                                                           \
  DATA_DECLARE_DATA_TYPE(type)                                                 \
  DATA_ALLOCATE(type)                                                          \
  DATA_UNWRAP(type)                                                            \
  DATA_PTR_NEW(c##base, TS##base, type)                                        \
  DATA_FROM_VALUE(TS##base *, type)

#define DATA_TYPE(klass, type)                                                 \
  typedef struct {                                                             \
    klass data;                                                                \
  } type##_t;

#define DATA_FREE(type)                                                        \
  static void type##_free(void *ptr) { xfree(ptr); }

#define DATA_FREE_PTR(type)                                                    \
  static void type##_free(void *ptr) {                                         \
    type##_t *type = (type##_t *)ptr;                                          \
    if (type->data != NULL) {                                                  \
      ts_##type##_delete(type->data);                                          \
    }                                                                          \
    xfree(ptr);                                                                \
  }

#define DATA_MEMSIZE(type)                                                     \
  static size_t type##_memsize(const void *ptr) {                              \
    type##_t *type = (type##_t *)ptr;                                          \
    return sizeof(type);                                                       \
  }

#define DATA_DECLARE_DATA_TYPE(type)                                           \
  const rb_data_type_t type##_data_type = {                                    \
      .wrap_struct_name = #type "",                                            \
      .function =                                                              \
          {                                                                    \
              .dmark = NULL,                                                   \
              .dfree = type##_free,                                            \
              .dsize = type##_memsize,                                         \
              .dcompact = NULL,                                                \
          },                                                                   \
      .flags = RUBY_TYPED_FREE_IMMEDIATELY,                                    \
  };

#define DATA_ALLOCATE(type)                                                    \
  static VALUE type##_allocate(VALUE klass) {                                  \
    type##_t *type;                                                            \
    VALUE res =                                                                \
        TypedData_Make_Struct(klass, type##_t, &type##_data_type, type);       \
    *type = (type##_t){0};                                                     \
    return res;                                                                \
  }

#define DATA_UNWRAP(type)                                                      \
  static type##_t *unwrap(VALUE self) {                                        \
    type##_t *type;                                                            \
    TypedData_Get_Struct(self, type##_t, &type##_data_type, type);             \
    return type;                                                               \
  }

#define SELF unwrap(self)->data

#define DATA_NEW(klass, struct, type)                                          \
  VALUE new_##type(const struct *ptr) {                                        \
    if (ptr == NULL) {                                                         \
      return Qnil;                                                             \
    }                                                                          \
    VALUE res = type##_allocate(klass);                                        \
    type##_t *type = unwrap(res);                                              \
    type->data = *ptr;                                                         \
    return res;                                                                \
  }                                                                            \
  VALUE new_##type##_by_val(struct ptr) {                                      \
    VALUE res = type##_allocate(klass);                                        \
    type##_t *type = unwrap(res);                                              \
    type->data = ptr;                                                          \
    return res;                                                                \
  }

#define DATA_FROM_VALUE(struct, type)                                          \
  struct value_to_##type(VALUE self) { return (unwrap(self))->data; }

#define DATA_PTR_NEW(klass, struct, type)                                      \
  VALUE new_##type(struct *ptr) {                                              \
    if (ptr == NULL) {                                                         \
      return Qnil;                                                             \
    }                                                                          \
    VALUE res = type##_allocate(klass);                                        \
    type##_t *type = unwrap(res);                                              \
    type->data = ptr;                                                          \
    return res;                                                                \
  }

#define DATA_DEFINE_GETTER(type, field, cast)                                  \
  static VALUE type##_get_##field(VALUE self) {                                \
    return cast((unwrap(self))->data.field);                                   \
  }

#define DATA_DEFINE_SETTER(type, field, cast)                                  \
  static VALUE type##_set_##field(VALUE self, VALUE val) {                     \
    type##_t *type = unwrap(self);                                             \
    type->data.field = cast(val);                                              \
    return Qnil;                                                               \
  }

#define DATA_DEFINE_ACCESSOR(type, field, cast_get, cast_set)                  \
  DATA_DEFINE_GETTER(type, field, cast_get)                                    \
  DATA_DEFINE_SETTER(type, field, cast_set)

#define DATA_FAST_FORWARD_FNV(type, fn, field)                                 \
  static VALUE type##_##fn(int argc, VALUE *argv, VALUE self) {                \
    type##_t *type = unwrap(self);                                             \
    if (!NIL_P(type->field)) {                                                 \
      return rb_funcallv(type->field, rb_intern(#fn ""), argc, argv);          \
    } else {                                                                   \
      return Qnil;                                                             \
    }                                                                          \
  }

// Shared body for types that embed a VALUE tree field for GC tracing.
// The caller must define type##_t and type##_free before invoking this.
#define _DATA_WRAP_TREE_SHARED(type, ctype)                                    \
  static size_t type##_memsize(const void *ptr) { return sizeof(type##_t); }   \
  static void type##_mark(void *ptr) {                                         \
    type##_t *t = (type##_t *)ptr;                                             \
    rb_gc_mark_movable(t->tree);                                               \
  }                                                                            \
  static void type##_compact(void *ptr) {                                      \
    type##_t *t = (type##_t *)ptr;                                             \
    t->tree = rb_gc_location(t->tree);                                         \
  }                                                                            \
  const rb_data_type_t type##_data_type = {                                    \
      .wrap_struct_name = #type "",                                            \
      .function =                                                              \
          {                                                                    \
              .dmark = type##_mark,                                            \
              .dfree = type##_free,                                            \
              .dsize = type##_memsize,                                         \
              .dcompact = type##_compact,                                      \
          },                                                                   \
      .flags = RUBY_TYPED_FREE_IMMEDIATELY,                                    \
  };                                                                           \
  static VALUE type##_allocate(VALUE klass) {                                  \
    type##_t *t;                                                               \
    VALUE res = TypedData_Make_Struct(klass, type##_t, &type##_data_type, t);  \
    t->data = (ctype){0};                                                      \
    t->tree = Qnil;                                                            \
    return res;                                                                \
  }                                                                            \
  static type##_t *unwrap(VALUE self) {                                        \
    type##_t *t;                                                               \
    TypedData_Get_Struct(self, type##_t, &type##_data_type, t);                \
    return t;                                                                  \
  }

// TypedData wrapper for a C struct that holds a VALUE tree reference.
// The tree keeps the owning TSTree alive for the GC; no additional C-level
// cleanup is needed.
//
//   DATA_WRAP_WITH_TREE(node, TSNode)
//   DATA_WRAP_WITH_TREE(query_match, TSQueryMatch)
//
#define DATA_WRAP_WITH_TREE(type, ctype)                                       \
  typedef struct {                                                             \
    ctype data;                                                                \
    VALUE tree;                                                                \
  } type##_t;                                                                  \
  static void type##_free(void *ptr) { xfree(ptr); }                           \
  _DATA_WRAP_TREE_SHARED(type, ctype)

// Like DATA_WRAP_WITH_TREE, but calls ts_<type>_delete() on the C data
// before freeing the Ruby wrapper.
//
//   DATA_WRAP_WITH_TREE_DELETE(tree_cursor, TSTreeCursor)
//
#define DATA_WRAP_WITH_TREE_DELETE(type, ctype)                                \
  typedef struct {                                                             \
    ctype data;                                                                \
    VALUE tree;                                                                \
  } type##_t;                                                                  \
  static void type##_free(void *ptr) {                                         \
    type##_t *t = (type##_t *)ptr;                                             \
    ts_##type##_delete(&t->data);                                              \
    xfree(ptr);                                                                \
  }                                                                            \
  _DATA_WRAP_TREE_SHARED(type, ctype)

#endif
