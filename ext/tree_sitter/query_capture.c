#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE cQueryCapture;

DATA_WRAP_WITH_TREE(query_capture, TSQueryCapture)

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
