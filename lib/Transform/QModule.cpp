#include "enfield/Analysis/Driver.h"
#include "enfield/Transform/QModule.h"
#include "enfield/Transform/IdTable.h"
#include "enfield/Transform/Utils.h"
#include "enfield/Support/RTTI.h"
#include "enfield/Support/uRefCast.h"
#include "enfield/Pass.h"

#include <cassert>
#include <iterator>

namespace efd {
    extern NDId::uRef SWAP_ID_NODE;
    extern NDId::uRef H_ID_NODE;
    extern NDId::uRef CX_ID_NODE;
}

efd::QModule::QModule() : mVersion(nullptr) {
}

void efd::QModule::registerSwapGate() {
    bool isSwapRegistered = getQGate("__swap__") != nullptr;
    if (!isSwapRegistered) {
        // The quantum arguments that will be used
        auto qargLhs = NDId::Create("a");
        auto qargRhs = NDId::Create("b");

        auto qargsLhs = NDList::Create();
        auto qargsRhs = NDList::Create();
        auto qargsLhsRhs = NDList::Create();

        qargsLhs->addChild(qargLhs->clone());
        qargsRhs->addChild(qargRhs->clone());
        qargsLhsRhs->addChild(qargLhs->clone());
        qargsLhsRhs->addChild(qargRhs->clone());

        // The quantum operations
        auto gop = NDGOpList::Create();
        // cx a, b;
        gop->addChild(NDQOpGeneric::Create
                (uniqueCastForward<NDId>(CX_ID_NODE->clone()), NDList::Create(),
                 uniqueCastForward<NDList>(qargsLhsRhs->clone())));
        // h a;
        gop->addChild(NDQOpGeneric::Create
                (uniqueCastForward<NDId>(H_ID_NODE->clone()), NDList::Create(), 
                 uniqueCastForward<NDList>(qargsLhs->clone())));
        // h b;
        gop->addChild(NDQOpGeneric::Create
                (uniqueCastForward<NDId>(H_ID_NODE->clone()), NDList::Create(), 
                 uniqueCastForward<NDList>(qargsRhs->clone())));
        // cx a, b;
        gop->addChild(NDQOpGeneric::Create
                (uniqueCastForward<NDId>(CX_ID_NODE->clone()), NDList::Create(), 
                 uniqueCastForward<NDList>(qargsLhsRhs->clone())));
        // h a;
        gop->addChild(NDQOpGeneric::Create
                (uniqueCastForward<NDId>(H_ID_NODE->clone()), NDList::Create(),
                 uniqueCastForward<NDList>(qargsLhs->clone())));
        // h b;
        gop->addChild(NDQOpGeneric::Create
                (uniqueCastForward<NDId>(H_ID_NODE->clone()), NDList::Create(),
                 uniqueCastForward<NDList>(qargsRhs->clone())));
        // cx a, b;
        gop->addChild(NDQOpGeneric::Create
                (uniqueCastForward<NDId>(CX_ID_NODE->clone()), NDList::Create(),
                 uniqueCastForward<NDList>(qargsLhsRhs->clone())));

        auto swap = NDGateDecl::Create
                (uniqueCastForward<NDId>(SWAP_ID_NODE->clone()),
                 NDList::Create(),
                 std::move(qargsLhsRhs),
                 std::move(gop));

        insertGate(std::move(swap));
    }
}

efd::NDQasmVersion::Ref efd::QModule::getVersion() {
    return mVersion.get();
}

void efd::QModule::replaceAllRegsWith(std::vector<NDRegDecl::uRef> newRegs) {
    mRegs.clear();

    for (auto& reg : newRegs) {
        std::string id = reg->getId()->getVal();
        mRegs[id] = std::move(reg);
    }
}

efd::QModule::Iterator efd::QModule::inlineCall(NDQOpGeneric::Ref call) {
    Node::Ref parent = call->getParent();
    Iterator it = parent->findChild(call);
    unsigned dist = std::distance(parent->begin(), it);

    InlineGate(this, call);
    return parent->begin() + dist;
}

efd::QModule::Iterator efd::QModule::insertStatementAfter(Iterator it, Node::uRef ref) {
    mStatements->addChild(++it, std::move(ref));
    return it;
}

efd::QModule::Iterator efd::QModule::insertStatementBefore(Iterator it, Node::uRef ref) {
    mStatements->addChild(it, std::move(ref));
    return it;
}

efd::QModule::Iterator efd::QModule::insertStatementFront(Node::uRef ref) {
    mStatements->addChild(std::move(ref));
    return mStatements->begin();
}

efd::QModule::Iterator efd::QModule::insertStatementLast(Node::uRef ref) {
    auto it = mStatements->begin();
    mStatements->addChild(it, std::move(ref));
    return it;
}

efd::QModule::Iterator efd::QModule::insertSwapBefore(Iterator it, Node::Ref lhs, Node::Ref rhs) {
    Node::Ref parent = (*it)->getParent();
    unsigned dist = std::distance(parent->begin(), it);
    InsertSwapBefore(it->get(), lhs, rhs);

    registerSwapGate();
    return parent->begin() + dist;
}

efd::QModule::Iterator efd::QModule::insertSwapAfter(Iterator it, Node::Ref lhs, Node::Ref rhs) {
    Node::Ref parent = (*it)->getParent();
    unsigned dist = std::distance(parent->begin(), it);
    InsertSwapAfter(it->get(), lhs, rhs);

    registerSwapGate();
    return parent->begin() + dist;
}

