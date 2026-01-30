#include <cstdlib>
#include <iostream>
#include <ctime>
#include <limits>
#include <string>
#include <cstring>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>

using namespace std;

#define RESET "\u001B[0m"
#define RED "\u001B[31m"
#define GREEN "\u001B[32m"
#define YELLOW "\u001B[33m"
#define BLUE "\u001B[34m"
#define CYAN "\u001B[36m"

#define SIZE 15 
#define CELL_NUMBER 52
#define MAX_PLAYER 4
#define MAX_DE 6
sem_t sem;

class Cell
{
public:
    int x;
    int y;
    int value;
};

class Player
{
public:
    int x;
    int y;
    int team;
    int index;
    int id;
    int removed;   //killed opponent?
    int position;  // winning position
    bool tokenInHouse; // tokens in house   
    bool ingame; // is playing game?
    int counter;
    string name;
};
// Global Variables
Cell cells[CELL_NUMBER];
Cell houses[MAX_PLAYER][4];
char board[SIZE][SIZE]; // Grid
int n_players = -1;       // No of players
Player players[MAX_PLAYER][4]; // Players
int position = 1;
int winning_position = 1;
int n_tokens=-1;
bool *isfinished = new bool[4];
int *diceroll = new int[3];





bool isValidChoice(int diceRoll, int currentPlayer, int selectedToken)
{
    // Check if the choice is within valid range and respects token count
    if (selectedToken < 1 || selectedToken > 4 || selectedToken > n_tokens)
    {
        return false;
    }

    // Check if the token can be moved (either it's not on the board or would exceed the maximum index)
    bool invalidCondition = (diceRoll != 6 && players[currentPlayer][selectedToken - 1].index == -1) ||
                            ((players[currentPlayer][selectedToken - 1].index + diceRoll) > 106);
    if (invalidCondition)
    {
        return false;
    }

    // If all checks pass, the choice is valid
    return true;
}

void verifyInput()
{
    if (!cin)
    {
        cout << "ERROR - Enter a valid number";

        cin.clear();

        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}



// int getIndexByTurn(int turn)
// {
//     return turn * 13;
// }


void renderBoard()
{
    char tempGrid[SIZE][SIZE];

    // Clone the current board layout into a temporary grid
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            tempGrid[i][j] = board[i][j];
        }
    }

    // Update player token positions on the temporary grid
    for (int playerIdx = 0; playerIdx < MAX_PLAYER; playerIdx++)
    {
        for (int tokenIdx = 0; tokenIdx < 4; tokenIdx++)
        {
            tempGrid[players[playerIdx][tokenIdx].y][players[playerIdx][tokenIdx].x] = '0' + players[playerIdx][tokenIdx].id;
        }
    }

    // Display the board with enhanced visuals
    cout << "\nCurrent Board State:\n";
    for (int row = 0; row < SIZE; row++)
    {
        cout << " ";
        for (int col = 0; col < SIZE; col++)
        {
            string cellContent = "";
            cellContent += tempGrid[row][col];
            cellContent += " ";

            // Apply colors and symbols based on the cell's content
            if (tempGrid[row][col] == 'U')
            {
                cellContent = "O ";
                cellContent = RED + cellContent + RESET;
            }
            else if (tempGrid[row][col] == 'R')
            {
                cellContent = "O ";
                cellContent = GREEN + cellContent + RESET;
            }
            else if (tempGrid[row][col] == 'A')
            {
                cellContent = "O ";
                cellContent = CYAN + cellContent + RESET;
            }
            else if (tempGrid[row][col] == 'H')
            {
                cellContent = "O ";
                cellContent = YELLOW + cellContent + RESET;
            }
            else if (isdigit(tempGrid[row][col])) // Player tokens
            {
                int team = -1;

                // Find the team of the token
                for (int playerIdx = 0; playerIdx < MAX_PLAYER; playerIdx++)
                {
                    for (int tokenIdx = 0; tokenIdx < 4; tokenIdx++)
                    {
                        if (row == players[playerIdx][tokenIdx].y && col == players[playerIdx][tokenIdx].x)
                        {
                            team = players[playerIdx][tokenIdx].team;
                            break;
                        }
                    }
                }

                // Apply color based on the team
                if (team == 0) cellContent = RED + cellContent + RESET;
                else if (team == 1) cellContent = GREEN + cellContent + RESET;
                else if (team == 2) cellContent = CYAN + cellContent + RESET;
                else if (team == 3) cellContent = YELLOW + cellContent + RESET;
            }

            cout << cellContent;
        }
        cout << endl;
    }
    cout << endl;
}




