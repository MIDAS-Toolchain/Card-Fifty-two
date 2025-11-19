/*
 * Built-in Terminal Commands
 */

#include "../../include/terminal/terminal.h"
#include "../../include/player.h"
#include "../../include/enemy.h"
#include "../../include/game.h"
#include "../../include/cardTags.h"

// External globals (from sceneBlackjack.c)
extern Player_t* g_human_player;
extern GameContext_t g_game;

// ============================================================================
// COMMAND IMPLEMENTATIONS
// ============================================================================

void CMD_Help(Terminal_t* terminal, const char* args) {
    (void)args;  // Unused

    TerminalPrint(terminal, "");
    TerminalPrint(terminal, "Available commands:");
    TerminalPrint(terminal, "  help              - Show this help message");
    TerminalPrint(terminal, "  clear             - Clear terminal output");
    TerminalPrint(terminal, "  echo <text>       - Print text to terminal");
    TerminalPrint(terminal, "  give_chips <amt>  - Add chips to player");
    TerminalPrint(terminal, "  set_hp <amt>      - Set enemy HP (combat only)");
    TerminalPrint(terminal, "  spawn_enemy <name> <hp> <atk> - Spawn combat enemy");
    TerminalPrint(terminal, "  add_tag <id|all> <tag> - Add tag to card (0-51 or 'all')");
    TerminalPrint(terminal, "");
}

void CMD_Clear(Terminal_t* terminal, const char* args) {
    (void)args;  // Unused
    TerminalClear(terminal);
}

void CMD_Echo(Terminal_t* terminal, const char* args) {
    if (!args || strlen(args) == 0) {
        TerminalPrint(terminal, "[Error] Usage: echo <text>");
        return;
    }

    TerminalPrint(terminal, "%s", args);
}

void CMD_GiveChips(Terminal_t* terminal, const char* args) {
    if (!args || strlen(args) == 0) {
        TerminalPrint(terminal, "[Error] Usage: give_chips <amount>");
        return;
    }

    int amount = atoi(args);
    if (amount <= 0) {
        TerminalPrint(terminal, "[Error] Amount must be positive");
        return;
    }

    if (!g_human_player) {
        TerminalPrint(terminal, "[Error] No player found");
        return;
    }

    g_human_player->chips += amount;
    TerminalPrint(terminal, "[Terminal] Added %d chips (Total: %d)", amount, g_human_player->chips);
}

void CMD_SetHP(Terminal_t* terminal, const char* args) {
    if (!args || strlen(args) == 0) {
        TerminalPrint(terminal, "[Error] Usage: set_hp <amount>");
        return;
    }

    int hp = atoi(args);
    if (hp < 0) {
        TerminalPrint(terminal, "[Error] HP cannot be negative");
        return;
    }

    if (!g_game.is_combat_mode || !g_game.current_enemy) {
        TerminalPrint(terminal, "[Error] Not in combat (no enemy)");
        return;
    }

    int old_hp = g_game.current_enemy->current_hp;
    g_game.current_enemy->current_hp = hp;

    // Update defeated status
    if (hp <= 0) {
        g_game.current_enemy->is_defeated = true;
    } else {
        g_game.current_enemy->is_defeated = false;
    }

    TerminalPrint(terminal, "[Terminal] Enemy HP: %d -> %d", old_hp, hp);
}

