Game pitch (one-paragraph)
- You’re trapped in a casino run by robot dealers. The only way out is to beat them at blackjack. You start with 1000 tokens (both currency and life). Each blackjack hand is “combat”: you stake tokens per hand; when you win you deal damage to the enemy robot equal to your staked bet × your final hand value (aces counted as in the final total). Bets are deducted immediately. Wins return your stake plus payout by standard blackjack rules (push returns stake, natural blackjack 3:2 payout). The deck is always a single 52-card deck — no reshuffle until the discard pile is empty — so card-counting and deck-awareness are core skills. Between fights you improve your deck via tagged card upgrades and relics (permanent run modifiers) that create emergent synergies and strategic depth.

Core mechanics (concise)
- Tokens:
  - Start: 1000 tokens.
  - Tokens = currency + HP. If tokens ≤ 0 → death / run over.
  - Bets are deducted immediately when placed.
- Combat:
  - One or more hands of blackjack until enemy HP ≤ 0 OR player tokens ≤ 0 OR player flees.
  - No reshuffle until discard pile reaches 0; show deck & discard counts.
- Damage:
  - Base damage = staked_bet × final_hand_value.
  - Apply per-card tag modifiers (multiplicative) and relic effects.
  - Apply enemy mitigation (flat defense and/or percent reduction).
  - Final damage = max(0, floor(base_damage × card_tag_multiplier × relic_multiplier × (1 - enemy_pct_def)) - enemy_flat_def).
- Payouts:
  - Win: stake returned + winnings (1:1). Blackjack natural: stake returned + 3/2 × stake.
  - Push: stake returned.
  - Lose: stake lost (already deducted).
- Doubles & Splits:
  - Double: allowed on first decision; double stake deducted immediately; player draws exactly one card and stands; damage uses doubled stake.
  - Split: allowed for same-rank initial 2 cards; creates two hands, each with a bet = original stake; resolve each hand independently (damage per winning hand).
- Deck Progression:
  - Player obtains tagged upgrades (cards with tags), relics (global modifiers), consumables, or replacement offers from events/shops/chests.
  - Upgrades attach tags to cards in your 52-card deck (see Tags section). You may be offered to replace a randomly chosen current card with a tagged card or offered choices with the target mapping shown.

UI (must-haves)
- Persistent HUD:
  - Current tokens (total).
  - Staked tokens (current hand).
  - Deck remaining / Discard size (numerical + small histogram by rank).
  - Potential payout preview (based on stake and whether blackjack).
  - Tooltip: “Damage = Bet × Hand Value (aces counted as in final total). Card tags & relics may modify damage. Dealer defense subtracts flat/percent.”
- Betting controls:
  - Quick presets (10, 25, 50, 100), slider, MAX button.
  - Predicted damage preview (range: expected min/avg/max) computed from current deck distribution optionally.
- Hand UI:
  - Show each card, its tags (icons + tooltip), per-card effects.
  - Show which cards in the hand contributed to tag multipliers.
- Post-offer UI:
  - When replacing/adding tags, show which existing card will be targeted and the candidate tag(s). Allow accept/decline.

Card tagging & relic systems (design)
- Tags are small modifiers attached to specific card instances in the deck (e.g., "7♠ has Flame tag").
- Relics are persistent global modifiers that affect gameplay (like Slay the Spire relics).
- Tags are intended to create emergent combos (e.g., tag synergy like “Flame” + “Heavy” turns low cards into consistent damage multipliers).

Tag properties
- id, name, rarity, type, numeric value, stacking rules, duration (permanent / temporary for N hands)
- Types of tags (examples):
  - MOD_DAMAGE_MULT (multiply damage when this card is in the winning hand)
  - MOD_VALUE_OVERRIDE (treat card as a different blackjack value)
  - MOD_PIERCE (ignore flat defense)
  - MOD_REFUND_ON_PUSH (refund X% stake on push)
  - MOD_SCRY_NEXT_N (when drawn, reveal next N cards in deck)
  - MOD_CRIT_CHANCE (chance to multiply damage)
  - MOD_SPLIT_BUFF (if card participates in split-winning hand, extra multiplier)
  - MOD_CONDITIONAL_BONUS (extra damage if enemy has armor)
