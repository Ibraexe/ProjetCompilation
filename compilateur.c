#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    TOK_DEBUT, TOK_FIN,
    TOK_INT, TOK_CHAR, TOK_FLOAT, TOK_TABLE,
    TOK_FONCTION, TOK_FINFONCTION,
    TOK_RETOURNER,
    TOK_ECRIRE, TOK_LIRE,
    TOK_POUR, TOK_FINPOUR,
    TOK_TANTQUE, TOK_FINTANTQUE,
    TOK_REPETER,
    TOK_SI, TOK_ALORS, TOK_SINON, TOK_FINSI,
    TOK_DE, TOK_A,
    TOK_ID, TOK_NUM, TOK_REEL, TOK_CHAR_LIT, TOK_STRING,
    TOK_AFFECT,
    TOK_PLUS, TOK_MOINS, TOK_MUL, TOK_DIV,
    TOK_EGAL, TOK_DIFF, TOK_INF, TOK_SUP, TOK_INFEG, TOK_SUPEG,
    TOK_PO, TOK_PF, TOK_VIRGULE,
    TOK_CO, TOK_CF,
    TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    char text[64];
    int line, col;
} Token;

typedef enum { TYPE_INT, TYPE_CHAR, TYPE_FLOAT, TYPE_TABLE } VarType;
typedef enum { SYM_VAR, SYM_FUNC } SymKind;

typedef struct {
    char name[64];
    SymKind kind;
    VarType vtype;
    int arraySize;
    int paramCount;
    VarType paramTypes[10];
    int scope;
} Symbol;

#define MAX_SYMS 256
Symbol symtab[MAX_SYMS];
int symCount = 0;

int currentScope = 0;
int inFunction = 0;

FILE *src, *out;
Token current;
int line = 1, col = 0, indent = 0;

void syn_error(Token t, const char *msg){
    fprintf(stderr,
        "ERREUR SYNTAXIQUE [%d:%d] %s -> '%s'\n",
        t.line, t.col, msg, t.text);
    exit(1);
}

void sem_error(Token t, const char *msg){
    fprintf(stderr,
        "ERREUR SEMANTIQUE [%d:%d] %s -> '%s'\n",
        t.line, t.col, msg, t.text);
    exit(1);
}

void printIndent(){
    for(int i=0;i<indent;i++) fprintf(out,"    ");
}

int findSymbol(const char *name, SymKind kind){
    for(int i=symCount-1;i>=0;i--){
        if(!strcmp(symtab[i].name,name) &&
           (kind==-1 || symtab[i].kind==kind))
            return i;
    }
    return -1;
}

VarType getSymbolType(int idx){
    if(idx<0) return TYPE_INT;
    return symtab[idx].vtype;
}

const char* typeToCStr(VarType t){
    switch(t){
        case TYPE_INT: return "int";
        case TYPE_CHAR: return "char";
        case TYPE_FLOAT: return "float";
        case TYPE_TABLE: return "int";
        default: return "int";
    }
}

void addSymbolTyped(const char *name, SymKind kind, int params, VarType vtype, int arrSize);

void addSymbol(const char *name, SymKind kind, int params){
    addSymbolTyped(name, kind, params, TYPE_INT, 0);
}

void addSymbolTyped(const char *name, SymKind kind, int params, VarType vtype, int arrSize){
    if(kind==SYM_VAR && findSymbol(name,SYM_VAR)!=-1 && currentScope==1)
        sem_error(current,"Double declaration de variable");

    strcpy(symtab[symCount].name,name);
    symtab[symCount].kind=kind;
    symtab[symCount].paramCount=params;
    symtab[symCount].vtype=vtype;
    symtab[symCount].arraySize=arrSize;
    symtab[symCount].scope=currentScope;
    symCount++;
}

void addFunctionSymbol(const char *name, int params, VarType paramTypes[]){
    if(findSymbol(name,SYM_FUNC)!=-1)
        sem_error(current,"Double declaration de fonction");

    strcpy(symtab[symCount].name,name);
    symtab[symCount].kind=SYM_FUNC;
    symtab[symCount].paramCount=params;
    symtab[symCount].vtype=TYPE_INT;
    symtab[symCount].arraySize=0;
    symtab[symCount].scope=currentScope;

    for(int i=0;i<params;i++){
        symtab[symCount].paramTypes[i]=paramTypes[i];
    }

    symCount++;
}