void efd::QModule::insertGate(NDGateDecl::uRef gate) {
    assert(gate.get() != nullptr && "Trying to insert a 'nullptr' gate.");
    assert(gate->getId() != nullptr && "Trying to insert a gate with 'nullptr' id.");

    std::string id = gate->getId()->getVal();
    assert(mGates.find(id) == mGates.end() && "Trying to insert a gate with repeated id.");

    mGates[id] = std::move(gate);
}

efd::QModule::RegIterator efd::QModule::reg_begin() {
    return mRegs.begin();
}

efd::QModule::RegConstIterator efd::QModule::reg_begin() const {
    return mRegs.begin();
}

efd::QModule::RegIterator efd::QModule::reg_end() {
    return mRegs.end();
}

efd::QModule::RegConstIterator efd::QModule::reg_end() const {
    return mRegs.end();
}

efd::QModule::GateIterator efd::QModule::gates_begin() {
    return mGates.begin();
}

efd::QModule::GateConstIterator efd::QModule::gates_begin() const {
    return mGates.begin();
}

efd::QModule::GateIterator efd::QModule::gates_end() {
    return mGates.end();
}

efd::QModule::GateConstIterator efd::QModule::gates_end() const {
    return mGates.end();
}

efd::QModule::Iterator efd::QModule::stmt_begin() {
    return mStatements->begin();
}

efd::QModule::ConstIterator efd::QModule::stmt_begin() const {
    return mStatements->begin();
}

efd::QModule::Iterator efd::QModule::stmt_end() {
    return mStatements->end();
}

efd::QModule::ConstIterator efd::QModule::stmt_end() const {
    return mStatements->end();
}

void efd::QModule::print(std::ostream& O, bool pretty, bool printGates) const {
    O << toString(pretty, printGates);
}

std::string efd::QModule::toString(bool pretty, bool printGates) const {
    std::string str;

    if (mVersion.get() != nullptr)
        str += mVersion->toString(pretty);

    for (auto& incl : mIncludes)
        str += incl->toString(pretty);

    if (printGates) {
        for (auto& gatePair : mGates)
            str += gatePair.second->toString(pretty);
    }

    for (auto& regPair : mRegs)
        str += regPair.second->toString(pretty);

    str += mStatements->toString(pretty);
    return str;
}

efd::Node::Ref efd::QModule::getQVar(std::string id, NDGateDecl::Ref gate) {
    if (gate != nullptr) {
        assert(mGateIdMap.find(gate) != mGateIdMap.end() && "No such gate found.");

        IdMap& idMap = mGateIdMap[gate];
        assert(idMap.find(id) != idMap.end() && "No such id inside this gate.");

        return idMap[id];
    }

    // If gate == nullptr, then we want a quantum register in the global context.
    return mRegs[id].get();
}

efd::NDGateDecl::Ref efd::QModule::getQGate(std::string id) {
    assert(mGates.find(id) != mGates.end() && "Gate not found.");
    return mGates[id].get();
}

void efd::QModule::runPass(Pass::Ref pass, bool force) {
    if (pass->wasApplied() && !force)
        return;

    pass->init(force);

    if (pass->isRegDeclPass()) {
        for (auto& regPair : mRegs)
            regPair.second->apply(pass);
    }
    
    if (pass->isGatePass()) {
        for (auto& gatePair : mGates)
            gatePair.second->apply(pass);
    }

    if (pass->isStatementPass()) {
        for (auto& stmt : *mStatements)
            stmt->apply(pass);
    }
}

efd::QModule::uRef efd::QModule::clone() const {
    auto qmod = new QModule();

    if (mVersion.get() != nullptr)
        qmod->mVersion = uniqueCastForward<NDQasmVersion>(mVersion->clone());

    for (auto& regPair : mRegs)
        qmod->mRegs[regPair.first] = uniqueCastForward<NDRegDecl>(regPair.second->clone());

    for (auto& gatePair : mGates)
        qmod->mGates[gatePair.first] = uniqueCastForward<NDGateDecl>(gatePair.second->clone());

    qmod->mStatements = uniqueCastForward<NDStmtList>(mStatements->clone());
    return uRef(qmod);
}

efd::QModule::uRef efd::QModule::Create(bool forceStdLib) {
    std::string program;
    program = "OPENQASM 2.0;\n";

    auto ast = efd::ParseString(program, forceStdLib);
    if (ast.get() != nullptr)
        return GetFromAST(std::move(ast));

    return uRef(nullptr);
}

efd::QModule::uRef efd::QModule::GetFromAST(Node::uRef ref) {
    uRef qmod(new QModule());
    return qmod;
}

efd::QModule::uRef efd::QModule::Parse(std::string filename, 
        std::string path, bool forceStdLib) {
    auto ast = efd::ParseFile(filename, path, forceStdLib);

    if (ast.get() != nullptr)
        return GetFromAST(std::move(ast));

    return uRef(nullptr);
}

efd::QModule::uRef efd::QModule::ParseString(std::string program, bool forceStdLib) {
    auto ast = efd::ParseString(program, forceStdLib);

    if (ast != nullptr)
        return GetFromAST(std::move(ast));

    return uRef(nullptr);
}
