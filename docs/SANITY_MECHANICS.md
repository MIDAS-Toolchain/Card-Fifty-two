# Card Fifty-Two: Sanity Mechanics

## Core Concept

Sanity is not a health bar. It's a **narrative unlock system disguised as a difficulty modifier**.

- **High Sanity**: Easier gameplay, surface story
- **Low Sanity**: Harder gameplay, true story

This creates a deliberate tension: players who want the full narrative must willingly make the game harder.

---

## Sanity Meter

### Range
- **100% (Full Sanity)**: You believe this is just a game. The dealers are just malfunctioning robots. You can win and leave.
- **50% (Compromised)**: Doubt creeps in. The casino doesn't follow normal rules. Time feels wrong.
- **0% (Fully Insane)**: You see the truth. You wish you didn't.

### Visibility
- Sanity meter displayed as a subtle visual effect (screen edges, color grading, audio distortion)
- No explicit percentage shown—player must infer their state from environmental feedback
- At 0%, the UI itself becomes unreliable (numbers flicker, cards briefly show different values)

---

## How Sanity Decreases

### Passive Decay
- **Losing Hands**: -2% sanity per loss
- **Going Bust**: -5% sanity (failure feels personal)
- **Dealer Wins on 21**: -3% sanity (the house always wins)

### Active Triggers
- **Dealer Dialogue**: Certain AI dealer lines drain sanity (-1% to -10% depending on content)
- **Environmental Tells**: Noticing inconsistencies in the casino (clocks running backwards, doors that weren't there before) costs sanity to perceive
- **Story Revelations**: Learning the truth about the casino, the dealers, yourself

### Restorative Actions
- **Winning Hands**: +1% sanity (false comfort)
- **Blackjack**: +3% sanity (moment of control)
- **Leaving Tables**: +2% sanity (avoiding the game feels safer)

**Critical Design Note**: It's easier to lose sanity than gain it. Players who want 0% sanity runs can get there through reckless play. Players who want high sanity must play conservatively and avoid story triggers.

---

## Gameplay Effects by Sanity Level

### High Sanity (100%-70%)
- **Difficulty**: Standard blackjack rules
- **Dealer Behavior**: Predictable, follows house rules
- **Narrative Access**: Surface story only—you're trying to escape a weird casino
- **Visual/Audio**: Clean, clear, normal

### Moderate Sanity (69%-30%)
- **Difficulty**: Dealers start bending rules occasionally
- **Dealer Behavior**: Unpredictable—may deal extra cards, change bet requirements
- **Narrative Access**: Hints of deeper story—missing persons posters, glitched dealer dialogue
- **Visual/Audio**: Slight distortion, color desaturation, reverb on dealer voices

### Low Sanity (29%-1%)
- **Difficulty**: Actively hostile—dealers cheat openly, rules change mid-hand
- **Dealer Behavior**: Conversational, philosophical, mocking
- **Narrative Access**: Major story revelations—who you are, what this place is, why you're here
- **Visual/Audio**: Heavy distortion, hallucinations, environmental impossibilities

### Zero Sanity (0%)
- **Difficulty**: Nearly impossible—dealers are merciless, physics-defying
- **Dealer Behavior**: Truth-telling—they explain the system you're trapped in
- **Narrative Access**: Full story unlocked, hidden endings accessible
- **Visual/Audio**: Reality breakdown—UI glitches, fourth wall breaks, meta-commentary

---

## Character-Specific Sanity Effects

Each character class responds differently to sanity loss (see [CHARACTER_CLASSES.md](CHARACTER_CLASSES.md)):

### The Desperate
- **High Sanity**: Plays like normal blackjack
- **Low Sanity**: Loses agency, forced into reckless bets, compulsive behavior
- **Zero Sanity**: Cannot fold, cannot walk away, addicted to the game itself

### The Dealer
- **High Sanity**: Understands dealer patterns
- **Low Sanity**: Gains rule-breaking powers, argues with AI
- **Zero Sanity**: Becomes indistinguishable from the AI dealers (horror reveal)

### The Detective
- **High Sanity**: Treats blackjack as puzzle-solving
- **Low Sanity**: Reality-warping powers, evidence fabrication
- **Zero Sanity**: Realizes they're investigating themselves

### The Dreamer
- **Starts at 50% Sanity**: Already compromised
- **Low Sanity**: Fourth wall breaks, save/load awareness
- **Zero Sanity**: Full meta-awareness, confronts The Dogma

---

## Strategic Implications

### For Story Hunters
- Deliberately lose hands to drop sanity fast
- Engage with every story trigger (costs sanity but reveals narrative)
- Play as all characters at 0% sanity to unlock The Dreamer

### For Challenge Seekers
- Maintain high sanity for easier gameplay
- Avoid story triggers that drain sanity
- Focus on optimal blackjack strategy

### For Completionists
- Multiple playthroughs required:
  1. High sanity run to learn mechanics
  2. Low sanity run to unlock true ending
  3. All-character 0% sanity runs to unlock The Dreamer
  4. The Dreamer 0% sanity run to confront The Dogma

---

## Design Philosophy

Sanity is the game asking: **How much truth can you handle?**

Most games punish failure. Card Fifty-Two rewards it—if you're willing to suffer for the story.

This creates a souls-like relationship with difficulty:
- Casual players experience the surface narrative (high sanity, standard ending)
- Dedicated players experience the true narrative (low sanity, hidden endings)
- Masochistic players experience the *meta* narrative (The Dreamer, The Dogma)

**The game respects player choice**: You can win comfortably and never know the truth. Or you can lose beautifully and understand everything.

---

## Technical Implementation Notes

### Sanity State Storage
```c
typedef struct GameContext {
    float sanity;  // 0.0 to 1.0 (maps to 0%-100%)
    int sanity_tier;  // 0=zero, 1=low, 2=moderate, 3=high
    bool story_triggers_seen[64];  // Track which reveals happened
    // ...
} GameContext_t;
```

### Visual Effects System
- Screen edge vignette intensity = `1.0 - sanity`
- Color desaturation = `sanity < 0.5 ? (0.5 - sanity) : 0.0`
- UI glitch frequency = `sanity < 0.3 ? (0.3 - sanity) * 10 : 0.0`

### Dealer AI Modifier
```c
float GetDealerCheatingChance(float sanity) {
    if (sanity > 0.7) return 0.0;  // High sanity: fair play
    if (sanity > 0.3) return 0.2;  // Moderate: occasional cheating
    return 0.6;  // Low/zero: blatant cheating
}
```

---

*"Losing your mind is the only way to find the truth. The question is: do you want the truth?"*
