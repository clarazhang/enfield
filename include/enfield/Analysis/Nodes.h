#ifndef __EFD_NODES_H__
#define __EFD_NODES_H__

#include "enfield/Support/WrapperVal.h"
#include "enfield/Support/RTTI.h"

#include <initializer_list>
#include <iostream>
#include <vector>
#include <memory>

namespace efd {
    class Node;
    class NodeVisitor;

    class NDQasmVersion;
    class NDInclude;
    class NDDecl;
    class NDGateDecl;
    class NDGateSign;
    class NDQOp;
    class NDQOpMeasure;
    class NDQOpReset;
    class NDQOpBarrier;
    class NDQOpCX;
    class NDQOpU;
    class NDBinOp;
    class NDUnaryOp;
    class NDQOpGen;
    class NDIdRef;
    class NDList;
    class NDStmtList;
    class NDGOpList;
    class NDIfStmt;

    typedef NDGateSign NDOpaque;

    /// \brief Base class for AST nodes.
    class Node {
        public:
            typedef Node* Ref;
            typedef std::unique_ptr<Node> uRef;
            typedef std::shared_ptr<Node> sRef;

            typedef std::vector<uRef>::iterator Iterator;
            typedef std::vector<uRef>::const_iterator ConstIterator;

            enum Kind {
                K_QASM_VERSION,
                K_INCLUDE,
                K_REG_DECL,
                K_GATE_DECL,
                K_GATE_OPAQUE,
                K_QOP,
                K_QOP_MEASURE,
                K_QOP_RESET,
                K_QOP_BARRIER,
                K_QOP_CX,
                K_QOP_U,
                K_QOP_GEN,
                K_BINOP,
                K_UNARYOP,
                K_ID_REF,
                K_LIST,
                K_STMT_LIST,
                K_GOP_LIST,
                K_IF_STMT,
                K_LIT_INT,
                K_LIT_REAL,
                K_LIT_STRING
            };

        protected:
            /// \brief The kind of the node.
            Kind mK;
            /// \brief Holds whether this node has no information.
            bool mIsEmpty;
            /// \brief True if it was generated by the compiler.
            bool mWasGenerated;
            /// \brief True if it is a node inside an include node.
            bool mInInclude;
            /// \brief The childrem nodes.
            std::vector<uRef> mChild;
            Ref mParent;

            /// \brief Constructs the node, initially empty (with no information).
            Node(Kind k, bool empty = false);

            /// \brief Adds a child to this node. It should be used only for
            /// construction of each derived class.
            void innerAddChild(uRef ref);

            /// \brief Extend the 'equals' implementation by implementing this method.
            virtual bool equalsImpl(Node::Ref ref) const;

            /// \brief Clones the current node (deep copy).
            virtual Node::uRef cloneImpl() const = 0;

        public:
            virtual ~Node();

            /// \brief Gets the i-th child.
            Ref getChild(uint32_t i) const;
            /// \brief Sets the i-th child.
            void setChild(uint32_t i, uRef ref);
            /// \brief Returns a iterator pointing to the given child.
            Iterator findChild(Node::Ref ref);

            /// \brief Returns a iterator to the beginning of the vector.
            Iterator begin();
            /// \brief Returns a iterator to the end of the vector.
            Iterator end();

            /// \brief Returns a const iterator to the beginning of the vector.
            ConstIterator begin() const;
            /// \brief Returns a const iterator to the end of the vector.
            ConstIterator end() const;

            /// \brief Returns the kind of this node.
            Kind getKind() const;

            /// \brief Prints from this node, recursively to \p O.
            void print(std::ostream& O = std::cout, bool pretty = false);
            /// \brief Prints to the standard output.
            void print(bool pretty = false);

            /// \brief Returns whether this node has any information.
            bool isEmpty() const;
            /// \brief Returns whether this node was generated by the compiler.
            bool wasGenerated() const;
            void setGenerated();
            /// \brief Returns whether this node is in a include node.
            bool isInInclude() const;
            void setInInclude();

            /// \brief Returns the parent node of the current node.
            Ref getParent() const;
            /// \brief Sets the parent node of the current node.
            void setParent(Ref ref);

