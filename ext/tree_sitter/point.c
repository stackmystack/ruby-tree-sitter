#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE cPoint;

DATA_WRAP(Point, point)
DATA_DEFINE_ACCESSOR(point, row, UINT2NUM, NUM2UINT)
DATA_DEFINE_ACCESSOR(point, column, UINT2NUM, NUM2UINT)

static VALUE point_inspect(VALUE self) {
  point_t *point = unwrap(self);
  return rb_sprintf("{row=%i, column=%i}", point->data.row, point->data.column);
}

/**
 * Edit a point to keep it in-sync with source code that has been edited.
 *
 * This function updates a single point's byte offset and row/column position
 * based on an edit operation. The point is mutated in place, and the updated
 * byte offset is returned. This is useful for editing points without requiring
 * a tree or node instance.
 *
 * @param byte_offset [Integer] the byte offset associated with this point.
 * @param input_edit  [InputEdit]
 *
 * @return [Integer] the updated byte offset.
 */
static VALUE point_edit(VALUE self, VALUE byte_offset, VALUE input_edit) {
  uint32_t byte = NUM2UINT(byte_offset);
  TSInputEdit edit = value_to_input_edit(input_edit);
  ts_point_edit(&SELF, &byte, &edit);
  return UINT2NUM(byte);
}

void init_point(void) {
  cPoint = rb_define_class_under(mTreeSitter, "Point", rb_cObject);

  rb_define_alloc_func(cPoint, point_allocate);

  /* Class methods */
  DECLARE_ACCESSOR(cPoint, point, row)
  DECLARE_ACCESSOR(cPoint, point, column)

  rb_define_method(cPoint, "edit", point_edit, 2);
  rb_define_method(cPoint, "inspect", point_inspect, 0);
  rb_define_method(cPoint, "to_s", point_inspect, 0);
}
