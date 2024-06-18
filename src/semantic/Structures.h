// tarik (c) Nikolas Wipper 2024

#ifndef TARIK_SRC_SEMANTIC_STRUCTURES_H_
#define TARIK_SRC_SEMANTIC_STRUCTURES_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ast/Statements.h"

class StructureNode {
    friend class StructureGraph;

    const aast::StructDeclareStatement *statement;

    std::vector<StructureNode *> fields = {};

    explicit StructureNode(const aast::StructDeclareStatement *st);

    bool has_child(const aast::StructDeclareStatement *st) const;

public:
    bool add_field(StructureNode *field);
};

// Basically just an acyclic graph, that we later transform into a semi-sorted array
class StructureGraph {
    // This is not to confused with roots as they exist in trees. These "roots" are all part of the same graph
    std::unordered_map<const aast::StructDeclareStatement *, StructureNode *> roots;
public:

    StructureNode *get_node(const aast::StructDeclareStatement *statement);
    std::vector<const aast::StructDeclareStatement *> flatten();
};

#endif //TARIK_SRC_SEMANTIC_STRUCTURES_H_