int nextChar(){
    int c = fgetc(src);
    col++;
    if(c=='\n'){ line++; col=0; }
    return c;
}

void unreadChar(int c){
    if(c=='\n') line--;
    else col--;
    ungetc(c,src);
}

Token nextToken(){
    Token t;
    int c;
    do { c = nextChar(); } while(isspace(c));

    t.line=line; t.col=col; t.text[0]=0;

    if(c==EOF){ t.type=TOK_EOF; return t; }

    if(isalpha(c)){
        int i=0;
        while(isalnum(c)){
            t.text[i++]=c;
            c=nextChar();
        }
        t.text[i]=0;
        unreadChar(c);

        if(!strcmp(t.text,"DEBUT")) t.type=TOK_DEBUT;
        else if(!strcmp(t.text,"FIN")) t.type=TOK_FIN;
        else if(!strcmp(t.text,"INT")) t.type=TOK_INT;
        else if(!strcmp(t.text,"CHAR")) t.type=TOK_CHAR;
        else if(!strcmp(t.text,"FLOAT")) t.type=TOK_FLOAT;
        else if(!strcmp(t.text,"TABLE")) t.type=TOK_TABLE;
        else if(!strcmp(t.text,"FONCTION")) t.type=TOK_FONCTION;
        else if(!strcmp(t.text,"FINFONCTION")) t.type=TOK_FINFONCTION;
        else if(!strcmp(t.text,"RETOURNER")) t.type=TOK_RETOURNER;
        else if(!strcmp(t.text,"ECRIRE")) t.type=TOK_ECRIRE;
        else if(!strcmp(t.text,"LIRE")) t.type=TOK_LIRE;
        else if(!strcmp(t.text,"POUR")) t.type=TOK_POUR;
        else if(!strcmp(t.text,"FINPOUR")) t.type=TOK_FINPOUR;
        else if(!strcmp(t.text,"TANTQUE")) t.type=TOK_TANTQUE;
        else if(!strcmp(t.text,"FINTANTQUE")) t.type=TOK_FINTANTQUE;
        else if(!strcmp(t.text,"REPETER")) t.type=TOK_REPETER;
        else if(!strcmp(t.text,"SI")) t.type=TOK_SI;
        else if(!strcmp(t.text,"ALORS")) t.type=TOK_ALORS;
        else if(!strcmp(t.text,"SINON")) t.type=TOK_SINON;
        else if(!strcmp(t.text,"FINSI")) t.type=TOK_FINSI;
        else if(!strcmp(t.text,"DE")) t.type=TOK_DE;
        else if(!strcmp(t.text,"A")) t.type=TOK_A;
        else t.type=TOK_ID;
        return t;
    }

    if(c=='\''){
        c=nextChar();
        t.text[0] = (char)c;
        t.text[1] = 0;
        if(nextChar() != '\'')
            fprintf(stderr,"ERREUR LEXICALE: caractere literal mal forme\n"), exit(1);
        t.type = TOK_CHAR_LIT;
        return t;
    }

    if(c == '"') {
        int i = 0;
        c = nextChar();
        while(c != '"' && c != EOF) {
            t.text[i++] = (char)c;
            c = nextChar();
        }
        if(c != '"') {
            fprintf(stderr, "ERREUR LEXICALE: chaîne non terminée\n");
            exit(1);
        }
        t.text[i] = 0;
        t.type = TOK_STRING;
        return t;
    }

    if(isdigit(c)){
        int i=0;
        int isFloat=0;
        while(isdigit(c) || (c=='.' && !isFloat)){
            if(c=='.') isFloat=1;
            t.text[i++]=c;
            c=nextChar();
        }
        t.text[i]=0;
        unreadChar(c);
        t.type = isFloat ? TOK_REEL : TOK_NUM;
        return t;
    }

    t.text[0]=c; t.text[1]=0;

    if(c == '=') {
        c = nextChar();
        if(c == '=') {
            t.text[1] = '=';
            t.text[2] = 0;
            t.type = TOK_EGAL;
        } else {
            unreadChar(c);
            t.type = TOK_AFFECT;
        }
        return t;
    }
    else if(c == '!') {
        c = nextChar();
        if(c == '=') {
            t.text[1] = '=';
            t.text[2] = 0;
            t.type = TOK_DIFF;
            return t;
        } else {
            unreadChar(c);
            fprintf(stderr, "ERREUR LEXICALE: '!' non suivi de '='\n");
            exit(1);
        }
    }
    else if(c == '<') {
        c = nextChar();
        if(c == '=') {
            t.text[1] = '=';
            t.text[2] = 0;
            t.type = TOK_INFEG;
        } else {
            unreadChar(c);
            t.type = TOK_INF;
        }
        return t;
    }
    else if(c == '>') {
        c = nextChar();
        if(c == '=') {
            t.text[1] = '=';
            t.text[2] = 0;
            t.type = TOK_SUPEG;
        } else {
            unreadChar(c);
            t.type = TOK_SUP;
        }
        return t;
    }

    switch(c){
        case '[': t.type=TOK_CO; break;
        case ']': t.type=TOK_CF; break;
        case '~': t.type=TOK_AFFECT; break;
        case '+': t.type=TOK_PLUS; break;
        case '-': t.type=TOK_MOINS; break;
        case '*': t.type=TOK_MUL; break;
        case '/': t.type=TOK_DIV; break;
        case '(': t.type=TOK_PO; break;
        case ')': t.type=TOK_PF; break;
        case ',': t.type=TOK_VIRGULE; break;
        default:
            fprintf(stderr,
                "ERREUR LEXICALE [%d:%d] Caractere inconnu -> '%c'\n",
                line,col,c);
            exit(1);
    }
    return t;
}

