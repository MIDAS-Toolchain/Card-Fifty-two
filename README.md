# Card Fifty-Two

A comprehensive card game framework built with **Archimedes** (SDL2 game framework) and **Daedalus** (data structures library). Features a 52-card deck engine supporting Blackjack, Poker, and other classic card games.

## 🎮 Features

- **Standard 52-card deck** with smart shuffling and dealing
- **Blackjack implementation** with dealer AI and basic strategy
- **Extensible architecture** - Add new card games easily
- **Cross-platform** - Native (Linux/macOS/Windows) and Web (Emscripten)
- **"Everything's a table or array"** - No raw pointers, memory-safe design
- **Visual card rendering** with texture caching
- **Smooth animations** for card dealing and movements

## 🏗️ Architecture Highlights

- **Deck**: `dArray_t` of 52 `Card_t` structs (value types)
- **Player Hands**: Each player has `dArray_t` for cards
- **Players**: `dTable_t` hash table keyed by player ID
- **Textures**: `dTable_t` cache mapping card IDs to SDL textures
- **Game State**: `dTable_t` for dynamic state variables

**Zero naked pointers. Zero manual `malloc` for game data.**

## 📋 Prerequisites

### Required Libraries

- **Archimedes** - SDL2-based game framework
  - SDL2, SDL2_image, SDL2_ttf, SDL2_mixer, cJSON

- **Daedalus** - Data structures and utilities library
  - No external dependencies

### System Requirements

```bash
# Debian/Ubuntu
sudo apt-get install build-essential libsdl2-dev libsdl2-image-dev \
    libsdl2-ttf-dev libsdl2-mixer-dev libcjson-dev

# Arch Linux
sudo pacman -S base-devel sdl2 sdl2_image sdl2_ttf sdl2_mixer cjson

# macOS
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer cjson
```

## 🚀 Installation

### 1. Install Archimedes
```bash
git clone https://github.com/McCoy1701/Archimedes.git
cd Archimedes
make shared
sudo make install
cd ..
```

### 2. Install Daedalus
```bash
git clone https://github.com/McCoy1701/Daedalus.git
cd Daedalus
make shared
sudo make install
cd ..
```

### 3. Build Card-Fifty-Two
```bash
git clone <this-repo-url>
cd card-fifty-two
make
```

## 🎲 Running the Game

### Native (Desktop)
```bash
make run
# or
./bin/card-fifty-two
```

### Web (Emscripten)
```bash
make web
python3 -m http.server 8000
# Visit http://localhost:8000/bin/index.html
```

## 🎯 How to Play (Blackjack)

1. **Place Bet** - Click bet amount (10, 25, 50, 100) or use number keys
2. **Initial Deal** - Receive 2 cards, dealer shows 1 card
3. **Player Turn** - Choose actions:
   - **Hit (H)** - Take another card
   - **Stand (S)** - End your turn
   - **Double (D)** - Double bet, take 1 card, end turn
4. **Dealer Turn** - Dealer reveals hidden card and plays (hits on 16, stands on 17)
5. **Showdown** - Compare hands, payouts:
   - Blackjack: 3:2 (bet × 2.5)
   - Win: 1:1 (bet × 2.0)
   - Push: Return bet

### Controls

| Input | Action |
|-------|--------|
| **ESC** | Back to menu / Quit |
| **H** | Hit |
| **S** | Stand |
| **D** | Double down |
| **Mouse** | Click buttons/cards |

## 📂 Project Structure

```
card-fifty-two/
├── __architecture/        # Architecture documentation
│   └── baseline/
│       ├── 00_project_setup.md
│       ├── 01_data_structures.md
│       ├── 02_rendering_pipeline.md
│       ├── 03_module_boundaries.md
│       └── 04_memory_strategy.md
├── __design/             # Design documentation
│   └── baseline/
│       ├── 00_game_core.md
│       ├── 01_card_system.md
│       ├── 02_deck_management.md
│       ├── 03_player_system.md
│       ├── 04_game_state_machine.md
│       ├── 05_blackjack_rules.md
│       ├── 06_rendering_strategy.md
│       └── 07_input_handling.md
├── include/              # Header files (to be created)
├── src/                  # Source files (to be created)
├── resources/            # Assets (fonts, textures, audio)
├── Makefile
└── README.md
```

## 🛠️ Development

### Build Modes

```bash
# Debug build
make CFLAGS="-g -O0 -DDEBUG"

# Release build
make CFLAGS="-O3 -DNDEBUG"

# Clean build
make clean
```

### Testing

```bash
# Run with Valgrind (memory leak detection)
valgrind --leak-check=full ./bin/card-fifty-two

# Run with Address Sanitizer
make CFLAGS="-fsanitize=address -g"
./bin/card-fifty-two
```

## 📚 Documentation

Comprehensive documentation available in `__architecture/` and `__design/` directories:

- **Architecture**: Build system, data structures, rendering, modules, memory management
- **Design**: Game core, card system, deck, players, state machine, rules, rendering, input

## 🎨 Extensibility

The architecture supports adding new card games:

### Poker (Future)
- **5-card hands** - Use same `Hand_t` structure
- **Hand ranking** - Add `PokerHandType_t` enum
- **Community cards** - Add `dArray_t* community_cards`
- **Betting rounds** - New states: PREFLOP, FLOP, TURN, RIVER

### Hearts (Future)
- **Passing cards** - Add `STATE_PASSING`
- **Trick tracking** - Add `dArray_t* current_trick`
- **Scoring** - Store per-round scores in `state_data` table

**All games share the same data structures - only rules change!**

## 🔧 Technical Stack

- **Language**: C (C11 standard)
- **Graphics**: SDL2 via Archimedes framework
- **Data Structures**: Daedalus (dArray, dTable, dString)
- **Build System**: Make
- **Cross-Platform**: Native + Emscripten/WebAssembly

## 📝 Design Philosophy

> **"Everything's a table or array"** - Daedalus ideology

- All dynamic collections use `dArray_t` or `dTable_t`
- Cards are value types (no heap allocation)
- Global texture cache for O(1) lookups
- Clear ownership model prevents memory leaks
- No raw pointers in game data

## 🤝 Contributing

This is a tech demo for the **MIDAS Toolchain**:
- **Metis**: C linter
- **Ixion**: Memory safety tools
- **Daedalus**: Core library (this project)
- **Archimedes**: UI framework (this project)
- **Sisyphus**: Testing framework

## 📄 License

[Specify license - GPL-2.0 like Archimedes/Daedalus?]

## 🙏 Acknowledgments

Built with:
- [Archimedes](https://github.com/McCoy1701/Archimedes) - SDL2 game framework
- [Daedalus](https://github.com/McCoy1701/Daedalus) - Data structures library

---

**Ready to play? Run `make && make run` to start!** 🎴
