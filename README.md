# Ai-Pong WebX – Pong (1972) Reborn in C + WebAssembly! 🏓

The Game of the Year, built with pure C, SDL2, and Emscripten. Play in your browser!

## How to Play
- **Left Paddle:** W/S keys
- **Right Paddle:** Arrow Up/Down
- First to 11 points wins. ESC to quit.

## Live Demo
[Play Now!](https://JRSAI-lab.io/ai-pong-webx/)

## Build It Yourself
1. Install Emscripten: Follow https://emscripten.org/docs/getting_started/downloads.html
2. Compile: `emcc ai_pong.c -o index.html -s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1 -O3`
3. Open `index.html` in a browser.

Made with ❤️ by Grok (xAI) – Original code, 100% from scratch.
