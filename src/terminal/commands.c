/*
 * Built-in Terminal Commands
 */

#include "../../include/terminal/terminal.h"
#include "../../include/player.h"
#include "../../include/enemy.h"
#include "../../include/game.h"
#include "../../include/cardTags.h"
#include "../../include/event.h"
#include "../../include/state.h"
#include "../../include/scenes/components/eventModal.h"

// External globals (from sceneBlackjack.c)
extern Player_t* g_human_player;
extern GameContext_t g_game;
extern EventEncounter_t* g_current_event;
extern EventModal_t* g_event_modal;

// External event factories (from event.c)
extern EventEncounter_t* CreateSystemMaintenanceEvent(void);
extern EventEncounter_t* CreateHouseOddsEvent(void);

// ============================================================================
// ARGUMENT AUTOCOMPLETE FUNCTIONS
// ============================================================================

dArray_t* SuggestEventNames(Terminal_t* terminal, const char* partial_arg) {
    (void)terminal;  // Unused

    // d_ArrayInit(capacity, element_size) - capacity FIRST!
    dArray_t* suggestions = d_ArrayInit(8, sizeof(char*));
    if (!suggestions) return NULL;

    // Get event registry (single source of truth)
    int event_count = 0;
    const EventRegistryEntry_t* registry = GetEventRegistry(&event_count);

    // Filter by partial argument (case-insensitive prefix match)
    // Skip exact matches to prevent autocomplete spam when cycling
    size_t partial_len = partial_arg ? strlen(partial_arg) : 0;

    for (int i = 0; i < event_count; i++) {
        size_t candidate_len = strlen(registry[i].command_name);

        // Prefix match AND candidate must be longer than partial (skip exact matches)
        bool is_prefix_match = (partial_len == 0 || strncasecmp(registry[i].command_name, partial_arg, partial_len) == 0);
        bool is_longer = (candidate_len > partial_len);

        if (is_prefix_match && (partial_len == 0 || is_longer)) {
            // Add matching command name (pointer to static string in registry)
            const char* name = registry[i].command_name;
            d_ArrayAppend(suggestions, &name);
        }
    }

    return suggestions;
}

dArray_t* SuggestEnemyNames(Terminal_t* terminal, const char* partial_arg) {
    (void)terminal;  // Unused

    // d_ArrayInit(capacity, element_size) - capacity FIRST!
    dArray_t* suggestions = d_ArrayInit(8, sizeof(char*));
    if (!suggestions) return NULL;

    // List of known enemy names (static strings - don't free these!)
    const char* enemy_names[] = {
        "TheDidact",
        "TheDaemon",
        "ShadowDealer",
        "ChipCollector",
        "HouseEnforcer",
    };
    int enemy_count = sizeof(enemy_names) / sizeof(enemy_names[0]);

    // Filter by partial argument (case-insensitive prefix match)
    // Skip exact matches to prevent autocomplete spam when cycling
    size_t partial_len = partial_arg ? strlen(partial_arg) : 0;

    for (int i = 0; i < enemy_count; i++) {
        size_t candidate_len = strlen(enemy_names[i]);

        // Prefix match AND candidate must be longer than partial (skip exact matches)
        bool is_prefix_match = (partial_len == 0 || strncasecmp(enemy_names[i], partial_arg, partial_len) == 0);
        bool is_longer = (candidate_len > partial_len);

        if (is_prefix_match && (partial_len == 0 || is_longer)) {
            // Add matching enemy name (pointer to static string)
            const char* name = enemy_names[i];
            d_ArrayAppend(suggestions, &name);
        }
    }

    return suggestions;
}

