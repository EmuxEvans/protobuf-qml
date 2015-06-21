#include "protobuf/qml/message_generator.h"
#include "protobuf/qml/util.h"

namespace protobuf {
namespace qml {

using namespace google::protobuf;

MessageGenerator::MessageGenerator(const Descriptor* t)
    : t_(t), name_(t ? t->name() : "") {
  if (!t) {
    throw std::invalid_argument("Null descriptor");
  }

  message_generators_.reserve(t_->nested_type_count());
  for (int i = 0; i < t_->nested_type_count(); i++) {
    message_generators_.emplace_back(t_->nested_type(i));
  }

  enum_generators_.reserve(t_->enum_type_count());
  for (int i = 0; i < t_->enum_type_count(); i++) {
    enum_generators_.emplace_back(t_->enum_type(i));
  }

  field_generators_.reserve(t_->field_count());
  for (int i = 0; i < t_->field_count(); ++i) {
    field_generators_.emplace_back(t_->field(i));
  }
}

void MessageGenerator::generateMessageConstructor(io::Printer& p) {
  // For some reason, oneof_decl_count sometimes reports wrong value in the
  // constructor. So we initialize it here as a dirty work around.
  if (oneof_generators_.empty() && t_->oneof_decl_count() > 0) {
    oneof_generators_.reserve(t_->oneof_decl_count());
    for (int i = 0; i < t_->oneof_decl_count(); ++i) {
      oneof_generators_.emplace_back(t_->oneof_decl(i));
    }
  }

  p.Print(
      "var $message_name$ = (function() {\n"
      "  var FIELD = 0;\n"
      "  var ONEOF = 1;\n"
      "  var type = function(values) {\n"
      "    this._raw = [new Array($field_count$), new "
      "Array($oneof_count$)];\n\n"
      "    this._mergeFromRawArray = function(rawArray) {\n"
      "      if (rawArray && rawArray instanceof Array) {\n",
      "message_name", name_, "field_count", std::to_string(t_->field_count()),
      "oneof_count", std::to_string(t_->oneof_decl_count()));
  for (auto& g : field_generators_) {
    if (!g.is_oneof()) {
      g.generateMerge(p, "rawArray");
    }
  }
  for (auto& g : oneof_generators_) {
    g.generateMerge(p, "rawArray");
  }
  p.Print(
      "      };\n"
      "    };\n\n");
  for (auto& g : field_generators_) {
    g.generateInit(p);
  }
  for (auto& g : oneof_generators_) {
    g.generateInit(p);
  }

  p.Print(
      "    Object.seal(this);\n"
      "    if (values instanceof $message_name$) {\n"
      "      this._mergeFromRawArray(values._raw);\n"
      "    } else if (values instanceof Array) {\n"
      "      this._mergeFromRawArray(values);\n"
      "    } else {\n"
      "      for (var k in values) {\n"
      "        if (!(this[k] instanceof Function)) {\n"
      "          // Without this, we end up with a cryptic error message.\n"
      "          throw new Error(k + ' is not a member of $message_name$');\n"
      "        }\n"
      "        this[k](values[k]);\n"
      "      }\n"
      "    }\n"
      "  };\n\n",
      "message_name", name_, "message_index", std::to_string(t_->index()));

  for (auto& g : enum_generators_) {
    g.generateEnum(p);
    g.generateNestedAlias(p);
  }
  for (auto& g : message_generators_) {
    g.generateMessage(p);
    g.generateNestedAlias(p);
  }

  p.Print(
      "  Protobuf.Message.createMessageType(type, "
      "_file.descriptor.messageType($message_index$));\n\n",
      "message_name", name_, "message_index", std::to_string(t_->index()));
  for (auto& g : oneof_generators_) {
    g.generate(p);
  }
  for (auto& g : field_generators_) {
    g.generateProperty(p);
  }
  p.Print(
      "  return type;\n"
      "})();\n\n");
}

void MessageGenerator::generateMessagePrototype(io::Printer& p) {
  p.Print(
      "Protobuf.Message.createMessageType($message_name$, "
      "_file.descriptor.messageType($message_index$));\n\n",
      "message_name", name_, "message_index", std::to_string(t_->index()));
}

void MessageGenerator::generateMessageProperties(io::Printer& p) {
}

void MessageGenerator::generateNestedAlias(google::protobuf::io::Printer& p) {
  p.Print("    type.$name$ = $name$;\n", "name", t_->name());
}
}
}