void eat(TokenType t){
    if(current.type!=t)
        syn_error(current,"Token inattendu");
    current=nextToken();
}

void EXPR_COMPLETE(VarType *outType);
void INSTRUCTION();

void FACT(VarType *outType){
    if(current.type==TOK_PO){
        // Expression entre parenthèses
        fprintf(out,"(");
        eat(TOK_PO);
        EXPR_COMPLETE(outType);
        eat(TOK_PF);
        fprintf(out,")");
    }
    else if(current.type==TOK_ID){
        char name[64];
        strcpy(name,current.text);
        eat(TOK_ID);

        if(current.type==TOK_PO){
            int argCount=0;
            int idx = findSymbol(name,SYM_FUNC);
            if(idx==-1)
                sem_error(current,"Fonction non declaree");

            fprintf(out,"%s(",name);
            eat(TOK_PO);

            if(current.type!=TOK_PF){
                VarType dummy;
                EXPR_COMPLETE(&dummy);

                if(argCount < symtab[idx].paramCount){
                    VarType paramType = symtab[idx].paramTypes[argCount];
                    if(dummy != paramType)
                        sem_error(current,"Type de parametre incorrect");
                }

                argCount++;
                while(current.type==TOK_VIRGULE){
                    fprintf(out,", ");
                    eat(TOK_VIRGULE);
                    EXPR_COMPLETE(&dummy);

                    if(argCount < symtab[idx].paramCount){
                        VarType paramType = symtab[idx].paramTypes[argCount];
                        if(dummy != paramType)
                            sem_error(current,"Type de parametre incorrect");
                    }

                    argCount++;
                }
            }
            eat(TOK_PF);
            fprintf(out,")");

            if(argCount!=symtab[idx].paramCount)
                sem_error(current,"Nombre de parametres incorrect");
            *outType = getSymbolType(idx);
        }
        else if(current.type==TOK_CO){
            int idx = findSymbol(name,SYM_VAR);
            if(idx==-1)
                sem_error(current,"Variable non declaree");
            if(symtab[idx].arraySize==0)
                sem_error(current,"Acces tableau sur variable scalaire");
            eat(TOK_CO);
            fprintf(out,"%s[",name);
            VarType idxType;
            EXPR_COMPLETE(&idxType);
            if(idxType != TYPE_INT)
                sem_error(current,"Indice de tableau doit etre de type INT");
            eat(TOK_CF);
            fprintf(out,"]");
            *outType = getSymbolType(idx);
        }
        else{
            int idx = findSymbol(name,SYM_VAR);
            if(idx==-1)
                sem_error(current,"Variable non declaree");
            *outType = getSymbolType(idx);
            fprintf(out,"%s",name);
        }
    }
    else if(current.type==TOK_NUM){
        fprintf(out,"%s",current.text);
        eat(TOK_NUM);
        *outType = TYPE_INT;
    }
    else if(current.type==TOK_REEL){
        fprintf(out,"%s",current.text);
        eat(TOK_REEL);
        *outType = TYPE_FLOAT;
    }
    else if(current.type==TOK_CHAR_LIT){
        fprintf(out,"'%s'",current.text);
        eat(TOK_CHAR_LIT);
        *outType = TYPE_CHAR;
    }
    else syn_error(current,"Facteur invalide");
}

