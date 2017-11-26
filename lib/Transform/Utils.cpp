#include "enfield/Transform/Utils.h"
#include "enfield/Analysis/NodeVisitor.h"
#include "enfield/Analysis/Driver.h"
#include "enfield/Support/RTTI.h"
#include "enfield/Support/uRefCast.h"

#include <cassert>
#include <unordered_map>
#include <iterator>
#include <iostream>

using namespace efd;

// ==--------------- Intrinsic Gates ---------------==
static std::vector<NDGateSign::uRef> IntrinsicGates;
static const std::string IntrinsicGatesStr =
#define EFD_LIB(...) #__VA_ARGS__
#include "enfield/StdLib/intrinsic.inc"
#undef EFD_LIB
;

static void ProcessIntrinsicGates() {
    if (IntrinsicGates.empty()) {
        auto ast = ParseString(IntrinsicGatesStr, false);
        assert(instanceOf<NDStmtList>(ast.get()) &&
                "Intrinsic gates root node of wrong type.");

        for (auto& gate : *ast) {
            auto gateNode = uniqueCastForward<NDGateSign>(std::move(gate));
            assert(gateNode.get() != nullptr && "Statement is not a gate declaration.");
            IntrinsicGates.push_back(std::move(gateNode));
        }
    }
}

std::vector<NDGateSign::uRef> efd::GetIntrinsicGates() {
    ProcessIntrinsicGates();

    std::vector<NDGateSign::uRef> gates;
    for (auto& gate : IntrinsicGates)
        gates.push_back(uniqueCastForward<NDGateSign>(gate->clone()));

    return gates;
}

namespace efd {
    /// \brief Special node for swap calls.
    struct NDQOpSwap : public NDQOpGen {
        static const std::string IdStr;

        NDQOpSwap(Node::uRef lhs, Node::uRef rhs) :
            NDQOpGen(NDId::Create(IdStr), NDList::Create(), NDList::Create()) {
                auto qargs = getQArgs();
                qargs->addChild(std::move(lhs));
                qargs->addChild(std::move(rhs));
            }
    };

    /// \brief Special node for long cnot calls.
    struct NDQOpLongCX : public NDQOpGen {
        static const std::string IdStr;

        NDQOpLongCX(Node::uRef lhs, Node::uRef middle, Node::uRef rhs) :
            NDQOpGen(NDId::Create(IdStr), NDList::Create(), NDList::Create()) {
                auto qargs = getQArgs();
                qargs->addChild(std::move(lhs));
                qargs->addChild(std::move(middle));
                qargs->addChild(std::move(rhs));
            }
    };

    /// \brief Special node for reversal cnot calls.
    struct NDQOpRevCX : public NDQOpGen {
        static const std::string IdStr;

        NDQOpRevCX(Node::uRef lhs, Node::uRef rhs) :
            NDQOpGen(NDId::Create(IdStr), NDList::Create(), NDList::Create()) {
                auto qargs = getQArgs();
                qargs->addChild(std::move(lhs));
                qargs->addChild(std::move(rhs));
            }
    };
}

const std::string efd::NDQOpSwap::IdStr = "intrinsic_swap__";
const std::string efd::NDQOpLongCX::IdStr = "intrinsic_lcx__";
const std::string efd::NDQOpRevCX::IdStr = "intrinsic_rev_cx__";

NDQOp::uRef efd::CreateISwap(Node::uRef lhs, Node::uRef rhs) {
    auto node = new NDQOpSwap(std::move(lhs), std::move(rhs));
    return NDQOp::uRef(node);
}

NDQOp::uRef efd::CreateILongCX
(Node::uRef lhs, Node::uRef middle, Node::uRef rhs) {
    auto node = new NDQOpLongCX(std::move(lhs), std::move(middle), std::move(rhs));
    return NDQOp::uRef(node);
}

NDQOp::uRef efd::CreateIRevCX(Node::uRef lhs, Node::uRef rhs) {
    auto node = new NDQOpRevCX(std::move(lhs), std::move(rhs));
    return NDQOp::uRef(node);
}

// ==--------------- QModulefy ---------------==
namespace efd {
    class QModulefyVisitor : public NodeVisitor {
        public:
            QModule& mMod;

            NDGateDecl::Ref mCurGate;
            NDInclude::Ref mCurIncl;

            QModulefyVisitor(QModule& qmod)
                : mMod(qmod), mCurGate(nullptr), mCurIncl(nullptr) {}

