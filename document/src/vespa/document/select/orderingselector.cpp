// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include "orderingselector.h"

#include <algorithm>
#include <vespa/document/base/documentid.h>
#include <vespa/document/base/idstring.h>
#include "node.h"
#include "valuenode.h"
#include "visitor.h"
#include "compare.h"
#include "branch.h"

namespace document {

using namespace document::select;

namespace {
    /**
     * Visitor class that is used for visiting a node tree generated by a
     * document selection expression.
     *
     * The visitor class contains the set of buckets expression can match.
     */
    struct OrderingVisitor : public document::select::Visitor
    {
        OrderingSpecification::UP _spec;
        OrderingSpecification::Order _order;

        OrderingVisitor(OrderingSpecification::Order order)
            : _order(order) {
        }

        OrderingSpecification::UP pickOrdering(const OrderingSpecification& a, const OrderingSpecification& b, bool isAnd) {
            if (a.getWidthBits() == b.getWidthBits() && a.getDivisionBits() == b.getDivisionBits() && a.getOrder() == b.getOrder()) {
                if ((a.getOrder() == OrderingSpecification::ASCENDING && isAnd) ||
                (a.getOrder() == OrderingSpecification::DESCENDING && !isAnd)) {
                    return OrderingSpecification::UP(
                            new OrderingSpecification(a.getOrder(), std::max(a.getOrderingStart(), b.getOrderingStart()), b.getWidthBits(), a.getDivisionBits()));
                } else {
                    return OrderingSpecification::UP(
                            new OrderingSpecification(a.getOrder(), std::min(a.getOrderingStart(), b.getOrderingStart()), b.getWidthBits(), a.getDivisionBits()));
                }
            }
            return OrderingSpecification::UP();
        }

        void visitAndBranch(const document::select::And& node) {
            OrderingVisitor left(_order);
            node.getLeft().visit(left);
            node.getRight().visit(*this);

            if (left._spec.get() == NULL) {
                return;
            }

            // If only left part is known return that part.
            if (_spec.get() == NULL) {
                _spec = std::move(left._spec);
                return;
            }

            // Both are set...
            _spec = pickOrdering(*_spec, *left._spec, true);
        }

        void visitOrBranch(const document::select::Or& node) {
            OrderingVisitor left(_order);
            node.getLeft().visit(left);
            node.getRight().visit(*this);

                // If one part is unknown we have to keep unknown status
            if (left._spec.get() == NULL || _spec.get() == NULL) {
                return;
            }

            _spec = pickOrdering(*_spec, *left._spec, false);
        }

        void visitNotBranch(const document::select::Not&) {
        }

        void compare(const select::IdValueNode& node,
                     const select::ValueNode& valnode,
                     const select::Operator& op, OrderingSpecification::Order order)
        {
            if (node.getType() != IdValueNode::ORDER) {
                return;
            }

            if (node.getWidthBits() == -1 || node.getDivisionBits() == -1) {
                return;
            }

            const IntegerValueNode* val(
                    dynamic_cast<const IntegerValueNode*>(&valnode));
            if (!val) return;

            if (op == document::select::FunctionOperator::EQ) {
                _spec.reset(new OrderingSpecification(order, val->getValue(), node.getWidthBits(), node.getDivisionBits()));
            }

            if (order == OrderingSpecification::ASCENDING) {
                if (op == document::select::FunctionOperator::LEQ) {
                    _spec.reset(new OrderingSpecification(order, 0, node.getWidthBits(), node.getDivisionBits()));
                }
                if (op == document::select::FunctionOperator::GT) {
                    _spec.reset(new OrderingSpecification(order, val->getValue() + 1, node.getWidthBits(), node.getDivisionBits()));
                }
                if (op == document::select::FunctionOperator::GEQ) {
                    _spec.reset(new OrderingSpecification(order, val->getValue(), node.getWidthBits(), node.getDivisionBits()));
                }
            } else {
                if (op == document::select::FunctionOperator::LT) {
                    _spec.reset(new OrderingSpecification(order, val->getValue() - 1, node.getWidthBits(), node.getDivisionBits()));
                }
                if (op == document::select::FunctionOperator::LEQ) {
                    _spec.reset(new OrderingSpecification(order, val->getValue(), node.getWidthBits(), node.getDivisionBits()));
                }
            }
        }

        void visitComparison(const document::select::Compare& node) {
            const IdValueNode* lid(dynamic_cast<const IdValueNode*>(
                        &node.getLeft()));
            if (lid) {
                compare(*lid, node.getRight(), node.getOperator(), _order);
            } else {
                const IdValueNode* rid(dynamic_cast<const IdValueNode*>(
                            &node.getRight()));
                if (rid) {
                    compare(*rid, node.getLeft(), node.getOperator(), _order);
                }
            }
        }

        void visitConstant(const document::select::Constant&) {
        }

        virtual void
        visitInvalidConstant(const document::select::InvalidConstant &)
        {
        }

        void visitDocumentType(const document::select::DocType&) {
        }

        virtual void
        visitArithmeticValueNode(const ArithmeticValueNode &)
        {
        }

        virtual void
        visitFunctionValueNode(const FunctionValueNode &)
        {
        }

        virtual void
        visitIdValueNode(const IdValueNode &)
        {
        }

        virtual void
        visitSearchColumnValueNode(const SearchColumnValueNode &)
        {
        }

        virtual void
        visitFieldValueNode(const FieldValueNode &)
        {
        }

        virtual void
        visitFloatValueNode(const FloatValueNode &)
        {
        }

        virtual void
        visitVariableValueNode(const VariableValueNode &)
        {
        }

        virtual void
        visitIntegerValueNode(const IntegerValueNode &)
        {
        }

        virtual void
        visitCurrentTimeValueNode(const CurrentTimeValueNode &)
        {
        }

        virtual void
        visitStringValueNode(const StringValueNode &)
        {
        }

        virtual void
        visitNullValueNode(const NullValueNode &)
        {
        }

        virtual void
        visitInvalidValueNode(const InvalidValueNode &)
        {
        }
    };
}

OrderingSpecification::UP
OrderingSelector::select(const document::select::Node& expression, OrderingSpecification::Order order) const
{
    OrderingVisitor v(order);
    expression.visit(v);
    return std::move(v._spec);
}

} // document
