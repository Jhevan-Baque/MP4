#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <algorithm>
#include <random>
#include <future>
#include <latch>
#include <barrier>
#include <chrono>
using namespace std;
using namespace chrono;

constexpr int NUM_ROUNDS = 3; // holds how many rounds.
mutex cout_mutex;
mutex input_mutex;

// Structure for each question, holds question text, options, and correct answer.
struct Question {
    string text;
    map<char, string> options;
    char correct;
};

// Player structure
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

// Questions list, holds all possible questions.
vector<Question> questions = {
    {"What is the capital of France?", {{'A', "London"}, {'B', "Paris"}, {'C', "Berlin"}, {'D', "Madrid"}}, 'B'},
    {"Which is the Red Planet?", {{'A', "Venus"}, {'B', "Earth"}, {'C', "Mars"}, {'D', "Jupiter"}}, 'C'},
    {"Who wrote Romeo and Juliet?", {{'A', "Shakespeare"}, {'B', "Hemingway"}, {'C', "Tolkien"}, {'D', "Rowling"}}, 'A'},
    {"In Greek mythology, who is the god of the sea?", {{'A',"Poseidon"}, {'B', "Zeus"}, {'C', "Apollo"}, {'D', "Hades"}}, 'A'},
    {"What do you call a word that is the same forwards and backwards?", {{'A', "synonym"},{'B', "antonym"},{'C', "homophone"},{'D', "palindrome"}},'D'},
    {"In computing, what does CPU stand for?", {{'A', "control program unit"},{'B', "computer power utility"},{'C', "central processing unit"},{'D', "central power unit"}},'C'},
    {"Which scientist proposed the three laws of motion?", {{'A', "Galilei"},{'B', "Einstein"},{'C', "Newton"},{'D', "Tesla"}},'C'},
    {"What is the hypotenuse of a right triangle when its legs are 3 and 4?", {{'A', "5"},{'B', "6"},{'C', "7"},{'D', "8"}},'A'},
    {"Which painter is known for cutting off part of his own ear?", {{'A', "Picasso"},{'B', "Van Gogh"},{'C', "Monet"},{'D', "Da Vinci"}},'B'}
};

// Just shows the answer options example after 50/50
void showOptions(const map<char, string>& options, const vector<char>& hide = {}) {
    for (const auto& [key, text] : options)
        if (find(hide.begin(), hide.end(), key) == hide.end())
            cout << "[" << key << "] " << text << "\n";
}

// 50/50 lifeline – removes 2 wrong answers
void use5050(const Question& q, Player& player, vector<char>& hiddenOptions) {
    if (player.used5050) {
        cout << "You already used 50/50.\n";
        return;
    }
    player.used5050 = true;
    vector<char> wrongChoices;
    for (const auto& [opt, _] : q.options)
        if (opt != q.correct)
            wrongChoices.push_back(opt);
    shuffle(wrongChoices.begin(), wrongChoices.end(), mt19937{ random_device{}() });
    hiddenOptions.push_back(wrongChoices[0]);
    hiddenOptions.push_back(wrongChoices[1]);
    cout << "[50/50] Two wrong options removed!\n";
}

// Call a friend lifeline – 80% chance they'll say the correct answer
void useCallFriend(const Question& q, Player& player) {
    if (player.usedCall) {
        cout << "You already used Call a Friend.\n";
        return;
    }
    player.usedCall = true;
    bool friendCorrect = (rand() % 100 < 80);
    char suggestion = friendCorrect ? q.correct : 'A' + rand() % 4;
    cout << "[Call a Friend] Your friend thinks the answer is: " << suggestion << "\n";
}

// Ask the audience – simulated poll that boosts the correct answer
void useAskAudience(const Question& q, Player& player) {
    if (player.usedAsk) {
        cout << "You already used Ask the Audience.\n";
        return;
    }
    player.usedAsk = true;
    map<char, int> poll = {{'A', 10}, {'B', 10}, {'C', 10}, {'D', 10}};
    poll[q.correct] += 60;
    cout << "[Ask the Audience] Audience Poll Results:\n";
    for (const auto& [opt, votes] : poll)
        cout << "  " << opt << ": " << votes << "%\n";
}

