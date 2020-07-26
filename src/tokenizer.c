#include "cc_sakura.h"

bool isblock(char *str){
	return *str=='{' || *str=='}';
}

bool at_eof(){
	return token->kind==TK_EOF;
}

int is_alnum(char c){
	return	('a'<=c && c<='z')||
		('A'<=c && c<='Z')||
		('0'<=c && c<='9')||
		(c=='_');
}

int len_val(char *str){
	int counter=0;
	for(;(('a' <= *str && *str <= 'z') || ('0' <= *str && *str <= '9'));str++)
		counter++;

	return counter;
}

bool issymbol(char *str, bool *flag){
	int i;
	char single_tokens[]="+-*/&()<>=,;[]";
	char multi_tokens[]="<=>!";
	int size;
	
	//Is multi token? (<=,==,!=,>=)
	size=sizeof(multi_tokens)/sizeof(char);
	for(i=0;i<size;i++){
		if(*str==multi_tokens[i] && *(str+1)=='='){
			*flag=false;
			return true;
		}
	}
	
	//Is single token? (+,-,*,/,<,>)
	size=sizeof(single_tokens)/sizeof(char);
	for(i=0;i<size;i++){
		if(*str==single_tokens[i]){
			*flag=true;
			return true;
		}
	}

	return false;
}

Token *new_token(TokenKind kind,Token *cur,char *str){
	Token *new=calloc(1,sizeof(Token));
	new->kind=kind;
	//Remaining characters
	new->str=str;
	new->len=1;
	cur->next=new;
	return new;
}

bool consume_reserved(char **p,char *str,int len,Token **now,TokenKind tk_kind){
	if(strncmp(*p,str,len)!=0 || is_alnum((*p)[len]))
		return false;

	*now=new_token(tk_kind,*now,*p);
	(*now)->len=len;
	(*now)->str=*p;
	*p+=len;

	return true;
}

Token *tokenize(char *p){
	bool is_single_token;
	Token head;
	head.next=NULL;

	//set head pointer to cur
	Token *now=&head;

	while(*p){
		if(isspace(*p)){
			p++;
			continue;
		}

		//judge single token or multi token or isn't token
		if(issymbol(p,&is_single_token)){
			now=new_token(TK_RESERVED,now,p);
			if(is_single_token) p++;
			else{
				p+=2;
				now->len=2;
			}
			continue;
		}

		if(consume_reserved(&p,"int",3,&now,TK_TYPE))	   continue;
		if(consume_reserved(&p,"char",4,&now,TK_TYPE))	   continue;
		if(consume_reserved(&p,"if",2,&now,TK_IF))	   continue;
		if(consume_reserved(&p,"else",4,&now,TK_ELSE))	   continue;
		if(consume_reserved(&p,"while",5,&now,TK_WHILE))   continue;
		if(consume_reserved(&p,"sizeof",6,&now,TK_SIZEOF)) continue;
		if(consume_reserved(&p,"return",6,&now,TK_RETURN)) continue;

		//Is block? '{' or '}'
		if(isblock(p)){
			now=new_token(TK_BLOCK,now,p);
			now->len=1;
			now->val=*p;
			now->str=p;
			p++;
			continue;
		}

		//Is string?
		if(*p=='"'){
			p++;
			while(*p!='"')	now=new_token(TK_STR,now,p++);
			p++;
			continue;
		}
		
		//Is valiable?
		if('a'<=*p && *p<='z'){
			now=new_token(TK_IDENT,now,p++);
			now->len=1;
			continue;
		}

		if(isdigit(*p)){
			//add number token
			now=new_token(TK_NUM,now,p);
			//set number
			now->val=strtol(p,&p,10);
			continue;
		}

		error(token->str,"cat not tokenize.");
	}

	//add EOF token
	new_token(TK_EOF,now,p);
	return head.next;
}