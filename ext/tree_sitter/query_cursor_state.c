#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE cQueryCursorState;

DATA_WRAP(QueryCursorState, query_cursor_state)
DATA_DEFINE_GETTER(query_cursor_state, current_byte_offset, UINT2NUM)

static VALUE query_cursor_state_inspect(VALUE self) {
  query_cursor_state_t *state = unwrap(self);
  return rb_sprintf("{current_byte_offset=%i}",
                    state->data.current_byte_offset);
}

void init_query_cursor_state(void) {
  cQueryCursorState =
      rb_define_class_under(mTreeSitter, "QueryCursorState", rb_cObject);

  rb_define_alloc_func(cQueryCursorState, query_cursor_state_allocate);

  // Class methods
  DECLARE_GETTER(cQueryCursorState, query_cursor_state, current_byte_offset)
  rb_define_method(cQueryCursorState, "inspect", query_cursor_state_inspect, 0);
  rb_define_method(cQueryCursorState, "to_s", query_cursor_state_inspect, 0);
}
