#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE cLanguageMetadata;

DATA_WRAP(LanguageMetadata, language_metadata)
DATA_DEFINE_ACCESSOR(language_metadata, major_version, UINT2NUM, NUM2UINT)
DATA_DEFINE_ACCESSOR(language_metadata, minor_version, UINT2NUM, NUM2UINT)
DATA_DEFINE_ACCESSOR(language_metadata, patch_version, UINT2NUM, NUM2UINT)

/**
 * Create a TreeSitter::LanguageMetadata from a C struct.
 *
 * Called from language.c which has the TSLanguage* unwrapper.
 *
 * @api private
 */
VALUE language_metadata_new(const TSLanguageMetadata *ptr) {
  if (ptr == NULL) {
    return Qnil;
  }
  VALUE res = language_metadata_allocate(cLanguageMetadata);
  language_metadata_t *t = unwrap(res);
  t->data = *ptr;
  return res;
}

static VALUE language_metadata_to_s(VALUE self) {
  language_metadata_t *meta = unwrap(self);
  return rb_sprintf("%d.%d.%d", meta->data.major_version,
                    meta->data.minor_version, meta->data.patch_version);
}

void init_language_metadata(void) {
  cLanguageMetadata =
      rb_define_class_under(mTreeSitter, "LanguageMetadata", rb_cObject);

  rb_define_alloc_func(cLanguageMetadata, language_metadata_allocate);

  /* Class methods */
  DECLARE_ACCESSOR(cLanguageMetadata, language_metadata, major_version)
  DECLARE_ACCESSOR(cLanguageMetadata, language_metadata, minor_version)
  DECLARE_ACCESSOR(cLanguageMetadata, language_metadata, patch_version)
  rb_define_method(cLanguageMetadata, "to_s", language_metadata_to_s, 0);
  rb_define_method(cLanguageMetadata, "inspect", language_metadata_to_s, 0);
}
