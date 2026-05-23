#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE cQueryCursor;

// Manual struct — holds a tree reference pinned from the exec'd node.
typedef struct {
  TSQueryCursor *data;
  VALUE tree;
} query_cursor_t;

static void query_cursor_free(void *ptr) {
  query_cursor_t *type = (query_cursor_t *)ptr;
  if (type->data != NULL) {
    ts_query_cursor_delete(type->data);
  }
  xfree(ptr);
}

static size_t query_cursor_memsize(const void *ptr) {
  query_cursor_t *type = (query_cursor_t *)ptr;
  return sizeof(type);
}

static void query_cursor_mark(void *ptr) {
  query_cursor_t *cursor = (query_cursor_t *)ptr;
  rb_gc_mark_movable(cursor->tree);
}

static void query_cursor_compact(void *ptr) {
  query_cursor_t *cursor = (query_cursor_t *)ptr;
  cursor->tree = rb_gc_location(cursor->tree);
}

const rb_data_type_t query_cursor_data_type = {
    .wrap_struct_name = "query_cursor",
    .function =
        {
            .dmark = query_cursor_mark,
            .dfree = query_cursor_free,
            .dsize = query_cursor_memsize,
            .dcompact = query_cursor_compact,
        },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE query_cursor_allocate(VALUE klass) {
  query_cursor_t *cursor;
  VALUE res = TypedData_Make_Struct(klass, query_cursor_t,
                                    &query_cursor_data_type, cursor);
  cursor->data = ts_query_cursor_new();
  cursor->tree = Qnil;
  return res;
}

static query_cursor_t *unwrap(VALUE self) {
  query_cursor_t *cursor;
  TypedData_Get_Struct(self, query_cursor_t, &query_cursor_data_type, cursor);
  return cursor;
}

TSQueryCursor *value_to_query_cursor(VALUE self) { return SELF; }

/**
 * Start running a given query on a given node.
 *
 * @param query [Query]
 * @param node  [Node]
 *
 * @return [QueryCursor]
 */
static VALUE query_cursor_exec_static(VALUE self, VALUE query, VALUE node) {
  VALUE res = query_cursor_allocate(cQueryCursor);
  query_cursor_t *cursor = unwrap(res);
  ts_query_cursor_exec(cursor->data, value_to_query(query),
                       value_to_node(node));
  // Pin the tree so created matches/captures stay alive.
  cursor->tree = node_tree(node);
  return res;
}

/**
 * Start running a given query on a given node.
 *
 * @param query [Query]
 * @param node  [Node]
 *
 * @return [QueryCursor]
 */
static VALUE query_cursor_exec(VALUE self, VALUE query, VALUE node) {
  query_cursor_t *cursor = unwrap(self);
  ts_query_cursor_exec(cursor->data, value_to_query(query),
                       value_to_node(node));
  cursor->tree = node_tree(node);
  return self;
}

/**
 * Manage the maximum number of in-progress matches allowed by this query
 * cursor.
 *
 * Query cursors have an optional maximum capacity for storing lists of
 * in-progress captures. If this capacity is exceeded, then the
 * earliest-starting match will silently be dropped to make room for further
 * matches. This maximum capacity is optional — by default, query cursors allow
 * any number of pending matches, dynamically allocating new space for them as
 * needed as the query is executed.
 */
static VALUE query_cursor_did_exceed_match_limit(VALUE self) {
  return ts_query_cursor_did_exceed_match_limit(SELF) ? Qtrue : Qfalse;
}

/**
 * @param limit [Integer]
 *
 * @return [nil]
 */
static VALUE query_cursor_set_match_limit(VALUE self, VALUE limit) {
  ts_query_cursor_set_match_limit(SELF, NUM2UINT(limit));
  return Qnil;
}

/**
 * @return [Integer]
 */
static VALUE query_cursor_get_match_limit(VALUE self) {
  return UINT2NUM(ts_query_cursor_match_limit(SELF));
}

/**
 * Set the maximum start depth for a query cursor.
 *
 * This prevents cursors from exploring children nodes at a certain depth.
 * Note if a pattern includes many children, then they will still be checked.
 *
 * The zero max start depth value can be used as a special behavior and
 * it helps to destructure a subtree by staying on a node and using captures
 * for interested parts. Note that the zero max start depth only limit a search
 * depth for a pattern's root node but other nodes that are parts of the pattern
 * may be searched at any depth what defined by the pattern structure.
 *
 * @param max_start_depth [Integer|nil] set to nil to remove the maximum start
 * depth.
 *
 * @return [nil]
 */
static VALUE query_cursor_set_max_start_depth(VALUE self,
                                              VALUE max_start_depth) {
  uint32_t max = UINT32_MAX;
  if (!NIL_P(max_start_depth)) {
    max = NUM2UINT(max_start_depth);
  }
  ts_query_cursor_set_max_start_depth(SELF, max);
  return Qnil;
}

/**
 * Advance to the next capture of the currently running query.
 *
 * @return [Array<Integer|Match>|nil] If there is a capture,
 *  return a tuple [Integer, Match], otherwise return +nil+.
 */
static VALUE query_cursor_next_capture(VALUE self) {
  TSQueryMatch match;
  uint32_t index;
  if (ts_query_cursor_next_capture(SELF, &match, &index)) {
    VALUE tree = unwrap(self)->tree;
    VALUE res = rb_ary_new_capa(2);
    rb_ary_push(res, UINT2NUM(index));
    rb_ary_push(res, new_query_match(&match, tree));
    return res;
  } else {
    return Qnil;
  }
}

/**
 * Advance to the next match of the currently running query.
 *
 * @return [Match|nil] The match, or nil if exhausted.
 */
static VALUE query_cursor_next_match(VALUE self) {
  TSQueryMatch match;
  if (ts_query_cursor_next_match(SELF, &match)) {
    return new_query_match(&match, unwrap(self)->tree);
  } else {
    return Qnil;
  }
}

static VALUE query_cursor_remove_match(VALUE self, VALUE id) {
  ts_query_cursor_remove_match(SELF, NUM2UINT(id));
  return Qnil;
}

/**
 * @param from [Integer]
 * @param to   [Integer]
 *
 * @return [Boolean]
 */
static VALUE query_cursor_set_byte_range(VALUE self, VALUE from, VALUE to) {
  return ts_query_cursor_set_byte_range(SELF, NUM2UINT(from), NUM2UINT(to))
             ? Qtrue
             : Qfalse;
}

/**
 * @param from [Point]
 * @param to   [Point]
 *
 * @return [Boolean]
 */
static VALUE query_cursor_set_point_range(VALUE self, VALUE from, VALUE to) {
  return ts_query_cursor_set_point_range(SELF, value_to_point(from),
                                         value_to_point(to))
             ? Qtrue
             : Qfalse;
}

/**
 * Set the byte range within which all matches must be fully contained.
 *
 * In contrast to {#set_byte_range}, this restricts the query cursor to only
 * return matches where <em>all</em> nodes are <em>fully</em> contained within
 * the given range. Both functions can be used together.
 *
 * @param from [Integer]
 * @param to   [Integer]
 *
 * @return [Boolean]
 */
static VALUE query_cursor_set_containing_byte_range(VALUE self, VALUE from,
                                                    VALUE to) {
  return ts_query_cursor_set_containing_byte_range(SELF, NUM2UINT(from),
                                                   NUM2UINT(to))
             ? Qtrue
             : Qfalse;
}

/**
 * Set the point range within which all matches must be fully contained.
 *
 * In contrast to {#set_point_range}, this restricts the query cursor to only
 * return matches where <em>all</em> nodes are <em>fully</em> contained within
 * the given range. Both functions can be used together.
 *
 * @param from [Point]
 * @param to   [Point]
 *
 * @return [Boolean]
 */
static VALUE query_cursor_set_containing_point_range(VALUE self, VALUE from,
                                                     VALUE to) {
  return ts_query_cursor_set_containing_point_range(SELF, value_to_point(from),
                                                    value_to_point(to))
             ? Qtrue
             : Qfalse;
}

/**
 * Start running a given query on a given node, with options.
 *
 * The options carry a progress callback ({QueryCursorOptions#initialize}).
 * The callback receives a {QueryCursorState} and can return +true+ to
 * cancel the query early.
 *
 * @example with an explicit options object
 *   opts = TreeSitter::QueryCursorOptions.new(->(state) {
 *     state.current_byte_offset > 10_000
 *   })
 *   cursor.exec_with_options(query, node, opts)
 *
 * @param query   [Query]
 * @param node    [Node]
 * @param options [QueryCursorOptions]
 *
 * @return [QueryCursor]
 */
static VALUE query_cursor_exec_with_options(VALUE self, VALUE query, VALUE node,
                                            VALUE options) {
  query_cursor_t *cursor = unwrap(self);
  ts_query_cursor_exec_with_options(cursor->data, value_to_query(query),
                                    value_to_node(node),
                                    value_to_query_cursor_options(options));
  cursor->tree = node_tree(node);
  return self;
}

void init_query_cursor(void) {
  cQueryCursor = rb_define_class_under(mTreeSitter, "QueryCursor", rb_cObject);

  rb_define_alloc_func(cQueryCursor, query_cursor_allocate);

  // Class methods
  rb_define_singleton_method(cQueryCursor, "exec", query_cursor_exec_static, 2);

  // Accessors
  DECLARE_ACCESSOR(cQueryCursor, query_cursor, match_limit)

  // Other
  rb_define_method(cQueryCursor, "exec", query_cursor_exec, 2);
  rb_define_private_method(cQueryCursor, "_exec_with_options",
                           query_cursor_exec_with_options, 3);
  rb_define_method(cQueryCursor, "exceed_match_limit?",
                   query_cursor_did_exceed_match_limit, 0);
  rb_define_method(cQueryCursor,
                   "max_start_depth=", query_cursor_set_max_start_depth, 1);
  rb_define_method(cQueryCursor, "next_capture", query_cursor_next_capture, 0);
  rb_define_method(cQueryCursor, "next_match", query_cursor_next_match, 0);
  rb_define_method(cQueryCursor, "remove_match", query_cursor_remove_match, 1);
  rb_define_method(cQueryCursor, "set_byte_range", query_cursor_set_byte_range,
                   2);
  rb_define_method(cQueryCursor, "set_containing_byte_range",
                   query_cursor_set_containing_byte_range, 2);
  rb_define_method(cQueryCursor, "set_containing_point_range",
                   query_cursor_set_containing_point_range, 2);
  rb_define_method(cQueryCursor, "set_point_range",
                   query_cursor_set_point_range, 2);
}