- Tag stacking: define stacking behavior per tag (stack multiplicatively, additive, or single highest).

Relic system spec (quick)

    Rarity tiers: Common, Uncommon, Rare, Epic, Legendary.
    Each relic is passive or has a limited-use counter (charges / cooldown tied to hands or combat).
    Effects are applied via a small set of event hooks:
        ON_DRAW(card_instance_id, hand)
        ON_BET(amount)
        ON_HAND_RESOLVE(hand, staked, result) // result = WIN/LOSS/PUSH
        ON_DAMAGE_CALC(hand, base_damage*, modifiers*) // allow modifying damage pre- or post-mitigation
        ON_SHUFFLE()
        ON_COMBAT_START()/ON_COMBAT_END()
        ON_REPLACEMENT_OFFER(offer_list*)
    Balance guardrails:
        Cap multiplicative effects (e.g., max ×3 total damage).
        Prefer small %s, charges, or conditional activation.
        Use cooldowns (e.g., once per combat) for stronger relics.

OFFENSIVE RELICS

    Voltaic Gauntlet (Uncommon)

    Effect: +15% damage when any face card (J/Q/K) is part of the winning hand.
    Hooks: ON_DAMAGE_CALC
    Notes: Encourages holding/targeting face cards; pairs with tags that buff face cards.
    Synergy: With Dual on a face card (alt value high) → huge burst potential.

    Overclocked Chip (Rare)

    Effect: For the first win each combat, treat stake as +50% for damage calc only (payout unchanged).
    Hooks: ON_HAND_RESOLVE (consumes single-use flag)
    Notes: Strong first-hit burst; once-per-combat.
    Synergy: Use with Scry to set up first-hand win.

    Ruthless Coin (Epic)

    Effect: On any winning hand, 10% chance to double the damage dealt (applies before enemy mitigation).
    Hooks: ON_DAMAGE_CALC (random)
    Notes: RNG-driven critical; avoid stacking with other multiplicative relics without cap.
    Synergy: Stack with Flame to make many small wins huge.

    Scorched Ring (Uncommon)

    Effect: All cards with Flame tag gain an additional +0.05 (i.e., Flame ×1.25 → ×1.30 total when present).
    Hooks: ON_DAMAGE_CALC
    Notes: Simple synergy amplifier for tag builds.

    Bet Amplifier (Rare)

    Effect: When you place a bet, +20% effective bet for damage calculation only (not token payout).
    Hooks: ON_BET
    Notes: Powerful; must be tuned or made single-use/charge-based to avoid runaway.
    Synergy: With Dual and high alt values to maximize damage without extra token spend.

DEFENSIVE RELICS
6) Stake Insurance (Uncommon)

    Effect: When you lose a hand, refund 30% of the stake (once every 3 hands).
    Hooks: ON_HAND_RESOLVE
    Notes: Softens variance; cooldown prevents exploitation.
    Synergy: Helps aggressive strategies survive bad streaks.

    Shielded Wallet (Rare)

    Effect: Per-hand cap: you cannot lose more than 20% of your tokens from a single hand (rounded down); applies to stake losses only.
    Hooks: ON_HAND_RESOLVE
    Notes: Great for new players; reduces extreme variance.
    Synergy: Encourages bigger bets safely, but prevents one-shot bankruptcies.

    Ward of Resilience (Epic)

    Effect: After reshuffle, gain 10% of max tokens back as a field heal (once per combat).
    Hooks: ON_SHUFFLE
    Notes: Strong defensive; useful for long fights.
    Synergy: Works well when combined with tags that preserve small wins.

