/*
 * Test Stubs - Minimal implementations for globals/functions that tests don't need
 *
 * This allows us to link core game code without pulling in SDL, audio, etc.
 */

#include "../include/common.h"
#include "../include/player.h"
#include "../include/tween/tween.h"
#include "../include/trinket.h"

// Global stubs
dTable_t* g_players = NULL;
dTable_t* g_card_textures = NULL;
TweenManager_t g_tween_manager;
aAudioClip_t g_push_chips_sound;
aAudioClip_t g_victory_sound;

// Function stubs
void SetStatusEffectDrainAmount(int amount) { (void)amount; }
TweenManager_t* GetTweenManager(void) { return &g_tween_manager; }
void* GetCardTransitionManager(void) { return NULL; }
void StartCardDealAnimation(void* mgr, void* tween, void* hand, size_t idx, int sx, int sy, int tx, int ty) {
    (void)mgr; (void)tween; (void)hand; (void)idx; (void)sx; (void)sy; (void)tx; (void)ty;
}
void TweenEnemyHP(void* enemy) { (void)enemy; }
void SpawnDamageNumber(int x, int y, int amount, int is_healing) {
    (void)x; (void)y; (void)amount; (void)is_healing;
}
Trinket_t* CreateDegenerateGambitTrinket(void) { return NULL; }