            /// \brief Compares recursively the equality between two nodes.
            ///
            /// One \em Node is said to be equal to another one if, and only if, all its
            /// operands and names are equal.
            bool equals(Node::Ref ref) const;

            /// \brief Returns a std::string representation of the operation.
            virtual std::string getOperation() const;
            /// \brief Returns the number of childrem of this node. 
            virtual uint32_t getChildNumber() const = 0;
            /// \brief Returns a std::string representation of this Node and its childrem.
            virtual std::string toString(bool pretty = false) const = 0;
            /// \brief Used by visitor classes.
            virtual void apply(NodeVisitor* visitor) = 0;

            /// \brief Clones the current node (deep copy).
            Node::uRef clone() const;

            friend class QModule;
    };

    /// \brief Node for literal types.
    template <typename T>
        class NDValue : public Node {
            public:
                typedef NDValue* Ref;
                typedef std::unique_ptr<NDValue> uRef;

            protected:
                T mVal;

                NDValue(T val);
                bool equalsImpl(Node::Ref ref) const override;
                Node::uRef cloneImpl() const override;

            public:
                typedef std::shared_ptr< NDValue<T> > NDRef;

                /// \brief Returns a copy to the setted value.
                T getVal() const;

                std::string getOperation() const override;
                std::string toString(bool pretty = false) const override;

                uint32_t getChildNumber() const override;

                void apply(NodeVisitor* visitor) override;

                /// \brief Returns whether the \p node is an instance of this class.
                static bool ClassOf(const Node* node);
                /// \brief Creates a new instance of this node.
                static uRef Create(T val);
        };

    template class NDValue<IntVal>;
    template <> NDValue<IntVal>::NDValue(IntVal val);
    template <> bool NDValue<IntVal>::ClassOf(const Node* node);
    template <> void NDValue<IntVal>::apply(NodeVisitor* visitor);

    template class NDValue<RealVal>;
    template <> NDValue<RealVal>::NDValue(RealVal val);
    template <> bool NDValue<RealVal>::ClassOf(const Node* node);
    template <> void NDValue<RealVal>::apply(NodeVisitor* visitor);

    template class NDValue<std::string>;
    template <> NDValue<std::string>::NDValue(std::string val);
    template <> bool NDValue<std::string>::ClassOf(const Node* node);
    template <> std::string NDValue<std::string>::getOperation() const;
    template <> std::string NDValue<std::string>::toString(bool pretty) const;
    template <> void NDValue<std::string>::apply(NodeVisitor* visitor);

    typedef NDValue<IntVal> NDInt;
    typedef NDValue<RealVal> NDReal;
    typedef NDValue<std::string> NDId;
    typedef NDValue<std::string> NDString;

    /// \brief Node for declaration of registers (concrete and quantum).
    class NDDecl : public Node {
        public:
            typedef NDDecl* Ref;
            typedef std::unique_ptr<NDDecl> uRef;

        protected:
            enum ChildType {
                I_ID = 0
            };

            NDDecl(Kind k, NDId::uRef idNode);

        public:
            /// \brief Gets the id node.
            NDId::Ref getId() const;
            /// \brief Sets the id node.
            void setId(NDId::uRef ref);

            /// \brief Returns true if this is a register declaration.
            bool isReg() const;
            /// \brief Returns true if this is a gate declaration.
            bool isGate() const;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
    };

    class NDRegDecl : public NDDecl {
        public:
            typedef NDRegDecl* Ref;
            typedef std::unique_ptr<NDRegDecl> uRef;

        protected:
            enum ChildType {
                I_SIZE = 1
            };

            /// \brief The possible types of declaration.
            enum Type {
                CONCRETE,
                QUANTUM
            };

            Type mT;

            NDRegDecl(Type t, NDId::uRef idNode, NDInt::uRef sizeNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the size node.
            NDInt::Ref getSize() const;
            /// \brief Sets the size node.
            void setSize(NDInt::uRef ref);

            /// \brief Returns true if it is a concrete register declaration.
            bool isCReg() const;
            /// \brief Returns true if it is a quantum register declaration.
            bool isQReg() const;

            uint32_t getChildNumber() const override;
            std::string getOperation() const override;
            std::string toString(bool pretty = false) const override;
            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);

