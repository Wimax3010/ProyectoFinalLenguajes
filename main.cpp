#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

// =============================================================================
// 1. ABSTRACT SYNTAX TREE (AST) DEFINITION
// =============================================================================
enum NodeType { NODE_PROGRAM, NODE_RULE, NODE_AND, NODE_COMPARISON, NODE_FACT };

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual NodeType getType() = 0;
};

class ConditionNode : public ASTNode {
public:
    virtual bool evaluate(const map<string, int>& vars, const set<string>& facts) = 0;
    virtual string toString() = 0;
};

class ProgramNode : public ASTNode {
public:
    vector<ASTNode*> rules;
    NodeType getType() override { return NODE_PROGRAM; }
    ~ProgramNode() override {
        for (auto r : rules) delete r;
    }
};

class RuleNode : public ASTNode {
public:
    string id;
    ConditionNode* condition;
    string action;
    NodeType getType() override { return NODE_RULE; }
    ~RuleNode() override {
        delete condition;
    }
};

class AndConditionNode : public ConditionNode {
public:
    ConditionNode* left;
    ConditionNode* right;
    AndConditionNode(ConditionNode* l, ConditionNode* r) : left(l), right(r) {}
    NodeType getType() override { return NODE_AND; }
    bool evaluate(const map<string, int>& vars, const set<string>& facts) override {
        return left->evaluate(vars, facts) && right->evaluate(vars, facts);
    }
    string toString() override {
        return "(" + left->toString() + " AND " + right->toString() + ")";
    }
    ~AndConditionNode() override {
        delete left;
        delete right;
    }
};

class ComparisonNode : public ConditionNode {
public:
    string id;
    string op;
    int value;
    ComparisonNode(string i, string o, int v) : id(i), op(o), value(v) {}
    NodeType getType() override { return NODE_COMPARISON; }
    bool evaluate(const map<string, int>& vars, const set<string>& facts) override {
        auto it = vars.find(id);
        if (it == vars.end()) return false;
        int varVal = it->second;
        if (op == ">") return varVal > value;
        if (op == "<") return varVal < value;
        if (op == "=") return varVal == value;
        return false;
    }
    string toString() override {
        return id + op + to_string(value);
    }
};

class FactNode : public ConditionNode {
public:
    string id;
    explicit FactNode(string i) : id(std::move(i)) {}
    NodeType getType() override { return NODE_FACT; }
    bool evaluate(const map<string, int>& /*vars*/, const set<string>& facts) override {
        return facts.find(id) != facts.end();
    }
    string toString() override {
        return id;
    }
};

// =============================================================================
// 2. LEXICAL ANALYSIS (LEXER)
// =============================================================================
enum TokenType {
    TOKEN_RULE, TOKEN_IF, TOKEN_THEN, TOKEN_AND,
    TOKEN_COLON, TOKEN_GT, TOKEN_LT, TOKEN_EQ,
    TOKEN_ID, TOKEN_VALUE, TOKEN_EOF, TOKEN_ERROR
};

struct Token {
    TokenType type;
    string lexeme;
    int value;
};

class Lexer {
    string src;
    size_t pos;
public:
    explicit Lexer(string s) : src(std::move(s)), pos(0) {}

    Token nextToken() {
        while (pos < src.size() && isspace(src[pos])) {
            pos++;
        }
        if (pos >= src.size()) return {TOKEN_EOF, "", 0};

        char c = src[pos];
        if (c == ':') { pos++; return {TOKEN_COLON, ":", 0}; }
        if (c == '>') { pos++; return {TOKEN_GT, ">", 0}; }
        if (c == '<') { pos++; return {TOKEN_LT, "<", 0}; }
        if (c == '=') { pos++; return {TOKEN_EQ, "=", 0}; }

        if (isalpha(c)) {
            string ident;
            while (pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')) {
                ident += src[pos];
                pos++;
            }
            if (ident == "rule") return {TOKEN_RULE, ident, 0};
            if (ident == "if") return {TOKEN_IF, ident, 0};
            if (ident == "then") return {TOKEN_THEN, ident, 0};
            if (ident == "AND") return {TOKEN_AND, ident, 0};
            return {TOKEN_ID, ident, 0};
        }

        if (isdigit(c)) {
            string valStr;
            while (pos < src.size() && isdigit(src[pos])) {
                valStr += src[pos];
                pos++;
            }
            return {TOKEN_VALUE, valStr, stoi(valStr)};
        }

        pos++;
        return {TOKEN_ERROR, string(1, c), 0};
    }
};

// =============================================================================
// 3. SYNTACTIC ANALYSIS (PARSER - LL(1) Optimized)
// =============================================================================
class Parser {
    Lexer lexer;
    Token currentToken;

    void advance() {
        currentToken = lexer.nextToken();
    }

    void expect(TokenType type, const string& msg) {
        if (currentToken.type != type) {
            cerr << "Syntax Error: " << msg << " Got: '" << currentToken.lexeme << "'" << endl;
            exit(1);
        }
        advance();
    }

public:
    explicit Parser(const string& src) : lexer(src) { advance(); }

    ProgramNode* parseProgram() {
        auto* prog = new ProgramNode();
        while (currentToken.type == TOKEN_RULE) {
            prog->rules.push_back(parseRule());
        }
        return prog;
    }

    RuleNode* parseRule() {
        auto* rule = new RuleNode();
        expect(TOKEN_RULE, "Expected 'rule' keyword");

        if (currentToken.type != TOKEN_ID) {
            cerr << "Syntax Error: Expected rule identifier" << endl;
            exit(1);
        }
        rule->id = currentToken.lexeme;
        advance();

        expect(TOKEN_COLON, "Expected ':' after rule identifier");
        expect(TOKEN_IF, "Expected 'if' keyword");

        rule->condition = parseCondition();

        expect(TOKEN_THEN, "Expected 'then' keyword");

        if (currentToken.type != TOKEN_ID) {
            cerr << "Syntax Error: Expected action identifier" << endl;
            exit(1);
        }
        rule->action = currentToken.lexeme;
        advance();

        return rule;
    }

