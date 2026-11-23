# Shift Agent

A C++ chat agent that uses llama.cpp and MCP to view and assign employee shifts.

## 1. Install

**Ubuntu / WSL**

```bash
sudo apt update
sudo apt install build-essential git curl nlohmann-json3-dev libcurl4-openssl-dev
curl -LsSf https://llama.app/install.sh | sh
```

Follow the installer’s instructions, then open a new terminal.

**Mac**

Install [Homebrew](https://brew.sh/) if needed, then run:

```bash
brew install llama.cpp nlohmann-json curl
```

## 2. Clone

```bash
git clone https://github.com/Jor123kna/shift-agent.git
cd shift-agent
```

Make sure `data/employees.json` and `data/shifts.json` exist. If the repo includes `.example.json` versions, copy them to those names.

## 3. Build

**Ubuntu / WSL**

```bash
g++ -std=c++17 agent.cpp employee.cpp shift.cpp storage.cpp scheduler.cpp -o shift-mcp
g++ -std=c++17 chat.cpp -lcurl -o shift-chat
```

**Mac**

```bash
clang++ -std=c++17 -I"$(brew --prefix nlohmann-json)/include" \
  agent.cpp employee.cpp shift.cpp storage.cpp scheduler.cpp -o shift-mcp

clang++ -std=c++17 -I"$(brew --prefix nlohmann-json)/include" \
  -I"$(brew --prefix curl)/include" chat.cpp \
  -L"$(brew --prefix curl)/lib" -lcurl -o shift-chat
```

## 4. Run

In **Terminal 1**, start the model.

**Ubuntu / WSL**

```bash
llama serve -hf ggml-org/Qwen3-1.7B-GGUF:Q4_K_M --jinja --port 9931 -ngl 0 -c 4096
```

**Mac**

```bash
llama-server -hf ggml-org/Qwen3-1.7B-GGUF:Q4_K_M --jinja --port 9931 -c 4096
```

Wait for the model to load and leave Terminal 1 open. In **Terminal 2**, open the cloned `shift-agent` folder and run:

```bash
./shift-chat
```

At `You:`, type `Show me the current shifts.` You should see `[Calling list_shifts]` followed by the schedule.