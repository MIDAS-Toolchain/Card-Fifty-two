# Card Fifty-Two

**A Blackjack game demonstrating clean Scene → Section → Component architecture in C.**

Tech demo for [Archimedes](https://github.com/McCoy1701/Archimedes) (SDL2 framework) + [Daedalus](https://github.com/McCoy1701/Daedalus) (data structures). Shows how to build maintainable game UIs without raw pointers or manual memory chaos.

---

## 🎯 Tech Demo Highlights

- **Scene → Section → Component** architecture with clear boundaries
- **String handling patterns**: Static storage (`char[]`) vs dynamic building (`dString_t*`)
- **Memory safety**: Everything's a table or array - zero naked pointers
- **Reusable components**: Button, MenuItem, CardGridModal used across multiple scenes
- **Tutorial system**: Step-by-step overlay with spotlight and event listeners

---

## 🏗️ Architecture in Action

### Scene → Section → Component Hierarchy

| Layer | File | Responsibility | Example |
|-------|------|----------------|---------|
| **Scene** | [`sceneBlackjack.c`](src/scenes/sceneBlackjack.c) | Game loop, state machine, input routing | Blackjack game |
| **Section** | [`playerSection.c`](src/scenes/sections/playerSection.c) | Layout group (title, cards, chips) | Player's hand area |
| **Component** | [`button.c`](src/scenes/components/button.c) | Reusable UI element | Hit/Stand/Double buttons |

### Real Implementation Examples

**Button Component** (Pattern 1: Static Storage)
```c
// include/scenes/components/button.h
typedef struct Button {
    char label[256];           // Fixed buffer - set once, rarely changes
    char hotkey_hint[64];      // Static storage with strncpy()
    int x, y, w, h;
    bool enabled;
} Button_t;
```
[→ button.h](include/scenes/components/button.h) | [→ button.c](src/scenes/components/button.c)

**PlayerSection** (Pattern 2: Dynamic Building)
```c
// src/scenes/sections/playerSection.c
dString_t* info = d_StringInit();
d_StringFormat(info, "%s: %d%s", player->name, player->score,
               player->is_bust ? " (BUST)" : "");
a_DrawText((char*)d_StringPeek(info), x, y, ...);
d_StringDestroy(info);  // Temporary string for this frame
```
[→ playerSection.c](src/scenes/sections/playerSection.c)

**Component Reuse**
- `Button_t` used in: ActionPanel (betting/actions), TopBarSection (settings), PauseMenuSection (resume/quit)
- `MenuItem_t` used in: MainMenuSection, PauseMenuSection, SettingsScene
- `CardGridModal_t` used in: DrawPileModal, DiscardPileModal

[→ All Components](src/scenes/components/) | [→ All Sections](src/scenes/sections/)

---

## 🔑 Key Design Patterns

### 1. String Handling Decision Guide

**Use `char[]` for static storage:**
```c
// Labels that rarely change
char label[256];
strncpy(button->label, "Hit", sizeof(button->label) - 1);
```
Examples: [button.c:43](src/scenes/components/button.c#L43), [menuItem.c:43](src/scenes/components/menuItem.c#L43), [actionPanel.h:20](include/scenes/sections/actionPanel.h#L20)

**Use `dString_t*` for dynamic building:**
```c
// Formatting with variables (created/destroyed each frame)
dString_t* str = d_StringInit();
d_StringFormat(str, "Score: %d | Bet: %d", score, bet);
a_DrawText((char*)d_StringPeek(str), x, y, ...);
d_StringDestroy(str);
```
Examples: [playerSection.c:113](src/scenes/sections/playerSection.c#L113), [dealerSection.c:109](src/scenes/sections/dealerSection.c#L109), [deckViewPanel.c:94](src/scenes/sections/deckViewPanel.c#L94)

### 2. Memory Ownership

**Stack-allocated singletons** (scene-scoped):
```c
static Deck_t g_test_deck;        // Scene singleton
static GameContext_t g_game;      // Scene singleton
InitDeck(&g_test_deck, 1);        // Init/Cleanup pattern
```
[→ sceneBlackjack.c:47-48](src/scenes/sceneBlackjack.c#L47-L48)

**Heap-allocated components** (created/destroyed):
```c
Button_t* btn = CreateButton(x, y, w, h, "Hit");
// ... use button ...
DestroyButton(&btn);  // Frees memory, sets pointer to NULL
```
[→ button.c:18](src/scenes/components/button.c#L18)

### 3. Data Flow Example: Button Click

1. **User clicks** → `app.mouse.pressed` set by `a_DoInput()` (Archimedes)
2. **Button detects** → `IsButtonClicked(button)` checks mouse bounds ([button.c:85](src/scenes/components/button.c#L85))
3. **Scene handles** → `HandleBettingInput()` processes click ([sceneBlackjack.c:431](src/scenes/sceneBlackjack.c#L431))
4. **Game logic** → `ProcessBettingInput()` validates and updates state ([game.c](src/game.c))
5. **Render reflects** → `RenderButton()` shows updated state ([button.c:130](src/scenes/components/button.c#L130))

---

## 🚀 Quick Start

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install build-essential libsdl2-dev libsdl2-image-dev \
    libsdl2-ttf-dev libsdl2-mixer-dev libcjson-dev

# Install Archimedes + Daedalus (see their repos for instructions)
```

### Build & Run
```bash
git clone <this-repo>
cd card-fifty-two
make
./bin/card-fifty-two
```

### Controls
| Key | Action |
|-----|--------|
| **ESC** | Pause / Menu |
| **H** | Hit (draw card) |
| **S** | Stand (end turn) |
| **D** | Double down |
| **1-3** | Place bet (10/50/100) |
| **V** | View draw pile |
| **C** | View discard pile |

---

## 📚 For Developers

**Architecture Documentation:**
- [Data Structures](__architecture/baseline/01_data_structures.md) - Deck, Hand, Player, string patterns
- [Components](__architecture/rendering/02_components.md) - Button, MenuItem, CardGridModal
- [Scenes](__architecture/rendering/01_scenes.md) - Scene structure, FlexBox layout

**Design Documentation:**
- [Game State Machine](__design/baseline/04_game_state_machine.md) - State transitions, input flow
- [Blackjack Rules](__design/baseline/01_blackjack_core.md) - Game logic, scoring, dealer AI

**Key Files:**
- [`CLAUDE.md`](CLAUDE.md) - Constitutional patterns for AI collaboration (explains "Everything's a table or array")
- [`common.h`](include/common.h) - Global includes, constants, color palette
- [`Makefile`](Makefile) - Build system with native + web targets

---

## 🛠️ Technical Stack

- **Language**: C11
- **Graphics**: SDL2 via Archimedes
- **Data Structures**: Daedalus (`dArray_t`, `dTable_t`, `dString_t`)
- **Build**: Make + Emscripten (web)
- **Platforms**: Linux, macOS, Windows, Web

---

## 🎨 Design Philosophy

> **"Everything's a table or array"** - No raw pointers, no manual `malloc` for collections.

**Constitutional Patterns** (from [`CLAUDE.md`](CLAUDE.md)):
1. All collections use `dArray_t` or `dTable_t` (never raw arrays)
2. Cards are value types (48 bytes, copied by value)
3. Players/components are pointer types (heap allocated)
4. Scenes use Archimedes delegate pattern (`app.delegate.logic`, `app.delegate.draw`)
5. String handling: `char[]` for static, `dString_t*` for dynamic

**Benefits:**
- ✅ Memory safety (Daedalus handles allocations)
- ✅ Clear ownership model (who frees what)
- ✅ Testable (mockable tables, clear data flow)
- ✅ Portable (pure C, works on web via Emscripten)

---

## 🤝 Contributing

This is a **reference implementation** demonstrating:
- Clean C architecture without OOP
- Scene/Section/Component separation
- Memory-safe data structures
- String handling best practices
- Tutorial system implementation

Pull requests welcome for:
- Bug fixes
- New card games (Poker, Hearts)
- Component improvements
- Documentation clarity

---

## 📄 License

[To be determined - likely GPL-2.0 to match Archimedes/Daedalus]

---

## 🙏 Credits

Built with:
- [Archimedes](https://github.com/McCoy1701/Archimedes) by McCoy1701
- [Daedalus](https://github.com/McCoy1701/Daedalus) by McCoy1701

Part of the **MIDAS Toolchain** ecosystem.

---

**Ready to explore?** Start with [`sceneBlackjack.c`](src/scenes/sceneBlackjack.c) → [`playerSection.c`](src/scenes/sections/playerSection.c) → [`button.c`](src/scenes/components/button.c) 🎴
