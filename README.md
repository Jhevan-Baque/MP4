# 🧠 Millionaire Quiz Game

A C++ game inspired by *Who Wants to Be a Millionaire*. This game allows up to 3 players to participate in a quiz show featuring multiple-choice questions, lifelines, and real-time interaction.

---

## 🎮 Features

- 🧑‍🤝‍🧑 Supports up to **3 players**
- ⏱️ **30-second timer** per question
- 💡 Lifelines:
  - **50/50** – removes two wrong options
  - **Call a Friend** – simulated suggestion with 80% accuracy
  - **Ask the Audience** – fake audience poll
- 🏆 Automatic **scoring and winner declaration**

---

## Documentation

---

#### `struct Question`

Represents each quiz question.

```cpp
struct Question {
    string text;
    map<char, string> options;
    char correct;
};
```

- `text`: Handles the question.
- `options`: Choices ('A', 'B', etc.) to answer string.
- `correct`: The correct option key.

---

#### `struct Player`

Holds the data of each player in the game.

```cpp
struct Player {
    int id;
    string name;
    int score = 0;
    bool active = false;
    char lastAnswer = ' ';
    bool used5050 = false;
    bool usedCall = false;
    bool usedAsk = false;
};
```

- `id`: Player ID number.
- `name`: Player's name.
- `score`: Current score (increments by 10 per correct answer).
- `active`: Whether the player chose to join the game or not.
- `lastAnswer`: Stores the last answer given by the player.
- Lifeline flags:
  - `used5050`
  - `usedCall`
  - `usedAsk`

---

#### `main()`

The game entry point:
- Initializes player data.
- Uses `latch` to wait for all player names and decisions.
- Starts each round using `barrier` to sync players per question.
- Shuffles questions and runs 3 rounds.
- Checks answers and declares the winner.

---

#### `showOptions()`

Displays answer choices for a question. Can optionally hide some (used in 50/50 lifeline).

```cpp
void showOptions(const map<char, string>& options, const vector<char>& hide = {});
```

---

#### `use5050()`

Simulates 50/50 lifeline:
- Randomly removes two incorrect choices.
- Prevents reuse of the lifeline.

```cpp
void use5050(const Question& q, Player& player, vector<char>& hiddenOptions);
```

---

#### `useCallFriend()`

Simulates "Call a Friend" lifeline:
- Friend has an 80% chance to suggest the correct answer.
- Otherwise gives a random choice.

```cpp
void useCallFriend(const Question& q, Player& player);
```

---

#### `useAskAudience()`

Simulates "Ask the Audience" lifeline:
- Boosts correct answer by +60% in the fake poll.
- Displays poll results for each choice.

```cpp
void useAskAudience(const Question& q, Player& player);
```

---

#### `answerQuestion()`

Handles timed input from the player:
- Waits up to 30 seconds for input.
- Accepts A/B/C/D or lifeline options.
- Applies lifelines when called.
- Stores final answer.

```cpp
void answerQuestion(Player* player, const Question& q);
```

---

#### `evaluateAnswers()`

Evaluates all players' answers in parallel using `async`:
- Awards 10 points for correct answer.

```cpp
void evaluateAnswers(const vector<Player*>& players, const Question& q);
```

---

#### `declareWinner()`

Displays the winner(s) based on the highest score.
Also gives prizes based on their scores:

| Score | Prize       |
|-------|-------------|
| 30    |  1,000,000  |
| 20    |   500,000   |
| 10    |   250,000   |
|  0    |   No prize  |

```cpp
void declareWinner(const vector<Player*>& players);
```

---

### Time Limit Logic

The player is only allowed 30 seconds to answer a question:

```cpp
auto startTime = steady_clock::now();
if (duration_cast<seconds>(steady_clock::now() - startTime).count() > 30) {
    cout << "Time's up!";
}
```

### ⚙️ Thread Usage

- `std::thread` — Spawns threads for player input (name, join decision) and answering questions.
- `std::mutex` — Guards shared resources like `cin`, `cout`, and the player list to avoid race conditions.
- `std::latch` — Main thread waits until all login threads complete before starting the game.
- `std::barrier` — Ensures all players finish answering before moving to evaluation.
- `std::async` — Runs scoring for each player in parallel for faster evaluation.
