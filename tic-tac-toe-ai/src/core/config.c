/**
 * @file config.c
 * @brief Implementation of INI configuration file parser.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief Trims leading and trailing whitespace from a string.
 * @param str String to trim (modified in place).
 * @return Pointer to trimmed string.
 */
static char* TrimWhitespace(char* str) {
	/* Trim leading whitespace. */
	while (isspace((unsigned char)*str)) {
		str++;
	}

	if (*str == '\0') {
		return str;
	}

	/* Trim trailing whitespace. */
	char* end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) {
		end--;
	}
	end[1] = '\0';

	return str;
}

/**
 * @brief Parses a single line of the INI file.
 * @param line The line to parse.
 * @param key Output buffer for the key.
 * @param value Output buffer for the value.
 * @param keySize Size of key buffer.
 * @param valueSize Size of value buffer.
 * @return 1 if key-value pair found, 0 otherwise.
 */
static int ParseLine(const char* line, char* key, char* value, 
					 size_t keySize, size_t valueSize) {
	/* Skip comments and empty lines. */
	if (line[0] == '#' || line[0] == ';' || line[0] == '[' || line[0] == '\0') {
		return 0;
	}

	const char* equals = strchr(line, '=');
	if (!equals) {
		return 0;
	}

	/* Extract key. */
	size_t keyLen = (size_t)(equals - line);
	if (keyLen >= keySize) {
		keyLen = keySize - 1;
	}
	strncpy(key, line, keyLen);
	key[keyLen] = '\0';

	/* Extract value. */
	const char* valueStart = equals + 1;
	strncpy(value, valueStart, valueSize - 1);
	value[valueSize - 1] = '\0';

	/* Trim both. */
	char* trimmedKey = TrimWhitespace(key);
	char* trimmedValue = TrimWhitespace(value);

	/* Move trimmed strings to beginning of buffers. */
	if (trimmedKey != key) {
		memmove(key, trimmedKey, strlen(trimmedKey) + 1);
	}
	if (trimmedValue != value) {
		memmove(value, trimmedValue, strlen(trimmedValue) + 1);
	}

	return 1;
}

int Config_LoadServer(const char* filepath, ServerConfig* config) {
	/* Set defaults first. */
	config->port = 12345;
	config->default_difficulty = 2;
	config->max_clients = 10;

	FILE* file = fopen(filepath, "r");
	if (!file) {
		fprintf(stderr, "[config] Could not open '%s', using defaults.\n", filepath);
		return 0;
	}

	char line[256];
	char key[64];
	char value[128];

	while (fgets(line, sizeof(line), file)) {
		char* trimmed = TrimWhitespace(line);
		if (ParseLine(trimmed, key, value, sizeof(key), sizeof(value))) {
			if (strcmp(key, "port") == 0) {
				config->port = (uint16_t)atoi(value);
			} else if (strcmp(key, "default_difficulty") == 0) {
				config->default_difficulty = atoi(value);
				if (config->default_difficulty < 1) config->default_difficulty = 1;
				if (config->default_difficulty > 3) config->default_difficulty = 3;
			} else if (strcmp(key, "max_clients") == 0) {
				config->max_clients = atoi(value);
			}
		}
	}

	fclose(file);
	printf("[config] Server config loaded: port=%d, difficulty=%d, max_clients=%d\n",
		   config->port, config->default_difficulty, config->max_clients);
	return 1;
}

int Config_LoadClient(const char* filepath, ClientConfig* config) {
	/* Set defaults first. */
	strncpy(config->server_ip, "127.0.0.1", sizeof(config->server_ip));
	config->server_port = 12345;

	FILE* file = fopen(filepath, "r");
	if (!file) {
		fprintf(stderr, "[config] Could not open '%s', using defaults.\n", filepath);
		return 0;
	}

	char line[256];
	char key[64];
	char value[128];

	while (fgets(line, sizeof(line), file)) {
		char* trimmed = TrimWhitespace(line);
		if (ParseLine(trimmed, key, value, sizeof(key), sizeof(value))) {
			if (strcmp(key, "server_ip") == 0) {
				strncpy(config->server_ip, value, sizeof(config->server_ip) - 1);
				config->server_ip[sizeof(config->server_ip) - 1] = '\0';
			} else if (strcmp(key, "server_port") == 0) {
				config->server_port = (uint16_t)atoi(value);
			}
		}
	}

	fclose(file);
	printf("[config] Client config loaded: server=%s:%d\n",
		   config->server_ip, config->server_port);
	return 1;
}
