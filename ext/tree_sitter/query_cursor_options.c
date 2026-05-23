#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE cQueryCursorOptions;

typedef struct {
  TSQueryCursorOptions data;
  VALUE callback;
} query_cursor_options_t;

static void query_cursor_options_free(void *ptr) { xfree(ptr); }

static size_t query_cursor_options_memsize(const void *ptr) {
  query_cursor_options_t *opts = (query_cursor_options_t *)ptr;
  return sizeof(opts);
}

static void query_cursor_options_mark(void *ptr) {
  query_cursor_options_t *opts = (query_cursor_options_t *)ptr;
  rb_gc_mark_movable(opts->callback);
}

static void query_cursor_options_compact(void *ptr) {
  query_cursor_options_t *opts = (query_cursor_options_t *)ptr;
  opts->callback = rb_gc_location(opts->callback);
}

const rb_data_type_t query_cursor_options_data_type = {
    .wrap_struct_name = "query_cursor_options",
    .function =
        {
            .dmark = query_cursor_options_mark,
            .dfree = query_cursor_options_free,
            .dsize = query_cursor_options_memsize,
            .dcompact = query_cursor_options_compact,
        },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static query_cursor_options_t *unwrap(VALUE self) {
  query_cursor_options_t *opts;
  TypedData_Get_Struct(self, query_cursor_options_t,
                       &query_cursor_options_data_type, opts);
  return opts;
}

static VALUE query_cursor_options_allocate(VALUE klass) {
  query_cursor_options_t *opts;
  VALUE res = TypedData_Make_Struct(klass, query_cursor_options_t,
                                    &query_cursor_options_data_type, opts);
  opts->data = (TSQueryCursorOptions){0};
  opts->callback = Qnil;
  return res;
}

static bool query_cursor_progress_callback(TSQueryCursorState *state) {
  query_cursor_options_t *opts = (query_cursor_options_t *)state->payload;
  VALUE rb_state = new_query_cursor_state(state);
  VALUE result = rb_funcall(opts->callback, rb_intern("call"), 1, rb_state);
  return RTEST(result);
}

/**
 * Create a new query-cursor options object.
 *
 * The callback is called periodically during query execution with a
 * {QueryCursorState} argument.  Return +true+ from the callback to
 * cancel the query early.
 *
 * Any Ruby object that responds to +#call+ is accepted: a Proc, a lambda,
 * a Method object, or even a custom callable.
 *
 * @example
 *   opts = TreeSitter::QueryCursorOptions.new(->(state) {
 *     state.current_byte_offset > 10_000
 *   })
 *
 * @param callback [#call]
 *
 * @return [QueryCursorOptions]
 */
static VALUE query_cursor_options_initialize(int argc, VALUE *argv,
                                             VALUE self) {
  query_cursor_options_t *opts = unwrap(self);
  VALUE callback;
  rb_scan_args(argc, argv, "01", &callback);

  opts->callback = callback;
  opts->data.payload = (void *)opts;
  opts->data.progress_callback = query_cursor_progress_callback;

  return self;
}

/**
 * @!visibility private
 */
const TSQueryCursorOptions *value_to_query_cursor_options(VALUE self) {
  return &(unwrap(self)->data);
}

void init_query_cursor_options(void) {
  cQueryCursorOptions =
      rb_define_class_under(mTreeSitter, "QueryCursorOptions", rb_cObject);

  rb_define_alloc_func(cQueryCursorOptions, query_cursor_options_allocate);

  // Class methods
  rb_define_method(cQueryCursorOptions, "initialize",
                   query_cursor_options_initialize, -1);
}