    ConditionNode* parseCondition() {
        ConditionNode* left = parseAtom();
        while (currentToken.type == TOKEN_AND) {
            advance();
            ConditionNode* right = parseAtom();
            left = new AndConditionNode(left, right);
        }
        return left;
    }

    ConditionNode* parseAtom() {
        if (currentToken.type != TOKEN_ID) {
            cerr << "Syntax Error: Expected identifier in condition atom" << endl;
            exit(1);
        }
        string id = currentToken.lexeme;
        advance();

        if (currentToken.type == TOKEN_GT || currentToken.type == TOKEN_LT || currentToken.type == TOKEN_EQ) {
            string op = currentToken.lexeme;
            advance();
            if (currentToken.type != TOKEN_VALUE) {
                cerr << "Syntax Error: Expected integer value after operator" << endl;
                exit(1);
            }
            int val = currentToken.value;
            advance();
            return new ComparisonNode(id, op, val);
        } else {
            return new FactNode(id);
        }
    }
};

// =============================================================================
// 4. ENVIRONMENT AND INPUT PARSING UTILITIES
// =============================================================================
void parseState(const string& stateSrc, map<string, int>& vars, set<string>& initialFacts) {
    stringstream ss(stateSrc);
    string line;
    while (getline(ss, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line == "State:") continue;

        size_t eqPos = line.find('=');
        if (eqPos != string::npos) {
            string varName = line.substr(0, eqPos);
            string varValStr = line.substr(eqPos + 1);

            varName.erase(0, varName.find_first_not_of(" \t"));
            varName.erase(varName.find_last_not_of(" \t") + 1);
            varValStr.erase(0, varValStr.find_first_not_of(" \t"));
            varValStr.erase(varValStr.find_last_not_of(" \t") + 1);

            vars[varName] = stoi(varValStr);
        } else {
            initialFacts.insert(line);
        }
    }
}

// =============================================================================
// 5. MAIN EXECUTION (INTERPRETER & STATIC ANALYSIS)
// =============================================================================
int main() {
    // Read total input from standard input stream
    string fullInput, line;
    while (getline(cin, line)) {
        fullInput += line + "\n";
    }

    // Split Rules and State sections
    size_t statePos = fullInput.find("State:");
    string rulesSrc = (statePos != string::npos) ? fullInput.substr(0, statePos) : fullInput;
    string stateSrc = (statePos != string::npos) ? fullInput.substr(statePos) : "";

    // Parse AST
    Parser parser(rulesSrc);
    ProgramNode* program = parser.parseProgram();

    // Parse State
    map<string, int> variables;
    set<string> activeFacts;
    parseState(stateSrc, variables, activeFacts);

    // Fixed-Point Iteration Engine
    set<string> activatedDuringExecution;
    set<string> firedRules;
    bool changed = true;

    while (changed) {
        changed = false;
        vector<string> factsToActivateThisIteration;
        vector<RuleNode*> rulesFiredThisIteration;

        for (auto node : program->rules) {
            auto* rule = static_cast<RuleNode*>(node);
            if (rule->condition->evaluate(variables, activeFacts)) {
                rulesFiredThisIteration.push_back(rule);
                if (activeFacts.find(rule->action) == activeFacts.end()) {
                    factsToActivateThisIteration.push_back(rule->action);
                }
            }
        }

        for (auto* r : rulesFiredThisIteration) {
            firedRules.insert(r->id);
        }

        for (const string& f : factsToActivateThisIteration) {
            if (activeFacts.insert(f).second) {
                activatedDuringExecution.insert(f);
                changed = true;
            }
        }
    }

    // --- OUTPUT GENERATION ---
    if (activatedDuringExecution.empty()) {
        cout << "(no output)" << endl;
    } else {
        for (const auto& fact : activatedDuringExecution) {
            cout << fact << endl;
        }
    }

    // --- STATIC ANALYSIS MODULE ---
    // 1. Conflicts Analysis
    map<string, set<string>> actionToRules;
    for (auto node : program->rules) {
        auto* rule = static_cast<RuleNode*>(node);
        actionToRules[rule->action].insert(rule->id);
    }
    for (const auto& [actionId, ruleSet] : actionToRules) {
        if (ruleSet.size() > 1) {
            cout << "Action " << actionId << " generated by ";
            bool first = true;
            for (const auto& rId : ruleSet) {
                if (!first) cout << ", ";
                cout << rId;
                first = false;
            }
            cout << endl;
        }
    }

    // 2. Redundancies Analysis
    map<string, set<string>> conditionActionToRules;
    for (auto node : program->rules) {
        auto* rule = static_cast<RuleNode*>(node);
        string key = rule->condition->toString() + " -> " + rule->action;
        conditionActionToRules[key].insert(rule->id);
    }
    for (const auto& [key, ruleSet] : conditionActionToRules) {
        if (ruleSet.size() > 1) {
            cout << "Redundant rules: ";
            bool first = true;
            for (const auto& rId : ruleSet) {
                if (!first) cout << ", ";
                cout << rId;
                first = false;
            }
            cout << endl;
        }
    }

    // 3. Potentially Inactive Rules Analysis
    for (auto node : program->rules) {
        auto* rule = static_cast<RuleNode*>(node);
        if (firedRules.find(rule->id) == firedRules.end()) {
            cout << "Potentially inactive rule: " << rule->id << endl;
        }
    }

    delete program;
    return 0;
}