string idToColor(int id)
{
    if (id == 0)
    {
        return RED;
    }
    else if (id == 1)
    {
        return GREEN;
    }
    else if (id == 2)
    {
        return CYAN;
    }
    else if (id == 3)
    {
        return YELLOW;
    }
    else
    {
        return "";
    }
}

bool is_safepoint(int index)
{
    if((index == 0)||(index == 8)||(index == 13)||(index == 21)||(index == 26)||(index == 34)|| (index == 39) || (index == 47))
    {
        return true;
    }
    return false;
}


bool isBlock(int currentTurn, int selectedToken)
{
    int blockCount = 0;

    // Iterate over all players to check for blocking conditions
    for (int playerIndex = 0; playerIndex < MAX_PLAYER; playerIndex++)
    {
        // Skip the current player
        if (playerIndex == currentTurn)
        {
            continue;
        }

        // Check all tokens of the other players
        for (int tokenIndex = 0; tokenIndex < n_tokens; tokenIndex++)
        {
            Player &opponentToken = players[playerIndex][tokenIndex];
            Player &currentToken = players[currentTurn][selectedToken - 1];

            // Check if tokens are on the same position and from different teams
            if (opponentToken.x == currentToken.x &&
                opponentToken.y == currentToken.y &&
                opponentToken.team != currentToken.team)
            {
                // If the position is not a safe point, increase block count
                if (!is_safepoint(opponentToken.index))
                {
                    blockCount++;
                }

                // If two or more tokens block the position, it's a block
                if (blockCount >= 2)
                {
                    return true;
                }
            }
        }
    }

    // No block found
    return false;
}


bool isMovePossible(int playerTurn, int diceResult)
{
    // Check if the player's game is already finished or dice result is zero
    if (isfinished[playerTurn] == 1 || diceResult == 0)
    {
        return false;
    }

    // Check if there are tokens in the starting position (not yet moved)
    bool hasTokensInStart = false;
    for (int tokenIndex = 0; tokenIndex < n_tokens; tokenIndex++)
    {
        if (players[playerTurn][tokenIndex].index == -1)
        {
            hasTokensInStart = true;
            break;
        }
    }

    // If dice result is 6 and there are tokens in the start position, a move is possible
    if (diceResult == 6 && hasTokensInStart)
    {
        return true;
    }

    // Check if any token can move within valid limits
    for (int tokenIndex = 0; tokenIndex < n_tokens; tokenIndex++)
    {
        int currentIndex = players[playerTurn][tokenIndex].index;
        if (currentIndex != -1 && currentIndex != 106 && (currentIndex + diceResult) <= 106)
        {
            return true;
        }
    }

    // No valid moves found
    return false;
}

bool isTokenOnHomePath(int playerTurn, int tokenChoice, int tokenIndex)
{
    // Check if the token is entering the final home path based on player turn
    if (playerTurn == 0 && tokenIndex == 50 && players[playerTurn][tokenChoice - 1].removed > 0)
    {
        return true;
    }
    else if (playerTurn == 1 && tokenIndex == 11 && players[playerTurn][tokenChoice - 1].removed > 0)
    {
        return true;
    }
    else if (playerTurn == 2 && tokenIndex == 24 && players[playerTurn][tokenChoice - 1].removed > 0)
    {
        return true;
    }
    else if (playerTurn == 3 && tokenIndex == 37 && players[playerTurn][tokenChoice - 1].removed > 0)
    {
        return true;
    }
    return false;
}

void moveOnHomePath(Player tokens[MAX_PLAYER][4], int playerTurn, int tokenChoice)
{
    // Move the token based on the player's turn and final path direction
    if (playerTurn == 0) // Player 0 moves to the right
    {
        tokens[playerTurn][tokenChoice - 1].x += 1;
    }
    else if (playerTurn == 2) // Player 2 moves to the left
    {
        tokens[playerTurn][tokenChoice - 1].x -= 1;
    }
    else if (playerTurn == 1) // Player 1 moves downward
    {
        tokens[playerTurn][tokenChoice - 1].y += 1;
    }
    else if (playerTurn == 3) // Player 3 moves upward
    {
        tokens[playerTurn][tokenChoice - 1].y -= 1;
    }
}

