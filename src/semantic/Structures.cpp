// tarik (c) Nikolas Wipper 2024

#include "Structures.h"

StructureNode::StructureNode(const aast::StructDeclareStatement *st)
    : statement(st) {}

bool StructureNode::has_child(const aast::StructDeclareStatement *st) const {
    if (statement == st)
        return true;

    for (auto field : fields)
        if (field->has_child(st))
            return true;

    return false;
}

bool StructureNode::add_field(StructureNode *field) {
    if (field->has_child(statement))
        return false;

    fields.push_back(field);

    return true;
}

StructureNode *StructureGraph::get_node(const aast::StructDeclareStatement *statement) {
    if (roots.contains(statement))
        return roots.at(statement);
    return roots[statement] = new StructureNode(statement);
}

std::vector<const aast::StructDeclareStatement *> StructureGraph::flatten() {
    std::vector<const aast::StructDeclareStatement *> result;
    std::unordered_set<const aast::StructDeclareStatement *> witness;
    std::stack<StructureNode *> root_stack;

    witness.reserve(roots.size());
    result.reserve(roots.size());

    for (auto [root, node] : roots) {
        root_stack.push(node);
    }

    while (!root_stack.empty()) {
        auto node = root_stack.top();

        if (witness.contains(node->statement)) {
            root_stack.pop();
            continue;
        }

        if (node->fields.empty()) {
            result.push_back(node->statement);
            witness.emplace(node->statement);

            root_stack.pop();
            continue;
        }

        for (auto field : node->fields) {
            root_stack.push(field);
        }

        node->fields.clear();
    }

    return result;
}