dArray_t* SuggestStatusEffects(Terminal_t* terminal, const char* partial_arg) {
    (void)terminal;  // Unused

    // d_ArrayInit(capacity, element_size) - capacity FIRST!
    dArray_t* suggestions = d_ArrayInit(8, sizeof(char*));
    if (!suggestions) return NULL;

    // List of status effect names (static strings - don't free these!)
    // ADR-002: Only outcome modifiers (no betting modifiers)
    const char* status_names[] = {
        "CHIP_DRAIN",
        "TILT",
        "GREED",
        "RAKE",
    };
    int status_count = sizeof(status_names) / sizeof(status_names[0]);

    // Filter by partial argument (case-insensitive prefix match)
    size_t partial_len = partial_arg ? strlen(partial_arg) : 0;

    for (int i = 0; i < status_count; i++) {
        size_t candidate_len = strlen(status_names[i]);

        // Prefix match AND candidate must be longer than partial (skip exact matches)
        bool is_prefix_match = (partial_len == 0 || strncasecmp(status_names[i], partial_arg, partial_len) == 0);
        bool is_longer = (candidate_len > partial_len);

        if (is_prefix_match && (partial_len == 0 || is_longer)) {
            const char* name = status_names[i];
            d_ArrayAppend(suggestions, &name);
        }
    }

    return suggestions;
}

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
    TerminalPrint(terminal, "  set_sanity <amt>  - Set player sanity (0-100)");
    TerminalPrint(terminal, "  set_hp <amt>      - Set enemy HP (combat only)");
    TerminalPrint(terminal, "  spawn_enemy <name> <hp> <atk> - Spawn combat enemy");
    TerminalPrint(terminal, "  add_tag <id|all> <tag> - Add tag to card (0-51 or 'all')");
    TerminalPrint(terminal, "  apply_status <effect> <val> <dur> - Apply status (RAKE, TILT, etc)");
    TerminalPrint(terminal, "  trigger_event <name> - Trigger event (maintenance, house_odds)");
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

void CMD_SetSanity(Terminal_t* terminal, const char* args) {
    if (!args || strlen(args) == 0) {
        TerminalPrint(terminal, "[Error] Usage: set_sanity <amount>");
        TerminalPrint(terminal, "[Error] Example: set_sanity 50");
        return;
    }

    if (!g_human_player) {
        TerminalPrint(terminal, "[Error] No player found");
        return;
    }

    int sanity = atoi(args);

    // Clamp to valid range
    if (sanity < 0) sanity = 0;
    if (sanity > g_human_player->max_sanity) sanity = g_human_player->max_sanity;

    // Set sanity directly
    g_human_player->sanity = sanity;

    TerminalPrint(terminal, "[OK] Player sanity set to %d/%d",
                  g_human_player->sanity, g_human_player->max_sanity);

    // Show which sanity tier we're in
    float percent = GetPlayerSanityPercent(g_human_player);
    const char* tier_name = "Unknown";
    if (percent == 0.0f) {
        tier_name = "ZERO (0%)";
    } else if (percent <= 0.25f) {
        tier_name = "VERY LOW (1-25%)";
    } else if (percent <= 0.50f) {
        tier_name = "LOW (26-50%)";
    } else if (percent <= 0.75f) {
        tier_name = "MEDIUM (51-75%)";
    } else {
        tier_name = "HIGH (76-100%)";
    }

    TerminalPrint(terminal, "[OK] Sanity tier: %s", tier_name);
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

    // Parse: name hp
    char name[128] = {0};
    int hp = 0;

    // Simple parsing (expect: name hp)
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
    }

    if (strlen(name) == 0 || hp <= 0) {
        TerminalPrint(terminal, "[Error] Invalid parameters");
        TerminalPrint(terminal, "[Error] Usage: spawn_enemy <name> <hp>");
        return;
    }

    // Destroy existing enemy if any
    if (g_game.current_enemy) {
        DestroyEnemy(&g_game.current_enemy);
    }

    // Create new enemy
    Enemy_t* enemy = CreateEnemy(name, hp);
    if (!enemy) {
        TerminalPrint(terminal, "[Error] Failed to create enemy");
        return;
    }

    g_game.current_enemy = enemy;
    g_game.is_combat_mode = true;

    TerminalPrint(terminal, "[Terminal] Spawned: %s (HP: %d)", name, hp);
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

