#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE cQueryCapture;

// Manual struct — holds a tree reference so captured nodes don't dangle.
typedef struct {
  TSQueryCapture data;
  VALUE tree;
} query_capture_t;

static void query_capture_free(void *ptr) { xfree(ptr); }

static size_t query_capture_memsize(const void *ptr) {
  query_capture_t *cap = (query_capture_t *)ptr;
  return sizeof(cap);
}

static void query_capture_mark(void *ptr) {
  query_capture_t *cap = (query_capture_t *)ptr;
  rb_gc_mark_movable(cap->tree);
}

static void query_capture_compact(void *ptr) {
  query_capture_t *cap = (query_capture_t *)ptr;
  cap->tree = rb_gc_location(cap->tree);
}

const rb_data_type_t query_capture_data_type = {
    .wrap_struct_name = "query_capture",
    .function =
        {
            .dmark = query_capture_mark,
            .dfree = query_capture_free,
            .dsize = query_capture_memsize,
            .dcompact = query_capture_compact,
        },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE query_capture_allocate(VALUE klass) {
  query_capture_t *cap;
  VALUE res = TypedData_Make_Struct(klass, query_capture_t,
                                    &query_capture_data_type, cap);
  cap->tree = Qnil;
  return res;
}

static query_capture_t *unwrap(VALUE self) {
  query_capture_t *cap;
  TypedData_Get_Struct(self, query_capture_t, &query_capture_data_type, cap);
  return cap;
}

VALUE new_query_capture(const TSQueryCapture *ptr, VALUE tree) {
  if (ptr == NULL) {
    return Qnil;
  }
  VALUE res = query_capture_allocate(cQueryCapture);
  query_capture_t *cap = unwrap(res);
  cap->data = *ptr;
  cap->tree = tree;
  return res;
}

DATA_DEFINE_GETTER(query_capture, index, UINT2NUM)

// Getter that creates a Node pinned to our tree.
static VALUE query_capture_get_node(VALUE self) {
  return new_node_by_val(SELF.node, unwrap(self)->tree);
}

static VALUE query_capture_inspect(VALUE self) {
  TSQueryCapture query_capture = SELF;
  return rb_sprintf("{index=%d, node=%+" PRIsVALUE "}", query_capture.index,
                    new_node(&query_capture.node, unwrap(self)->tree));
}

void init_query_capture(void) {
  cQueryCapture =
      rb_define_class_under(mTreeSitter, "QueryCapture", rb_cObject);

  rb_define_alloc_func(cQueryCapture, query_capture_allocate);

  /* Class methods */
  DECLARE_GETTER(cQueryCapture, query_capture, index)
  DECLARE_GETTER(cQueryCapture, query_capture, node)
  rb_define_method(cQueryCapture, "inspect", query_capture_inspect, 0);
  rb_define_method(cQueryCapture, "to_s", query_capture_inspect, 0);
}