void TERM(VarType *outType){
    VarType t1;
    FACT(&t1);
    while(current.type==TOK_MUL||current.type==TOK_DIV){
        fprintf(out," %c ",current.type==TOK_MUL?'*':'/');
        eat(current.type);
        VarType t2;
        FACT(&t2);
        if(t1 != t2)
            sem_error(current,"Operation entre types differents (mul/div)");
        t1 = t2;
    }
    *outType = t1;
}

void EXPR(VarType *outType){
    VarType t1;
    TERM(&t1);
    while(current.type==TOK_PLUS||current.type==TOK_MOINS){
        fprintf(out," %c ",current.type==TOK_PLUS?'+':'-');
        eat(current.type);
        VarType t2;
        TERM(&t2);
        if(t1 != t2)
            sem_error(current,"Operation entre types differents (add/sub)");
        t1 = t2;
    }
    *outType = t1;
}

void EXPR_COMPLETE(VarType *outType){
    VarType t1;
    EXPR(&t1);

    if(current.type == TOK_EGAL || current.type == TOK_DIFF ||
       current.type == TOK_INF || current.type == TOK_SUP ||
       current.type == TOK_INFEG || current.type == TOK_SUPEG) {

        TokenType op = current.type;
        const char* opStr;
        switch(op){
            case TOK_EGAL: opStr = "=="; break;
            case TOK_DIFF: opStr = "!="; break;
            case TOK_INF: opStr = "<"; break;
            case TOK_SUP: opStr = ">"; break;
            case TOK_INFEG: opStr = "<="; break;
            case TOK_SUPEG: opStr = ">="; break;
            default: opStr = "==";
        }

        fprintf(out," %s ",opStr);
        eat(current.type);

        VarType t2;
        EXPR(&t2);

        if(t1 != t2)
            sem_error(current,"Comparaison entre types differents");

        *outType = TYPE_INT;
    } else {
        *outType = t1;
    }
}

void AFFECT(){
    char name[64];
    strcpy(name,current.text);
    int symIdx = findSymbol(name,SYM_VAR);
    if(symIdx==-1)
        sem_error(current,"Variable non declaree");

    eat(TOK_ID);
    VarType lhsType;
    printIndent();

    if(current.type==TOK_CO){
        if(symtab[symIdx].arraySize==0)
            sem_error(current,"Acces tableau sur variable scalaire");
        eat(TOK_CO);
        fprintf(out,"%s[",name);
        VarType idxType;
        EXPR_COMPLETE(&idxType);
        if(idxType != TYPE_INT)
            sem_error(current,"Indice de tableau doit etre de type INT");
        eat(TOK_CF);
        fprintf(out,"] = ");
        lhsType = TYPE_INT;
    }
    else{
        lhsType = getSymbolType(symIdx);
        fprintf(out,"%s = ",name);
    }

    eat(TOK_AFFECT);
    VarType rhsType;
    EXPR_COMPLETE(&rhsType);
    if(lhsType != rhsType)
        sem_error(current,"Affectation: types incompatibles");
    fprintf(out,";\n");
}

void RETOURNER(){
    if(!inFunction)
        sem_error(current,"RETOURNER hors fonction");

    eat(TOK_RETOURNER);
    printIndent();
    fprintf(out,"return ");
    VarType dummy;
    EXPR_COMPLETE(&dummy);
    fprintf(out,";\n");
}

void ECRIRE(){
    eat(TOK_ECRIRE);
    printIndent();

    if(current.type == TOK_STRING) {
        fprintf(out, "printf(\"%s\\n\");\n", current.text);
        eat(TOK_STRING);
    } else {
        VarType exprType;

        char *exprBuf = NULL;
        size_t exprLen = 0;
        FILE *mem = open_memstream(&exprBuf, &exprLen);
        FILE *saveOut = out;
        out = mem;
        EXPR_COMPLETE(&exprType);
        fflush(mem);
        fclose(mem);
        out = saveOut;

        const char *fmt = (exprType==TYPE_INT)?"%d":(exprType==TYPE_FLOAT)?"%f":"%c";
        fprintf(out,"printf(\"%s\\n\", %s);\n", fmt, exprBuf ? exprBuf : "");
        if(exprBuf) free(exprBuf);
    }
}

