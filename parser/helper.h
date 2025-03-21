#ifndef SORBET_PARSER_HELPER_H
#define SORBET_PARSER_HELPER_H

#include "parser/parser.h"

using namespace std;

namespace sorbet::parser {

class MK {
public:
    /* nodes */

    static unique_ptr<parser::Node> Array(core::LocOffsets loc, parser::NodeVec args) {
        return make_unique<parser::Array>(loc, move(args));
    }

    static unique_ptr<parser::Node> Cbase(core::LocOffsets loc) {
        return make_unique<parser::Cbase>(loc);
    }

    static unique_ptr<parser::Node> Const(core::LocOffsets loc, unique_ptr<parser::Node> parent, core::NameRef name) {
        return make_unique<parser::Const>(loc, move(parent), name);
    }

    static unique_ptr<parser::Node> Hash(core::LocOffsets loc, bool kwargs, parser::NodeVec pairs) {
        return make_unique<parser::Hash>(loc, kwargs, move(pairs));
    }

    static unique_ptr<parser::Node> Self(core::LocOffsets loc) {
        return make_unique<parser::Self>(loc);
    }

    static unique_ptr<parser::Node> Send(core::LocOffsets loc, unique_ptr<parser::Node> recv, core::NameRef name,
                                         core::LocOffsets nameLoc, parser::NodeVec args) {
        return make_unique<parser::Send>(loc, move(recv), name, nameLoc, move(args));
    }

    static unique_ptr<parser::Node> Send0(core::LocOffsets loc, unique_ptr<parser::Node> recv, core::NameRef name,
                                          core::LocOffsets nameLoc) {
        return make_unique<parser::Send>(loc, move(recv), name, nameLoc, parser::NodeVec());
    }

    static unique_ptr<parser::Node> Send1(core::LocOffsets loc, unique_ptr<parser::Node> recv, core::NameRef name,
                                          core::LocOffsets nameLoc, unique_ptr<parser::Node> arg) {
        auto args = parser::NodeVec();
        args.reserve(1);
        args.push_back(move(arg));
        return make_unique<parser::Send>(loc, move(recv), name, nameLoc, move(args));
    }

    static unique_ptr<parser::Node> Send2(core::LocOffsets loc, unique_ptr<parser::Node> recv, core::NameRef name,
                                          core::LocOffsets nameLoc, unique_ptr<parser::Node> arg1,
                                          unique_ptr<parser::Node> arg2) {
        auto args = parser::NodeVec();
        args.reserve(2);
        args.push_back(move(arg1));
        args.push_back(move(arg2));
        return make_unique<parser::Send>(loc, move(recv), name, nameLoc, move(args));
    }

    static unique_ptr<parser::Node> String(core::LocOffsets loc, core::NameRef name) {
        return make_unique<parser::String>(loc, name);
    }

    static unique_ptr<parser::Node> Symbol(core::LocOffsets loc, core::NameRef name) {
        return make_unique<parser::Symbol>(loc, name);
    }

    static unique_ptr<parser::Node> True(core::LocOffsets loc) {
        return make_unique<parser::True>(loc);
    }

    /* T Classes */

    static unique_ptr<parser::Node> T(core::LocOffsets loc) {
        return Const(loc, Cbase(loc), core::Names::Constants::T());
    }

    static unique_ptr<parser::Node> T_Array(core::LocOffsets loc) {
        return Const(loc, T(loc), core::Names::Constants::Array());
    }

    static unique_ptr<parser::Node> T_Boolean(core::LocOffsets loc) {
        return Const(loc, T(loc), core::Names::Constants::Boolean());
    }

    static unique_ptr<parser::Node> T_Class(core::LocOffsets loc) {
        return Const(loc, T(loc), core::Names::Constants::Class());
    }

    static unique_ptr<parser::Node> T_Enumerable(core::LocOffsets loc) {
        return Const(loc, T(loc), core::Names::Constants::Enumerable());
    }

    static unique_ptr<parser::Node> T_Enumerator(core::LocOffsets loc) {
        return Const(loc, T(loc), core::Names::Constants::Enumerator());
    }

    static unique_ptr<parser::Node> T_Enumerator_Lazy(core::LocOffsets loc) {
        return Const(loc, T_Enumerator(loc), core::Names::Constants::Lazy());
    }

    static unique_ptr<parser::Node> T_Enumerator_Chain(core::LocOffsets loc) {
        return Const(loc, T_Enumerator(loc), core::Names::Constants::Chain());
    }

    static unique_ptr<parser::Node> T_Hash(core::LocOffsets loc) {
        return Const(loc, T(loc), core::Names::Constants::Hash());
    }

