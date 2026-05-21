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
end