void moveToken(int playerTurn, int tokenChoice, int diceRoll)
{
    // Handle case where the token is not yet in play and the dice roll is 6
    if (players[playerTurn][tokenChoice - 1].index == -1 && diceRoll == 6)
    {
        // Move the token from home to the starting position
        int startIndex = playerTurn * 13;
        players[playerTurn][tokenChoice - 1].index = startIndex;
        players[playerTurn][tokenChoice - 1].x = cells[startIndex].x;
        players[playerTurn][tokenChoice - 1].y = cells[startIndex].y;
    }
    else if (players[playerTurn][tokenChoice - 1].index != -1)
    {
        // Token is already in play; move it along the board based on dice roll
        int stepsRemaining = diceRoll;

        while (stepsRemaining > 0)
        {
            // Check if the token is entering the final stretch (home path)
            if (isTokenOnHomePath(playerTurn, tokenChoice, players[playerTurn][tokenChoice - 1].index))
            {
                players[playerTurn][tokenChoice - 1].index = 100; // Mark as in the final path
            }

            // Move token along the main board path if it's within the valid range
            if (players[playerTurn][tokenChoice - 1].index < CELL_NUMBER)
            {
                // Update token's position to the next cell
                players[playerTurn][tokenChoice - 1].index = (players[playerTurn][tokenChoice - 1].index + 1) % CELL_NUMBER;
                players[playerTurn][tokenChoice - 1].x = cells[players[playerTurn][tokenChoice - 1].index].x;
                players[playerTurn][tokenChoice - 1].y = cells[players[playerTurn][tokenChoice - 1].index].y;
            }
            else
            {
                // Move token on the final home path
                players[playerTurn][tokenChoice - 1].index += 1; // Increment index to simulate movement
                moveOnHomePath(players, playerTurn, tokenChoice);

                // Special condition: Check if the token has reached the final winning position
                if (players[playerTurn][tokenChoice - 1].x == 5 && players[playerTurn][tokenChoice - 1].y == 5)
                {
                    renderBoard(); // Update the board state if the token reaches the final point
                }
            }
            stepsRemaining--; // Decrement the remaining steps
        }
    }
}


void *MasterThread(void *args)
{
    // Extract turn and choice from the arguments
    int currentTurn = (*(pair<int, int> *)args).first;
    int selectedToken = (*(pair<int, int> *)args).second;

    // Check if the player has exceeded the turn limit without a valid move or dice roll of 6
    if (players[currentTurn][0].counter >= 20)
    {
        cout << "Player " << currentTurn + 1 << " is out of the game due to 20 consecutive turns without a valid move or dice roll of 6." << endl;
        players[currentTurn][0].ingame = false;
        players[currentTurn][0].position = -1; // Mark player as out of the game
        position++;
        isfinished[currentTurn] = 1; // Mark the player's game status as finished
    }

    // Handle token elimination and update hit rate
    for (int opponent = 0; opponent < MAX_PLAYER; opponent++)
    {
        if (opponent == currentTurn) // Skip the current player's turn
            continue;

        for (int token = 0; token < n_tokens; token++)
        {
            // Check if tokens are on the same position and not on a safe point
            if (players[opponent][token].x == players[currentTurn][selectedToken - 1].x &&
                players[opponent][token].y == players[currentTurn][selectedToken - 1].y &&
                !is_safepoint(players[opponent][token].index))
            {
                // Reset the eliminated token's position to its home
                players[opponent][token].index = -1;
                players[opponent][token].x = houses[opponent][token].x;
                players[opponent][token].y = houses[opponent][token].y;

                // Increment the current player's removal count for all their tokens
                for (int k = 0; k < n_tokens; k++)
                {
                    players[currentTurn][k].removed++;
                }
            }
        }
    }

    // Verify if the current player has won
    int completedTokens = 0;
    for (int token = 0; token < n_tokens; token++)
    {
        if (players[currentTurn][token].index == 106) // Check if the token is in the winning position
        {
            completedTokens++;
        }
    }

    if (completedTokens == n_tokens && players[currentTurn][0].ingame)
    {
        // Update player status as a winner
        players[currentTurn][0].ingame = false;
        players[currentTurn][0].position = winning_position; // Assign the winning position
        winning_position++;
        position++;
        isfinished[currentTurn] = 1; // Mark the player's game status as finished
    }

    return NULL;
}

void *createMasterThread(int turn = 0 , int choice = -1)
{
    pthread_t master;
    pair<int, int> args = make_pair(turn, choice);
    pthread_create(&master, NULL, MasterThread, (void *)&args);
    void *status;
    pthread_join(master, &status);
    return status;
}