            /// \brief Creates a new register node.
            static std::unique_ptr<NDRegDecl> Create(Type t, NDId::uRef idNode, NDInt::uRef sizeNode);
            /// \brief Creates a new quantum register node of this node.
            static std::unique_ptr<NDRegDecl> CreateQ(NDId::uRef idNode, NDInt::uRef sizeNode);
            /// \brief Creates a new concrete register node of this node.
            static std::unique_ptr<NDRegDecl> CreateC(NDId::uRef idNode, NDInt::uRef sizeNode);
    };

    /// \brief Node for id references (register specific positions).
    class NDIdRef : public Node {
        public:
            typedef NDIdRef* Ref;
            typedef std::unique_ptr<NDIdRef> uRef;

        protected:
            enum ChildType {
                I_ID = 0,
                I_N
            };

            NDIdRef(NDId::uRef idNode, NDInt::uRef nNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the id.
            NDId::Ref getId() const;
            /// \brief Sets the id.
            void setId(NDId::uRef ref);
            /// \brief Gets an integer representing the position.
            NDInt::Ref getN() const;
            /// \brief Sets an integer representing the position.
            void setN(NDInt::uRef ref);

            std::string toString(bool pretty = false) const override;

            uint32_t getChildNumber() const override;

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(NDId::uRef idNode, NDInt::uRef nNode);
    };

    /// \brief Base class for list of nodes.
    class NDList : public Node {
        public:
            typedef NDList* Ref;
            typedef std::unique_ptr<NDList> uRef;

        protected:
            NDList(Kind k);
            Node::uRef cloneImpl() const override;

        public:
            virtual ~NDList();

            /// \brief Deep-copies the childrem of \p list.
            void cloneChildrem(const NDList* list);

            /// \brief Appends a child to the end of the list.
            Iterator addChild(Node::uRef child);
            /// \brief Inserts a child in the iterator \p it. The iterator goes to
            /// the new inserted element.
            Iterator addChild(Iterator it, Node::uRef child);
            /// \brief Appends \p children to the end of the list.
            Iterator addChildren(std::vector<Node::uRef> children);
            /// \brief Inserts \p children in the iterator \p it. The iterator goes to
            /// the first new inserted element.
            Iterator addChildren(Iterator it, std::vector<Node::uRef> children);
            /// \brief Removes the child in the iterator \p it. The iterator moves
            /// to the next element.
            Iterator removeChild(Iterator it);
            /// \brief Removes the \p child (must exist).
            void removeChild(Node::Ref child);

            uint32_t getChildNumber() const override;

            std::string toString(bool pretty = false) const override;
            void apply(NodeVisitor* visitor) override;


            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node with a specific \p k.
            static uRef Create(Kind k);
            /// \brief Creates a new instance of this node with the kind \em K_LIST.
            static uRef Create();
    };

    /// \brief Node for list of qubit operation sequences.
    class NDStmtList : public NDList {
        public:
            typedef NDStmtList* Ref;
            typedef std::unique_ptr<NDStmtList> uRef;

        protected:
            NDStmtList();
            Node::uRef cloneImpl() const override;

        public:
            std::string toString(bool pretty = false) const override;
            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create();
    };

    /// \brief Node for list of qubit operation sequences inside gate declarations.
    class NDGOpList : public NDList {
        public:
            typedef NDGOpList* Ref;
            typedef std::unique_ptr<NDGOpList> uRef;

        protected:
            NDGOpList();
            Node::uRef cloneImpl() const override;

        public:
            std::string toString(bool pretty = false) const override;
            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create();
    };
    
    /// \brief Node that holds the current Qasm version and the
    /// rest of the program.
    class NDQasmVersion : public Node {
        public:
            typedef NDQasmVersion* Ref;
            typedef std::unique_ptr<NDQasmVersion> uRef;

        protected:
            enum ChildType {
                I_VERSION = 0,
                I_STMTS
            };

