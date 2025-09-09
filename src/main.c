#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "typeChecker.h"
#include "codegen.h"
#include "utils.h"

char* parseArgs(int argc, char* argv[]) {

	// Currently only one argument (the about to be compiled file) is accepted
	// TODO: Add check for custom file extension to ONLY compile files with that extension
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename> \n", argv[0]);
		exit(EXIT_FAILURE);
	}

	return argv[1];
}

char* readFileToBuffer(char* filepath) {

    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error allocating memory for file buffer.\n");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "Error reading file.\n");
        fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

void printStatement(Statement* stmt, int indent);
void printExpression(Expression* expr, int indent);
void printBlockStmt(BlockStmt* block, int indent);
void printFunctionStmt(FunctionStmt* function, int indent);
void printFunctionCall(FunctionCall* function, int indent);

// Helper to print indentation
void printIndent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

// Helper to print an operator type
const char* opTypeToString(BinOperationType type) {
    switch (type) {
        case ADD_OP: return "+";
        case SUB_OP: return "-";
        case MUL_OP: return "*";
        case DIV_OP: return "/";
        case MOD_OP: return "%";
        case EQ_OP:  return "==";
        case NEQ_OP: return "!=";
	case ST_OP: return "<";
	case STE_OP: return "<=";
	case GT_OP: return ">";
	case GTE_OP: return ">=";
        default:     return "?";
    }
}

const char* unaryOpTypeToString(TokenType type) {
    switch (type) {
        case NOT:   return "!";
        case MINUS: return "- (negate)";
        default:    return "?";
    }
}

void printVariable(Variable* var, int indent) {
	if (!var) return;
	printIndent(indent);
	if (var->id == NULL) {
		printf("Variable: (NULL ID)\n");
	}
	else {
		printf("Variable: %s\n", var->id);
		printIndent(indent+1);
		printf("Type: %d\n", var->type);
	}
}

void printValue(Value* val, int indent) {
    if (!val) return;
    printIndent(indent);
    switch (val->type) {
	case LONG_TYPE:
		printf("Value: %ld\n", val->as.i_32);
		break;
	case DOUBLE_TYPE: 
		printf("Value: %f\n", val->as.df);
		break;
	default:
		printf("Unknown Value/Not Implemented\n");
    }
}

void printFunctionCall(FunctionCall* function, int indent) {
	if (!function) return;
	printIndent(indent);
	printf("FunctionCall:\n");
	printIndent(indent+1);
	printf("Id: %s\n", function->id);
	printIndent(indent+1);
	printf("Params:\n");
	for (int i = 0; i < function->params->size; i++) {
		printIndent(indent+2);
		printf("Param %d\n", i);
		printExpression((Expression*)function->params->array[i], indent+4);
	}
	printIndent(indent+1);
	printf("Return Type: %d\n", function->returnType);
}

void printAssignment(Assignment* assign, int indent) {
    if (!assign) return;
    printIndent(indent);
    printf("Assignment:\n");
    printVariable(assign->variable, indent + 1);
    printExpression(assign->expression, indent + 1);
}

void printBinOperation(BinOperation* binOp, int indent) {
    if (!binOp) return;
    printIndent(indent);
    printf("BinaryOp: %s\n", opTypeToString(binOp->type));
    printExpression(binOp->left, indent + 1);
    printExpression(binOp->right, indent + 1);
}

void printUnaryOperation(UnaryOperation* unOp, int indent) {
    if (!unOp) return;
    printIndent(indent);
    printf("UnaryOp: %s\n", unaryOpTypeToString(unOp->type));

    printExpression(unOp->right, indent + 1);
}

void printExpression(Expression* expr, int indent) {
    if (expr == NULL) {
		printIndent(indent);
		printf("(Null Expression)\n");
		return;
	}
    switch (expr->type) {
	case EXPR_WRAPPER_EXPR:
	    printExpression(expr->as.expWrap, indent);
	    break;
	case FUNCTIONCALL_EXPR:
	    printFunctionCall(expr->as.functionCall, indent);
	    break;
        case BINOP_EXPR:
            printBinOperation(expr->as.binop, indent);
            break;
        case ASSIGN_EXPR:
            printAssignment(expr->as.assignment, indent);
            break;
	case UNARY_EXPR:
	    printUnaryOperation(expr->as.unop, indent);
	    break;
        case VARIABLE_EXPR:
            printVariable(expr->as.variable, indent);
            break;
        case VALUE_EXPR:
            printValue(expr->as.value, indent);
            break;
    }
}

void printIfStmt(IfStmt* ifStmt, int indent) {
    if (!ifStmt) return;
    printIndent(indent);
    printf("IfStmt:\n");

    printIndent(indent + 1);
    printf("Condition:\n");
    printExpression(ifStmt->condition, indent + 2);

    printIndent(indent + 1);
    printf("Then Branch:\n");
    printBlockStmt(ifStmt->trueBody, indent + 2);

    if (ifStmt->type != ONLYIF) {
        printIndent(indent + 1);

	if (ifStmt->type == IF_ELSE) {
        printf("Else Branch:\n");
        printBlockStmt(ifStmt->as.ifElse, indent + 2);
	}
	else {
	printf("Else If Branch:\n");
	printIfStmt(ifStmt->as.ifElseIf, indent + 2);
	}
    }
}

void printWhileStmt(WhileStmt* whileStmt, int indent) {
    if (!whileStmt) return;
    printIndent(indent);
    printf("WhileStmt:\n");

    printIndent(indent + 1);
    printf("Condition:\n");
    printExpression(whileStmt->condition, indent + 2);

    printIndent(indent + 1);
    printf("Body:\n");
    printBlockStmt(whileStmt->body, indent + 2);
}

void printBlockStmt(BlockStmt* block, int indent) {
    if (!block) return;
    printIndent(indent);
    printf("BlockStmt:\n");
    for (int i = 0; i < block->stmts->size; i++) {
        printStatement(block->stmts->array[i], indent + 1);
    }
}

void printFunctionStmt(FunctionStmt* function, int indent) {
	if (!function) return;
	printIndent(indent);
	printf("FunctionStmt:\n");
	printIndent(indent+1);
	printf("Id: %s\n", function->id);
	printIndent(indent+1);
	printf("Params:\n");
	for (int i = 0; i < function->params->size; i++) {
		printIndent(indent+2);
		printf("Param %d\n", i);
		printIndent(indent+3);
		printf("Id:   %s\n", ((Variable*)(function->params->array[i]))->id);
		printIndent(indent+3);
		printf("Type: %d\n", ((Variable*)(function->params->array[i]))->type);
	}
	printBlockStmt(function->blockStmt, indent+1);
	printIndent(indent+1);
	printf("Return Type: %d\n", function->returnType);
}

void printStatement(Statement* stmt, int indent) {
    if (!stmt) return;
    switch (stmt->type) {
        case EXPRESSION_STMT:
            printIndent(indent);
            printf("ExpressionStmt:\n");
            printIndent(indent);
	    printf("Expression Valuetype: %d\n", stmt->as.expression->valueType);
            printExpression(stmt->as.expression, indent + 1);
            break;
	case FUNCTION_STMT:
	    printFunctionStmt(stmt->as.function, indent);
	    break;
        case BLOCK_STMT:
            printBlockStmt(stmt->as.blockStmt, indent);
            break;
        case WHILE_STMT:
            printWhileStmt(stmt->as.whileStmt, indent);
            break;
        case IF_STMT:
            printIfStmt(stmt->as.ifStmt, indent);
            break;
	case RETURN_STMT:
	    printIndent(indent);
	    printf("ReturnStmt:\n");
	    printExpression(stmt->as.returnStmt->expression, indent+1);
	    break;
        // Add other statement types like DECLARATION_STMT if needed
        default:
            printIndent(indent);
            printf("Unknown Statement\n");
            break;
    }
}

int main(int argc, char* argv[]) {

	char* filepath = parseArgs(argc, argv);
	char* buffer = readFileToBuffer(filepath);

	if (buffer == NULL) {
		return 1;
	}

	Parser* parser = initializeParser(buffer);

	if (parser == NULL) {
		free(buffer);
		return 1;
	}

	DynamicArray* ast = parseBuffer(parser);

	if (ast == NULL) {
		fprintf(stderr, "Parsing failed\n");
		free(buffer);
		freeParser(parser);
		return 1;
	}
	
	printf("Parsing Success!\n");

	TypeChecker* typeChecker = initializeChecker(ast);

	if (typeChecker == NULL) {
		free(buffer);
		freeParser(parser);
		freeArray(ast);
		return 1;
	}

	if(!checkTypes(typeChecker)) {
		fprintf(stderr, "TypeChecking failed\n");
		free(buffer);
		freeParser(parser);
		freeArray(ast);
		freeChecker(typeChecker);
		return 1;
	}

	printf("TypeChecking Success!\n");

	Codegen* codegen = initializeCodegen(ast);

	if (codegen == NULL) {
		freeChecker(typeChecker);
		freeParser(parser);
		freeArray(ast);
		free(buffer);
		return 1;
	}

	if (!generate(codegen)) {
		fprintf(stderr, "Generating Failed!\n");
		freeChecker(typeChecker);
		freeParser(parser);
		freeCodegen(codegen);
		freeArray(ast);
		free(buffer);
		return 1;
	}

	printf("Generation Success!\n\n");
	writeToFile(codegen, "compiled_test.txt");

	for (int i = 0; i < ast->size; i++) {
		printStatement(ast->array[i], 0);
		printf("\n");
	}

	freeParser(parser);
	freeArray(ast);
	freeChecker(typeChecker);
	freeCodegen(codegen);
	free(buffer);
}