void *PlayerThread(void * arg)
{
        sem_wait(&sem);
        int turn = *(int *) arg;
        int j=0;
        int result;
        for(int i=0; i<3; i++)
        {
            diceroll[i] = 0;
        }
        renderBoard();
        int choice = -1;
        if(isfinished[turn]!=1)
        {
            cout << endl << ("It's up to player " + idToColor(turn)) << players[turn][0].name << " to play." << RESET << endl;
            j=0;
            do // again turn on case of 6
            {
            result = (rand() % MAX_DE) + 1;
            cout << "Launch of the dice ... Result : " << result << endl;
            diceroll[j] = result;
            j++;
                if(diceroll[2] == 6)
                {
                    cout << "Result : 6,6,6 so, Passing the turn\n";
                    
                    for(int i=0; i<3; i++)
                    {
                        diceroll[i] = 0;
                    }
                    createMasterThread(turn,choice);
                    sem_post(&sem);
                    break;
                }
            }while(result==6); 
        }
        j=0;
        bool is_any_movepossible = isMovePossible(turn,diceroll[0]);
        if(!is_any_movepossible && players[turn][0].ingame==true)
        {
            players[turn][0].counter++;
        }
        else
        {
            players[turn][0].counter = 0;
        }
        createMasterThread(turn,choice);
        if(!is_any_movepossible)
        {
            sem_post(&sem);
            return NULL;
        }
        while ((is_any_movepossible) && diceroll[j]!=0 && j<3)
        {
            cout << "Which token do you want to move a throw of " << diceroll[j] << endl << endl;
            cout << ">";
            cin >> choice;
            verifyInput();
            int c;
            c=0;
            while(!isValidChoice(diceroll[j],turn,choice) || c == 4 && isBlock(turn,choice))
            {
                cout << "Invalid token! Enter valid token : ";
                cin >> choice;
                verifyInput();
                c++;  // c is used just to be safe in case some unexpected situation occurs
            }
            if(c!=4)
            {
                moveToken(turn,choice,diceroll[j]);
                createMasterThread(turn,choice);
            }
            j++;
        } 
        bool win = true;
        for(int i=0; i<n_tokens; i++)
        {
            if(players[turn][i].index!=106)
            {
                win = false;
            }
        }
        if(win)
        {
            createMasterThread(turn,choice);
        }
    sem_post(&sem);
    return NULL;
}

void display()
{
    cout << endl << endl << "-------------Result of the Game---------------" << endl << endl << endl;
    cout << "Number of Tokens : " << n_tokens << endl << endl << endl;
    for(int i=0; i<n_players; i++)
    {
        cout << "Player " << i+1 << "'s Name : " << players[i][0].name << endl;
        if(players[i][0].position == -1)
        {
            cout << "Player " << i+1 << "'s Position : None (out of Game)" << endl;
        }
        else if(players[i][0].position == n_players)
        {
            cout << "Player " << i+1 << "'s Position : Last " << endl;
        }
        else
        {
        cout << "Player " << i+1 << "'s Position : " << players[i][0].position << endl;
        }    
        cout << "Player " << i+1 << "'s Hit Rate : " << players[i][0].removed << endl;
        cout << endl << endl ;
    }
}


void menu(int &choice)
{
    while (choice > 2 || choice < 1)
    {
        cout << endl
             <<"Select an option from below(1/2):"<<endl
             << "1 - Play" << endl;
        cout << "2 - Quit " << endl
             << endl
             << ">";
        cin >> choice;
        verifyInput();
    }

    if (choice == 1)
    {
        while (n_players < 2 || n_players > 4)
        {
            cout << endl
                 << "Select number of players between 2 and 4 ?" << endl
                 << endl
                 << ">";
            cin >> n_players;
            verifyInput();
        }
        while (n_tokens < 1 || n_tokens > 4)
        {
            cout << endl
                 << "Select number of tokens between 1 and 4) ?" << endl
                 << endl
                 << ">";
            cin >> n_tokens;
            verifyInput();
        }
    }
}

