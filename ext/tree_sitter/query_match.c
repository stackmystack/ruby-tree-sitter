#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE cQueryMatch;

// Manual struct — holds a tree reference so captured nodes don't dangle.
typedef struct {
  TSQueryMatch data;
  VALUE tree;
} query_match_t;

static void query_match_free(void *ptr) { xfree(ptr); }

static size_t query_match_memsize(const void *ptr) {
  query_match_t *match = (query_match_t *)ptr;
  return sizeof(match);
}

static void query_match_mark(void *ptr) {
  query_match_t *match = (query_match_t *)ptr;
  rb_gc_mark_movable(match->tree);
}

static void query_match_compact(void *ptr) {
  query_match_t *match = (query_match_t *)ptr;
  match->tree = rb_gc_location(match->tree);
}

const rb_data_type_t query_match_data_type = {
    .wrap_struct_name = "query_match",
    .function =
        {
            .dmark = query_match_mark,
            .dfree = query_match_free,
            .dsize = query_match_memsize,
            .dcompact = query_match_compact,
        },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE query_match_allocate(VALUE klass) {
  query_match_t *match;
  VALUE res = TypedData_Make_Struct(klass, query_match_t,
                                    &query_match_data_type, match);
  match->tree = Qnil;
  return res;
}

static query_match_t *unwrap(VALUE self) {
  query_match_t *match;
  TypedData_Get_Struct(self, query_match_t, &query_match_data_type, match);
  return match;
}

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
