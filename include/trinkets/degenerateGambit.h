#ifndef DEGENERATE_GAMBIT_H
#define DEGENERATE_GAMBIT_H

#include "../trinket.h"

/**
 * CreateDegenerateGambitTrinket - Create "Degenerate's Gambit" trinket
 *
 * @return Trinket_t* - Starter trinket for Degenerate class
 *
 * Passive: When you HIT on 15+, deal 10 damage (+5 per active use)
 * Active: Double a card with rank ≤5 (3-turn cooldown)
 *
 * Design: Risk/reward gambler archetype
 */
Trinket_t* CreateDegenerateGambitTrinket(void);

#endif // DEGENERATE_GAMBIT_H
