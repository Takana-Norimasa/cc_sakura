#include "cc_sakura.h"

LVar *locals;
GVar  *globals;
Struc *structs;
// int alloc_size;
// Token *token;
// LVar *locals;
// Func *func_list[100];


Node *declare_global_variable(int star_count, Token* def_name, Type *toplv_type){
	// if not token -> error
	if(!def_name) error_at(token->str, "not a variable.");

	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_GVAR;

	GVar *gvar = calloc(1, sizeof(GVar));
	gvar->next = globals;
	gvar->name = def_name->str;
	gvar->len  = def_name->len;
	gvar->type = toplv_type;

	// add type list
	Type *newtype = gvar->type;
	for(int i = 0;i<star_count;i++){
		newtype->ptr_to = calloc(1, sizeof(Type));
		newtype->ptr_to->ty = newtype->ty;
		newtype->ty = PTR;
		newtype = newtype->ptr_to;
	}

	if(star_count == 0) newtype->ptr_to = calloc(1, sizeof(Type));

	// Is array
	if(consume("[")){
		int isize = -1;
		node->val = -1;
		node->kind = ND_GARRAY;

		if(!check("]")){
			// body
			isize = token->val;
			gvar->memsize = align_array_size(token->val, gvar->type->ptr_to->ty);
			token = token->next;
		}

		gvar->type->ptr_to = calloc(1, sizeof(Type));
		gvar->type->ptr_to->ty = gvar->type->ty;
		gvar->type->index_size = isize;
		gvar->type->ty = ARRAY;
		expect("]");
	}else{
		gvar->memsize = type_size(gvar->type->ty);
	}

	// globals == new lvar
	globals = gvar;

	node->type = gvar->type;
	node->str  = gvar->name;
	node->val  = gvar->len;

	return node;
}

Node *declare_local_variable(Node *node, Token *tok, int star_count){
	int i;

	LVar *lvar = find_lvar(tok);
	if(lvar) error_at(token->str, "this variable has already existed.");

	lvar = calloc(1, sizeof(LVar));
	lvar->next = locals;
	lvar->name = tok->str;
	lvar->len  = tok->len;
	lvar->type = node->type;

	// add type list
	Type *newtype = lvar->type;
	for(i = 0;i<star_count;i++){
		newtype->ptr_to = calloc(1, sizeof(Type));
		newtype->ptr_to->ty = newtype->ty;
		newtype->ty = PTR;
		newtype = newtype->ptr_to;
	}

	if(star_count == 0){
		newtype->ptr_to = NULL;
	}


	// Is array
	if(consume("[")){
		int isize = -1;
		node->val = -1;
		node->kind = ND_LARRAY;

		lvar->type->ptr_to = calloc(1, sizeof(Type));
		lvar->type->ptr_to->ty = lvar->type->ty;
		lvar->type->ty = ARRAY;

		//if(*(token->str)!=']'){
		if(!check("]")){
			int asize = align_array_size(token->val, lvar->type->ptr_to->ty);
			alloc_size+=asize;
			lvar->offset = ((locals) ? (locals->offset) :0) + asize;
			isize = token->val;
			token = token->next;
		}

		lvar->type->index_size = isize;

		expect("]");
	}else{
		if(locals){
			lvar->offset = (locals->offset)+8;
		}else{
			lvar->offset = 8;
		}
		alloc_size+=8;
	}

	node->type = lvar->type;
	node->offset = lvar->offset;
	// locals == new lvar
	locals = lvar;

	return node;
}


void declare_struct(Struc *new_struc){
	int asize	  = 0;
	int star_count	  = 0;
	Member *new_memb  = NULL;
	Member *memb_head = NULL;

	while(1){
		if(token->kind != TK_TYPE){
			error_at(token->str, "not a type.");
		}

		new_memb = calloc(1,sizeof(Member));
		new_memb->type = calloc(1,sizeof(Type));

		// check type
		if(consume_reserved_word("int", TK_TYPE))	  new_memb->type->ty = INT;
		else if(consume_reserved_word("char", TK_TYPE))   new_memb->type->ty = CHAR;
		else if(consume_reserved_word("struct", TK_TYPE)) new_memb->type->ty = STRUCT;

		// count asterisk
		while(token->kind == TK_RESERVED && *(token->str) == '*'){
			star_count++;
			token = token->next;
		}

		// add type list
		Type *newtype = new_memb->type;
		for(int i = 0;i < star_count;i++){
			newtype->ptr_to	    = calloc(1, sizeof(Type));
			newtype->ptr_to->ty = newtype->ty;
			newtype->ty         = PTR;
			newtype = newtype->ptr_to;
		}

		Token *def_name  = consume_ident();
		new_memb->name   = def_name->str;
		new_memb->len    = def_name->len;
		new_memb->offset = ((memb_head)? memb_head->offset : 0) + type_size(new_memb->type->ty);
		asize += new_memb->offset;

		new_memb->next = memb_head;
		memb_head      = new_memb;

		expect(";");
		if(consume("}")) break;
	}
	
	asize = (asize%8) ? asize/8*8+8 : asize;
	new_struc->memsize = asize;
	new_struc->member  = memb_head;
	new_struc->next	   = structs;
	structs = new_struc;

	return;
}