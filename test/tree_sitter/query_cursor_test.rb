# frozen_string_literal: true

require_relative '../test_helper'

ruby = TreeSitter.lang('ruby')
parser = TreeSitter::Parser.new
parser.language = ruby

program = <<~RUBY
  def mul(a, b)
    res = a * b
    puts res.inspect
    return res
  end
RUBY

tree = parser.parse_string(nil, program)
root = tree.root_node

query = TreeSitter::Query.new(ruby, '(identifier) @id')

# --- helpers for progress-callback tests -----------------------------------
# Use a large program + wildcard pattern so the callback (checked every
# 100 internal operations) is guaranteed to fire at least once during
# full iteration.
BIG_PROGRAM = (1..400).map { |i| "x#{i}" }.join(' + ')
BIG_QUERY_STR = '(_) @all'

describe 'QueryCursor' do
  it 'must not emit redefined warnings when loading the extension' do
    lib_path = File.expand_path('../../lib', __dir__)
    output = IO.popen(
      [RbConfig.ruby, '-W', '-I', lib_path, '-e', "require 'tree_sitter'"],
      err: %i[child out],
      &:read
    )
    refute_match(/redefined/, output,
                 "Loading tree_sitter should not produce 'redefined' warnings:\n#{output}")
  end

  describe 'set_byte_range' do
    it 'returns true when start <= end' do
      cursor = TreeSitter::QueryCursor.exec(query, root)
      assert cursor.set_byte_range(0, 10)
    end

    it 'returns false when start > end' do
      cursor = TreeSitter::QueryCursor.exec(query, root)
      refute cursor.set_byte_range(10, 5)
    end
  end

  describe 'set_point_range' do
    it 'returns true when start <= end' do
      cursor = TreeSitter::QueryCursor.exec(query, root)
      start_pt = TreeSitter::Point.new.tap { |p|
        p.row = 0
        p.column = 0
      }
      end_pt = TreeSitter::Point.new.tap { |p|
        p.row = 0
        p.column = 5
      }
      assert cursor.set_point_range(start_pt, end_pt)
    end

    it 'returns false when start > end' do
      cursor = TreeSitter::QueryCursor.exec(query, root)
      start_pt = TreeSitter::Point.new.tap { |p|
        p.row = 5
        p.column = 0
      }
      end_pt = TreeSitter::Point.new.tap { |p|
        p.row = 3
        p.column = 0
      }
      refute cursor.set_point_range(start_pt, end_pt)
    end
  end

  describe 'set_containing_byte_range' do
    it 'returns true when start <= end' do
      cursor = TreeSitter::QueryCursor.exec(query, root)
      assert cursor.set_containing_byte_range(0, 10)
    end

    it 'returns false when start > end' do
      cursor = TreeSitter::QueryCursor.exec(query, root)
      refute cursor.set_containing_byte_range(10, 5)
    end
  end

  describe 'set_containing_point_range' do
    it 'returns true when start <= end' do
      cursor = TreeSitter::QueryCursor.exec(query, root)
      start_pt = TreeSitter::Point.new.tap { |p|
        p.row = 0
        p.column = 0
      }
      end_pt = TreeSitter::Point.new.tap { |p|
        p.row = 0
        p.column = 5
      }
      assert cursor.set_containing_point_range(start_pt, end_pt)
    end

    it 'returns false when start > end' do
      cursor = TreeSitter::QueryCursor.exec(query, root)
      start_pt = TreeSitter::Point.new.tap { |p|
        p.row = 5
        p.column = 0
      }
      end_pt = TreeSitter::Point.new.tap { |p|
        p.row = 3
        p.column = 0
      }
      refute cursor.set_containing_point_range(start_pt, end_pt)
    end
  end

  describe 'matches_with_options' do
    let(:big_query) { TreeSitter::Query.new(ruby, BIG_QUERY_STR) }
    let(:big_root) do
      parser.parse_string(nil, BIG_PROGRAM).root_node
    end
    let(:big_src) { BIG_PROGRAM }

    def count(cursor)
      c = 0
      c += 1 while cursor.next_match
      c
    end

    it 'calls a lambda callback during iteration' do
      called = false
      opts = TreeSitter::QueryCursorOptions.new(->(_s) {
        called = true
        false
      })
      cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      matches = cursor.matches_with_options(big_query, big_root, big_src, opts)
      matches.each { |_| } # fully iterate
      assert called, 'the callback should have been invoked'
    end

    it 'calls a Proc callback during iteration' do
      called = false
      opts = TreeSitter::QueryCursorOptions.new(proc { |_s|
        called = true
        false
      })
      cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      matches = cursor.matches_with_options(big_query, big_root, big_src, opts)
      matches.to_a
      assert called
    end

    it 'calls a Method callback during iteration' do
      handler = Class.new do
        attr_reader :called

        def call(state)
          @called = true
          false
        end
      end.new
      opts = TreeSitter::QueryCursorOptions.new(handler.method(:call))
      cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      cursor.matches_with_options(big_query, big_root, big_src, opts).to_a
      assert handler.called
    end

    it 'accepts a block' do
      called = false
      cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      cursor.matches_with_options(big_query, big_root, big_src) do |_s|
        called = true
        false
      end.to_a
      assert called
    end

    it 'cancels the query when the callback returns true' do
      # Without cancellation
      full_cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      full_count = count(full_cursor)
      assert full_count.positive?

      # With cancellation
      opts = TreeSitter::QueryCursorOptions.new(->(_s) { true })
      cancel_cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      cancel_cursor.send(:_exec_with_options, big_query, big_root, opts)
      cancel_count = count(cancel_cursor)

      assert cancel_count < full_count,
             "expected cancellation to reduce matches (got #{cancel_count}, full=#{full_count})"
    end

    it 'raises ArgumentError when neither options nor block is given' do
      cursor = TreeSitter::QueryCursor.exec(query, root)
      assert_raises(ArgumentError) do
        cursor.matches_with_options(query, root, program)
      end
    end

    it 'prefers an explicit options object over a block' do
      block_called = false
      opts_called = false
      opts = TreeSitter::QueryCursorOptions.new(->(_s) {
        opts_called = true
        false
      })
      cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      cursor.matches_with_options(big_query, big_root, big_src, opts) { |state|
        block_called = true
        false
      }.to_a
      assert opts_called, 'the explicit options callback should be called'
      refute block_called, 'the block should be ignored when options is given'
    end

    it 'keeps the options alive during iteration (GC safety)' do
      called = false
      cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      matches = cursor.matches_with_options(big_query, big_root, big_src) do |state|
        called = true
        false
      end
      # Force GC to verify the callback proc wasn't collected.
      GC.start
      matches.to_a
      assert called
    end
  end

  describe 'captures_with_options' do
    let(:big_query) { TreeSitter::Query.new(ruby, BIG_QUERY_STR) }
    let(:big_root) do
      parser.parse_string(nil, BIG_PROGRAM).root_node
    end
    let(:big_src) { BIG_PROGRAM }

    def capture_count(cursor)
      c = 0
      c += 1 while cursor.next_capture
      c
    end

    it 'calls the callback during iteration' do
      called = false
      opts = TreeSitter::QueryCursorOptions.new(->(_s) {
        called = true
        false
      })
      cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      cursor.captures_with_options(big_query, big_root, big_src, opts).to_a
      assert called
    end

    it 'cancels when the callback returns true' do
      full_count = capture_count(TreeSitter::QueryCursor.exec(big_query, big_root))
      assert full_count.positive?

      opts = TreeSitter::QueryCursorOptions.new(->(_s) { true })
      cancel_cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      cancel_cursor.send(:_exec_with_options, big_query, big_root, opts)
      cancel_count = capture_count(cancel_cursor)

      assert cancel_count < full_count,
             "expected cancellation to reduce captures (got #{cancel_count}, full=#{full_count})"
    end
  end

  describe 'QueryCursorState' do
    let(:big_query) { TreeSitter::Query.new(ruby, BIG_QUERY_STR) }
    let(:big_root) do
      parser.parse_string(nil, BIG_PROGRAM).root_node
    end
    let(:big_src) { BIG_PROGRAM }

    it 'exposes current_byte_offset via the callback' do
      states = []
      opts = TreeSitter::QueryCursorOptions.new(->(s) {
        states << s
        false
      })
      cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      cursor.matches_with_options(big_query, big_root, big_src, opts).to_a
      refute_empty states
      assert(states.all? { |s| s.is_a?(TreeSitter::QueryCursorState) })
      assert(states.all? { |s| s.current_byte_offset.is_a?(Integer) && s.current_byte_offset >= 0 })
    end

    it 'has a readable inspect' do
      state = nil
      opts = TreeSitter::QueryCursorOptions.new(->(s) {
        state = s
        false
      })
      cursor = TreeSitter::QueryCursor.exec(big_query, big_root)
      cursor.matches_with_options(big_query, big_root, big_src, opts).to_a
      refute_nil state
      assert_match(/current_byte_offset/, state.inspect)
    end
  end

  describe 'QueryCursorOptions' do
    it 'initializes with a callable' do
      opts = TreeSitter::QueryCursorOptions.new(->(_s) { true })
      assert_instance_of TreeSitter::QueryCursorOptions, opts
    end

    it 'creates an object even without a callback' do
      opts = TreeSitter::QueryCursorOptions.new
      assert_instance_of TreeSitter::QueryCursorOptions, opts
    end
  end
end