UTILITY RELICS
9) Oracle Lens (Rare)

    Effect: Twice per combat, peek top 4 cards and reorder them arbitrarily.
    Hooks: ON_DRAW (peek + reorder via UI)
    Notes: Huge deck-control utility; limited charges keep it balanced.
    Synergy: Combine with Dual & Scry tags to orchestrate exact draws.

    Deckwright’s Anvil (Uncommon — improved Decksmith)

    Effect: Whenever offered a replacement, you may choose which existing card in your deck to replace (instead of random target).
    Hooks: ON_REPLACEMENT_OFFER
    Notes: Powerful agency; common/uncommon rarity recommended.
    Synergy: Extremely strong with high-value tags; forces tough trade decisions.

    Collector’s Glove (Rare)

    Effect: When you accept a tagged-card replacement, also receive a weaker copy (a tag of half strength) to apply to any other random card.
    Hooks: ON_REPLACEMENT_OFFER (post-accept)
    Notes: Great for building synergies; reward for investing tokens.

    Timekeeper’s Chip (Epic)

    Effect: Once per combat, undo the last drawn card (move it back to top of deck and refund drawing effects); you may redraw instead.
    Hooks: ON_DRAW / ON_COMBAT_START
    Notes: Very powerful—one use per combat prevents abuse.
    Synergy: Save a disastrous draw (e.g., dual alt low) and force a redraw.

    Lucky Token (Common)

    Effect: 12% chance on a losing hand to refund the stake fully.
    Hooks: ON_HAND_RESOLVE
    Notes: RNG safety net; low chance but satisfying.

RISK / REWARD RELICS
14) Double-Edged Coin (Rare)

    Effect: +25% token payout on wins, but on losses you lose an extra 10% of current tokens (house edge).
    Hooks: ON_HAND_RESOLVE
    Notes: Strong but dangerous: makes each decision higher-stakes.
    Synergy: Creates tension for aggressive builds.

    House’s Gambit (Epic)

    Effect: All wins deal +30% damage, but all enemies have +25% HP while this relic active.
    Hooks: ON_COMBAT_START (scale enemy hp) and ON_DAMAGE_CALC
    Notes: Great for players who like higher variance and slower fights.

    Croupier’s Gauntlet (Rare)

    Effect: Dealers hit on soft 17 (house rule: worse for player) but any win vs such dealers ignores 50% of enemy flat defense.
    Hooks: ON_COMBAT_START, ON_DAMAGE_CALC
    Notes: A deliberate risk-reward; good for players who can exploit the defense reduction.

    Gambler’s Edge (Legendary)

    Effect: Once per run, convert your entire token pool into a single all-in bet for a guaranteed critical (×3 damage) if you win the next hand, but if you lose you die instantly.
    Hooks: ON_BET
    Notes: Story-mode relic; use only for high-stakes runs or as late-game gamble.

META / PROGRESSION RELICS
18) Bank Vault Key (Uncommon)

    Effect: In shops, get a 15% discount on relics and tagged cards.
    Hooks: Shop UI
    Notes: Economic benefit; avoids in-combat power imbalance.

    Archivist’s Codex (Rare)

    Effect: Start each run with knowledge: one extra relic choice at the start and 1 additional replacement offer per level.
    Hooks: RUN_START/REWARD_FLOW
    Notes: Meta-progression feel.

    Cardbinder (Epic)

    Effect: When receiving a relic, you may bind one tag permanently to your deck (persist between runs for a price).
    Hooks: ON_RELIC_ACQUIRE / Meta
    Notes: Powerful permanent progression—make bindings expensive.

    Echo of Fortune (Uncommon)

    Effect: On a push, gain a small random buff for the next hand (e.g., +10% damage or +1 value on a random card).
    Hooks: ON_HAND_RESOLVE
    Notes: Keeps pushes exciting and useful.

    Hearth of Learning (Common)

    Effect: After each combat you gain +1 XP towards unlocking a random relic (meta-progression). Not in-combat.
    Hooks: POST_COMBAT

How tag replacement works (player-facing)
- Option A (recommended UX): Offer 3 candidate tagged cards. For each candidate, randomly highlight a target card in the player's deck that would be replaced. Show mapping preview (e.g., "Replace 4♦ → 4♦ (With Flame tag: +20% damage)"). Player accepts per candidate or declines whole offer.
- Option B (player choice): Let player pick which of their existing cards to tag, and then pick tag from a 3-option pool (more agency, less RNG).
- Keep deck size constant (52). Tagging modifies the card instance, not the entire card_id set, so colors/texture remain but behavior altered.

