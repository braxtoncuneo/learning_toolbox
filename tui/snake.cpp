#include <cmath>
#include <thread>
#include <queue>
#include "tui.h"

struct Position {
    int x;
    int y;
};

// Updates game state information based upon key presses from the user
void handle_input(bool *done, char *dir) {
    while (!*done) {
        char c = std::getchar();
        switch (c) {
            case 'Q': case 'q':
                (*done) = true;
            break;
            case 'W': case 'w':
                (*dir) = 'w';
            break;
            case 'A': case 'a':
                (*dir) = 'a';
            break;
            case 'S': case 's':
                (*dir) = 's';
            break;
            case 'D': case 'd':
                (*dir) = 'd';
            break;
            default:
            break;
        }
    }
}

int main() {

    int const SNAKE_STARTING_SIZE = 10;
    int const WIDTH  = 32;
    int const HEIGHT = 32;

    // Make the canvas twice the ostensible width to account for characters
    // being taller than they are wide
    TUI::Canvas canvas(WIDTH*2,HEIGHT,4,4);

    // Black out display
    for (int y=0; y<HEIGHT; y++) {
        for (int x=0; x<(WIDTH*2); x++) {
            canvas(x,y) = TUI::RGB{0,0,0};
        }
    }

    TUI::Input::raw_mode();

    std::deque<Position> snake_body;

    // Display the full canvas
    canvas.full_display();

    // Game state variables
    bool done = false;
    bool lost = false;
    char last_dir = 'd';
    char dir = 'd';

    // Start up helper thread that handles the effects of user input
    std::thread input_handler(handle_input,&done,&dir);

    // Initialize the food to be at a random location in the world
    Position food = {rand()%WIDTH, rand()%HEIGHT};
    canvas(food.x*2,  food.y) = TUI::Tile{
        "🍎",
        TUI::RGB{0,0,0},
        TUI::RGB{0,0,0},
    };

    // Draw the body of the snake on the first row of tiles
    Position pos {0,0};
    for (int i=0; i<SNAKE_STARTING_SIZE; i++) {
        pos = Position{i,0};
        canvas(pos.x*2,  pos.y) = TUI::Tile{
            "🟩",
            TUI::RGB{0,0,0},
            TUI::RGB{0,0,0}
        };
        snake_body.push_back(pos);
    }

    // Keep going until we are done
    while (!done) {

        // Throttle framerate to make the program slow enough for
        // humans to play
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Prevent snake from doubling back on itself
        switch (last_dir) {
            case 'w': if (dir == 's') {dir = 'w';} break;
            case 'a': if (dir == 'd') {dir = 'a';} break;
            case 's': if (dir == 'w') {dir = 's';} break;
            case 'd': if (dir == 'a') {dir = 'd';} break;
            default: break;
        }

        // Move snake in its current direction of travel
        switch (dir) {
            case 'w': pos.y = (pos.y+HEIGHT-1) % HEIGHT;  break;
            case 'a': pos.x = (pos.x+WIDTH-1) % WIDTH;    break;
            case 's': pos.y = (pos.y+1) % HEIGHT;         break;
            case 'd': pos.x = (pos.x+1) % WIDTH;          break;
            default: break;
        }

        // Remember the most recent direction of movement
        last_dir = dir;

        // Hide last segment of snake tail and remove it from body queue
        if ( (pos.x == food.x) && (pos.y == food.y) ){
            food = {rand()%WIDTH, rand()%HEIGHT};
            canvas(food.x*2,  food.y) = TUI::Tile{
                "🍎",
                TUI::RGB{0,0,0},
                TUI::RGB{0,0,0},
            };
            canvas.reposition(rand()%10,rand()%10);
        } else {
            Position tail_pos = snake_body.front();
            canvas(tail_pos.x*2,  tail_pos.y) = TUI::Tile{TUI::RGB{0,0,0}};
            canvas(tail_pos.x*2+1,tail_pos.y) = TUI::Tile{TUI::RGB{0,0,0}};
            snake_body.pop_front();
        }

        // Add new segment of snake to the front of the body queue and
        // write it to the canvas
        snake_body.push_back(pos);
        canvas(pos.x*2,  pos.y) = TUI::Tile{
            "🟩",
            TUI::RGB{0,0,0},
            TUI::RGB{0,0,0}
        };


        // Check for snake self-collisions and trigger break if one is found
        size_t length = snake_body.size();
        for (size_t i=0; i<(length-1); i++) {
            if ((snake_body[i].x == pos.x) && (snake_body[i].y == pos.y)) {
                done = true;
                lost = true;
            }
        }

        // Update changed portions of the canvas
        canvas.lazy_display();
    }

    // Display game over screen if the player lost
    if (lost) {

        // Fill the canvas with 50% grey
        for (int y=0; y<HEIGHT; y++) {
            for (int x=0; x<WIDTH*2; x++) {
                canvas(x,y) = TUI::Tile{TUI::RGB{127,127,127}};
            }
        }

        // Write "GAME OVER" to the center of the canvas
        std::string lose_text = "GAME OVER";
        int y      = HEIGHT/2;
        int length = lose_text.size();
        int start  = WIDTH-length/2;
        for (int i=0; i<length; i++) {
            canvas(start+i,y) = TUI::Tile{
                std::string(1,lose_text[i]),
                TUI::RGB{255,0,0},
                TUI::RGB{0,0,0}
            };
        }

        canvas(start+length,y) = TUI::Tile{
            "😭",
            TUI::RGB{255,0,0},
            TUI::RGB{0,0,0}
        };

        // Again, update the display
        canvas.lazy_display();
    }

    // Wait for the input handler to finish up
    input_handler.join();

    // Make sure the terminal has been restored to cooked mode, then
    // hide the canvas
    TUI::Input::cooked_mode();
    canvas.hide();
}

