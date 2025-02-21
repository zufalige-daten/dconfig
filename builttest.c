#include <stdio.h>
#include <stdlib.h>
#include <dconfig.h>
#include <dhash_map.h>
#include <darray.h>
#include <err.h>
#include <assert.h>

int main(void){
	FILE *config_file;
	char *example_config;
	int config_size;
	dconfig_t root;

	config_file = fopen("test.config", "r");
	fseek(config_file, 0, SEEK_END);
	config_size = ftell(config_file);
	fseek(config_file, 0, SEEK_SET);
	example_config = malloc(config_size);
	fread(example_config, config_size, 1, config_file);
	fclose(config_file);

	root = dconfig_parse_string(example_config);
	if(root.type == CFG_NONE)
		error(1, 0, "Config error invalid config.\n");

	printf("text: %s.\nconfig:", example_config);
	printf("%s.\n", dconfig_resolve_array(&root, 0, 2, "program", "londre")->content.value_string);

	return 0;
}

