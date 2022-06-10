#include "tree_sitter.h"
#include <sys/_types/_u_int32_t.h>

extern VALUE mTreeSitter;

VALUE cTree;

/*
** TSTree is special. You cannot construct it with ts_tree_new for example, so
** we can't just * assign a free method to its Ruby wrapper.
**
** However, it has a ts_tree_delete! So we're going to include it because it's
** int he public * API, but not as a normal Ruby method `delete` instead of a
** Ruby Object free hook.
*/

TSTree *value_to_tree(VALUE self) {
  TSTree *tree = NULL;

  if (!NIL_P(self)) {
    Data_Get_Struct(self, TSTree, tree);
  }

  return tree;
}

VALUE new_tree(const TSTree *tree) {
  return Data_Wrap_Struct(cTree, NULL, NULL, (void *)tree);
}

static VALUE tree_copy(VALUE self) {
  TSTree *tree = value_to_tree(self);

  return new_tree(ts_tree_copy(tree));
}

static VALUE tree_delete(VALUE self) {
  TSTree *tree = value_to_tree(self);

  ts_tree_delete(tree);

  return Qnil;
}

static VALUE tree_root_node(VALUE self) {
  TSTree *tree = value_to_tree(self);

  TSNode root = ts_tree_root_node(tree);

  return new_node(&root);
}

static VALUE tree_language(VALUE self) {
  TSTree *tree = value_to_tree(self);

  const TSLanguage *language = ts_tree_language(tree);

  return new_language(language);
}

static VALUE tree_edit(VALUE self, VALUE edit) {
  TSTree *tree = value_to_tree(self);
  TSInputEdit *in = value_to_input_edit(edit);

  ts_tree_edit(tree, in);

  return Qnil;
}

static VALUE tree_changed_ranges(VALUE _self, VALUE old_tree, VALUE new_tree) {
  TSTree *old = value_to_tree(old_tree);
  TSTree *new = value_to_tree(new_tree);
  uint32_t length;
  TSRange *ranges = ts_tree_get_changed_ranges(old, new, &length);
  VALUE res = rb_ary_new_capa(length);

  for (uint32_t i = 0; i < length; i++) {
    rb_ary_push(res, new_range(&ranges[i]));
  }

  free(ranges);

  return res;
}

static VALUE tree_print_dot_graph(VALUE self, VALUE file) {
  Check_Type(file, T_STRING);

  TSTree *parser = value_to_tree(self);
  char *path = StringValueCStr(file);
  FILE *fd = fopen(path, "w+");

  ts_tree_print_dot_graph(parser, fd);
  fclose(fd);

  return Qnil;
}

void init_tree(void) {
  cTree = rb_define_class_under(mTreeSitter, "Tree", rb_cObject);

  /* Class methods */
  rb_define_method(cTree, "copy", tree_copy, 0);
  rb_define_method(cTree, "delete", tree_delete, 0);
  rb_define_method(cTree, "root_node", tree_root_node, 0);
  rb_define_method(cTree, "language", tree_language, 0);
  rb_define_method(cTree, "edit", tree_edit, 1);
  rb_define_module_function(cTree, "changed_ranges", tree_changed_ranges, 2);
  rb_define_method(cTree, "print_dot_graph", tree_print_dot_graph, 1);
}