void LIRE(){
    eat(TOK_LIRE);
    eat(TOK_PO);

    char name[64];
    strcpy(name,current.text);
    int symIdx = findSymbol(name,SYM_VAR);
    if(symIdx==-1)
        sem_error(current,"Variable non declaree");
    eat(TOK_ID);

    VarType vtype;
    printIndent();

    if(current.type==TOK_CO){
        if(symtab[symIdx].arraySize==0)
            sem_error(current,"Acces tableau sur variable scalaire");
        eat(TOK_CO);
        fprintf(out,"scanf(\"%%d\", &%s[",name);
        VarType idxType;
        EXPR_COMPLETE(&idxType);
        if(idxType != TYPE_INT)
            sem_error(current,"Indice de tableau doit etre de type INT");
        eat(TOK_CF);
        fprintf(out,"]);\n");
        vtype = TYPE_INT;
    }
    else{
        vtype = getSymbolType(symIdx);
        const char *fmt = (vtype==TYPE_INT)?"%d":(vtype==TYPE_FLOAT)?"%f":" %c";
        fprintf(out,"scanf(\"%s\", &%s);\n", fmt, name);
    }

    eat(TOK_PF);
}

void TANTQUE_BOUCLE(){
    eat(TOK_TANTQUE);
    printIndent();
    fprintf(out,"while(");
    VarType condType;
    EXPR_COMPLETE(&condType);
    fprintf(out,"){\n");
    indent++;
    while(current.type!=TOK_FINTANTQUE)
        INSTRUCTION();
    eat(TOK_FINTANTQUE);
    indent--;
    printIndent();
    fprintf(out,"}\n");
}

void REPETER_BOUCLE(){
    eat(TOK_REPETER);
    printIndent();
    fprintf(out,"do{\n");
    indent++;
    while(current.type!=TOK_TANTQUE)
        INSTRUCTION();
    eat(TOK_TANTQUE);
    printIndent();
    fprintf(out,"} while(");
    VarType condType;
    EXPR_COMPLETE(&condType);
    fprintf(out,");\n");
}

void POUR_BOUCLE(){
    eat(TOK_POUR);

    if(current.type!=TOK_ID)
        syn_error(current,"Identifiant attendu apres POUR");
    char var[64];
    strcpy(var,current.text);
    int idx = findSymbol(var,SYM_VAR);
    if(idx==-1)
        sem_error(current,"Variable de boucle non declaree");
    if(getSymbolType(idx)!=TYPE_INT)
        sem_error(current,"Variable de boucle POUR doit etre de type INT");
    eat(TOK_ID);

    eat(TOK_DE);

    printIndent();
    fprintf(out,"for(%s = ",var);
    VarType tDebut;
    EXPR_COMPLETE(&tDebut);
    if(tDebut!=TYPE_INT)
        sem_error(current,"Borne de debut POUR doit etre de type INT");

    fprintf(out,"; %s <= ",var);
    eat(TOK_A);
    VarType tFin;
    EXPR_COMPLETE(&tFin);
    if(tFin!=TYPE_INT)
        sem_error(current,"Borne de fin POUR doit etre de type INT");
    fprintf(out,"; %s++){\n",var);

    indent++;
    while(current.type!=TOK_FINPOUR)
        INSTRUCTION();
    eat(TOK_FINPOUR);
    indent--;
    printIndent();
    fprintf(out,"}\n");
}

void SI_CONDITION(){
    eat(TOK_SI);
    printIndent();
    fprintf(out,"if(");

    VarType condType;
    EXPR_COMPLETE(&condType);

    eat(TOK_ALORS);
    fprintf(out,"){\n");

    indent++;
    while(current.type != TOK_SINON && current.type != TOK_FINSI){
        INSTRUCTION();
    }

    if(current.type == TOK_SINON){
        eat(TOK_SINON);
        indent--;
        printIndent();
        fprintf(out,"} else ");

        if(current.type == TOK_SI){
            SI_CONDITION();
        } else {
            fprintf(out,"{\n");
            indent++;
            while(current.type != TOK_FINSI){
                INSTRUCTION();
            }
            eat(TOK_FINSI);
            indent--;
            printIndent();
            fprintf(out,"}\n");
        }
    } else {
        eat(TOK_FINSI);
        indent--;
        printIndent();
        fprintf(out,"}\n");
    }
}

