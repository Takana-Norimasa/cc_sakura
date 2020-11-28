#include "cc_sakura.h"

LVar     *locals;
GVar     *globals;
Struc    *structs;
Enum     *enumerations;
Def_Type *defined_types;
LVar     *outside_lvar;
Struc    *outside_struct;
Enum     *outside_enum;
Def_Type *outside_deftype;
// int alloc_size;
// Token *token;
// LVar *locals;
// Func *func_list[100];

Type *set_type(TypeKind kind){
	Token *tok;
	Enum  *enum_found;
	Struc *struc_found;
	int INSIDE_SCOPE = 1;
	Type *type       = calloc(1, sizeof(Type));

	switch(kind){
		case VOID:
		case CHAR:
		case INT:
		case PTR:
		case ARRAY:
			type->ty = kind;
			break;
		case STRUCT:
			tok         = consume_ident();
			struc_found = find_struc(tok, INSIDE_SCOPE);
			if(struc_found){
				type->size   = struc_found->memsize;
				type->member = struc_found->member;
				type->ty     = STRUCT;
			}else{
				if(consume("{")){
					Struc *new_struc = calloc(1,sizeof(Struc));
					new_struc->len   = tok->len;
					new_struc->name  = tok->str;

					declare_struct(new_struc);
				}else{
					error_at(token->str, "does not exist such struct");
				}
			}
			break;
		case ENUM:
			tok       = consume_ident();
			enum_found = find_enum(tok, INSIDE_SCOPE);
			if(enum_found){
				type->size   = 4;
				type->member = enum_found->member;
				type->ty     = ENUM;
			}else{
				if(consume("{")){
					Enum *new_enum = calloc(1,sizeof(Enum));
					new_enum->len  = tok->len;
					new_enum->name = tok->str;

					declare_enum(new_enum);
				}else{
					error_at(token->str, "does not exist such enum");
				}
			}
			break;
	}

	return type;
}

Type *parse_type(void){
	Type *type;
	int star_count = 0;

	// check type
	if(consume_reserved_word("void", TK_TYPE)){
		type = set_type(VOID);
	}else if(consume_reserved_word("char", TK_TYPE)){
		type = set_type(CHAR);
	}else if(consume_reserved_word("int", TK_TYPE)){
		type = set_type(INT);
	}else if(consume_reserved_word("struct", TK_TYPE)){
		type = set_type(STRUCT);
	}else if(consume_reserved_word("enum", TK_TYPE)){
		type = set_type(ENUM);
	}else{
		int out_of_scope = 0;
		Token *tok       = consume_ident();

		for (Def_Type *var = defined_types;var;var = var->next){
			if(var == outside_deftype) out_of_scope = 1;
			if(out_of_scope) break;
			if(var->len == tok->len && !memcmp(tok->str, var->name, var->len)){
				type = set_type(var->type->ty);
				break;
			}
		}
	}

	type->size  = type_size(type);
	type->align = type_align(type);
	
	// count asterisk
	while(token->kind == TK_RESERVED && *(token->str) == '*'){
		star_count++;
		token = token->next;
	}

	// add ptr
	type = insert_ptr_type(type, star_count);

	return type;
}

Type *insert_ptr_type(Type *prev, int star_count){
	Type *newtype;
	if(star_count){
		for(int i = 0;i<star_count;i++){
			newtype = calloc(1, sizeof(Type));
			newtype->ty     = PTR;
			newtype->size   = type_size(newtype);
			newtype->align  = type_align(newtype);
			newtype->ptr_to = prev;
			prev = newtype;
		}
		
		return newtype;
	}else{
		return prev;
	}
}

Node *declare_global_variable(int star_count, Token* def_name, Type *toplv_type){
	// if not token -> error
	if(!def_name) error_at(token->str, "not a variable.");

	int index_num;
	Type *newtype;
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_GVAR;

	GVar *gvar = calloc(1, sizeof(GVar));
	gvar->next = globals;
	gvar->name = def_name->str;
	gvar->len  = def_name->len;
	gvar->type = toplv_type;
	gvar->type->size  = type_size(toplv_type);
	gvar->type->align = type_align(toplv_type);

	// add type list
	gvar->type = insert_ptr_type(gvar->type, star_count);

	// Is array
	if(check("[")){
		int isize  = -1;
		node->val  = -1;
		node->kind = ND_GARRAY;
		while(consume("[")){
			index_num = -1;
			if(!check("]")){
				// body
				if(isize == -1){
					isize = token->val;
				}else{
					isize *= token->val;
				}
				index_num = token->val;
				token = token->next;
			}

			newtype = calloc(1, sizeof(Type));
			newtype->ty          = ARRAY;
			newtype->ptr_to      = gvar->type;
			newtype->index_size  = index_num;
			newtype->size        = type_size(newtype);
			newtype->align       = type_align(newtype);
			gvar->type = newtype;
			expect("]");
		}
		gvar->memsize = align_array_size(isize, gvar->type);
	}else{
		gvar->memsize  = type_size(gvar->type);
	}

	// globals == new lvar
	globals = gvar;

	node->type = gvar->type;
	node->str  = gvar->name;
	node->val  = gvar->len;

	return node;
}

