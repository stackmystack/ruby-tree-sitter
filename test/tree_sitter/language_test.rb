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

# NOTE: one should be weary of testing with data structures that are owned by
# parsers.   They are not reliable and we should expect them to break when these
# parsers evolve.

ruby_path =
  if p = ENV.fetch('TREE_SITTER_PARSERS', nil)
    Pathname(p) / "libtree-sitter-ruby.#{TreeSitter.ext}"
  else
    PARSERS_INSTALL_PATH / "libtree-sitter-ruby.#{TreeSitter.ext}"
  end

describe 'language' do
  it 'must raise a TreeSitter::ParserNotFoundError when a parser is not found' do
    _ { TreeSitter.lang('rubyyyyyyyyyy') }.must_raise TreeSitter::ParserNotFoundError
    _ { TreeSitter::Language.load('rubyyyyyyyyyy', ruby_path) }.must_raise TreeSitter::SymbolNotFoundError
  end

  it 'must be able to load a library from `Pathname` (or any object that has `to_s`)' do
    _(TreeSitter::Language.load('ruby', ruby_path).field_count).must_be :positive?
  end

  it 'must throw an exception when the library is not found' do
    _ { TreeSitter::Language.load('ruby', Pathname('tmp/none')) }.must_raise TreeSitter::ParserNotFoundError
  end

  it 'must throw an exception when the name is not correctly found' do
    _ { TreeSitter::Language.load('nada', ruby_path) }.must_raise TreeSitter::SymbolNotFoundError
  end

  it 'must return state count' do
    assert ruby.state_count.positive?
  end

  it 'must return symbol count' do
    assert ruby.symbol_count.positive?
  end

  it 'must return symbol name' do
    assert_equal 'end', ruby.symbol_name(0)
  end

  it 'must return symbol id for string name' do
    assert ruby.symbol_for_name(root.type, root.named?).positive?
  end

  it 'must return field count' do
    assert ruby.field_count.positive?
  end

  it 'must return field name for id' do
    assert_equal 'alias', ruby.field_name_for_id(1)
  end

  it 'must return field name for id' do
    assert_equal 1, ruby.field_id_for_name('alias')
  end

  it 'must return field symbol type' do
    assert_equal TreeSitter::SymbolType::REGULAR, ruby.symbol_type(1)
  end

  it 'must return the language name' do
    name = ruby.name
    assert_instance_of String, name
    refute_empty name
    assert_equal 'ruby', name
  end

  it 'must be of correct version' do
    assert ruby.version.between?(TreeSitter::MIN_COMPATIBLE_LANGUAGE_VERSION, TreeSitter::LANGUAGE_VERSION)
  end

  describe 'supertypes' do
    it 'returns an Array of Symbol' do
      supers = ruby.supertypes
      assert_instance_of Array, supers
      refute_empty supers
      assert supers.all? { |s| s.is_a?(Symbol) }, 'all elements must be Symbols'
    end
  end

  describe 'subtypes' do
    it 'returns the known subtypes for the _simple_numeric supertype' do
      subs = ruby.subtypes(:_simple_numeric)
      assert_equal %i[complex float integer rational], subs.sort,
                   '_simple_numeric must have these four subtypes'
    end
  end

  describe 'metadata' do
    it 'returns a LanguageMetadata object' do
      meta = ruby.metadata
      assert_instance_of TreeSitter::LanguageMetadata, meta
    end

    it 'has integer version fields' do
      meta = ruby.metadata
      assert_instance_of Integer, meta.major_version
      assert_instance_of Integer, meta.minor_version
      assert_instance_of Integer, meta.patch_version
    end

    it 'returns a SemVer string from to_s' do
      meta = ruby.metadata
      assert_match(/\A\d+\.\d+\.\d+\z/, meta.to_s)
    end

    it 'allows setting version fields' do
      meta = TreeSitter::LanguageMetadata.new
      meta.major_version = 1
      meta.minor_version = 2
      meta.patch_version = 3
      assert_equal 1, meta.major_version
      assert_equal 2, meta.minor_version
      assert_equal 3, meta.patch_version
      assert_equal '1.2.3', meta.to_s
    end
  end
end
