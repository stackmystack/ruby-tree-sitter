# frozen_string_literal: true

module TreeSitter
  # A Cursor for {Query}.
  class QueryCursor
    # Iterate over all of the matches in the order that they were found.
    #
    # Each match contains the index of the pattern that matched, and a list of
    # captures. Because multiple patterns can match the same set of nodes,
    # one match may contain captures that appear *before* some of the
    # captures from a previous match.
    def matches(query, node, src)
      self.exec(query, node)
      QueryMatches.new(self, query, src)
    end

    # Iterate over all of the individual captures in the order that they
    # appear.
    #
    # This is useful if you don't care about which pattern matched, and just
    # want a single, ordered sequence of captures.
    def captures(query, node, src)
      self.exec(query, node)
      QueryCaptures.new(self, query, src)
    end

    # Iterate over matches with a progress callback that can cancel the query.
    #
    # Accepts either an explicit {QueryCursorOptions} object or a block.
    # Any Ruby object that responds to +#call+ works: Proc, lambda, Method,
    # or custom callable.
    #
    # @example with an explicit options object
    #   opts = TreeSitter::QueryCursorOptions.new(->(state) { ... })
    #   cursor.matches_with_options(query, node, src, opts).each do |match|
    #     ...
    #   end
    #
    # @example with a block
    #   cursor.matches_with_options(query, node, src) do |state|
    #     state.current_byte_offset < 10_000
    #   end.each do |match|
    #     ...
    #   end
    #
    # @param query   [Query]
    # @param node    [Node]
    # @param src     [String] source document
    # @param options [QueryCursorOptions, nil]
    # @yieldparam state [QueryCursorState]
    # @yieldreturn [Boolean] +true+ to cancel
    # @return [QueryMatches]
    def matches_with_options(query, node, src, options = nil, &block)
      opts = options || QueryCursorOptions.new(block || raise(ArgumentError, 'callback required'))
      _exec_with_options(query, node, opts)
      QueryMatches.new(self, query, src, opts)
    end

    # Iterate over captures with a progress callback that can cancel the query.
    #
    # @see matches_with_options
    #
    # @param query   [Query]
    # @param node    [Node]
    # @param src     [String] source document
    # @param options [QueryCursorOptions, nil]
    # @yieldparam state [QueryCursorState]
    # @yieldreturn [Boolean] +true+ to cancel
    # @return [QueryCaptures]
    def captures_with_options(query, node, src, options = nil, &block)
      opts = options || QueryCursorOptions.new(block || raise(ArgumentError, 'callback required'))
      _exec_with_options(query, node, opts)
      QueryCaptures.new(self, query, src, opts)
    end
  end
end
