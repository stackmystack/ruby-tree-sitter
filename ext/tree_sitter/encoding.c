#include "tree_sitter.h"

extern VALUE mTreeSitter;

VALUE mEncoding;

const char *utf8_s = "utf8";
const char *utf16le_s = "utf16le";
const char *utf16be_s = "utf16be";
const char *custom_s = "custom";

TSInputEncoding value_to_encoding(VALUE encoding) {
  VALUE enc = SYM2ID(encoding);
  VALUE utf16le = SYM2ID(rb_const_get_at(mEncoding, rb_intern("UTF16LE")));
  VALUE utf16be = SYM2ID(rb_const_get_at(mEncoding, rb_intern("UTF16BE")));
  VALUE custom = SYM2ID(rb_const_get_at(mEncoding, rb_intern("CUSTOM")));

  if (enc == utf16le) {
    return TSInputEncodingUTF16LE;
  } else if (enc == utf16be) {
    return TSInputEncodingUTF16BE;
  } else if (enc == custom) {
    return TSInputEncodingCustom;
  } else {
    return TSInputEncodingUTF8;
  }
}

void init_encoding(void) {
  mEncoding = rb_define_module_under(mTreeSitter, "Encoding");

  /* Constants */
  rb_define_const(mEncoding, "UTF8", ID2SYM(rb_intern(utf8_s)));
  rb_define_const(mEncoding, "UTF16LE", ID2SYM(rb_intern(utf16le_s)));
  rb_define_const(mEncoding, "UTF16BE", ID2SYM(rb_intern(utf16be_s)));
  rb_define_const(mEncoding, "CUSTOM", ID2SYM(rb_intern(custom_s)));
}
