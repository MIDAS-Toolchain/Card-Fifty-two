# Card Fifty-Two: Game Concept

## Core Premise

**Card Fifty-Two** is a survival horror game where blackjack is the combat system.

You wake in an abandoned casino—abandoned except for deranged, absurd dark robotic AI dealers. The only way to defeat them is to play blackjack against them. And win.

## Setting

- **Location**: A decaying, liminal casino space
- **Enemies**: Malfunctioning AI dealer robots—absurd, comedic, and deeply unsettling
- **Atmosphere**: Cosmic horror meets universal indifference meets stand-up tragedy
- **Core Loop**: Navigate casino → encounter dealer → play blackjack → survive → progress deeper

## Tone & Philosophy

This is **absurdist comedy that intensifies the horror, not reduces it.**

The humor doesn't relieve tension—it compounds the dread. When you're fighting for your life at a blackjack table against a robot that tells jokes about mortality while dealing cards, the absurdity *is* the horror.

**Connection to Recursive Absurdism:**
- You build strategies that trap you
- You follow rules that destroy you
- You play games that define you
- `return system;` — the game gives you the tools, and you give yourself to the game

## Victory Conditions

- **Escape**: Win enough hands to unlock deeper casino sections
- **Survive**: Manage your sanity alongside your chip count
- **Understand**: Piece together why you're here, what the dealers want, and what you've become

## MIDAS Toolchain Integration

Card Fifty-Two is both a game and a **tech demo** for the MIDAS framework:
- **Archimedes**: SDL2 rendering engine handling scene delegates
- **Daedalus**: Data structures managing cards, hands, dealers, sanity state
- **Tutorial System**: Narrative delivery engine repurposed for horror storytelling

The codebase demonstrates constitutional patterns: everything's a table or array, Card_t as value types, delegate-driven scene architecture.

## Key Features

- **Three playable character classes** (plus one unlockable)
- **Sanity meter** affecting gameplay difficulty and narrative access
- **Multi-playthrough structure** revealing hidden story on subsequent runs
- **Souls-like narrative design**: Low sanity runs unlock true endings
- **Two-tier final boss**: The Dragon (spectacle) vs The Dogma (truth)

## Design Pillars

1. **Blackjack is combat** — Every hand matters, every bet is risk
2. **Absurdity amplifies horror** — Comedy and dread are not opposites
3. **Sanity unlocks truth** — Losing your mind reveals the real story
4. **Rules are prisons** — The game you think you're playing isn't the game you're in

---

*"The house always wins. But what is the house? What are you?"*
