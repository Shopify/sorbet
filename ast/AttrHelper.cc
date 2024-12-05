#include "ast/AttrHelper.h"
#include "absl/strings/escaping.h"
#include "core/errors/internal.h"

using namespace std;

namespace sorbet::ast {
pair<core::NameRef, core::LocOffsets> AttrHelper::getName(core::MutableContext ctx, ast::ExpressionPtr &name) {
    core::LocOffsets loc;
    core::NameRef res;
    if (auto *lit = ast::cast_tree<ast::Literal>(name)) {
        if (lit->isSymbol()) {
            res = lit->asSymbol();
            loc = lit->loc;
            ENFORCE(ctx.locAt(loc).exists());
            ENFORCE(ctx.locAt(loc).source(ctx).value().size() > 1 && ctx.locAt(loc).source(ctx).value()[0] == ':');
            loc = core::LocOffsets{loc.beginPos() + 1, loc.endPos()};
        } else if (lit->isString()) {
            core::NameRef nameRef = lit->asString();
            auto shortName = nameRef.shortName(ctx);
            bool validAttr = (isalpha(shortName.front()) || shortName.front() == '_') &&
                             absl::c_all_of(shortName, [](char c) { return isalnum(c) || c == '_'; });
            if (validAttr) {
                res = nameRef;
            } else {
                if (auto e = ctx.beginIndexerError(name.loc(), core::errors::Internal::InternalError)) {
                    e.setHeader("Bad attribute name \"{}\"", absl::CEscape(shortName));
                }
                res = core::Names::empty();
            }
            loc = lit->loc;
            DEBUG_ONLY({
                auto l = ctx.locAt(loc);
                ENFORCE(l.exists());
                auto source = l.source(ctx).value();
                ENFORCE(source.size() > 2);
                ENFORCE(source[0] == '"' || source[0] == '\'');
                auto lastChar = source[source.size() - 1];
                ENFORCE(lastChar == '"' || lastChar == '\'');
            });
            loc = core::LocOffsets{loc.beginPos() + 1, loc.endPos() - 1};
        }
    }
    if (!res.exists()) {
        if (auto e = ctx.beginIndexerError(name.loc(), core::errors::Internal::InternalError)) {
            e.setHeader("arg must be a Symbol or String");
        }
    }
    return make_pair(res, loc);
}
} // namespace sorbet::ast