void CMD_SpawnEnemy(Terminal_t* terminal, const char* args) {
    if (!args || strlen(args) == 0) {
        TerminalPrint(terminal, "[Error] Usage: spawn_enemy <name> <hp> <attack>");
        TerminalPrint(terminal, "[Error] Example: spawn_enemy \"Shadow Dealer\" 100 10");
        return;
    }

    // Parse: name hp attack
    char name[128] = {0};
    int hp = 0;
    int chip_threat = 0;

    // Simple parsing (expect: name hp chip_threat)
    const char* ptr = args;

    // Skip leading spaces
    while (*ptr && isspace(*ptr)) ptr++;

    // Parse name (quoted or first word)
    int name_idx = 0;
    if (*ptr == '"') {
        // Quoted name
        ptr++;
        while (*ptr && *ptr != '"' && name_idx < 127) {
            name[name_idx++] = *ptr++;
        }
        if (*ptr == '"') ptr++;
    } else {
        // Unquoted name (first word)
        while (*ptr && !isspace(*ptr) && name_idx < 127) {
            name[name_idx++] = *ptr++;
        }
    }
    name[name_idx] = '\0';

    // Parse HP
    while (*ptr && isspace(*ptr)) ptr++;
    if (*ptr) {
        hp = atoi(ptr);
        while (*ptr && !isspace(*ptr)) ptr++;
    }

    // Parse chip_threat
    while (*ptr && isspace(*ptr)) ptr++;
    if (*ptr) {
        chip_threat = atoi(ptr);
    }

    if (strlen(name) == 0 || hp <= 0 || chip_threat <= 0) {
        TerminalPrint(terminal, "[Error] Invalid parameters");
        TerminalPrint(terminal, "[Error] Usage: spawn_enemy <name> <hp> <chip_threat>");
        return;
    }

    // Destroy existing enemy if any
    if (g_game.current_enemy) {
        DestroyEnemy(&g_game.current_enemy);
    }

    // Create new enemy
    Enemy_t* enemy = CreateEnemy(name, hp, chip_threat);
    if (!enemy) {
        TerminalPrint(terminal, "[Error] Failed to create enemy");
        return;
    }

    g_game.current_enemy = enemy;
    g_game.is_combat_mode = true;

    TerminalPrint(terminal, "[Terminal] Spawned: %s (HP: %d, Threat: %d)", name, hp, chip_threat);
}

void CMD_AddTag(Terminal_t* terminal, const char* args) {
    if (!args || strlen(args) == 0) {
        TerminalPrint(terminal, "[Error] Usage: add_tag <card_id|all> <tag_name>");
        TerminalPrint(terminal, "[Error] Example: add_tag 0 cursed");
        TerminalPrint(terminal, "[Error] Example: add_tag all vampiric");
        return;
    }

    // Parse: card_id_or_all tag_name
    char card_str[32] = {0};
    char tag_name[32] = {0};

    sscanf(args, "%31s %31s", card_str, tag_name);

    if (strlen(tag_name) == 0) {
        TerminalPrint(terminal, "[Error] Missing tag name");
        TerminalPrint(terminal, "[Error] Valid tags: cursed, vampiric, lucky, brutal, doubled");
        return;
    }

    // Convert tag name to enum
    CardTag_t tag;
    if (strcasecmp(tag_name, "cursed") == 0) {
        tag = CARD_TAG_CURSED;
    } else if (strcasecmp(tag_name, "vampiric") == 0) {
        tag = CARD_TAG_VAMPIRIC;
    } else if (strcasecmp(tag_name, "lucky") == 0) {
        tag = CARD_TAG_LUCKY;
    } else if (strcasecmp(tag_name, "brutal") == 0) {
        tag = CARD_TAG_BRUTAL;
    } else if (strcasecmp(tag_name, "doubled") == 0) {
        tag = CARD_TAG_DOUBLED;
    } else {
        TerminalPrint(terminal, "[Error] Unknown tag: %s", tag_name);
        TerminalPrint(terminal, "[Error] Valid tags: cursed, vampiric, lucky, brutal, doubled");
        return;
    }

    // Check if "all" or specific card ID
    if (strcasecmp(card_str, "all") == 0) {
        // Add tag to all 52 cards (0-51)
        for (int i = 0; i < 52; i++) {
            AddCardTag(i, tag);
        }
        TerminalPrint(terminal, "[Terminal] Added %s tag to all 52 cards", tag_name);
    } else {
        // Parse card ID
        int card_id = atoi(card_str);

        if (card_id < 0 || card_id > 51) {
            TerminalPrint(terminal, "[Error] card_id must be 0-51 or 'all'");
            return;
        }

        // Add tag to specific card
        AddCardTag(card_id, tag);
        TerminalPrint(terminal, "[Terminal] Added %s tag to card %d", tag_name, card_id);
    }
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void RegisterBuiltinCommands(Terminal_t* terminal) {
    RegisterCommand(terminal, "help", CMD_Help, "Show available commands");
    RegisterCommand(terminal, "clear", CMD_Clear, "Clear terminal output");
    RegisterCommand(terminal, "echo", CMD_Echo, "Print text to terminal");
    RegisterCommand(terminal, "give_chips", CMD_GiveChips, "Add chips to player");
    RegisterCommand(terminal, "set_hp", CMD_SetHP, "Set enemy HP (combat only)");
    RegisterCommand(terminal, "spawn_enemy", CMD_SpawnEnemy, "Spawn combat enemy");
    RegisterCommand(terminal, "add_tag", CMD_AddTag, "Add tag to card(s) by ID or 'all'");
}