// Handles player input during their turn
void answerQuestion(Player* player, const Question& q) {
    char finalAnswer = ' ';
    vector<char> hiddenOptions;
    auto startTime = steady_clock::now();

    while (true) {
        if (duration_cast<seconds>(steady_clock::now() - startTime).count() > 30) {
            cout << "\n[" << player->name << "] Time's up!\n";
            break;
        }
        {   scoped_lock input_lock(input_mutex);
            cout << "[" << player->name << "] Choose A/B/C/D or lifeline (5050 / call / ask): ";
            string input;
            getline(cin, input);
            transform(input.begin(), input.end(), input.begin(), ::tolower);

            if (input == "5050") {
                use5050(q, *player, hiddenOptions);
                showOptions(q.options, hiddenOptions);
            } else if (input == "call") {
                useCallFriend(q, *player);
            } else if (input == "ask") {
                useAskAudience(q, *player);
            } else if (input.size() == 1 && q.options.count(toupper(input[0])) &&
                       find(hiddenOptions.begin(), hiddenOptions.end(), toupper(input[0])) == hiddenOptions.end()) {
                finalAnswer = toupper(input[0]);
                break;
            } else {
                cout << "Invalid input.\n";
            }
        }
    }
    player->lastAnswer = finalAnswer;
}

// After every round, check if answers are correct and update scores
void evaluateAnswers(const vector<Player*>& players, const Question& q) {
    vector<future<void>> futures;
    for (Player* player : players) {
        futures.push_back(async(launch::async, [=]() {
            scoped_lock lock(cout_mutex);
            cout << player->name << " answered: " << player->lastAnswer << " - ";
            if (player->lastAnswer == q.correct) {
                player->score += 10;
                cout << "Correct! +10 points.\n";
            } else {
                cout << "Wrong. No points.\n";
            }
        }));
    }
    for (auto& f : futures) f.wait();
}

// Check who got the highest score and print result with prize
void declareWinner(const vector<Player*>& players) {
    int highest = 0;
    for (const Player* p : players)
        highest = max(highest, p->score);
    cout << "\nWinner(s): \n";
    for (const Player* p : players) {
        if (p->score == highest) {
            cout << p->name << " with " << p->score << " points";
            if (p->score == 30) cout << " - Prize: 1,000,000\n";
            else if (p->score == 20) cout << " - Prize: 500,000\n";
            else if (p->score == 10) cout << " - Prize: 250,000\n";
            else cout << " - Prize: :(\n";
        }
    }
}

int main() {
    int maxPlayers = 3;
    vector<Player> players(maxPlayers);
    vector<Player*> activePlayers;
    mutex activeMutex;
    cout << "Waiting for players to login...\n";
    vector<thread> threads;
    vector<bool> joined(maxPlayers, false);
    latch latch(maxPlayers);

    // Wait for everyone to log in and ask if they wanna play
    for (int i = 0; i < maxPlayers; ++i) {
        players[i].id = i + 1;
        threads.emplace_back([&, i]() {
            scoped_lock input_lock(input_mutex);
            cout << "Player " << (i + 1) << ", enter your name: ";
            getline(cin, players[i].name);
            cout << "[" << players[i].name << "] Do you want to join the quiz? (yes/no): ";
            string response;
            getline(cin, response);
            if (response == "yes") {
                players[i].active = true;
                scoped_lock lock(cout_mutex);
                cout << "[" << players[i].name << "] has joined the quiz!\n";
                scoped_lock lock2(activeMutex);
                activePlayers.push_back(&players[i]);
            } else {
                scoped_lock lock(cout_mutex);
                cout << "[" << players[i].name << "] chose not to join.\n";
            }
            latch.count_down();
        });
    }
    latch.wait(); // wait for all threads to finish asking players
    for (auto& t : threads) t.join();
    if (activePlayers.empty()) {
        cout << "No players joined. Game canceled.\n";
        return 0;
    }
    cout << "\nGame Starting with " << activePlayers.size() << " players...\n";
    barrier roundBarrier((int)activePlayers.size());
    // Shuffle questions
    shuffle(questions.begin(), questions.end(), mt19937{ random_device{}() });
    // Game rounds
    for (int round = 0; round < NUM_ROUNDS; ++round) {
        const Question& q = questions[round];
        cout << "\nRound " << (round + 1) << ": " << q.text << "\n";
        showOptions(q.options);
        vector<thread> roundThreads;
        for (Player* player : activePlayers) {
            roundThreads.emplace_back([&, player]() {
                answerQuestion(player, q);
                roundBarrier.arrive_and_wait(); // make sure everyone finishes before we continue
            });
        }
        for (auto& t : roundThreads) t.join();
        cout << "\nResults for Round " << (round + 1) << ":\n";
        evaluateAnswers(activePlayers, q);
        cout << "\nScores:\n";
        for (const Player* p : activePlayers)
            cout << p->name << ": " << p->score << " points\n";
    }
    declareWinner(activePlayers);
    return 0;
}
