#include <expat.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 100000

struct config_parse_state {
	XML_Char last_content[BUFFER_SIZE];
	int level;
};


void start_tag(void *data, const XML_Char *tag_name, const XML_Char **attr)
{
	const XML_Char *attr_name  = NULL;
	const XML_Char *attr_id    = NULL;
	const XML_Char *attr_value = NULL;
	struct config_parse_state *state = data;
	unsigned int i;
	printf("\n");

	for ( i = 0; attr[i]; i += 2) {
		if (strcmp(attr[i], "name") == 0)
			attr_name = attr[i + 1];
		if (strcmp(attr[i], "id") == 0)
			attr_id = attr[i + 1];
		else if (strcmp(attr[i], "value") == 0)
			attr_value = attr[i + 1];
	}

	if(state->level ==1) {
		if (!strcmp(tag_name, "ctl")) {
			printf("INITIAL MIXER SETTING:: %s : %s\n", attr_name, attr_value);
		} else if (!strcmp(tag_name, "path")) {
			printf("USECASE:: %s\n", attr_name);
		}
	} else {
		printf("USE CASE:: %s\n", state->last_content);
		printf("Level: %d // Tag: %s // Attr_name: %s // Attr_val: %s\n", state->level, tag_name, attr_name, attr_value);
	}

	//if (strcmp(tag_name, "path") == 0) {
	//	if (attr_name == NULL) {
	//		printf("Barf! unnamed path.\n");
	//	} else {
	//		if (state->level == 1) {
	//			printf("PATH %s\n", attr_name);
	//			//printf("TOP LEVEL!\n");
	//		} else {
	//			printf("NOT LEVEL 1 PATH %s\n", attr_name);
	//		}
	//	}
	//} else if (strcmp(tag_name, "ctl") == 0) {
	//	printf("CTL %s\n", attr_name);
	//}

	if (state->level == 1 && attr_name)
		strcpy(state->last_content, attr_name);
	state->level++;
}

void end_tag(void *data, const XML_Char *el)
{
	struct config_parse_state *state = data;
	//int i;
	//for ( i = 0; i < state->level; i++)
	//	printf(" ");

	//printf("Content of element %s was \"%s\"\n", el, last_content);
	state->level--;
}
		

int xparse_main(int argc, char **argv)
{
	struct config_parse_state state;
	XML_Parser parser;
	FILE *fp;

	fp = fopen("example_android_mixer_paths.xml", "r");
	if (fp == NULL) {
		fprintf(stderr, "Oooft, err opening xml file\n");
		return -1;
	}

	parser = XML_ParserCreate(NULL);
	if (!parser) {
		fprintf(stderr, "Boo! failed to create parser!\n");
		return -1;
	}
	printf("Boo ya!\n");

	memset(&state, 0, sizeof(state));

	XML_SetUserData(parser, &state);
	XML_SetElementHandler(parser, start_tag, end_tag);
	// XML_SetCharacterDataHandler(parser, handle_data);

	char buffer[BUFFER_SIZE + 1];
	memset(buffer, 0, BUFFER_SIZE);
	printf("Strlen(buffer) before parsing: %zu\n", strlen(buffer));
	printf("Buffer size is : %d\n", BUFFER_SIZE);

	size_t items_num = 0;
	items_num = fread(&buffer, sizeof(char), BUFFER_SIZE, fp);
	printf("Read %zu items\n", items_num);
	printf("Strlen(buffer) after parsing: %zu\n", strlen(buffer));
	
	if (XML_Parse(parser, buffer, strlen(buffer), XML_TRUE) == XML_STATUS_ERROR) {
		printf("Errrr! %s\n", XML_ErrorString(XML_GetErrorCode(parser)));
		return -1;
	}

	fclose(fp);
	XML_ParserFree(parser);

	return 0;

}
	