Example tags & emergent combos
- Flame (MOD_DAMAGE_MULT, ×1.2): common.
- Heavy (MOD_VALUE_OVERRIDE, +1 blackjack value, e.g., a 4 becomes 5): common.
- Acefavor (MOD_VALUE_OVERRIDE for Ace → treat as 11 always when safe): rare.
- Pierce (MOD_PIERCE: ignores flat defense): uncommon.
- Chain (MOD_SPLIT_BUFF: when a chain-tagged card wins multiple hands in a round, successive wins have +10% bonus): rare.
- Scry (MOD_SCRY_NEXT_N: reveals next N cards when drawn): uncommon.
- Refund (MOD_REFUND_ON_PUSH: returns 25% of stake on push): uncommon.
- Combo: Flame + Pierce on many low cards allows many small wins to be meaningful; Pairing with a relic that reduces enemy percent defense amplifies that.

New tag: Dual (aka Wildcard / Flux)

    Purpose: Give a non-Ace card two selectable values for the current draw: its base blackjack value OR a randomized alt value in [1..10] that is guaranteed != base.
    Player-facing behavior: When the card is drawn, the alt value is rolled and shown. The player may choose which value to use for scoring that hand (base or alt). If no choice is made before resolution (auto-play), the system can auto-pick the option that best benefits the player (or follow a configurable policy).
    Scope: The alt value is generated once when the card is drawn and locked for that specific card instance for the current hand. If the same physical card is drawn in a later hand, roll again then.
    Restriction: Applies only to non-Ace cards. Face cards (J/Q/K) have base value 10; alt roll must not be 10 (so alt ∈ [1..9]).
