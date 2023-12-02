// tarik (c) Nikolas Wipper 2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_PATH_H_
#define TARIK_SRC_SEMANTIC_PATH_H_
#include "syntactic/expressions/Expression.h"

std::vector<std::string> flatten_path(Expression *path);

#endif //TARIK_SRC_SEMANTIC_PATH_H_
