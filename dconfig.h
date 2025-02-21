#pragma once

#include <darray.h>
#include <dhash_map.h>
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <error.h>

typedef struct dconfig_struct dconfig_t;
typedef struct dconfig_token_struct dconfig_token_t;

struct dconfig_struct{
	enum { CFG_NONE, VALUE_INT, VALUE_FLOAT, VALUE_STRING, PARENT_OBJECT, PARENT_ARRAY } type;
	union{
		int64_t value_int;
		double value_float;
		char *value_string;
		dhash_map_t *parent_object; // A dconfig_t hash map.
		darray_t *parent_array; // An array of dconfig_t.
	} content;
};

struct dconfig_token_struct{
	enum { TOK_STOP, TOK_IDENT, TOK_OP, TOK_VALUE } major;
	enum { TOK_EOF, TOK_UNKNOWN, TOK_NAME, TOK_INT, TOK_STRING, TOK_FLOAT, TOK_OP_EQ, TOK_OP_OPSQBR, TOK_OP_CLSQBR, TOK_OP_OPCUBR, TOK_OP_CLCUBR, TOK_OP_COMMA } type;
	union{
		char *tok_name;
		int64_t tok_int;
		char *tok_string;
		double tok_float;
	} content;
	int row;
	int column;
};

#define _forceinline inline __attribute__((always_inline))
_forceinline int _dconfig_index_to_column_index(const char *text, int index){
	int i, column = 1;

	for(i = 0; text[i]; i++){
		if(text[i] == '\n'){
			column = 1;
		}

		if(i == index){
			return column;
		}

		column++;
	}

	return -1;
}

const char *token_type_to_string[] = { "file end", "token unknown", "name", "int", "string", "float", "equals", "open square bracket", "close square bracket", "open curly bracket", "close curly bracket", "comma seperator" };

_forceinline int _dconfig_index_to_row_index(const char *text, int index){
	int i, row = 1;

	for(i = 0; text[i]; i++){
		if(text[i] == '\n'){
			row++;
		}

		if(i == index){
			return row;
		}
	}

	return -1;
}

static inline dconfig_t *dconfig_resolve(dconfig_t *parent, int n, ...){
	va_list args;
	dconfig_t *node;

	va_start(args, n);

	node = parent;

	for(int i = 0; i < n; i++){
		if(node == NULL)
			return NULL;

		node = dhash_get(&node->content.parent_object, va_arg(args, char *));
	}

	va_end(args);

	return node;
}

static inline dconfig_t *dconfig_resolve_array(dconfig_t *parent, int i, int n, ...){
	va_list args;
	dconfig_t *node;

	va_start(args, n);

	node = parent;

	for(int i = 0; i < n; i++){
		if(node == NULL)
			return NULL;

		node = dhash_get(&node->content.parent_object, va_arg(args, char *));
	}

	if(node == NULL)
		return NULL;

	node = (dconfig_t *)darray_get(&node->content.parent_array, i);

	va_end(args);

	return node;
}

