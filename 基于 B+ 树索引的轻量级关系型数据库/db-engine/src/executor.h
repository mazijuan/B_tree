#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "database.h"
#include "parser.h"

ResultSet* execute_ast(Database* db, const AstNode* ast);

#endif