            NDQasmVersion(NDReal::uRef vNode, NDStmtList::uRef stmtsNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the node that holds the version.
            NDReal::Ref getVersion() const;
            /// \brief Sets the node that holds the version.
            void setVersion(NDReal::uRef ref);

            /// \brief Gets the node that holds the statements.
            NDStmtList::Ref getStatements() const;
            /// \brief Sets the node that holds the statements.
            void setStatements(NDStmtList::uRef ref);

            std::string getOperation() const override;
            std::string toString(bool pretty = false) const override;

            uint32_t getChildNumber() const override;

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(NDReal::uRef vNode, NDStmtList::uRef stmtsNode);
    };

    /// \brief Node used to parse another file.
    /// 
    /// The AST from this other file is a child of this node.
    class NDInclude : public Node {
        public:
            typedef NDInclude* Ref;
            typedef std::unique_ptr<NDInclude> uRef;

        protected:
            enum ChildType {
                I_FILE = 0,
                I_INNER_AST
            };

            NDInclude(NDId::uRef fNode, Node::uRef astNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the node that holds the filename.
            NDString::Ref getFilename() const;
            /// \brief Sets the node that holds the filename.
            void setFilename(NDString::uRef ref);
            /// \brief Gets the node that holds the statements.
            Node::Ref getInnerAST() const;
            /// \brief Sets the node that holds the statements.
            void setInnerAST(Node::uRef ref);

            std::string getOperation() const override;
            std::string toString(bool pretty = false) const override;

            uint32_t getChildNumber() const override;

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(NDId::uRef fNode, Node::uRef astNode);
    };

    /// \brief Node for declaration of opaque quantum gates.
    class NDGateSign : public NDDecl {
        public:
            typedef NDGateSign* Ref;
            typedef std::unique_ptr<NDGateSign> uRef;

        protected:
            enum ChildType {
                I_ARGS = 1,
                I_QARGS
            };

            NDGateSign(NDId::uRef idNode, NDList::uRef aNode, NDList::uRef qaNode);
            NDGateSign(Kind k, NDId::uRef idNode, NDList::uRef aNode,
                    NDList::uRef qaNode);

            Node::uRef cloneImpl() const override;

        public:
            /// \brief Returns true if this is an opaque gate.
            bool isOpaque() const;

            /// \brief Gets the args node.
            NDList::Ref getArgs() const;
            /// \brief Sets the args node.
            void setArgs(NDList::uRef ref);
            /// \brief Gets the qargs node.
            NDList::Ref getQArgs() const;
            /// \brief Sets the qargs node.
            void setQArgs(NDList::uRef ref);

            std::string getOperation() const override;
            std::string toString(bool pretty = false) const override;

            uint32_t getChildNumber() const override;

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(NDId::uRef idNode, NDList::uRef aNode, NDList::uRef qaNode);
    };

    /// \brief Node for declaration of quantum gates.
    class NDGateDecl : public NDGateSign {
        public:
            typedef NDGateDecl* Ref;
            typedef std::unique_ptr<NDGateDecl> uRef;

        protected:
            enum ChildType {
                I_GOPLIST = 3
            };

            NDGateDecl(NDId::uRef idNode, NDList::uRef aNode, NDList::uRef qaNode, NDGOpList::uRef gopNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the goplist node.
            NDGOpList::Ref getGOpList() const;
            /// \brief Sets the goplist node.
            void setGOpList(NDGOpList::uRef ref);

            std::string getOperation() const override;
            std::string toString(bool pretty = false) const override;

            uint32_t getChildNumber() const override;

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(NDId::uRef idNode, NDList::uRef aNode, NDList::uRef qaNode, NDGOpList::uRef gopNode);
    };

    /// \brief Generic node for quantum operations.
    /// This is also the base node for the other quantum gates.
    class NDQOp : public Node {
        public:
            typedef NDQOp* Ref;
            typedef std::unique_ptr<NDQOp> uRef;

        protected:
            enum ChildType {
                I_ID = 0,
                I_ARGS,
                I_QARGS
            };

            NDQOp(Kind k, NDId::uRef idNode, NDList::uRef aNode, NDList::uRef qaNode);

        public:
            virtual ~NDQOp();

            /// \brief Gets the id.
            NDId::Ref getId() const;
            /// \brief Sets the id.
            void setId(NDId::uRef ref);
            /// \brief Gets the arguments.
            NDList::Ref getArgs() const;
            /// \brief Sets the arguments.
            void setArgs(NDList::uRef ref);
            /// \brief Gets the quantum arguments.
            NDList::Ref getQArgs() const;
            /// \brief Sets the quantum arguments.
            void setQArgs(NDList::uRef ref);