            void visit(NDQasmVersion::Ref ref) override;
            void visit(NDInclude::Ref ref) override;
            void visit(NDRegDecl::Ref ref) override;
            void visit(NDGateDecl::Ref ref) override;
            void visit(NDOpaque::Ref ref) override;
            void visit(NDQOpMeasure::Ref ref) override;
            void visit(NDQOpReset::Ref ref) override;
            void visit(NDQOpU::Ref ref) override;
            void visit(NDQOpCX::Ref ref) override;
            void visit(NDQOpBarrier::Ref ref) override;
            void visit(NDQOpGen::Ref ref) override;
            void visit(NDIfStmt::Ref ref) override;
            void visit(NDStmtList::Ref ref) override;
    };
}

void efd::QModulefyVisitor::visit(NDQasmVersion::Ref ref) {
    if (mCurIncl != nullptr) {
        auto vNum = uniqueCastForward<NDReal>(ref->getVersion()->clone());
        mMod.setVersion(NDQasmVersion::Create(std::move(vNum), NDStmtList::Create()));
    }

    visitChildren(ref);
}

void efd::QModulefyVisitor::visit(NDInclude::Ref ref) {
    auto fileNode = uniqueCastForward<NDString>(ref->getFilename()->clone());
    mMod.insertInclude(NDInclude::Create
            (std::move(fileNode), uniqueCastBackward<Node>(NDStmtList::Create())));

    mCurIncl = ref;
    visitChildren(ref);
    mCurIncl = nullptr;
}

void efd::QModulefyVisitor::visit(NDRegDecl::Ref ref) {
    mMod.insertReg(uniqueCastForward<NDRegDecl>(ref->clone()));
}

void efd::QModulefyVisitor::visit(NDGateDecl::Ref ref) {
    auto clone = uniqueCastForward<NDGateSign>(ref->clone());

    if (mCurIncl != nullptr) {
        clone->setInInclude();
    }

    mMod.insertGate(std::move(clone));
}

void efd::QModulefyVisitor::visit(NDOpaque::Ref ref) {
    mMod.insertGate(uniqueCastForward<NDGateSign>(ref->clone()));
}


void efd::QModulefyVisitor::visit(NDQOpMeasure::Ref ref) {
    mMod.insertStatementLast(ref->clone());
}


void efd::QModulefyVisitor::visit(NDQOpReset::Ref ref) {
    mMod.insertStatementLast(ref->clone());
}


void efd::QModulefyVisitor::visit(NDQOpU::Ref ref) {
    mMod.insertStatementLast(ref->clone());
}


void efd::QModulefyVisitor::visit(NDQOpCX::Ref ref) {
    mMod.insertStatementLast(ref->clone());
}

void efd::QModulefyVisitor::visit(NDQOpBarrier::Ref ref) {
    mMod.insertStatementLast(ref->clone());
}

void efd::QModulefyVisitor::visit(NDQOpGen::Ref ref) {
    mMod.insertStatementLast(ref->clone());
}

void efd::QModulefyVisitor::visit(NDIfStmt::Ref ref) {
    mMod.insertStatementLast(ref->clone());
}

void efd::QModulefyVisitor::visit(NDStmtList::Ref ref) {
    visitChildren(ref);
}

void efd::ProcessAST(QModule::Ref qmod, Node::Ref root) {
    QModulefyVisitor visitor(*qmod);
    root->apply(&visitor);
}

// ==--------------- Inlining ---------------==
static void QArgsReplaceVisitorVisit(NodeVisitor* visitor, NDQOp::Ref ref);

typedef std::unordered_map<std::string, Node::Ref> VarMap;

namespace efd {
    class QArgsReplaceVisitor : public NodeVisitor {
        public:
            VarMap& varMap;

            QArgsReplaceVisitor(VarMap& varMap) : varMap(varMap) {}

            void substituteChildrem(Node::Ref ref);
            Node::uRef replaceChild(Node::Ref ref);

            void visit(NDQOpU::Ref ref) override;
            void visit(NDQOpCX::Ref ref) override;
            void visit(NDList::Ref ref) override;
            void visit(NDQOpBarrier::Ref ref) override;
            void visit(NDQOpGen::Ref ref) override;
            void visit(NDBinOp::Ref ref) override;
            void visit(NDUnaryOp::Ref ref) override;
            void visit(NDIfStmt::Ref ref) override;
    };
}