Node *declare_local_variable(Node *node, Token *tok, int star_count){
	int INSIDE_SCOPE = 1;
	LVar *lvar = find_lvar(tok, INSIDE_SCOPE);
	if(lvar) error_at(token->str, "this variable has already existed.");

	lvar = calloc(1, sizeof(LVar));
	lvar->next = locals;
	lvar->name = tok->str;
	lvar->len  = tok->len;
	lvar->type = node->type;

	// Is array
	if(check("[")){
		Type *newtype;
		int index_num;
		int asize = 0;
		int isize = -1;
		node->kind = ND_LARRAY;
		while(consume("[")){
			index_num = -1;
			if(!check("]")){
				if(isize == -1){
					isize = token->val;
				}else{
					isize *= token->val;
				}
				
				index_num = token->val;
				token     = token->next;
			}

			newtype = calloc(1, sizeof(Type));
			newtype->ty          = ARRAY;
			newtype->ptr_to      = lvar->type;
			newtype->index_size  = index_num;
			newtype->size        = type_size(newtype);
			newtype->align       = type_align(newtype);
			lvar->type = newtype;

			expect("]");
		}

		asize = align_array_size(isize, lvar->type);
		alloc_size += asize;
		lvar->offset = ((locals) ? (locals->offset) : 0) + asize;
	}else{
		if(lvar->type->ty == STRUCT){
			lvar->offset =  (locals) ? (locals->offset) + lvar->type->size : lvar->type->size;
			alloc_size   += lvar->type->size;
		}else{
			lvar->offset =  (locals) ? (locals->offset)+8 : 8;
			alloc_size   += 8;
		}
	}

	node->type = lvar->type;
	node->offset = lvar->offset;
	// locals == new lvar
	locals = lvar;

	return node;
}


void declare_struct(Struc *new_struc){
	int size_of_type;
	int asize	  = 0;
	Member *new_memb  = NULL;
	Member *memb_head = NULL;

	while(1){
		if(token->kind != TK_TYPE){
			error_at(token->str, "not a type.");
		}

		new_memb = calloc(1,sizeof(Member));

		// parse member type
		new_memb->type    = parse_type();
		new_memb->memsize = new_memb->type->size;

		// add member name
		Token *def_name  = consume_ident();
		new_memb->name   = def_name->str;
		new_memb->len    = def_name->len;

		// Is array index
		int isize = -1;
		int index_num;
		Type *newtype;
		while(consume("[")){
			if(isize == -1){
				isize = token->val;
			}else{
				isize *= token->val;
			}
			index_num = token->val;
			token = token->next;

			newtype = calloc(1, sizeof(Type));
			newtype->ty         = ARRAY;
			newtype->ptr_to     = new_memb->type;
			newtype->index_size = index_num;
			newtype->size  = type_size(newtype);
			newtype->align = type_align(newtype);
			new_memb->type = newtype;

			expect("]");
		}

		// align member offset
		int padding = 0;
		if(new_memb->type->ty == ARRAY){
			size_of_type = 8;
		}else if (new_memb->type->ty == STRUCT){
			size_of_type = new_memb->memsize;
		}else{
			size_of_type = new_memb->type->size;
		}

		if(memb_head){
			int prev_tail    = (memb_head) ? (memb_head->offset + memb_head->type->size) : 0;
			padding          = (prev_tail % new_memb->type->align) ? (new_memb->type->align - (prev_tail % new_memb->type->align)) : 0;
			new_memb->offset = prev_tail + padding;
		}else{
			new_memb->offset = 0;
		}
		asize += size_of_type + padding;

		new_memb->next = memb_head;
		memb_head      = new_memb;

		expect(";");
		if(consume("}")) break;
	}
	
	asize = (asize%8) ? asize/8*8+8 : asize;
	new_struc->memsize = asize;
	new_struc->member  = memb_head;
	new_struc->next    = structs;
	structs = new_struc;
}

void declare_enum(Enum *new_enum){
	int counter = 0;
	Member *new_memb  = NULL;
	Member *memb_head = NULL;

	while(1){
		new_memb = calloc(1,sizeof(Member));

		if(token->kind != TK_IDENT) error_at(token->str, "expected ‘,’ or ‘}’");
		// add member name
		Token *def_name  = consume_ident();
		new_memb->name   = def_name->str;
		new_memb->len    = def_name->len;

		if(consume("=")){
			if(token->kind != TK_NUM){
				error_at(token->str, "enumerator value is not an integer constant");
			}

			counter	         = token->val;
			new_memb->offset = counter;
			token            = token->next;
		}else{
			new_memb->offset = counter;
			counter++;
		}

		new_memb->next   = memb_head;
		memb_head        = new_memb;

		if(consume(","));
		if(consume("}")) break;
	}

	new_enum->member = memb_head;
	new_enum->next   = enumerations;
	enumerations     = new_enum;
}