int play()
{
    // initGame();
     int newCells[CELL_NUMBER][2] = {
        {6, 1}, {6, 2}, {6, 3}, {6, 4}, {6, 5},
        {5, 6}, {4, 6}, {3, 6}, {2, 6}, {1, 6}, {0, 6},{0,7},
        {0, 8}, {1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8},
        {6, 9}, {6, 10}, {6, 11}, {6, 12}, {6, 13}, {6, 14},{7, 14},
        {8, 14}, {8, 13}, {8, 12}, {8, 11}, {8, 10}, {8, 9},
        {9, 8}, {10, 8}, {11, 8}, {12, 8}, {13, 8}, {14, 8}, {14, 7},
        {14, 6}, {13, 6}, {12, 6}, {11, 6}, {10, 6}, {9, 6},
        {8, 5}, {8, 4}, {8, 3}, {8, 2}, {8, 1}, {8, 0}, {7, 0},{6,0} };
    int newHouses[MAX_PLAYER][4][2] = 
    {
        {{2, 2}, {2, 3}, {3, 3}, {3, 2}},
        {{2, 11}, {2, 12}, {3, 12}, {3, 11}},
        {{11, 11}, {11, 12}, {12, 12}, {12, 11}},
        {{11, 2}, {11, 3}, {12, 3}, {12, 2}},
    };

    for (int i = 0; i < CELL_NUMBER; i++)
    {
        cells[i].y = newCells[i][0];
        cells[i].x = newCells[i][1];
        cells[i].value = 0;
    }

    for (int i = 0; i < MAX_PLAYER; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            houses[i][j].y = newHouses[i][j][0];
            houses[i][j].x = newHouses[i][j][1];
            houses[i][j].value = (j + 1) + (i) * 10;
        }
    }

    for (int i = 0; i < MAX_PLAYER; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            players[i][j].x = houses[i][j].x;
            players[i][j].y = houses[i][j].y;
            players[i][j].index = -1;
            players[i][j].team = i;
            players[i][j].id = j + 1;
            players[i][j].removed = 0;
            players[i][j].position=n_players;
            players[i][j].counter = 0;
            if(i < n_players)
            {
                players[i][j].ingame = true;
            }
            else
            {
                players[i][j].ingame = false;
            }
        }
    }
    // inputnames();
     string Name;
    for(int i=0; i<n_players; i++)
    {
        cout << "Enters Player " << i+1 << "'s Name (Without spaces) : ";
        cin >> Name;
        players[i][0].name = Name;
    }

    bool *visited = new bool[n_players];
    int counter=1;       // random turn
    int *turn = new int;   
    *turn = rand()%n_players;
    visited[*turn]=1;
    for(int i=0; i<n_players; i++) 
    {
        isfinished[i]=0;
        visited[i]=0;
        if(i<3)
        {
            diceroll[i] = 0;
        }
    }
    cout<<"Game Started"<<endl;
    pthread_t Playthreads[4];
    srand(time(NULL));
    while (position != n_players)
    {
        for (int i = 0; i < n_players; i++)
        {
            sleep(1);
            pthread_create(&Playthreads[i], NULL, PlayerThread, turn);
            if(counter == n_players)      // Random turn
            {
                counter = 0;
                for(int i=0; i<n_players; i++)
                    {   
                        visited[i]=0;
                    }
            }
            do
            {
                *turn = rand()%n_players;
            }while(visited[*turn]==1);
            visited[*turn]=1;
            counter++;
        }
        for (int i = 0; i < n_players; i++)
        {
            pthread_join(Playthreads[i], NULL);
        }
       
    }
    
    return 0;
}


int main()
{
    sem_init(&sem, 0, 1);
    srand(time(NULL));

    // Correct Ludo board layout
    char ludoBoard[SIZE][SIZE] = {
        {'H', 'H', 'H', 'H', 'H', 'H', '.', '.', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', '.', 'r', 'S', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', 'S', 'r', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', '.', 'r', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', '.', 'r', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', '.', 'r', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'.', 'S', '.', '.', '.', '.', '|', '|', '|', '.', '.', '.', 'S', '.', '.'},
        {'.', 'y', 'y', 'y', 'y', 'y', '|', '|', '|', 'g', 'g', 'g', 'g', 'g', '.'},
        {'.', '.', 'S', '.', '.', '.', '|', '|', '|', '.', '.', '.', '.', 'S', '.'},
        {'H', 'H', 'H', 'H', 'H', 'H', '.', 'b', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', '.', 'b', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', '.', 'b', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', '.', 'b', 'S', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', 'S', 'b', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
        {'H', 'H', 'H', 'H', 'H', 'H', '.', '.', '.', 'H', 'H', 'H', 'H', 'H', 'H'},
    };

    // Copy the Ludo board into the `board` variable
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            board[i][j] = ludoBoard[i][j];
        }
    }

    // Game loop
    bool isGameRunning = true;
    while (isGameRunning)
    {
        int choice = -1;
        menu(choice);

        if (choice == 2)
        {
            cout << endl
                 << "Stopping the game ..." << endl;
            isGameRunning = false;
        }
        else if (choice == 1)
        {
            play();
            display();
        }
        else
        {
            cout << "Invalid choice. Please select a valid option." << endl;
        }
    }

    return 0;
}
