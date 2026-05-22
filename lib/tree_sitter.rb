# frozen_string_literal: true

begin
  RUBY_VERSION =~ /(\d+\.\d+)/
  require "tree_sitter/#{Regexp.last_match(1)}/tree_sitter"
rescue LoadError
  require 'tree_sitter/tree_sitter'
end

require 'tree_sitter/version'

require 'tree_sitter/mixins/language'

require 'tree_sitter/error'
require 'tree_sitter/node'
require 'tree_sitter/query'
require 'tree_sitter/query_captures'
require 'tree_sitter/query_cursor'
require 'tree_sitter/query_match'
require 'tree_sitter/query_matches'
require 'tree_sitter/query_predicate'
require 'tree_sitter/text_predicate_capture'

require 'oppen'

# TreeSitter is a Ruby interface to the tree-sitter parsing library.
module TreeSitter
  extend Mixins::Language

  class << self
    alias_method :lang, :language

    # When +true+ (the default), {Node#[]}, {Node#fetch},
    # {Node#method_missing}, and {Node#respond_to_missing?} use the
    # named-child field API, which is more reliable but does not see
    # fields attached to anonymous children (e.g. +binary.operator+
    # pointing to +"*"+).
    #
    # Set to +false+ to restore the pre-3.0 behaviour that uses the
    # all-child field API for these four methods.
    #
    # This flag does *not* affect {Node#field} or {Node#field?}.
    # those accept an +anon:+ keyword argument for per-call control.
    #
    # @return [Boolean]
    attr_accessor :strict_field_access
  end

  self.strict_field_access = true
end
