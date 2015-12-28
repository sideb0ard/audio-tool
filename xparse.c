#include <expat.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 100000

struct mixer_setting {
    XML_Char *name;
    XML_Char *value;
    XML_Char *id;
};

struct config_parse_state {
	XML_Char last_content[BUFFER_SIZE];
    struct mixer_setting *initial_mixer_settings[10];
    int num_mixer_settings;
	int level;
};

void start_tag(void *data, const XML_Char *tag_name, const XML_Char **attr)
{
	const XML_Char *attr_name  = NULL;
	const XML_Char *attr_id    = NULL;
	const XML_Char *attr_value = NULL;
	struct config_parse_state *state = data;

	for ( int i = 0; attr[i]; i += 2) {
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
            struct mixer_setting *ms = malloc(sizeof(struct mixer_setting));
            ms->name = malloc(sizeof(XML_Char)*50);
            strncpy(ms->name, attr_name, 50);
            ms->value = malloc(sizeof(XML_Char)*50);
            strncpy(ms->value, attr_value, 50);
            ms->id = malloc(sizeof(XML_Char)*50);
            strncpy(ms->id, attr_id, 50);
            state->initial_mixer_settings[state->num_mixer_settings] = ms;
            state->num_mixer_settings++;

		//} else if (!strcmp(tag_name, "path")) {
		//	printf("USECASE:: %s\n", attr_name);
		}
	} else if (state->level == 2) {
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

	char buffer[BUFFER_SIZE + 1];
	memset(buffer, 0, BUFFER_SIZE);

	size_t items_num = 0;
	items_num = fread(&buffer, sizeof(char), BUFFER_SIZE, fp);
    if (items_num == 0) {
        printf("(okay) no items read.\n");
        return -1;
    }
	
	if (XML_Parse(parser, buffer, strlen(buffer), XML_TRUE) == XML_STATUS_ERROR) {
		printf("Errrr! %s\n", XML_ErrorString(XML_GetErrorCode(parser)));
		return -1;
	}

	fclose(fp);
	XML_ParserFree(parser);

    printf("INITSZZZ:: %d\n", state.num_mixer_settings);
    for ( int i = 0; i < state.num_mixer_settings; i++) {
        struct mixer_setting *ms = state.initial_mixer_settings[i];
        printf("INIT:: name: %s id: %s val: %s\n", ms->name, ms->id, ms->value);
    }


	return 0;

}