    static unique_ptr<parser::Node> T_Set(core::LocOffsets loc) {
        return Const(loc, T(loc), core::Names::Constants::Set());
    }

    static unique_ptr<parser::Node> T_Range(core::LocOffsets loc) {
        return Const(loc, T(loc), core::Names::Constants::Range());
    }

    /* T Methods */

    static unique_ptr<parser::Node> TAll(core::LocOffsets loc, parser::NodeVec args) {
        return Send(loc, T(loc), core::Names::all(), loc, move(args));
    }

    static unique_ptr<parser::Node> TAny(core::LocOffsets loc, parser::NodeVec args) {
        return Send(loc, T(loc), core::Names::any(), loc, move(args));
    }

    static unique_ptr<parser::Node> TAnything(core::LocOffsets loc) {
        return Send0(loc, T(loc), core::Names::anything(), loc);
    }

    static unique_ptr<parser::Node> TAttachedClass(core::LocOffsets loc) {
        return Send0(loc, T(loc), core::Names::attachedClass(), loc);
    }

    static unique_ptr<parser::Node> TCast(core::LocOffsets loc, unique_ptr<parser::Node> value,
                                          unique_ptr<parser::Node> type) {
        return Send2(loc, T(loc), core::Names::cast(), loc, move(value), move(type));
    }

    static unique_ptr<parser::Node> TClassOf(core::LocOffsets loc, unique_ptr<parser::Node> inner) {
        return Send1(loc, T(loc), core::Names::classOf(), loc, move(inner));
    }

    static unique_ptr<parser::Node> TLet(core::LocOffsets loc, unique_ptr<parser::Node> value,
                                         unique_ptr<parser::Node> type) {
        return Send2(loc, T(loc), core::Names::let(), loc, move(value), move(type));
    }

    static unique_ptr<parser::Node> TMust(core::LocOffsets loc, unique_ptr<parser::Node> value) {
        return Send1(loc, T(loc), core::Names::must(), loc, move(value));
    }

    static unique_ptr<parser::Node> TNilable(core::LocOffsets loc, unique_ptr<parser::Node> inner) {
        return Send1(loc, T(loc), core::Names::nilable(), loc, move(inner));
    }

    static unique_ptr<parser::Node> TNoReturn(core::LocOffsets loc) {
        return Send0(loc, T(loc), core::Names::noreturn(), loc);
    }

    static unique_ptr<parser::Node> TProc(core::LocOffsets loc, unique_ptr<parser::Hash> args,
                                          unique_ptr<parser::Node> ret) {
        auto builder = T(loc);
        builder = Send0(loc, move(builder), core::Names::proc(), loc);
        if (args != nullptr && !args->pairs.empty()) {
            auto argsVec = parser::NodeVec();
            argsVec.reserve(1);
            argsVec.push_back(move(args));
            builder = Send(loc, move(builder), core::Names::params(), loc, move(argsVec));
        }
        builder = Send1(loc, move(builder), core::Names::returns(), loc, move(ret));
        return builder;
    }

    static unique_ptr<parser::Node> TProcVoid(core::LocOffsets loc, unique_ptr<parser::Hash> args) {
        auto builder = T(loc);
        builder = Send0(loc, move(builder), core::Names::proc(), loc);
        if (args != nullptr && !args->pairs.empty()) {
            auto argsVec = parser::NodeVec();
            argsVec.reserve(1);
            argsVec.push_back(move(args));
            builder = Send(loc, move(builder), core::Names::params(), loc, move(argsVec));
        }
        return Send0(loc, move(builder), core::Names::void_(), loc);
    }

    static unique_ptr<parser::Node> TSelfType(core::LocOffsets loc) {
        return Send0(loc, T(loc), core::Names::selfType(), loc);
    }

    static unique_ptr<parser::Node> TTypeParameter(core::LocOffsets loc, unique_ptr<parser::Node> name) {
        return Send1(loc, T(loc), core::Names::typeParameter(), loc, move(name));
    }

    static unique_ptr<parser::Node> TUntyped(core::LocOffsets loc) {
        return make_unique<parser::Send>(loc, T(loc), core::Names::untyped(), loc, parser::NodeVec());
    }

    /* helpers */

    static bool isT(const unique_ptr<parser::Node> &expr) {
        auto t = parser::cast_node<parser::Const>(expr.get());
        return t != nullptr && t->name == core::Names::Constants::T() && isa_node<parser::Cbase>(t->scope.get());
    }

    static bool isTUntyped(const unique_ptr<parser::Node> &expr) {
        auto send = parser::cast_node<parser::Send>(expr.get());
        return send != nullptr && send->method == core::Names::untyped() && isT(send->receiver);
    }
};

} // namespace sorbet::parser

#endif // SORBET_PARSER_HELPER_H