Node::uRef efd::QArgsReplaceVisitor::replaceChild(Node::Ref child) {
    std::string _id = child->toString();

    if (varMap.find(_id) != varMap.end()) {
        return varMap[_id]->clone();
    }

    return Node::uRef(nullptr);
}

void efd::QArgsReplaceVisitor::substituteChildrem(Node::Ref ref) {
    for (uint32_t i = 0, e = ref->getChildNumber(); i < e; ++i) {
        auto newChild = replaceChild(ref->getChild(i));
        if (newChild.get() != nullptr) {
            ref->setChild(i, std::move(newChild));
        }
    }
}

void efd::QArgsReplaceVisitor::visit(NDList::Ref ref) {
    visitChildren(ref);
    substituteChildrem(ref);
}

void efd::QArgsReplaceVisitor::visit(NDQOpU::Ref ref) {
    QArgsReplaceVisitorVisit(this, (NDQOp::Ref) ref);
}

void efd::QArgsReplaceVisitor::visit(NDQOpCX::Ref ref) {
    QArgsReplaceVisitorVisit(this, (NDQOp::Ref) ref);
}

void efd::QArgsReplaceVisitor::visit(NDQOpBarrier::Ref ref) {
    QArgsReplaceVisitorVisit(this, (NDQOp::Ref) ref);
}

void efd::QArgsReplaceVisitor::visit(NDQOpGen::Ref ref) {
    QArgsReplaceVisitorVisit(this, (NDQOp::Ref) ref);
}

void efd::QArgsReplaceVisitor::visit(NDBinOp::Ref ref) {
    ref->getLhs()->apply(this);
    ref->getRhs()->apply(this);
    substituteChildrem(ref);
}

void efd::QArgsReplaceVisitor::visit(NDUnaryOp::Ref ref) {
    visitChildren(ref);
    substituteChildrem(ref);
}

void efd::QArgsReplaceVisitor::visit(NDIfStmt::Ref ref) {
    // We only need to visit the qop.
    ref->getQOp()->apply(this);
}

void efd::InlineGate(QModule::Ref qmod, NDQOp::Ref qop) {
    std::string gateId = qop->getId()->getVal();
    
    auto gate = qmod->getQGate(gateId);
    assert (!gate->isOpaque() && "Trying to inline opaque gate.");

    NDGateDecl::Ref gateDecl = dynCast<NDGateDecl>(gate);
    assert(gateDecl != nullptr && "No gate with such id found.");

    VarMap varMap;

    Node::Ref gateQArgs = gateDecl->getQArgs();
    Node::Ref qopQArgs = qop->getQArgs();
    for (uint32_t i = 0, e = gateQArgs->getChildNumber(); i < e; ++i)
        varMap[gateQArgs->getChild(i)->toString()] = qopQArgs->getChild(i);
    
    Node::Ref gateArgs = gateDecl->getArgs();
    Node::Ref qopArgs = qop->getArgs();
    for (uint32_t i = 0, e = gateArgs->getChildNumber(); i < e; ++i)
        varMap[gateArgs->getChild(i)->toString()] = qopArgs->getChild(i);

    // ------ Replacing
    QArgsReplaceVisitor visitor(varMap);
    auto gop = uniqueCastForward<NDGOpList>(gateDecl->getGOpList()->clone());

    // 'stmt' is the node we are going to replace.
    Node::Ref stmt = nullptr;
    auto ifstmt = dynCast<NDIfStmt>(qop->getParent());
    if (ifstmt != nullptr) stmt = ifstmt;
    else stmt = qop;

    // Replace the arguments.
    std::vector<Node::uRef> inlinedNodes;
    for (auto& op : *gop) {
        auto qop = std::move(op);
        // If its parent is an NDIfStmt, we wrap the the operation into
        // a clone of the if.
        if (ifstmt != nullptr) {
            auto ifclone = uniqueCastForward<NDIfStmt>(ifstmt->clone());
            ifclone->setQOp(uniqueCastForward<NDQOp>(std::move(qop)));
            qop.reset(ifclone.release());
        }

        // The 'visitor' is applied only in the 'qop'.
        qop->apply(&visitor);
        inlinedNodes.push_back(std::move(qop));
    }

    qmod->replaceStatement(stmt, std::move(inlinedNodes));
}

static void QArgsReplaceVisitorVisit(NodeVisitor* visitor, NDQOp::Ref ref) {
    ref->getArgs()->apply(visitor);
    ref->getQArgs()->apply(visitor);
}
