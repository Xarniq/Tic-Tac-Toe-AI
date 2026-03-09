/**
 * @file config.h
 * @brief Configuration file parser for server and client settings.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/**
 * @brief Server configuration structure.
 */
typedef struct {
	uint16_t port;
	int default_difficulty;
	int max_clients;
} ServerConfig;

/**
 * @brief Client configuration structure.
 */
typedef struct {
	char server_ip[64];
	uint16_t server_port;
} ClientConfig;

/**
 * @brief Loads server configuration from file.
 * @param filepath Path to the configuration file.
 * @param config Pointer to ServerConfig structure to fill.
 * @return 1 on success, 0 on failure (uses defaults).
 */
int Config_LoadServer(const char* filepath, ServerConfig* config);

/**
 * @brief Loads client configuration from file.
 * @param filepath Path to the configuration file.
 * @param config Pointer to ClientConfig structure to fill.
 * @return 1 on success, 0 on failure (uses defaults).
 */
int Config_LoadClient(const char* filepath, ClientConfig* config);

#endif /* CONFIG_H */