void INSTRUCTION(){
    if(current.type==TOK_ID) AFFECT();
    else if(current.type==TOK_RETOURNER) RETOURNER();
    else if(current.type==TOK_ECRIRE) ECRIRE();
    else if(current.type==TOK_LIRE) LIRE();
    else if(current.type==TOK_TANTQUE) TANTQUE_BOUCLE();
    else if(current.type==TOK_REPETER) REPETER_BOUCLE();
    else if(current.type==TOK_POUR) POUR_BOUCLE();
    else if(current.type==TOK_SI) SI_CONDITION();
    else syn_error(current,"Instruction inconnue");
}

void FONCTION_DECL(){
    eat(TOK_FONCTION);

    char fname[64];
    strcpy(fname,current.text);
    eat(TOK_ID);

    int paramCount=0;
    VarType paramTypes[10];
    char paramNames[10][64];
    int paramStartIdx = symCount;

    eat(TOK_PO);
    if(current.type==TOK_INT||current.type==TOK_CHAR||current.type==TOK_FLOAT){
        VarType ptype;
        if(current.type==TOK_INT){
            eat(TOK_INT);
            ptype=TYPE_INT;
        }
        else if(current.type==TOK_CHAR){
            eat(TOK_CHAR);
            ptype=TYPE_CHAR;
        }
        else{
            eat(TOK_FLOAT);
            ptype=TYPE_FLOAT;
        }

        strcpy(paramNames[paramCount],current.text);
        paramTypes[paramCount]=ptype;

        addSymbolTyped(current.text,SYM_VAR,0,ptype,0);
        paramCount++;
        eat(TOK_ID);

        while(current.type==TOK_VIRGULE){
            eat(TOK_VIRGULE);

            if(current.type==TOK_INT){
                eat(TOK_INT);
                ptype=TYPE_INT;
            }
            else if(current.type==TOK_CHAR){
                eat(TOK_CHAR);
                ptype=TYPE_CHAR;
            }
            else if(current.type==TOK_FLOAT){
                eat(TOK_FLOAT);
                ptype=TYPE_FLOAT;
            }
            else syn_error(current,"Type de parametre attendu");

            strcpy(paramNames[paramCount],current.text);
            paramTypes[paramCount]=ptype;

            addSymbolTyped(current.text,SYM_VAR,0,ptype,0);
            paramCount++;
            eat(TOK_ID);
        }
    }
    eat(TOK_PF);

    addFunctionSymbol(fname,paramCount,paramTypes);

    fprintf(out,"int %s(",fname);
    for(int i=0;i<paramCount;i++){
        if(i) fprintf(out,", ");
        fprintf(out,"%s %s", typeToCStr(paramTypes[i]), paramNames[i]);
    }
    fprintf(out,"){\n");

    inFunction=1;
    currentScope=1;
    indent++;

    while(current.type==TOK_INT||current.type==TOK_CHAR||current.type==TOK_FLOAT||current.type==TOK_TABLE){
        VarType vtype;
        int isTable = 0;

        if(current.type==TOK_INT){
            eat(TOK_INT);
            vtype=TYPE_INT;
        }
        else if(current.type==TOK_CHAR){
            eat(TOK_CHAR);
            vtype=TYPE_CHAR;
        }
        else if(current.type==TOK_FLOAT){
            eat(TOK_FLOAT);
            vtype=TYPE_FLOAT;
        }
        else{
            eat(TOK_TABLE);
            isTable = 1;

            if(current.type==TOK_INT){
                eat(TOK_INT);
                vtype=TYPE_INT;
            }
            else if(current.type==TOK_CHAR){
                eat(TOK_CHAR);
                vtype=TYPE_CHAR;
            }
            else if(current.type==TOK_FLOAT){
                eat(TOK_FLOAT);
                vtype=TYPE_FLOAT;
            }
            else syn_error(current,"Type de tableau attendu (INT, FLOAT, CHAR) apres TABLE");
        }

        char name[64];
        strcpy(name,current.text);
        eat(TOK_ID);

        int arrSize = 0;
        if(isTable || current.type==TOK_CO){
            if(current.type==TOK_CO){
                eat(TOK_CO);
                if(current.type!=TOK_NUM)
                    syn_error(current,"Taille de tableau doit etre une constante");
                arrSize = atoi(current.text);
                eat(TOK_NUM);
                eat(TOK_CF);
            }
            else if(isTable){
                syn_error(current,"Crochets attendus pour declaration de tableau");
            }
        }

        addSymbolTyped(name,SYM_VAR,0,vtype,arrSize);
        printIndent();
        fprintf(out,"%s %s", typeToCStr(vtype), name);
        if(arrSize>0) fprintf(out,"[%d]",arrSize);
        fprintf(out,";\n");
    }

    while(current.type!=TOK_FINFONCTION)
        INSTRUCTION();

    eat(TOK_FINFONCTION);

    indent--;
    fprintf(out,"}\n\n");

    inFunction=0;
    currentScope=0;
}