            /// \brief Returns true if this is a reset node.
            bool isReset() const;
            /// \brief Returns true if this is a barrier node.
            bool isBarrier() const;
            /// \brief Returns true if this is a measure node.
            bool isMeasure() const;
            /// \brief Returns true if this is a u node.
            bool isU() const;
            /// \brief Returns true if this is a cx node.
            bool isCX() const;
            /// \brief Returns true if this is a generic node.
            bool isGeneric() const;

            std::string getOperation() const override;

            virtual uint32_t getChildNumber() const override;
            virtual std::string toString(bool pretty = false) const override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
    };

    /// \brief NDQOp specialized for reset operation.
    class NDQOpReset : public NDQOp {
        public:
            typedef NDQOpReset* Ref;
            typedef std::unique_ptr<NDQOpReset> uRef;

        protected:
            enum ChildType {
                I_ONLY = 0
            };

            NDQOpReset(Node::uRef qaNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the quantum argument.
            Node::Ref getQArg() const;
            /// \brief Sets the quantum argument.
            void setQArg(Node::uRef ref);

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(Node::uRef qaNode);
    };

    /// \brief NDQOp specialized for barrier operation.
    class NDQOpBarrier : public NDQOp {
        public:
            typedef NDQOpBarrier* Ref;
            typedef std::unique_ptr<NDQOpBarrier> uRef;

        protected:
            enum ChildType {
                I_ONLY = 0
            };

            NDQOpBarrier(NDList::uRef qaNode);
            Node::uRef cloneImpl() const override;

        public:
            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(NDList::uRef qaNode);
    };

    /// \brief NDQOp specialized for measure operation.
    class NDQOpMeasure : public NDQOp {
        public:
            typedef NDQOpMeasure* Ref;
            typedef std::unique_ptr<NDQOpMeasure> uRef;

        protected:
            enum ChildType {
                I_QBIT = 0,
                I_CBIT = 3
            };

            NDQOpMeasure(Node::uRef qNode, Node::uRef cNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the qbit node.
            Node::Ref getQBit() const;
            /// \brief Sets the qbit node.
            void setQBit(Node::uRef ref);
            /// \brief Gets the cbit node.
            Node::Ref getCBit() const;
            /// \brief Sets the cbit node.
            void setCBit(Node::uRef ref);

            uint32_t getChildNumber() const override;
            std::string toString(bool pretty = false) const override;
            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(Node::uRef qNode, Node::uRef cNode);
    };

    /// \brief NDQOp specialized for barrier operation.
    class NDQOpU : public NDQOp {
        public:
            typedef NDQOpU* Ref;
            typedef std::unique_ptr<NDQOpU> uRef;

        protected:
            enum ChildType {
                I_ONLY = 0
            };

            NDQOpU(NDList::uRef aNode, Node::uRef qaNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the quantum argument.
            Node::Ref getQArg() const;
            /// \brief Sets the quantum argument.
            void setQArg(Node::uRef ref);

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(NDList::uRef aNode, Node::uRef qaNode);
    };

    /// \brief NDQOp specialized for barrier operation.
    class NDQOpCX : public NDQOp {
        public:
            typedef NDQOpCX* Ref;
            typedef std::unique_ptr<NDQOpCX> uRef;

        protected:
            enum ChildType {
                I_LHS = 0,
                I_RHS
            };

            NDQOpCX(Node::uRef lhsNode, Node::uRef rhsNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the left hand side of the gate.
            Node::Ref getLhs() const;
            /// \brief Sets the left hand side of the gate.
            void setLhs(Node::uRef ref);
            /// \brief Gets the right hand side of the gate.
            Node::Ref getRhs() const;
            /// \brief Sets the right hand side of the gate.
            void setRhs(Node::uRef ref);

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(Node::uRef lhsNode, Node::uRef rhsNode);
    };

    /// \brief Binary operation node.
    class NDBinOp : public Node {
        protected:
            enum ChildType {
                I_LHS = 0,
                I_RHS
            };

