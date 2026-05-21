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

describe TreeSitter::Encoding do
  describe 'constants' do
    it 'defines UTF8' do
      assert_equal :utf8, TreeSitter::Encoding::UTF8
    end

    it 'defines UTF16LE' do
      assert_equal :utf16le, TreeSitter::Encoding::UTF16LE
    end

    it 'defines UTF16BE' do
      assert_equal :utf16be, TreeSitter::Encoding::UTF16BE
    end

    it 'defines CUSTOM' do
      assert_equal :custom, TreeSitter::Encoding::CUSTOM
    end
  end
end

describe 'parse_string_encoding' do
  before do
    parser.reset
  end

  it 'parses a UTF-8 string with :utf8 encoding' do
    tree = parser.parse_string_encoding(nil, program, TreeSitter::Encoding::UTF8)
    assert_instance_of TreeSitter::Tree, tree
    assert_equal 1, tree.root_node.child_count
  end

  it 'parses a UTF-16LE string with :utf16le encoding' do
    prog16 = program.encode('utf-16le')
    tree = parser.parse_string_encoding(nil, prog16, TreeSitter::Encoding::UTF16LE)
    assert_instance_of TreeSitter::Tree, tree
    assert_equal 1, tree.root_node.child_count
  end

  it 'parses a UTF-16BE string with :utf16be encoding' do
    prog16 = program.encode('utf-16be')
    tree = parser.parse_string_encoding(nil, prog16, TreeSitter::Encoding::UTF16BE)
    assert_instance_of TreeSitter::Tree, tree
    assert_equal 1, tree.root_node.child_count
  end

  it 'defaults to UTF-8 for unknown encoding symbols' do
    tree = parser.parse_string_encoding(nil, program, :unknown_encoding)
    assert_instance_of TreeSitter::Tree, tree
    assert_equal 1, tree.root_node.child_count
  end
end
