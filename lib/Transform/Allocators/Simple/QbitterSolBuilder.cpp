#include "enfield/Transform/Allocators/Simple/QbitterSolBuilder.h"
#include "enfield/Support/BFSPathFinder.h"

efd::StdSolution efd::QbitterSolBuilder::build
(Mapping initial, DepsVector& deps, ArchGraph::Ref g) {
    auto mapping = initial;
    auto assign = GenAssignment(g->size(), mapping);
    auto finder = BFSPathFinder::Create();

    StdSolution solution { initial, StdSolution::OpSequences(deps.size()), 0 };

    for (uint32_t i = 0, e = deps.size(); i < e; ++i) {
        auto dep = deps[i][0];

        auto& ops = solution.mOpSeqs[i];
        ops.first = deps[i].mCallPoint;

        // (u, v) edge in Arch
        uint32_t a = dep.mFrom, b = dep.mTo;
        uint32_t u = mapping[a], v = mapping[b];

        Operation operation;
        if (g->hasEdge(u, v)) {
            operation = { Operation::K_OP_CNOT, a, b };
        } else if (g->isReverseEdge(u, v)) {
            solution.mCost += RevCost.getVal();
            operation = { Operation::K_OP_REV, a, b };
        } else {
            solution.mCost += LCXCost.getVal();
            operation = { Operation::K_OP_LCNOT, a, b };

            auto path = finder->find(g, u, v);

            if (path.size() != 3) {
                ERR << "Can't apply a long cnot. Path size: `"
                    << path.size() << "`." << std::endl;
                ExitWith(ExitCode::EXIT_unreachable);
            }

            operation.mW = assign[path[1]];
        }
        ops.second.push_back(operation);
    }

    return solution;
}

efd::QbitterSolBuilder::uRef efd::QbitterSolBuilder::Create() {
    return uRef(new QbitterSolBuilder());
}
