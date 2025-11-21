#ifndef STACK_TRACE_H
#define STACK_TRACE_H

#include "../trinket.h"

/**
 * CreateStackTraceTrinket - Create the "Stack Trace" trinket template
 *
 * Passive: When you BUST, deal 15 damage to enemy
 * Rarity: Uncommon
 * Theme: Debugging - crashes give useful error traces to weaponize
 *
 * @return Trinket_t* - Pointer to template in g_trinket_templates, or NULL on failure
 */
Trinket_t* CreateStackTraceTrinket(void);

#endif // STACK_TRACE_H
