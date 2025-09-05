# Lumina AST Elements

This document enumerates the different kinds of nodes that compose the Abstract Syntax Tree (AST) used by the Lumina compiler. Each node represents a specific syntactic element that can appear in Lumina source code.

## High level constructs
- **Namespace** – groups related declarations under a common scope.
- **Structure** – user defined type containing variables and methods.
- **AttributeBlock** – uniform block shared for a single draw call.
- **ConstantBlock** – uniform block shared across all pipeline executions.
- **Texture** – declaration of a 2D texture object.
- **PipelineDefinition** – declaration of data flow between shader stages.
- **PipelineBody** – body of a shader stage such as `VertexPass` or `FragmentPass`.
- **Function/Method** – function outside/inside a structure.
- **Include** – `#include` directive.

## Statements
- **Compound** – a list of child statements enclosed in braces.
- **VariableDeclaration** – declares a new variable optionally with an initializer.
- **Assignment** – assigns a value to an existing l-value. Includes compound assignments (+=, -=, ...).
- **IfStatement** – conditional execution with optional `else` branch.
- **ForLoop** – classic `for(initializer; condition; increment)` loop.
- **WhileLoop** – `while(condition)` loop.
- **ReturnStatement** – returns a value from a function or method.
- **DiscardStatement** – discards the current fragment.

## Expressions
- **BinaryExpression** – expression with a binary operator (`+`, `-`, `*`, `/`, etc.).
- **UnaryExpression** – expression with a unary operator (`-`, `!`, `++`, `--`, ...).
- **CallExpression** – function or method invocation.
- **MemberAccess** – access to a member of a structure (dot or arrow).
- **VariableReference** – use of an identifier.
- **Literal** – numeric, boolean or string constant.
- **TernaryExpression** – conditional operator `? :`.
- **LValue** – expression that can appear on the left side of an assignment.
- **RValue** – expression yielding a value.

This list defines the nodes that will be represented in the AST. Every syntactic element of the language is built from these primitives.