void PROGRAM(){
    fprintf(out,"#include <stdio.h>\n\n");

    while(current.type==TOK_FONCTION)
        FONCTION_DECL();

    fprintf(out,"int main(){\n");
    indent++;

    eat(TOK_DEBUT);

    while(current.type==TOK_INT||current.type==TOK_CHAR||current.type==TOK_FLOAT||current.type==TOK_TABLE){
        VarType vtype;
        int isTable = 0;

        if(current.type==TOK_INT){
            eat(TOK_INT);
            vtype=TYPE_INT;
        }
        else if(current.type==TOK_CHAR){
            eat(TOK_CHAR);
            vtype=TYPE_CHAR;
        }
        else if(current.type==TOK_FLOAT){
            eat(TOK_FLOAT);
            vtype=TYPE_FLOAT;
        }
        else{
            eat(TOK_TABLE);
            isTable = 1;

            if(current.type==TOK_INT){
                eat(TOK_INT);
                vtype=TYPE_INT;
            }
            else if(current.type==TOK_CHAR){
                eat(TOK_CHAR);
                vtype=TYPE_CHAR;
            }
            else if(current.type==TOK_FLOAT){
                eat(TOK_FLOAT);
                vtype=TYPE_FLOAT;
            }
            else syn_error(current,"Type de tableau attendu (INT, FLOAT, CHAR) apres TABLE");
        }

        char name[64];
        strcpy(name,current.text);
        eat(TOK_ID);

        int arrSize = 0;
        if(isTable || current.type==TOK_CO){
            if(current.type==TOK_CO){
                eat(TOK_CO);
                if(current.type!=TOK_NUM)
                    syn_error(current,"Taille de tableau doit etre une constante");
                arrSize = atoi(current.text);
                eat(TOK_NUM);
                eat(TOK_CF);
            }
            else if(isTable){
                syn_error(current,"Crochets attendus pour declaration de tableau");
            }
        }

        addSymbolTyped(name,SYM_VAR,0,vtype,arrSize);
        printIndent();
        fprintf(out,"%s %s", typeToCStr(vtype), name);
        if(arrSize>0) fprintf(out,"[%d]",arrSize);
        fprintf(out,";\n");
    }

    while(current.type!=TOK_FIN)
        INSTRUCTION();

    eat(TOK_FIN);

    indent--;
    fprintf(out,"    return 0;\n}\n");
}

int main(int argc, char *argv[]) {
    char *input_file = NULL;
    char *output_file = "output.c";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[i + 1];
                i++;
            } else {
                fprintf(stderr, "Error: -o option requires a filename\n");
                return 1;
            }
        } else if (input_file == NULL) {
            input_file = argv[i];
        } else {
            fprintf(stderr, "Error: Unexpected argument '%s'\n", argv[i]);
            return 1;
        }
    }

    if (input_file == NULL) {
        fprintf(stderr, "Usage: %s <input_file> [-o output_file]\n", argv[0]);
        return 1;
    }

    src=fopen(input_file,"r");
    out=fopen(output_file,"w");
    printf("Compilation du fichier %s\n", input_file);

    current=nextToken();
    PROGRAM();

    fclose(src);
    fclose(out);
    printf("Compilation reussie\n");
    printf("Fichier compile: %s\n", output_file);
    return 0;
}
