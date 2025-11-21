#ifndef ELITE_MEMBERSHIP_H
#define ELITE_MEMBERSHIP_H

#include "../trinket.h"

/**
 * CreateEliteMembershipTrinket - Create "Elite Membership" trinket
 *
 * @return Trinket_t* - Passive-only trinket (ID 1)
 *
 * Passive: Win 50% more chips, refund 50% chips on loss (GAME_EVENT_ROUND_ENDED)
 * Rarity: RARE
 *
 * Design: Casino loyalty program - passive income boost + loss protection
 * "The house always wins—unless you're VIP."
 */
Trinket_t* CreateEliteMembershipTrinket(void);

#endif // ELITE_MEMBERSHIP_H