        public:
            typedef NDBinOp* Ref;
            typedef std::unique_ptr<NDBinOp> uRef;

            /// \brief The possible binary operations.
            enum OpType {
                OP_ADD = 0,
                OP_SUB,
                OP_MUL,
                OP_DIV,
                OP_POW
            };

        protected:
            OpType mT;

            NDBinOp(OpType t, Node::uRef lhsNode, Node::uRef rhsNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the left hand side argument.
            Node::Ref getLhs() const;
            /// \brief Sets the left hand side argument.
            void setLhs(Node::uRef ref);
            /// \brief Gets the right hand side argument.
            Node::Ref getRhs() const;
            /// \brief Sets the right hand side argument.
            void setRhs(Node::uRef ref);

            /// \brief Returns the type of the binary operation of this node.
            OpType getOpType() const;

            /// \brief Returns whether this is an add operation.
            bool isAdd() const;
            /// \brief Returns whether this is an sub operation.
            bool isSub() const;
            /// \brief Returns whether this is an mul operation.
            bool isMul() const;
            /// \brief Returns whether this is an div operation.
            bool isDiv() const;
            /// \brief Returns whether this is an pow operation.
            bool isPow() const;

            std::string getOperation() const override;
            std::string toString(bool pretty = false) const override;

            uint32_t getChildNumber() const override;

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(OpType t, Node::uRef lhsNode, Node::uRef rhsNode);
            /// \brief Creates a new binary adition node.
            static uRef CreateAdd(Node::uRef lhsNode, Node::uRef rhsNode);
            /// \brief Creates a new binary subtraction node.
            static uRef CreateSub(Node::uRef lhsNode, Node::uRef rhsNode);
            /// \brief Creates a new binary multiplication node.
            static uRef CreateMul(Node::uRef lhsNode, Node::uRef rhsNode);
            /// \brief Creates a new binary division node.
            static uRef CreateDiv(Node::uRef lhsNode, Node::uRef rhsNode);
            /// \brief Creates a new binary power node.
            static uRef CreatePow(Node::uRef lhsNode, Node::uRef rhsNode);
    };

    /// \brief Unary operation node.
    class NDUnaryOp : public Node {
        protected:
            enum ChildType {
                I_ONLY = 0
            };

        public:
            typedef NDUnaryOp* Ref;
            typedef std::unique_ptr<NDUnaryOp> uRef;

            /// \brief Unary operations available.
            enum UOpType {
                UOP_SIN = 0,
                UOP_COS,
                UOP_TAN,
                UOP_EXP,
                UOP_LN,
                UOP_SQRT,
                UOP_NEG
            };

        protected:
            UOpType mT;

            NDUnaryOp(UOpType t, Node::uRef oNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the only operand.
            Node::Ref getOperand() const;
            /// \brief Sets the only operand.
            void setOperand(Node::uRef ref);

            /// \brief Returns the unary operation type.
            UOpType getUOpType() const;

            /// \brief Returns whether this is an neg operation.
            bool isNeg() const;
            /// \brief Returns whether this is an sin operation.
            bool isSin() const;
            /// \brief Returns whether this is an cos operation.
            bool isCos() const;
            /// \brief Returns whether this is an tan operation.
            bool isTan() const;
            /// \brief Returns whether this is an exp operation.
            bool isExp() const;
            /// \brief Returns whether this is an ln operation.
            bool isLn() const;
            /// \brief Returns whether this is an sqrt operation.
            bool isSqrt() const;

            std::string getOperation() const override;
            std::string toString(bool pretty = false) const override;

            uint32_t getChildNumber() const override;

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(UOpType t, Node::uRef oNode);
            /// \brief Creates a negative unary operand node.
            static uRef CreateNeg(Node::uRef oNode);
            /// \brief Creates a sin unary operand node.
            static uRef CreateSin(Node::uRef oNode);
            /// \brief Creates a cos unary operand node.
            static uRef CreateCos(Node::uRef oNode);
            /// \brief Creates a tan unary operand node.
            static uRef CreateTan(Node::uRef oNode);
            /// \brief Creates a exp unary operand node.
            static uRef CreateExp(Node::uRef oNode);
            /// \brief Creates a ln unary operand node.
            static uRef CreateLn(Node::uRef oNode);
            /// \brief Creates a sqrt unary operand node.
            static uRef CreateSqrt(Node::uRef oNode);
    };

