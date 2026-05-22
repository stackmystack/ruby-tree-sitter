#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE cQueryMatch;

DATA_WRAP_WITH_TREE(query_match, TSQueryMatch)

VALUE new_query_match(const TSQueryMatch *ptr, VALUE tree) {
  if (ptr == NULL) {
    return Qnil;
  }
  VALUE res = query_match_allocate(cQueryMatch);
  query_match_t *match = unwrap(res);
  match->data = *ptr;
  match->tree = tree;
  return res;
}

DATA_DEFINE_GETTER(query_match, id, UINT2NUM)
DATA_DEFINE_GETTER(query_match, pattern_index, INT2FIX)
DATA_DEFINE_GETTER(query_match, capture_count, INT2FIX)

static VALUE query_match_get_captures(VALUE self) {
  query_match_t *match = unwrap(self);

  uint16_t length = match->data.capture_count;
  VALUE res = rb_ary_new_capa(length);
  const TSQueryCapture *captures = match->data.captures;
  for (int i = 0; i < length; i++) {
    rb_ary_push(res, new_query_capture(&captures[i], match->tree));
  }

  return res;
}

static VALUE query_match_inspect(VALUE self) {
  TSQueryMatch query_match = SELF;
  return rb_sprintf("{id=%d, pattern_inex=%d, capture_count=%d}",
                    query_match.id, query_match.pattern_index,
                    query_match.capture_count);
}

void init_query_match(void) {
  cQueryMatch = rb_define_class_under(mTreeSitter, "QueryMatch", rb_cObject);

  rb_define_alloc_func(cQueryMatch, query_match_allocate);

  /* Class methods */
  DECLARE_GETTER(cQueryMatch, query_match, id)
  DECLARE_GETTER(cQueryMatch, query_match, pattern_index)
  DECLARE_GETTER(cQueryMatch, query_match, capture_count)
  DECLARE_GETTER(cQueryMatch, query_match, captures)
  rb_define_method(cQueryMatch, "inspect", query_match_inspect, 0);
  rb_define_method(cQueryMatch, "to_s", query_match_inspect, 0);
}