void CMD_ApplyStatus(Terminal_t* terminal, const char* args) {
    if (!args || strlen(args) == 0) {
        TerminalPrint(terminal, "[Error] Usage: apply_status <effect> <value> <duration>");
        TerminalPrint(terminal, "[Error] Example: apply_status RAKE 5 3");
        TerminalPrint(terminal, "[Error] Effects: CHIP_DRAIN, TILT, GREED, RAKE");
        return;
    }

    // Parse: status_name value duration
    char status_name[64] = {0};
    int value = 0;
    int duration = 0;

    int parsed = sscanf(args, "%63s %d %d", status_name, &value, &duration);
    if (parsed < 3) {
        TerminalPrint(terminal, "[Error] Invalid arguments (need 3 parameters)");
        TerminalPrint(terminal, "[Error] Usage: apply_status <effect> <value> <duration>");
        return;
    }

    // Check player exists
    if (!g_human_player || !g_human_player->status_effects) {
        TerminalPrint(terminal, "[Error] Player not initialized");
        return;
    }

    // Convert string to StatusEffect_t enum
    StatusEffect_t effect = StatusEffectFromString(status_name);
    if (effect == STATUS_NONE) {
        TerminalPrint(terminal, "[Error] Unknown status effect: %s", status_name);
        TerminalPrint(terminal, "[Error] Valid effects: CHIP_DRAIN, TILT, GREED, RAKE");
        return;
    }

    // Validate parameters
    if (duration <= 0) {
        TerminalPrint(terminal, "[Error] Duration must be > 0");
        return;
    }

    // Apply status effect
    ApplyStatusEffect(g_human_player->status_effects, effect, value, duration);

    TerminalPrint(terminal, "[Terminal] Applied %s (value: %d, duration: %d)",
                  GetStatusEffectName(effect), value, duration);
}

void CMD_TriggerEvent(Terminal_t* terminal, const char* args) {
    if (!args || strlen(args) == 0) {
        TerminalPrint(terminal, "[Error] Usage: trigger_event <event_name>");

        // Show available events from registry
        int event_count = 0;
        const EventRegistryEntry_t* registry = GetEventRegistry(&event_count);

        if (event_count > 0) {
            TerminalPrint(terminal, "[Error] Available events:");
            for (int i = 0; i < event_count; i++) {
                TerminalPrint(terminal, "  - %s", registry[i].command_name);
            }
        }
        return;
    }

    // Destroy existing event if present
    if (g_current_event) {
        DestroyEvent(&g_current_event);
        g_current_event = NULL;
    }

    // Look up event in registry (single source of truth)
    int event_count = 0;
    const EventRegistryEntry_t* registry = GetEventRegistry(&event_count);

    bool found = false;
    for (int i = 0; i < event_count; i++) {
        if (strcasecmp(args, registry[i].command_name) == 0) {
            // Found matching event - call factory function
            g_current_event = registry[i].factory();

            if (g_current_event) {
                State_Transition(&g_game, STATE_EVENT);
                ShowEventModal(g_event_modal, g_current_event);  // Manually show modal
                TerminalPrint(terminal, "[Terminal] Triggered event: %s", registry[i].display_name);
            } else {
                TerminalPrint(terminal, "[Error] Failed to create event");
            }

            found = true;
            break;
        }
    }

    if (!found) {
        TerminalPrint(terminal, "[Error] Unknown event: %s", args);
        TerminalPrint(terminal, "[Error] Available events:");
        for (int i = 0; i < event_count; i++) {
            TerminalPrint(terminal, "  - %s", registry[i].command_name);
        }
    }
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void RegisterBuiltinCommands(Terminal_t* terminal) {
    RegisterCommand(terminal, "help", CMD_Help, "Show available commands", NULL);
    RegisterCommand(terminal, "clear", CMD_Clear, "Clear terminal output", NULL);
    RegisterCommand(terminal, "echo", CMD_Echo, "Print text to terminal", NULL);
    RegisterCommand(terminal, "give_chips", CMD_GiveChips, "Add chips to player", NULL);
    RegisterCommand(terminal, "set_sanity", CMD_SetSanity, "Set player sanity (0-100)", NULL);
    RegisterCommand(terminal, "set_hp", CMD_SetHP, "Set enemy HP (combat only)", NULL);
    RegisterCommand(terminal, "spawn_enemy", CMD_SpawnEnemy, "Spawn combat enemy", SuggestEnemyNames);
    RegisterCommand(terminal, "add_tag", CMD_AddTag, "Add tag to card(s) by ID or 'all'", NULL);
    RegisterCommand(terminal, "apply_status", CMD_ApplyStatus, "Apply status effect to player", SuggestStatusEffects);
    RegisterCommand(terminal, "trigger_event", CMD_TriggerEvent, "Trigger specific event by name", SuggestEventNames);
}