    /// \brief NDQOp specialized for generic gates.
    class NDQOpGen : public NDQOp {
        public:
            typedef NDQOpGen* Ref;
            typedef std::unique_ptr<NDQOpGen> uRef;

            enum IntrinsicKind {
                K_INTRINSIC_SWAP,
                K_INTRINSIC_REV_CX,
                K_INTRINSIC_LCX
            };

        protected:
            /// \brief The intrinsic kind of the node.
            IntrinsicKind mIK;
            /// \brief True if it is an intrinsic compiler function.
            bool mIsIntrinsic;

            NDQOpGen(NDId::uRef idNode, NDList::uRef aNode, NDList::uRef qaNode);
            NDQOpGen(NDId::uRef idNode, NDList::uRef aNode, NDList::uRef qaNode, IntrinsicKind ik);
            Node::uRef cloneImpl() const override;

        public:
            void apply(NodeVisitor* visitor) override;

            /// \brief Returns the intrinsic kind of this node.
            ///
            /// Only to be used by intrinsic nodes.
            IntrinsicKind getIntrinsicKind() const;
            /// \brief Returns whether this node is in an intrinsic node.
            bool isIntrinsic() const;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(NDId::uRef idNode, NDList::uRef aNode, NDList::uRef qaNode);
    };

    /// \brief Node for conditional statement.
    class NDIfStmt : public Node {
         public:
            typedef NDIfStmt* Ref;
            typedef std::unique_ptr<NDIfStmt> uRef;

            enum ChildType {
                I_COND_ID = 0,
                I_COND_N,
                I_QOP
            };

        protected:
            NDIfStmt(NDId::uRef cidNode, NDInt::uRef nNode, NDQOp::uRef qopNode);
            Node::uRef cloneImpl() const override;

        public:
            /// \brief Gets the id inside the conditional.
            NDId::Ref getCondId() const;
            /// \brief Sets the id inside the conditional.
            void setCondId(NDId::uRef ref);
            /// \brief Gets the int inside the conditional.
            NDInt::Ref getCondN() const;
            /// \brief Sets the int inside the conditional.
            void setCondN(NDInt::uRef ref);
            /// \brief Gets the qop.
            NDQOp::Ref getQOp() const;
            /// \brief Sets the qop.
            void setQOp(NDQOp::uRef ref);

            std::string toString(bool pretty = false) const override;
            std::string getOperation() const override;

            uint32_t getChildNumber() const override;

            void apply(NodeVisitor* visitor) override;

            /// \brief Returns whether the \p node is an instance of this class.
            static bool ClassOf(const Node* node);
            /// \brief Creates a new instance of this node.
            static uRef Create(NDId::uRef cidNode, NDInt::uRef nNode,
                    NDQOp::uRef qopNode);
    };
};

// -------------- Literal -----------------
template <typename T>
bool efd::NDValue<T>::equalsImpl(Node::Ref ref) const {
    if (auto refval = dynCast<NDValue<T>>(ref)) {
        return mVal == refval->getVal();
    }

    return false;
}

template <typename T>
T efd::NDValue<T>::getVal() const {
    return mVal;
}

template <typename T>
std::string efd::NDValue<T>::getOperation() const {
    return std::to_string(mVal);
}

template <typename T>
std::string efd::NDValue<T>::toString(bool pretty) const {
    return std::to_string(mVal);
}

template <typename T>
uint32_t efd::NDValue<T>::getChildNumber() const {
    return 0;
}

template <typename T>
efd::Node::uRef efd::NDValue<T>::cloneImpl() const {
    return Node::uRef(NDValue<T>::Create(getVal()).release());
}

template <typename T>
bool efd::NDValue<T>::ClassOf(const Node* node) { 
    return node->getKind() == K_LIT_INT ||
        node->getKind() == K_LIT_REAL ||
        node->getKind() == K_LIT_STRING; 
}

template <typename T>
typename efd::NDValue<T>::uRef efd::NDValue<T>::Create(T val) {
    return uRef(new NDValue<T>(val));
}

#endif