_forceinline dconfig_token_t _dconfig_get_token(const char *text, int *i, int text_length){
	dconfig_token_t ret;
	int tok_name_sz, tok_string_sz;
	char hadicate, *tok_name, *tok_string, *tok_int_last;
	double tok_float;
	int64_t tok_int;

	while(*i < text_length && isspace(text[*i]))
		(*i)++;
	if(*i >= text_length)
		return (dconfig_token_t) {.major = TOK_STOP, .type=TOK_EOF, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};

	if(isalpha(text[*i]) || text[*i] == '_' || text[*i] == '-'){
		tok_name_sz = 0;
		tok_name = malloc(tok_name_sz);

		while(*i < text_length && isalnum(text[*i]) || text[*i] == '_' || text[*i] == '-'){
			tok_name = realloc(tok_name, tok_name_sz + 1);
			tok_name[tok_name_sz++] = text[*i];
			(*i)++;
		}
		tok_name = realloc(tok_name, tok_name_sz + 1);
		tok_name[tok_name_sz] = 0;

		ret = (dconfig_token_t) {.major = TOK_IDENT, .type = TOK_NAME, .content.tok_name = tok_name, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
	}
	else if(isdigit(text[*i])){
		tok_int = strtoll(text + *i, &tok_int_last, 0);

		if(text[tok_int_last - text] == '.'){
			tok_float = strtod(text + *i, &tok_int_last);

			*i = tok_int_last - text;

			ret = (dconfig_token_t) {.major = TOK_VALUE, .type = TOK_FLOAT, .content.tok_float = tok_float, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
		}
		else{
			*i = tok_int_last - text;

			ret = (dconfig_token_t) {.major = TOK_VALUE, .type = TOK_INT, .content.tok_int = tok_int, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
		}
	}
	else if(text[*i] == '"' || text[*i] == '\''){
		hadicate = text[*i];
		(*i)++;

		tok_string_sz = 0;
		tok_string = malloc(tok_string_sz);

		while(*i < text_length && text[*i] != hadicate){
			if(text[*i] == '\\'){
				(*i)++;
				if(*i < text_length){
					switch(text[*i]){
						case 'n':
							(*i)++;
							tok_string = realloc(tok_string, tok_string_sz + 1);
							tok_string[tok_string_sz++] = '\n';
							break;
						case 't':
							(*i)++;
							tok_string = realloc(tok_string, tok_string_sz + 1);
							tok_string[tok_string_sz++] = '\t';
							break;
						case 'r':
							(*i)++;
							tok_string = realloc(tok_string, tok_string_sz + 1);
							tok_string[tok_string_sz++] = '\r';
							break;
						default:		
							tok_string = realloc(tok_string, tok_string_sz + 1);
							tok_string[tok_string_sz++] = text[*i];
							(*i)++;
							break;
					}
				}
			}
			else{
				tok_string = realloc(tok_string, tok_string_sz + 1);
				tok_string[tok_string_sz++] = text[*i];
				(*i)++;
			}
		}
		tok_string = realloc(tok_string, tok_string_sz + 1);
		tok_string[tok_string_sz] = 0;

		(*i)++;

		ret = (dconfig_token_t) {.major = TOK_VALUE, .type = TOK_STRING, .content.tok_string = tok_string, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
	}
	else{
		switch(text[*i]){
			case '[':
				ret = (dconfig_token_t) {.major = TOK_OP, .type = TOK_OP_OPSQBR, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
				break;
			case ']':
				ret = (dconfig_token_t) {.major = TOK_OP, .type = TOK_OP_CLSQBR, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
				break;
			case '{':
				ret = (dconfig_token_t) {.major = TOK_OP, .type = TOK_OP_OPCUBR, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
				break;
			case '}':
				ret = (dconfig_token_t) {.major = TOK_OP, .type = TOK_OP_CLCUBR, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
				break;
			case '=':
				ret = (dconfig_token_t) {.major = TOK_OP, .type = TOK_OP_EQ, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
				break;
			case ',':
				ret = (dconfig_token_t) {.major = TOK_OP, .type = TOK_OP_COMMA, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
				break;
			default:
				return (dconfig_token_t) {.major = TOK_STOP, .type=TOK_UNKNOWN, .row = _dconfig_index_to_row_index(text, *i), .column = _dconfig_index_to_column_index(text, *i)};
				break;
		}

		(*i)++;
	}

	return ret;
}

static inline int _dconfig_get_statement(dconfig_t *parent, dconfig_token_t *token, const char *text, int *i, int text_length);

static inline int _dconfig_get_value(dconfig_t *parent, dconfig_token_t *token, const char *text, int *i, dconfig_t *temp, int text_length){
	dconfig_t temp_0;
	int status;
	if(token->major == TOK_VALUE){
		switch(token->type){
			case TOK_STRING:
				*temp = (dconfig_t) {.type = VALUE_STRING, .content.value_string = token->content.tok_string};
				break;
			case TOK_FLOAT:
				*temp = (dconfig_t) {.type = VALUE_FLOAT, .content.value_float = token->content.tok_float};
				break;
			case TOK_INT:
				*temp = (dconfig_t) {.type = VALUE_INT, .content.value_int = token->content.tok_int};
				break;
			default:
				break;
		}
	}
	else if(token->type == TOK_OP_OPSQBR){
		*temp = (dconfig_t) {.type = PARENT_ARRAY, .content.parent_array = darray_new(sizeof(dconfig_t))};

		*token = _dconfig_get_token(text, i, text_length);

		while(token->major != TOK_STOP && token->type != TOK_OP_CLSQBR){
			status = _dconfig_get_value(temp, token, text, i, &temp_0, text_length);
			if(status == 1)
				return 1;

			darray_add(&temp->content.parent_array, &temp_0);
		}

		if(token->type == TOK_UNKNOWN) error(1, 0, "<dconfig.h> [%d, %d] Invalid statement (unknown token).\n", token->row, token->column);
	}
	else{
		*temp = (dconfig_t) {.type = PARENT_OBJECT, .content.parent_object = dhash_new(sizeof(dconfig_t))};

		*token = _dconfig_get_token(text, i, text_length);

		while(token->major != TOK_STOP && token->type != TOK_OP_CLCUBR){
			status = _dconfig_get_statement(temp, token, text, i, text_length);
			if(status == 1)
				return 1;
		}

		if(token->type == TOK_UNKNOWN) error(1, 0, "<dconfig.h> [%d, %d] Invalid statement (unknown token).\n", token->row, token->column);
	}

	*token = _dconfig_get_token(text, i, text_length);

	return 0;
}

static inline int _dconfig_get_statement(dconfig_t *parent, dconfig_token_t *token, const char *text, int *i, int text_length){
	dconfig_t temp;
	char *attribute_nm, *value_string;
	int64_t value_int;
	double value_float;
	int status, n, specifier = 0;

	if(*i >= text_length){
		return 1;
	}

	if(token->type != TOK_NAME) error(1, 0, "<dconfig.h> [%d, %d] Invalid statement expected name, got, %s.\n", token->row, token->column, token_type_to_string[token->type]);
	attribute_nm = token->content.tok_name;

	*token = _dconfig_get_token(text, i, text_length);
	if(token->type != TOK_OP_EQ) error(1, 0, "<dconfig.h> [%d, %d] Invalid statement expected equals, got, %s.\n", token->row, token->column, token_type_to_string[token->type]);

	*token = _dconfig_get_token(text, i, text_length);
	if(token->major != TOK_VALUE && token->type != TOK_OP_OPSQBR && token->type != TOK_OP_OPCUBR) error(1, 0, "<dconfig.h> [%d, %d] Invalid statement expected value, got, %s.\n", token->row, token->column, token_type_to_string[token->type]);

	status = _dconfig_get_value(parent, token, text, i, &temp, text_length);
	if(status == 1)
		return 1;

	dhash_add(&parent->content.parent_object, attribute_nm, &temp);

	return 0;
}

static _forceinline dconfig_t dconfig_parse_string(const char *text){
	dconfig_t ret;
	dconfig_token_t token;
	int i = 0, text_length, status;

	ret = (dconfig_t) {.type = PARENT_OBJECT, .content.parent_object = dhash_new(sizeof(dconfig_t))};

	text_length = strlen(text);

	token = _dconfig_get_token(text, &i, text_length);

	while(token.major != TOK_STOP && token.type != TOK_EOF){
		status = _dconfig_get_statement(&ret, &token, text, &i, text_length);
		if(status == 1)
			return (dconfig_t) {.type = CFG_NONE};
	}

	if(token.major == TOK_STOP && token.type == TOK_UNKNOWN) error(1, 0, "<dconfig.h> [%d, %d] Invalid statement (unknown token).\n", token.row, token.column);

	return ret;
}
#undef _forceinline