Example emergent use-cases (why it's interesting)

    Low card becomes high-risk/high-reward: a 4 that can become 9 gives option to push aggressive damage but might increase bust chance on hitting.
    Combining Dual with Flame (×1.2) on many low cards: you can risk alt values to achieve many small wins that scale via flame multipliers.
    Combine Dual with Scry: reveal upcoming cards, then if you see a dual card alt is high, you might bet more.
    Combine Dual on mid-range cards with a relic that reduces enemy flat defense to zero: choosing alt high value becomes huge damage.


Implementation notes & data structures (fits dArray/dTable)
- Extend Card_t with a reference key (non-owning) to tag metadata, or keep an external table mapping card instance index → tag list. Prefer external mapping so the base Card_t remains simple and serialization is easier.

Suggested structs:
```c
typedef enum { TAG_DAMAGE_MULT, TAG_VALUE_OVERRIDE, TAG_PIERCE, TAG_SCRY, TAG_REFUND_ON_PUSH, TAG_CRIT, TAG_SPLIT_BUFF } TagType_t;

typedef struct {
    int tag_id;             // unique id for this tag instance
    TagType_t type;
    float value;            // multiplier or override value
    int duration_hands;     // -1 for permanent, or number of hands it lasts
    int rarity;             // 0..n, for shop weighting
    char name[64];
    char description[128];
} CardTag_t;

// Map per-card-instance tags: key = deck_index (or unique card instance id), value = dArray_t* of CardTag_t*
dTable_t* g_card_tags_map;  // Key: int (card_instance_index), Value: dArray_t* (CardTag_t*)
```

- Relics:
```c
typedef struct {
    int relic_id;
    char name[64];
    char desc[128];
    dString_t* script_key; // optional identifier for effect handlers
    int rarity;
} Relic_t;

dArray_t* g_player_relics; // list of Relic_t*
```

Tag / relic application in damage resolution (pseudocode)
```c
double ComputeDamage(CombatContext_t* cc, Hand_t* hand, int staked) {
    double base = (double)staked * (double)hand->total_value;
    double mult = 1.0;
    double relic_mult = 1.0;
    bool pierce = false;
    int pierce_count = 0;
    // Per card tags:
    for (i = 0; i < hand->cards->count; ++i) {
        Card_t* c = d_IndexDataFromArray(hand->cards, i);
        int instance_index = GetCardInstanceIndex(c); // index in deck dArray
        dArray_t* tags = (dArray_t*)d_GetDataFromTable(g_card_tags_map, &instance_index);
        if (!tags) continue;
        for (j=0;j<tags->count;j++) {
            CardTag_t* tag = d_IndexDataFromArray(tags, j);
            switch(tag->type) {
                case TAG_DAMAGE_MULT: mult *= tag->value; break;
                case TAG_VALUE_OVERRIDE: /* adjust hand->total_value or compute bonus */ break;
                case TAG_PIERCE: pierce = true; pierce_count++; break;
                case TAG_CRIT: if (RandChance(tag->value)) base *= 2.0; break;
                /* scry/refund handled elsewhere (scry on draw; refund at resolution) */
            }
        }
    }
    // Relic modifiers:
    for each relic in g_player_relics: apply relic effect to relic_mult or set pierce flag

    double pre_def = base * mult * relic_mult;
    // Apply enemy mitigation:
    double after_pct = pre_def * (1.0 - cc->enemy->pct_defense);
    int final = (int)floor(after_pct - (pierce ? 0 : cc->enemy->flat_defense));
    final = max(0, final);
    return final;
}
```

How to attach tags in the deck (practical)
- If you store the deck as dArray_t of Card_t in Deck_t, the per-card-instance index = array index. Use that as the key in g_card_tags_map.
- When you shuffle the deck, you must also update/move tag mappings accordingly (or tag by a unique card instance id stored on the Card_t). Simpler: add a field card_instance_id to Card_t (unique and persistent regardless of shuffle) and use that as key in g_card_tags_map.
```c
// New field in Card_t
int instance_id; // unique per physical card in deck, assigned on deck creation, never changed
```
- On deck creation, give each card instance a unique instance_id 0..51 (or more for multi-deck). Tag mappings keyed by instance_id survive shuffles.

Relic handling approach
- For clean code, have a Relic handler registry mapping relic.script_key → C function pointer to apply the effect during events (on_draw, on_win, on_resolve, on_bet, on_shuffle).
- Example events:
  - ON_DRAW(card_instance_id)
  - ON_HAND_RESOLVE(hand, staked)
  - ON_SHUFFLE()
  - ON_ENEMY_HIT()

Balancing & safety measures (to avoid runaway)
- Limit high multipliers: cap total damage multiplier to a reasonable max (e.g., ×4 or ×6) or make pierce/crit rare.
- Enemy defense tiers: early enemies low defense, bosses have percent reduction + flat armor.
- Scaling: increase enemy HP faster than potential maximum damage per hand, so wins require multiple hands unless player builds for burst.
- Soft penalty / rollback: optional “reshuffle penalty” (enemy recovers a small percent when deck is reshuffled) to prevent infinite stall. Use sparingly.

Testing & metrics
- Simulate runs with random betting policies to measure token burn per enemy.
- Log per-hand: bet, hand_value, damage, tokens_before, tokens_after, deck_count, discard_count.
- Unit tests: tag application, relic application, damage formula, deck shuffle preserving instance_id mapping.

Example progression & shop flow (simple)
- After a win (enemy down) present 3 choices:
  - Treasure: random token amount (small) OR 1 random tagged card candidate (show which deck card it would replace).
  - Shop: offer fixed-menu items (tagged cards for tokens, relics for larger tokens).
  - Event: a mixture (trade a card for tokens, cursed upgrade, free relic).
- Example shop items (prices):
  - Common tag card (×1.1 damage on that card): 75 tokens
  - Uncommon tag (value override or scry2): 200 tokens
  - Relic (small effect): 500–1000 tokens

Onboarding & UX suggestions
- Tutorial fight: slow pace, explain tokens deduction, preview predicted damage, show how tags look in hand and their effect.
- Deck overlay: a small modal allowing player to inspect deck frequency (counts by rank/suit), search for specific card, and see tagged cards highlighted.
- In-battle HUD: show next-card probability when a card is scryed or when the player toggles a “show distribution” button.

Next concrete steps I can produce for you
- Combat prototype (C) implementing: bets deducted, dealing loop, doubling/doubling rules, deck/discard UI counters, damage resolved with basic tag support (MOD_DAMAGE_MULT & MOD_VALUE_OVERRIDE), plus a simple relic system.
- Tag & Relic API header (C) + example implementations and unit tests.
- A balancing spreadsheet (CSV) with example enemy HP / defense / expected hands-to-kill at given bet presets.

Which of those would you like next? Or tell me which parts you want tightened (e.g., exact JSON schema for tags & relics, or a UI wireframe).